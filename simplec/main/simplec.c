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

/*
 * simple c 单元测试主函数
 * return	: void
 */
void 
simplec_test(void) {
	// 单元测试 - 测试url 编码转换
	extern void test_scurl(void);
	// 单元测试 - 测试对象池用法
	extern void test_objs(void);
	// 单元测试 - 测试协程库
	extern void test_scoroutine(void);
	// 单元测试 - 测试轮询器
	extern void test_scrunloop(void);

	test_scrunloop();
}
