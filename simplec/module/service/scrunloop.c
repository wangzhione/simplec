#include "mq.h"
#include "scrunloop.h"

// 具体轮询器
struct srl {
    mq_t q;                 // 消息队列
    node_f frun;            // 每个消息都会调用 run(pop())
    node_f fdie;            // 每个消息体的善后工作
    sem_t block;            // 线程阻塞用的信号量
    pthread_t id;           // 具体奔跑的线程
    volatile bool loop;     // true表示还在继续 
    volatile bool wait;     // true表示当前轮序器正在等待
};

//
// srl_delete - 销毁轮询对象,回收资源
// s        : 轮询对象
// return   : void 
//
inline void
srl_delete(srl_t s) {
    if (s) {
        s->loop = false;
        sem_post(&s->block);
        // 等待线程结束, 然后退出
        pthread_join(s->id, NULL);
        sem_destroy(&s->block);
        mq_delete(s->q, s->fdie);
        free(s);
    }
}

//
// srl_push - 将消息压入到轮询器中
// s        : 轮询对象
// msg      : 待加入的消息地址
// return   : void
// 
inline void 
srl_push(srl_t s, void * msg) {
    assert(s && msg);
    mq_push(s->q, msg);
    if (s->wait) {
        s->wait = false;
        sem_post(&s->block);
    }
}

static void srl_run(struct srl * s) {
    while (s->loop) {
        void * pop = mq_pop(s->q);
        if (NULL == pop) {
            s->wait = true;
            sem_wait(&s->block);
        } else {
            // 开始处理消息
            s->frun(pop);
            s->fdie(pop);
        }
    }
}

//
// srl_create - 创建轮询服务对象
// frun     : 轮序处理每条消息体, 弹出消息体的时候执行
// fdie     : srl_push msg 销毁函数
// return   : srl_t 轮询器对象 
//
inline srl_t
srl_create_(node_f frun, node_f fdie) {
    struct srl * s = malloc(sizeof(struct srl));
    assert(s && frun);
    s->q = mq_create();
    s->frun = frun;
    s->fdie = fdie;
    s->loop = true;
    s->wait = true;
    // 初始化 POSIX 信号量, 进程内线程共享, 初始值 0
    sem_init(&s->block, 0, 0);
    // 创建线程,并启动
    if (pthread_create(&s->id, NULL, (start_f)srl_run, s)) {
        mq_delete(s->q, fdie);
        free(s);
        RETNUL("pthread_create create error !!!");
    }
    return s;
}
