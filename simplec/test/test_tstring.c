#include <tstr.h>

/*
 * 测试报告
 * 关于 字符串模块,测试通过
 *
 * 希望, VS能对C有更多的支持
 */
void test_tstring(void) {
	//先在堆上 申请内存
	tstr_t tstr = tstr_create("123");
	tstr_cstr(tstr);
	printf("str:%s, len:%zu, size:%zu.\n", tstr->str, tstr->len, tstr->cap);

	tstr_appendc(tstr, 'A');
	tstr_cstr(tstr);
	printf("str:%s, len:%zu, size:%zu.\n", tstr->str, tstr->len, tstr->cap);
	tstr_appends(tstr, "11111111111111111111111111111111111111111111111111111111111112你好你好你好\"你好");
	printf("str:%s, len:%zu, size:%zu.\n", tstr->str, tstr->len, tstr->cap);

	tstr_delete(tstr);

	//在栈上测试
	TSTR_CREATE(ts);
	tstr_appendc(ts, 'w');
	tstr_cstr(ts);
	printf("str:%s, len:%zu, size:%zu.\n", ts->str, ts->len, ts->cap);
	tstr_appends(ts, "AN,but WAI!");
	printf("str:%s, len:%zu, size:%zu.\n", ts->str, ts->len, ts->cap);
	TSTR_DELETE(ts);

#ifdef __GNUC__
	exit(EXIT_SUCCESS);
#endif
}