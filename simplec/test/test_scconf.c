#include <scconf.h>

// 写完了,又能怎样,一个人
void test_scconf(void) {
	const char* value;
	mconf_start();

	//简单测试 配置读取内容
	value = mconf_get("heoo");
	puts(value);

	value = mconf_get("heoo2");
	if (value)
		puts(value);
	else
		puts("heoo2不存在");

	value = mconf_get("yexu");
	puts(value);

	value = mconf_get("end");
	puts(value);

#ifdef __GNUC__
	exit(EXIT_FAILURE);
#endif
}