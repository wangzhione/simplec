#include "scthreads.h"

// old - 全局计时器, 存在锁问题
static int old;

static inline void run(void * arg) {
    printf("run arg = %p | %p.\n", run, arg);
}

// ppt - 简单的线程打印函数
static inline void ppt(const char * str) {
    printf("%d => %s\n", ++old, str);
}

// doc - 另一个线程测试函数
static inline void doc(void * arg) {
    printf("p = %d, 技术不决定项目的成败! 我老大哭了\n", ++old);
}

void test_scthreads(void) {
    // 创建线程池
    threads_t pool = threads_create();

    async_run(run, test_scthreads);

    //添加任务到线程池中
    for (int i = 0; i < BUFSIZ; ++i) {
        threads_insert(pool, ppt, "你为你负责的项目拼命过吗. 流过泪吗");
        threads_insert(pool, doc, NULL);
    }

    //等待5s 再结束吧
    sh_msleep(5000);
    //清除当前线程池资源, 实战上线程池是常驻内存,不要清除.
    threads_delete(pool);
}
