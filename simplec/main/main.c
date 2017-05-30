#include <scconf.h>
#include <iop_util.h>

// stderr 信息重定位到文件中
#define _STR_STDERRLOG		_STR_LOGDIR "/stderr.log"

static FILE * _err;
static inline void _err_exit(void) {
	fclose(_err);
}

/*
 * simple c 框架业务启动入口的代码
 */
extern void simplec_main(void);

//
// simple c 开发框架的启动函数总入口
// 函数启动顺序, 改动要慎重, atexit 先注册, 后调用 等同于数组栈
//
int main(int argc, char * argv[]) {
	// 简单创建 _STR_LOGDIR 日志目录
	sh_mkdir(_STR_LOGDIR);

	// stderr 错误信息重定位, 需要在日志系统启动之后
	_err = freopen(_STR_STDERRLOG, "ab", stderr);
	if (NULL == _err) {
		CERR_EXIT("freopen ab " _STR_STDERRLOG " is error!");
	}
	atexit(_err_exit);

	// 目前系统是默认启动 clog 日志
	cl_start();
	// 启动基础配置系统, 并得到配置的单例对象
	mconf_start();
	// 初始化socket 库操作
	socket_start();

	// 开启_DEBUG模式下结束等待
	INIT_PAUSE();

	/*
	 * simple c -> 具体业务跑起来 , -> ︿(￣︶￣)︿
	 */
	/* 8-> */ simplec_main(); /* <-8 */

	return 0;
}