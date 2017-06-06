#include <scconf.h>

/*
 * simple c 简单描述
 * return	: void
 */
void simplec_go(void);

/*
 * simple c 单元测试主函数
 * return	: void
 */
void simplec_test(void);

/*
 * simple c 框架业务层启动的代码
 */
void simplec_main(void) {

#if defined(_MSC_VER) && defined(_DEBUG)
	// 开始简单的测试
	simplec_test();
#endif

	// 开始动画部分
	simplec_go();

	/*
	 * Hero later is your world 
	 * . . .
	 */

}

// 第一次见面的函数
void 
simplec_go(void) {
	const char * image;

	// 得到配置的版本信息
	image = mconf_get("Image");

	// 打印简单信息
	puts(image);
}

#if defined(_MSC_VER) && defined(_DEBUG)

/*
 * simple c 单元测试主函数
 * return	: void
 */
void 
simplec_test(void) {
	// 单元测试 - 测试 plog 日志系统
	extern void test_plog(void);
	// 单元测试 - 测试 scthreads 线程池
	extern void test_scthreads(void);
	// 单元测试 - 测试 iop 网络io
	extern void test_iopserver(void);
	// 单元测试 - 测试 libcurl http 库
	extern void test_httputil(void);
	// 单元测试 - 测试 玩具 ddos攻击
	extern void test_ddos(void);

	test_ddos();
}

#endif
