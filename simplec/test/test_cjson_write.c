#include <scjson.h>
#include <sclog.h>

// 测试 cjson 函数
void test_cjson_write(void) {

	//启动日志记录功能
	sl_start();

	// 测试json 串
	char jstr[] = "{\n\"name\": \"Jack (\\\"Bee\\\") Nimble\", \n\"format\": {\"type\":[1, 3, 4, 5.66], \n\"height\":     1080, \n\"interlace\":  false}\n}";

	printf("源码串 :\n %s\n", jstr);

	// 先生成 json 对象
	tstr_t str = tstr_create(jstr);
	cjson_t root = cjson_newtstr(str);
	if (root == NULL) {
		puts("jstr 解析失败! 程序退出中....");
		exit(EXIT_FAILURE);
	}

	//这里简单测试输出内容
	char* njstr = cjson_print(root);

	if (njstr == NULL) {
		puts("输出内容失败,程序退出中!");
		cjson_delete(root);
		exit(EXIT_FAILURE);
	}

	//合法范围直接输出 内容
	printf("解析串 :\n %s\n", njstr);

	//解析完需要释放
	free(njstr);

	//解析好 一定要注意释放操作
	cjson_delete(root);
	tstr_delete(str);

	//另一个测试 输出内存值
	printf("d = %d\n", (int)strlen("{\"name\":\"Jack (\\\"Bee\\\") Nimble\",\"format\":{\"type\":[1,3,4,5.660000],\"height\":1080,\"interlace\":false}}"));

#ifdef __GNUC__
	exit(EXIT_SUCCESS);
#endif
}