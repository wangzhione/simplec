#include <schead.h>
#include <sclog.h>
#include <scconf.h>

// stderr 信息重定位到文件中
#define _STR_STDERRLOG	"./" _STR_SCLOG_DIR "/stderr.log"

/*
 * simple c 框架业务启动入口的代码
 */
extern void simplec_main(void);

/*
 * simple c 开发框架的启动函数总入口
 */
int main(int argc, char * argv[]) {
	sconf_t conf;
	FILE * errlog;

	// 开启_DEBUG模式下结束等待
	INIT_PAUSE();

	// 启动日志系统
	sl_start();

	// stderr 错误信息重定位, 需要在日志系统启动之后
	errlog = freopen(_STR_STDERRLOG, "wb", stderr);
	if (NULL == errlog) {
		CERR("freopen wb " _STR_STDERRLOG " is error!");
		exit(EXIT_FAILURE);
	}
	// 启动基础配置系统, 并得到配置的单例对象
	conf = mconf_start();

	/*
	 * simple c -> 具体业务跑起来 , -> ︿(￣︶￣)︿
	 */
	/* 8-> */ simplec_main(); /* <-8 */

	// 销毁申请的资源 or 句柄
	sconf_delete(conf);
	fclose(errlog);

	return 0;
}