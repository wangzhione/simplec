#include <scjson.h>

// 测试 cjson 函数
void test_cjson(void) {
	// 第一个测试, 测试一下简单数组
	puts("测试 cjson array number");
	char text0[] = "[ .64, -.99, 0.11, +.88, 12 ]";
	TSTR_CREATE(jstr0);
	jstr0->str = text0;
	cjson_t js0 = cjson_newtstr(jstr0);

	cjson_t one = cjson_getarray(js0, 0);
	printf("one = %lf.\n", one->vd);

	cjson_t two = cjson_getarray(js0, 1);
	printf("two = %lf.\n", two->vd);

	cjson_delete(js0);

	// 第二个 测试 json 串的解析
	puts("测试 cjson 是否可用");
	char text1[] = "{\n\"name\": \"Jack (\\\"Bee\\\") Nimble\", \n\"format\": {\"type\":       \"rect\", \n\"width\":      1920, \n\"height\":     1080, \n\"interlace\":  false,\"frame rate\": 24\n}\n}";
	TSTR_CREATE(jstr1);
	jstr1->str = text1;
	cjson_t js = cjson_newtstr(jstr1);

	cjson_t name = cjson_getobject(js, "name");
	printf("name => %s\n", name->vs);

	cjson_t format = cjson_getobject(js, "format");
	printf("len(format) => %zu\n", cjson_getlen(format));

	cjson_t interlace = cjson_getobject(format, "interlace");
	printf("interlace => %d\n", cjson_getvi(interlace));

	// 测试输出
	char * pjs = cjson_getstr(js);
	puts(pjs);
	free(pjs);

	cjson_delete(js);

	//进行第三组测试

	puts(" 测试 数组的读取");
	char text2[] = "[\"Sunday\", \"Monday\", \"Tuesday\", \"Wednesday\", \"Thursday\", \"Friday\", \"Saturday\"]";
	TSTR_CREATE(jstr2);
	jstr2->str = text2;
	js = cjson_newtstr(jstr2);
	size_t i, len = cjson_getlen(js);
	for (i = 0; i < len; ++i) {
		cjson_t item = cjson_getarray(js,i);
		printf("%zu => %s.\n", i, item->vs);
	}
	cjson_delete(js);


	puts("第三组测试");
	char text3[] = "[\n    [0, -1, 0],\n    [1, 0, 0],\n    [0, 0, 1]\n	]\n";
	TSTR_CREATE(jstr3);
	jstr3->str = text3;
	js = cjson_newtstr(jstr3);
	len = cjson_getlen(js);
	for (i = 0; i < len; ++i) {
		cjson_t item = cjson_getarray(js, i);
		printf("%zu => %zu.\n", i, cjson_getlen(item));
	}

	cjson_delete(js);
}