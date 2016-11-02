#include <tstr.h>

/*
 * 测试报告
 * 关于 字符串模块,测试通过
 *
 * 希望, VS能对C有更多的支持
 */
void test_tstring(void) {
	//先在堆上 申请内存
	tstr_t tstr = tstr_new("123");
	printf("str:%s, len:%d, size:%d.\n", tstr->str, tstr->len, tstr->size);

	tstr_append(tstr, 'A');
	printf("str:%s, len:%d, size:%d.\n", tstr->str, tstr->len, tstr->size);
	tstr_appends(tstr, "11111111111111111111111111111111111111111111111111111111111112你好你好你好\"你好");
	printf("str:%s, len:%d, size:%d.\n", tstr->str, tstr->len, tstr->size);

	tstr_delete(tstr);

	//在栈上测试
	TSTR_NEW(ts);
	tstr_append(ts, 'w');
	printf("str:%s, len:%d, size:%d.\n", ts->str, ts->len, ts->size);
	tstr_appends(ts, "AN,but WAI!");
	printf("str:%s, len:%d, size:%d.\n", ts->str, ts->len, ts->size);
	TSTR_DELETE(ts);

#ifdef __GNUC__
	exit(EXIT_FAILURE);
#endif
}