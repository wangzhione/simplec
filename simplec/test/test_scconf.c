#include <scconf.h>
#include <sciconv.h>

// 写完了,又能怎样,一个人
void test_scconf(void) {
    const char * value;

    // 简单测试 配置读取内容
    value = cnf_get("heoo");
    printf("%s\n", value);

    value = cnf_get("Description");
    if (!value)
        puts("Description is empty!");
    else {
        if (!si_isutf8(value))
        puts(value);
        else {
            // utf-8 转码 -> gbk
            char * nvals = strdup(value);
            si_utf8togbks(nvals);
            puts(nvals);
            free(nvals);
        }
    }
}
