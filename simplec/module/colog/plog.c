#include <mq.h>
#include <plog.h>
#include <objs.h>
#include <scrunloop.h>

//
// _UINT_PLOG		初始化16MB, 就重新构建日志文件
//
#define _UINT_PLOG			(1u << 24)

// 写的日志内容, 写小日志
struct log {
	size_t len;
	char str[_UINT_LOGS];
};

struct plog {
	FILE * log;				// 写的文件句柄
	uint32_t size;			// 当前文件大小
	char path[_UINT_PATH];	// 文件详细路径名称

	objs_t pool;			// 对象池
	srl_t loop;				// 消息轮序器
};

// plog 单例对象
static struct plog _plog;

// 打开新的文件系统写入
static inline void _pl_openfile(void) {
	stu_getmstrn(_plog.path, sizeof(_plog.path), _STR_PLOG_NAME);
	FILE * log = fopen(_plog.path, "ab");
	if (_plog.log) {
		if (NULL == log)
			RETURN(NIL, "fopen ab error = %s.", _plog.path);
		fclose(_plog.log);
	}
	_plog.log = log;
}

// 消息队列中消息对象销毁
static inline void _die(struct log * log) {
	fputs(log->str, _plog.log);
	objs_free(_plog.pool, log);
}

// 轮询器的主体
static inline void _run(struct log * log) {
	// 重新构建文件信息
	if (_plog.size >= _UINT_PLOG) {
		_plog.size = 0;
		_pl_openfile();
	}

	// 这里打印信息
	_plog.size += log->len;
}

//
// sl_start - 开启单机日志库
// return	: void
//
void
pl_start(void) {
	if (_plog.log) return;
	
	// 构建文件, 对象池, 消息轮询器
	_pl_openfile();
	_plog.pool = objs_create(sizeof(struct log));
	_plog.loop = srl_create(_run, _die);

	// 退出资源销毁, 全部交给操作系统
	if (!_plog.log || !_plog.pool || !_plog.loop)
		CERR_EXIT("pl_start error log pool loop = %p, %p, %p.", _plog.log, _plog.pool, _plog.loop);
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

	// 串:得到时间串并返回长度 [2016-07-10 22:38:34 999]
	log->len = stu_getmstrn(log->str, sizeof(log->str), _STR_LOGTIME);

	// 串:开始数据填充
	va_start(ap, fmt);
	log->len += vsnprintf(log->str + log->len, sizeof(log->str) - log->len, fmt, ap);
	va_end(ap);

	// 串:压入
	srl_push(_plog.loop, log);
}