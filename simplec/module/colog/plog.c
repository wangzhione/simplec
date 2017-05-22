#include <mq.h>
#include <plog.h>
#include <objs.h>
#include <scrunloop.h>

//
// _UINT_PLOG		初始化16MB, 就重新构建日志文件
// _STR_PLOG_NAME	logs/simplec_[2016-7-10 22:38:34 500].log
//
#define _UINT_PLOG			(1 << 24)
#define _STR_PLOG_NAME		_STR_LOGDIR "/simplec_[%s].log"

struct plog {
	FILE * log;				// 写的文件句柄
	uint32_t size;			// 当前文件大小
	char path[_INT_PATH];	// 文件详细路径名称

	objs_t pool;			// 对象池
	srl_t loop;				// 消息轮序器
};

// plog 单例对象
static struct plog _plog;

static void _pl_end(void) {
	if (_plog.log) {
		srl_delete(_plog.loop);
		objs_delete(_plog.pool);
		fclose(_plog.log);
		BZERO(_plog);
	}
}

//
// sl_start - 开启单机日志库
// return	: void
//
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