#include <scfile.h>

#define _STR_CNF    "test/config/scfile.ini"

static void _scfile(void * arg, FILE * cnf) {
    char txt[BUFSIZ];
    fgets(txt, sizeof txt, cnf);
    puts(txt);
}

//
// test config file update
//
void test_scfile(void) {
    // 开始注册一个
    file_set(_STR_CNF, NULL, _scfile);

    // 定式刷新
    for (;;) {
        file_update();
        // 1s 一测试
        sh_msleep(1000);
    }
}
