#include <plog.h>

//
// _UINT_PLOG 初始化16MB, 就重新构建日志文件
//
#define _UINT_PLOG		(1 << 24)

struct plog {
	FILE * log;
	uint32_t size;
	char path[_INT_PATH];
};

//
// sl_start - 开启单机日志库
// return	: void
//
#define _STR_PLOG_NAME	_STR_LOGDIR "/simplec_%s.log"
void
pl_start(void) {

}

//
// sl_printf - 具体输出日志内容
// fmt		: 必须双引号包裹起来的串
// ...		: 对映fmt参数
// return	: void
//
void 
pl_printf(const char * fmt, ...) {

}