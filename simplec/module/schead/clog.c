#include <clog.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

//
// 急速, 清洁, 可靠配合 lograte的 c多线程单机日志库 clog.h
//					by simplec wz 2017年4月26日
//

static FILE * _log;

//
// cl_start - !单例! 开启单机日志库
// path		: 初始化日志系统文件名
// return	: void
//
inline void 
cl_start(const char * path) {
	if (NULL == _log) {
		_log = fopen(path, "ab");
		if (NULL == _log) {
			fprintf(stderr, "fopen ab err path = %s!\n", path);
			exit(EXIT_FAILURE);
		}
	}
}

//
// _INT_LOGSZ	- 每条日志的大小
// _STR_LOGDIR	- 存放所有日志的目录
//
#define _INT_LOGSZ		(2048u)
#define _STR_LOGTIME	"[" _STR_MTIME "]"

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
	char str[_INT_LOGSZ];

	// 串:得到时间串并返回长度 [2016-07-10 22:38:34 999]
	len = stu_getmstrn(str, sizeof(str), _STR_LOGTIME);

	// 开始数据填充
	va_start(ap, fmt);
	vsnprintf(str + len, sizeof(str) - len, fmt, ap);
	va_end(ap);
	
	// 下数据到文本中
	fputs(str, _log);
}