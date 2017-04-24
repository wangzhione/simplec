#include <scconf.h>

// 写完了,又能怎样,一个人
void test_scconf(void) {
	const char * value;
	// 同系统声明周期
	mconf_start();

	// 简单测试 配置读取内容
	value = mconf_get("heoo");
	printf("%s\n", value);

	value = mconf_get("Description");
	if (value)
		puts(value);
	else
		puts("Description is empty!");

#ifdef __GNUC__
	exit(EXIT_SUCCESS);
#endif
}