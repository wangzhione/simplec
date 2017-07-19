#include <clog.h>

//
// 急速, 清洁, 可靠配合 lograte的 c多线程单机日志库 clog.h
//					by simplec wz 2017年4月26日
//

static FILE * _log;

//
// cl_start - 开启单机日志库
// return	: void
//
inline void 
cl_start(void) {
	if (NULL == _log) {
		_log = fopen(_STR_CLOG_NAME, "ab");
		if(NULL == _log)
			CERR_EXIT("fopen " _STR_CLOG_NAME " ab is error!");
	}
}

//
// cl_printf - 具体输出日志内容
// fmt		: 必须双引号包裹起来的串
// ...		: 对映fmt参数
// return	: void
//
void 
cl_printf(const char * fmt, ...) {
	va_list ap;
	size_t len;
	char str[_UINT_LOGS];

	// 串:得到时间串并返回长度 [2016-07-10 22:38:34 999]
	len = stu_getmstrn(str, sizeof(str), _STR_LOGTIME);

	// 开始数据填充
	va_start(ap, fmt);
	vsnprintf(str + len, sizeof(str) - len, fmt, ap);
	va_end(ap);
	
	// 下数据到文本中
	fputs(str, _log);
}