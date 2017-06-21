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

//
// TEST_RUN - simplec 单元测试宏, 方便进行单元测试
// test		: 需要单元测试的函数名, 通test 目录下文件名
//
#define TEST_RUN(test) \
	do {\
		extern void test(void);\
		test();\
	} while(0)

/*
 * simple c 单元测试主函数
 * return	: void
 */
void 
simplec_test(void) {
	TEST_RUN(test_sciconv);
	TEST_RUN(test_httputil);
}

#endif
