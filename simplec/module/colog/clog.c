#include <clog.h>

//
// 一个非常简单, 非常快速单例c多线程单机日志库 clog.h
//					by simplec wz 2017年4月26日
//

static FILE * _log;

// 在当前日志库退出的时候所做的事情
static inline void _cl_end(void) {
	if (_log) {
		fclose(_log);
		_log = NULL;
	}
}

//
// cl_start - 开启单机日志库
// return	: void
//
void cl_start(void) {
	if (NULL == _log) {
		_log = fopen(_STR_CLOG_NAME, "ab");
		if(NULL == _log)
			CERR_EXIT("fopen %s ab is error!", _STR_CLOG_NAME);
		atexit(_cl_end);
	}
}

//
// cl_printf - 具体输出日志内容
// fmt		: 必须双引号包裹起来的串
// ...		: 对映fmt参数
// return	: void
//
void cl_printf(const char * fmt, ...) {
	va_list ap;
	size_t len;
	char logs[_INT_LOG];

	DEBUG_CODE({
		if (!_log || !fmt || _INT_LOG <= _INT_STULEN * 2) {
			CERR("params log fmt is empty %p | %p.", _log, fmt);
			return;
		}
	});

	// 得到时间串并返回长度 [2016-7-10 22:38:34 1000]
	logs[0] = '[';
	len = stu_getmstr(logs + 1);
	logs[len + 1] = ']';
	len += 2;

	// 开始数据填充
	va_start(ap, fmt);
	vsnprintf(logs + len, LEN(logs) - len, fmt, ap);
	va_end(ap);
	
	// 下数据到文本中
	fputs(logs, _log);
}
