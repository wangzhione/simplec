#include <sctimer.h>

static void timer(void * arg) {
    static int sm;

    stime_t str; stu_getntstr(str);
    printf("%p + %d => %s\n", arg, ++sm, str);
}

// 连环施法
static void timertwo(void * arg) {
    printf("2s after _timer arg = %p.\n", arg);
    timer_add(2000, timer, arg);
}

void test_sctimer(void) {
    int id;

    timer_add(0, timer, 1);
    timer_add(3000, timer, 2);
    timer_add(4000, timer, 3);

    // 开启一个多线程的永久异步方法
    id = timer_add(1000, timer, 4);

    // 等待5秒后关闭 上面永久的定时器事件
    sh_msleep(5000);
    timer_del(id);

    // 测试一个连环施法
    timer_add(1000, timertwo, 5);
}
