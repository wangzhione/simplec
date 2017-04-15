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

#ifdef _MSC_VER
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
	// 单元测试 - 协程库
	extern void test_scoroutine(void);
	// 单元测试 - 支持循环的定时器
	extern void test_sctimer(void);
	// 单元测试 - 测试csv读写
	extern void test_csv(void);
	// 单元测试 - scjson 测试读取
	extern void test_json_read(void);
	// 单元测试 - 'xlsm' to json 简单测试
	extern void test_xlstojson(void);
	// 单元测试 - 时间业务测试
	extern void test_sctimeutil(void);
	// 单元测试 - 配置文件读写
	extern void test_scconf(void);
	// 单元测试 - 测试可变数组操作
	extern void test_array(void);
	// 单元测试 - 测试线程信号量
	extern void test_pthread_sem(void);

	test_pthread_sem();
}
