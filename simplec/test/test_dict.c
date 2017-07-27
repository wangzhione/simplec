#include <dict.h>

// 
// test dict use
//
void test_dict(void) {
	const char * ggs;
	dict_t d = dict_create(NULL);
	assert(d != NULL);

	dict_set(d, "wangzhi", "19921120");
	dict_set(d, "2", "3");
	dict_set(d, "heoo", "woled");
	dict_set(d, "tt", "gg");
	dict_set(d, "ggs", "heod");

	ggs = dict_get(d, "ggs");
	printf("=> %s.\n", ggs);

	dict_die(d, "tt");
	ggs = dict_get(d, "ggs");
	printf("=> %s.\n", ggs);

	dict_die(d, "ggs");
	ggs = dict_get(d, "ggs");
	printf("=> %s.\n", ggs);

	dict_delete(d);
}