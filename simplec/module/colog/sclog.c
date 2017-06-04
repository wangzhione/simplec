#include <sclog.h>
#include <scatom.h>
#include <pthread.h>

//-------------------------------------------------------------------------------------------|
// 第二部分 对日志信息体操作的get和set,这里隐藏了信息体的实现
//-------------------------------------------------------------------------------------------|

#define _STR_LOGID _STR_LOGDIR "/.logid" // 保存logid,持久化

static struct {
	pthread_key_t	key;	//全局线程私有变量
	pthread_once_t	once;	//全局初始化用的类型
	unsigned		logid;	//默认的全局logid, 唯一标识
	FILE *			log;	//log文件指针
	FILE *			wf;		//wf文件指针
} _slmain = { (pthread_key_t)0, PTHREAD_ONCE_INIT, 0, NULL, NULL };

//内部简单的释放函数,服务于pthread_key_create 防止线程资源泄露
static void _slinfo_destroy(void * slinfo) {
	free(slinfo);
}

static void _gkey(void) {
	pthread_key_create(&_slmain.key, _slinfo_destroy);
}

struct slinfo {
	unsigned		logid;					//请求的logid,唯一id
	char			reqip[_INT_LITTLE];		//请求方ip
	char			times[_INT_LITTLE];		//当前时间串
	struct timeval	timev;					//处理时间,保存值,统一用毫秒
	char			mod[_INT_LITTLE];		//当前线程的模块名称,不能超过_INT_LITTLE - 1
};

/**
*	线程的私有数据初始化
**
** mod   : 当前线程名称
** reqip : 请求的ip
** logid : 分配的唯一标识id, 默认0
** return :	_RT_OK 表示正常,_RF_EM内存分配错误
**/
int
sl_init(const char mod[_INT_LITTLE], const char reqip[_INT_LITTLE], unsigned logid) {
	struct slinfo * pl;

	//保证 _gkey只被执行一次
	pthread_once(&_slmain.once, _gkey);

	if ((pl = pthread_getspecific(_slmain.key)) == NULL) {
		//重新构建
		if ((pl = malloc(sizeof(struct slinfo))) == NULL)
			return Error_Alloc;
	}

	gettimeofday(&pl->timev, NULL);
	//设置日志logid, 有设置, 没有默认原子自增
	pl->logid = logid ? logid : ATOM_ADD_FETCH(_slmain.logid, 1);
	strncpy(pl->mod, mod, _INT_LITTLE); //复制一些数据
	strncpy(pl->reqip, reqip, _INT_LITTLE);

	//设置私有变量
	pthread_setspecific(_slmain.key, pl);

	return Success_Base;
}

/**
*	获取日志信息体的唯一的logid
**/
unsigned
sl_getlogid(void) {
	struct slinfo * pl = pthread_getspecific(_slmain.key);
	if (NULL == pl) //返回0表示没有找见
		return 0u;
	return pl->logid;
}

/**
*	获取日志信息体的请求ip串,返回NULL表示没有初始化
**/
const char *
sl_getreqip(void) {
	struct slinfo * pl = pthread_getspecific(_slmain.key);
	if (NULL == pl) //返回NULL表示没有找见
		return NULL;
	return pl->reqip;
}

/**
*	获取日志信息体的名称,返回NULL表示没有初始化
**/
const char *
sl_getmod(void) {
	struct slinfo * pl = pthread_getspecific(_slmain.key);
	if (NULL == pl) //返回NULL表示没有找见
		return NULL;
	return pl->mod;
}


//-------------------------------------------------------------------------------------------|
// 第三部分 对日志系统具体的输出输入接口部分
//-------------------------------------------------------------------------------------------|

