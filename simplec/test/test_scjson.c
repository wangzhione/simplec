#include <scjson.h>

// test scjson write
static void _scjson_write(void) {
	// 测试json 串
	char jstr[] = "{\n\"name\": \"Jack (\\\"Bee\\\") 你好\", \n\"format\": {\"type\":[1, 3, 4, 5.66], \n\"height\":     1080, \n\"interlace\":  false}\n}";
	printf("源码串 :\n %s\n", jstr);

	// 先生成 json 对象
	cjson_t root = cjson_newstr(jstr);
	if (root == NULL) {
		EXIT("jstr 解析失败! 程序退出中....");
	}

	//这里简单测试输出内容
	char * njstr = cjson_getstr(root);

	if (njstr == NULL) {
		cjson_delete(root);
		EXIT("输出内容失败,程序退出中!");
	}

	//合法范围直接输出 内容
	printf("解析串 :\n %s\n", njstr);

	//解析完需要释放
	free(njstr);

	//解析好 一定要注意释放操作
	cjson_delete(root);

	//另一个测试 输出内存值
	printf("d = %zu\n", strlen(jstr));
}

#define _STR_FILE "test/config/firefighting_rule.json"

// 测试文件读写
static void _scjson_read(void) {
	cjson_t rule = cjson_newfile(_STR_FILE);
	if (NULL == rule)
		EXIT("cjson_dofile " _STR_FILE " is error!");

	// 数据合法 这里开始得到一部分
	cjson_t firefighting_rule = cjson_detachobject(rule, "firefighting_rule");
	// 得到真正的内容
	cjson_t key1 = cjson_detachobject(firefighting_rule, "key1");
	cjson_t jinbi_buff_price = cjson_getobject(key1, "jinbi_buff_price");
	printf("jinbi_buff_price = %lf\n", jinbi_buff_price->vd);

	//这里得到 key1 输出内容
	char * nkey = cjson_getstr(key1);
	if (NULL == nkey)
		EXIT("cjson_print key1 is error!");
	
	// 这里打印数据测试 
	puts(nkey);
	free(nkey);

	// 再简单测试一下 
	cjson_t id = cjson_getobject(key1, "id");
	printf("\nid = %d\n", cjson_getvi(id));

	//得到数组对象 测试
	cjson_t level_contain = cjson_getobject(key1, "level_contain");
	printf("\ncount(level_contain) = %zu\n", cjson_getlen(level_contain));

	cjson_delete(key1);
	cjson_delete(firefighting_rule);
	// rule 释放
	cjson_delete(rule);
}

// 测试基础解析
static void _scjson_parse(void) {
	puts("test _scjson_one cjson_newstr");
	const char * jstr = "[\n    [0, -1, 0],\n    [1, 0, 0],\n    [0, 0, 1]\n	]\n";
	cjson_t js = cjson_newstr(jstr);
	size_t len = cjson_getlen(js);
	for (size_t i = 0; i < len; ++i) {
		cjson_t item = cjson_getarray(js, i);
		printf("%zu => %zu.\n", i, cjson_getlen(item));
	}
	cjson_delete(js);
}

//
// test use scjson
//
void test_scjson(void) {
	_scjson_parse();
	_scjson_read();
	_scjson_write();
}