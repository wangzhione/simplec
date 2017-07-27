#include <scconf.h>
#include <scsocket.h>

// stderr 信息重定位到文件中
#define _STR_STDERRLOG		_STR_LOGDIR "/stderr.log"

/*
 * simple c 框架业务启动入口的代码
 */
void simplec_main(void);

//
// simple c 开发框架的启动函数总入口
// 变量函数环境启动初始化, 改动要慎重. atexit 先注册, 后调用 等同于数组栈
//
int main(int argc, char * argv[]) {
	// 初始化随机序列
	sh_srand((int32_t)time(NULL));

	// 简单创建 _STR_LOGDIR 日志目录
	sh_mkdir(_STR_LOGDIR);

	// stderr 错误信息重定位, 跟随系统长生不死
	if (!freopen(_STR_STDERRLOG, "ab", stderr)) {
		CERR_EXIT("freopen ab " _STR_STDERRLOG " is error!");
	}

	// 目前系统是默认启动 clog 日志
	cl_start();
	// 启动基础配置系统, 并得到配置的单例对象
	mcnf_start();
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