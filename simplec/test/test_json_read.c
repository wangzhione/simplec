#include <schead.h>
#include <sclog.h>
#include <scjson.h>

#define _STR_FILE "test/config/firefighting_rule.json"

/*
 * 这里 是解析 上面的json文件内容
 */
void test_json_read(void) {
	// 开启日志记录功能
	sl_start();

	cjson_t rule = cjson_newfile(_STR_FILE);
	if (NULL == rule)
		CERR_EXIT("cjson_dofile " _STR_FILE " is error!");

	// 数据合法 这里开始得到一部分
	cjson_t firefighting_rule = cjson_detachobject(rule, "firefighting_rule");
	// 得到真正的内容
	cjson_t key1 = cjson_detachobject(firefighting_rule, "key1");
	cjson_t jinbi_buff_price = cjson_getobject(key1, "jinbi_buff_price");
	printf("jinbi_buff_price = %lf\n", jinbi_buff_price->vd);

	//这里得到 key1 输出内容
	char * nkey = cjson_print(key1);
	if (NULL == nkey)
		CERR_EXIT("cjson_print key1 is error!");
	
	// 这里打印数据测试 
	puts(nkey);
	free(nkey);

	// 再简单测试一下 
	cjson_t id = cjson_getobject(key1, "id");
	printf("\nid = %d\n", cjson_getint(id));

	//得到数组对象 测试
	cjson_t level_contain = cjson_getobject(key1, "level_contain");
	printf("\ncount(level_contain) = %d\n", cjson_getlen(level_contain));

	cjson_delete(key1);
	cjson_delete(firefighting_rule);
	// rule 释放
	cjson_delete(rule);

#ifdef __GNUC__
	exit(EXIT_SUCCESS);
#endif
}