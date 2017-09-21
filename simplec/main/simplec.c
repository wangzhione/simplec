#include <schead.h>
#include <scconf.h>

/*
 * simple c 框架业务层启动的代码
 */
void simplec_main(void) {

#if defined(_MSC_VER) && defined(_DEBUG)
	// 开始简单的测试
	EXTERN_RUN(simplec_test);
#endif

	// 开始动画部分
	EXTERN_RUN(simplec_go);

	/*
	 * Hero later is your world 
	 * . . .
	 */

}

// 第一次见面的函数
void 
simplec_go(void) {
	// 通过配置版本信息, 简单打印
	puts(mcnf_get("Image"));

    // 测试发布版本时间测试
    EXTERN_RUN(test_filet);
}

#if defined(_MSC_VER) && defined(_DEBUG)

/*
 * simple c 单元测试主函数
 * return	: void
 */
void 
simplec_test(void) {
	EXTERN_RUN(test_scpipe);
}

#endif
