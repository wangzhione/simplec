#include <mq.h>
#include <plog.h>
#include <objs.h>
#include <scrunloop.h>

//
// _UINT_PLOG		初始化16MB, 就重新构建日志文件
// _STR_PLOG_NAME	logs/simplec_[2016-7-10 22:38:34 500].log
//
#define _UINT_PLOG			(1u << 5)
#define _STR_PLOG_NAME		_STR_LOGDIR "/simplec-%04d%02d%02d-%02d%02d%02d-%03d.log"

// 写的日志内容
struct log {
	size_t len;
	char str[_UINT_LOG];
};

struct plog {
	FILE * volatile log;	// 写的文件句柄
	uint32_t size;			// 当前文件大小
	char path[_UINT_PATH];	// 文件详细路径名称

	objs_t pool;			// 对象池
	srl_t loop;				// 消息轮序器
};

// plog 单例对象
static struct plog _plog;

// plog 日志库退出的时候执行体
static void _pl_end(void) {
	if (_plog.log) {
		srl_delete(_plog.loop);
		// 业务越封装越复杂, C++相比C真的不仅仅是改进也是噩梦
		objs_delete(_plog.pool);
		fclose(_plog.log);
		BZERO(_plog);
	}
}

// 打开新的文件系统写入
static void _pl_openfile(void) {
	time_t t;
	struct tm st;
	struct timeval tv;

	// 构建时间串
	gettimeofday(&tv, NULL);
	t = tv.tv_sec;
	localtime_r(&t, &st);
	snprintf(_plog.path, _UINT_PATH, _STR_PLOG_NAME,
		st.tm_year + _INT_YEAROFFSET, st.tm_mon + _INT_MONOFFSET, st.tm_mday,
		st.tm_hour, st.tm_min, st.tm_sec, 
		tv.tv_usec / 1000);

	_plog.log = fopen(_plog.path, "ab");
	if (NULL == _plog.log)
		CERR_EXIT("fopen path ab error = %s.", _plog.path);
}

// 消息队列中消息对象销毁
static inline void _die(struct log * log) {
	fputs(log->str, _plog.log);
	objs_free(_plog.pool, log);
}

// 轮询器的主体
static void _run(struct log * log) {

	// 重新构建文件信息
	if (_plog.size >= _UINT_PLOG) {
		_plog.size = 0;
		fclose(_plog.log);
		_pl_openfile();
	}

	// 这里打印信息
	_plog.size += log->len;

	// 回收消息体
	_die(log);
}

//
// sl_start - 开启单机日志库
// return	: void
//
void
pl_start(void) {
	if (_plog.log) return;
	// 构建串, 处理消息写入
	_pl_openfile();

	// 构建对象池
	_plog.pool = objs_create(sizeof(struct log), 0);
	if (NULL == _plog.pool) {
		fclose(_plog.log);
		CERR_EXIT("objs_create _plog.pool is error = 4 + %u.", _UINT_LOG);
	}

	// 构建消息轮询器
	_plog.loop = srl_create(_run, _die);
	if (NULL == _plog.loop) {
		objs_delete(_plog.pool);
		fclose(_plog.log);
		CERR_EXIT("srl_create _run, _die is error! _run | _die = %p, %p.", _run, _die);
	}

	// 开始注册退出处理事件
	atexit(_pl_end);
}

//
// sl_printf - 具体输出日志内容
// fmt		: 必须双引号包裹起来的串
// ...		: 对映fmt参数
// return	: void
//
void 
pl_printf(const char * fmt, ...) {
	va_list ap;

	// 从对象池中拉内存, 填充数据
	struct log * log = objs_malloc(_plog.pool);
	if (NULL == log) {
		RETURN(NIL, "objs_malloc error _plog.pool = %p.", _plog.pool);
	}

	// 串:得到时间串并返回长度 [2016-7-10 22:38:34 1000]
	log->str[0] = '[';
	log->len = stu_getmstr(log->str + 1);
	log->str[log->len + 1] = ']';
	log->len += 2;

	// 串:开始数据填充
	va_start(ap, fmt);
	log->len += vsnprintf(log->str + log->len, LEN(log->str) - log->len, fmt, ap);
	va_end(ap);

	// 串:压入
	srl_push(_plog.loop, log);
}