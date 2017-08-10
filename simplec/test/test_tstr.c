#include <tstr.h>

//
// tstr simplec 内置的字符串操作
//
void test_tstr(void) {
	//先在堆上 申请内存
	tstr_t tstr = tstr_creates("123");
	printf("str:%s, len:%zu, size:%zu.\n", tstr->str, tstr->len, tstr->cap);
	tstr_appendc(tstr, 'A');
	printf("str:%s, len:%zu, size:%zu.\n", tstr_cstr(tstr), tstr->len, tstr->cap);
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
}