/**
*	日志关闭时候执行,这个接口,关闭打开的文件句柄
**/
static void _sl_end(void) {
	FILE * lid;
	void * pl;

	// 在简单地方多做安全操作值得,在核心地方用算法优化的才能稳固
	if (NULL == _slmain.log)
		return;

	// 重置当前系统打开文件结构体
	fclose(_slmain.log);
	fclose(_slmain.wf);

	// 写入文件
	lid = fopen(_STR_LOGID, "wb");
	if (NULL != lid) {
		fprintf(lid, "%u", _slmain.logid);
		fclose(lid);
	}

	// 主动释放私有变量,其实主进程 相当于一个线程是不合理的!还是不同的生存周期的
	pl = pthread_getspecific(_slmain.key);
	_slinfo_destroy(pl);
	pthread_setspecific(_slmain.key, NULL);

	BZERO(_slmain);
}

/**
*	日志系统首次使用初始化,找对对映日志文件路径,创建指定路径
**/
void
sl_start(void) {
	FILE * lid;

	// 单例只执行一次, 打开普通log文件
	if (NULL == _slmain.log) {
		_slmain.log = fopen(_STR_LOGDIR "/" _STR_SCLOG_LOG, "a+");
		if (NULL == _slmain.log)
			CERR_EXIT("__slmain.log fopen %s error!", _STR_SCLOG_LOG);
	}

	// 继续打开 wf 文件
	if (NULL == _slmain.wf) {
		_slmain.wf = fopen(_STR_LOGDIR "/" _STR_SCLOG_WFLOG, "a+");
		if (!_slmain.wf) {
			fclose(_slmain.log); // 其实这都没有必要,图个心安
			CERR_EXIT("__slmain.log fopen %s error!", _STR_SCLOG_WFLOG);
		}
	}

	// 读取文件内容,读取文件内容,持久化
	if ((lid = fopen(_STR_LOGID, "rb")) != NULL) {
		if (0 >= fscanf(lid, "%u", &_slmain.logid))
			CERR_EXIT("__slmain.log fopen " _STR_LOGID " read is error!");
		fclose(lid);
	}

	// 简单判断是否有初始化的必要
	if (_slmain.log && _slmain.wf) {
		// 这里可以单独开启一个线程或进程,处理日志整理但是 这个模块可以让运维做,按照规则搞
		sl_init("main thread", "0.0.0.0", 0);

		// 注册退出操作
		atexit(_sl_end);
	}
}

/**
*	获取日志信息体的时间串,返回NULL表示没有初始化
**/
static const char * _sl_gettimes(void) {
	unsigned td;
	struct timeval et; //记录时间

	struct slinfo * pl = pthread_getspecific(_slmain.key);
	if (NULL == pl) //返回NULL表示没有找见
		return NULL;

	gettimeofday(&et, NULL);
	//同一用微秒记
	td = 1000000 * (et.tv_sec - pl->timev.tv_sec) + et.tv_usec - pl->timev.tv_usec;
	snprintf(pl->times, sizeof(pl->times), "%u", td);

	return pl->times;
}

int
sl_printf(const char * format, ...) {
	int len;
	char str[_UINT_LOG]; //这个不是一个好的设计,最新c 中支持 int a[n];
	va_list ap;
	stime_t tstr;
	
	// 从性能方面优化试试
	DEBUG_CODE({
		if (NULL == _slmain.log) {
			CERR("%s fopen %s | %s error!", _STR_LOGDIR, _STR_SCLOG_LOG, _STR_SCLOG_WFLOG);
			return Error_Fd;
		}
	});

	//初始化时间参数
	stu_getntstr(tstr);
	len = snprintf(str, sizeof(str), "[%s %s]", tstr, _sl_gettimes());

	va_start(ap, format);
	vsnprintf(str + len, sizeof(str) - len, format, ap);
	va_end(ap);

	// 写普通文件 log
	fputs(str, _slmain.log); //把锁机制去掉了,fputs就是线程安全的

							  // 写警告文件 wf
	if (format[1] == 'F' || format[1] == 'W') //当为FATAL或WARNING需要些写入到警告文件中
		fputs(str, _slmain.wf);

	return Success_Base;
}