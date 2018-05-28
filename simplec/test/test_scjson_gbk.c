#include <scjson.h>

//
// test scjson gbk 
//
void test_scjson_gbk(void) {
    char * json = u8"{ \"name\" : \"n好h吗8\" }";

    printf("json :\n %s\n", json);

    // 先生成 json 对象
    cjson_t root = cjson_newstr(json);
    IF(NULL == root);

    cjson_t name = cjson_getobject(root, "name");
    printf("name -> %s\n", name->vs);
    // 输出 UTF-8 到文件中
    tstr_fwrites("logs/jstr.log", name->vs);

    // 通过 json 对象构建 json 串内容
    char * jstr = cjson_getstr(root);
    IF(NULL == jstr);

    //合法范围直接输出 内容
    printf("jstr :\n %s\n", jstr);

    // 释放内存
    free(jstr);
    cjson_delete(root);
}
