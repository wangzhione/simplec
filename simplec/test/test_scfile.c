#include <scfile.h>

#define _STR_CNF    "test/config/scfile.ini"

static inline void _scfile(FILE * cnf, void * arg) {
    char txt[BUFSIZ];
    fgets(txt, sizeof txt, cnf);
    puts(txt);
}

//
// test config file update
//
void test_scfile(void) {
    printf("sizeof \"65535\" = %zu.\n", sizeof "65535");

    // 开始注册一个
    file_set(_STR_CNF, _scfile, NULL);

    // 定时刷新, 凡事追求 61分.
    for (int i = 0; i < 61; ++i) {
        file_update();
        // 1s 一测试
        sh_msleep(1000);
    }

    // 最后清除一下
    file_delete();
}
