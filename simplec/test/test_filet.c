#include <tstr.h>
#include <schead.h>

#define _INT_LINE   (10000000)
#define _STR_FILE   "test.log"

// 数据初始化
static void _finit(void) {
    FILE * log = fopen(_STR_FILE, "rb");
    if (log != NULL) {
        fclose(log);
        RETURN(NIL, "fopen rb error = " _STR_FILE);
    }
    
    log = fopen(_STR_FILE, "wb");
    if (NULL == log) 
        CERR_EXIT("fopen wb is error = " _STR_FILE);

    // 开始构建数据
    for (int i = 0; i < _INT_LINE; ++i) {
        // 每行输出数据
        fputs("int j = 0;\r\n" , log);
    }

    fclose(log);
}

//
// test file write
//
void test_filet(void) {
    int n = 0, cnt = 0;
    char buf[BUFSIZ], c;
    // 初始化监测
    _finit();
    // 开始读取数据
    tstr_t log = tstr_freadend(_STR_FILE);

    // 测试单元
    TIME_PRINT({
        // 开始解释数据
        char * si = log->str;
        char * ei = log->str + log->len;
        while (si < ei) {
            buf[n++] = c = *si++;
            // 重新开始保存
            if (c == '\n') {
                n = 0;
                ++cnt;
            }
        }
    });

    printf("len = %uz, cnt = %d, buf[0] = %c.\n", log->len, cnt, buf[0]);

    tstr_delete(log);
}