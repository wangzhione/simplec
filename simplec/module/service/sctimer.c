#include "scatom.h"
#include "sctimer.h"
#include <pthread.h>

//
// pthread_async - 异步启动分离线程
// frun     : 运行的主体
// arg      : 运行参数
// return   : return 0 is success
// 
#define pthread_async(frun, arg)                                    \
pthread_async_((node_f)(frun), (void *)(intptr_t)(arg))
inline int pthread_async_(node_f frun, void * arg) {
    pthread_t id;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int ret = pthread_create(&id, &attr, (start_f)frun, arg);
    pthread_attr_destroy(&attr);
    return ret;
}

// timer_node 定时器结点
struct timer_node {
    $LIST

    int id;            // 定时器 id
    void * arg;        // 执行函数参数
    node_f ftimer;     // 执行的函数事件
    struct timespec t; // 运行的具体时间
};

// timer_node id compare
inline static int timer_node_cmp_id(int id, 
                                    const struct timer_node * r) {
    return id - r->id;
}

// timer_node time compare 比较
inline static int timer_node_cmp_time(const struct timer_node * l, 
                                      const struct timer_node * r) {
    if (l->t.tv_sec != r->t.tv_sec)
        return (int)(l->t.tv_sec - r->t.tv_sec);
    return (int)(l->t.tv_nsec - r->t.tv_nsec);
}

// timer_list 链表对象管理器
struct timer_list {
    int id;                     // 当前 timer node id
    int lock;                   // 自旋锁
    bool status;                // true is thread loop, false is stop
    struct timer_node * list;   // timer list list
};

// timer_list_sus - 得到等待的微秒事件, <= 0 表示可以执行
inline int timer_list_sus(struct timer_list * list) {
    struct timespec * v = &list->list->t, t[1];
    timespec_get(t, TIME_UTC);
    return (int)((v->tv_sec - t->tv_sec) * 1000000 + 
        (v->tv_nsec - t->tv_nsec) / 1000);
}

// timer_list_run - 线程安全, 需要再 loop 之后调用
inline void timer_list_run(struct timer_list * list) {
    struct timer_node * node;
    ATOM_LOCK(list->lock);
    node = list->list;
    list->list = list_next(node);
    ATOM_UNLOCK(list->lock);

    node->ftimer(node->arg);
    free(node);
}

// 定时器管理单例对象
static struct timer_list timer;

//
// timer_del - 删除定时器事件
// id       : 定时器 id
// return   : void
//
inline void 
timer_del(int id) {
    if (timer.list) {
        ATOM_LOCK(timer.lock);
        free(list_pop(timer.list, timer_node_cmp_id, id));
        ATOM_UNLOCK(timer.lock);
    }
}

// timer_node_new - timer_node 定时器结点构建
inline static struct timer_node * timer_node_new(int s, node_f ftimer, void * arg) {
    struct timer_node * node = malloc(sizeof(struct timer_node));
    node->id = ATOM_INC(timer.id);
    node->arg = arg;
    node->ftimer = ftimer;
    timespec_get(&node->t, TIME_UTC);
    node->t.tv_sec += s / 1000;
    // nano second
    node->t.tv_nsec += (s % 1000) * 1000000;
    return node;
}

// 运行的主 loop, 基于 timer 管理器 
static void timer_run(struct timer_list * list) {
    // 正常轮循, 检查时间
    while (list->list) {
        int sus = timer_list_sus(list);
        if (sus > 0) {
            usleep(sus);
            continue;
        }

        timer_list_run(list);
    }

    // 已经运行结束
    list->status = false;
}

//
// timer_add - 添加定时器事件
// tvl      : 执行间隔(毫秒), <= 0 表示立即执行
// ftimer   : 定时器行为
// arg      : 定时器参数
// return   : 返回定时器 id
//
int 
timer_add_(int tvl, node_f ftimer, void * arg) {
    int id;
    struct timer_node * node;
    if (tvl <= 0) {
        ftimer(arg);
        return 0;
    }

    node = timer_node_new(tvl, ftimer, arg);
    id = node->id;
    ATOM_LOCK(timer.lock);

    list_add(timer.list, timer_node_cmp_time, node);

    // 判断是否需要开启新的线程
    if (!timer.status) {
        if (!pthread_async(timer_run, &timer))
            timer.status = true;
        else {
            free(node);
            id = -1;
        } 
    }

    ATOM_UNLOCK(timer.lock);
    return id;
}
