#include "scthreads.h"

// struct job 任务链表 结构 和 构造
struct job {
    struct job * next;      // 指向下一个任务节点
    node_f frun;            // 任务节点执行的函数体
    void * arg;
};

inline struct job * job_new(node_f frun, void * arg) {
    struct job * job = malloc(sizeof(struct job));
    job->next = NULL;
    job->frun = frun;
    job->arg = arg;
    return job;
}

// struct thread 线程结构体, 每个线程一个信号量, 定点触发
struct thread {
    struct thread * next;   // 下一个线程对象
    pthread_cond_t cond;    // 线程条件变量
    volatile bool wait;     // true 表示当前线程被挂起
    pthread_t id;           // 当前线程 id
};

// 定义线程池(线程集)定义
struct threads {
    int size;               // 线程池大小, 线程体最大数量
    int curr;               // 当前线程池中总的线程数
    int idle;               // 当前线程池中空闲的线程数
    struct thread * thrs;   // 线程结构体对象集
    pthread_mutex_t mutx;   // 线程互斥量
    volatile bool cancel;   // true 表示当前线程池正在 delete
    struct job * head;      // 线程任务链表的链头, 队列结构
    struct job * tail;      // 线程任务队列的表尾, 后进后出
};

// threads_add - 线程池中添加线程
static void threads_add(struct threads * pool, pthread_t id) {
    struct thread * thrd = malloc(sizeof(struct thread));
    thrd->next = pool->thrs;
    thrd->cond = PTHREAD_COND_INITIALIZER;
    thrd->wait = false;
    thrd->id = id;

    pool->thrs = thrd;
    ++pool->curr;
}

// threads_del - 根据 cond 内存地址删除 pool->thrs 中指定数据
static void threads_del(struct threads * pool, pthread_cond_t * cond) {
    struct thread * head = pool->thrs, * prev = NULL;
    while (head) {
        // 找见了, 否则开始记录前置位置
        if (cond == &head->cond) {
            if (prev)
                prev->next = head->next;
            else
                pool->thrs = head->next;
            return free(head);
        }
        prev = head;
        head = head->next;
    }
}

// threads_get - 找到线程 id 对应的条件变量地址
static struct thread * threads_get(struct threads * pool, pthread_t id) {
    struct thread * head = pool->thrs;
    while (head) {
        if (pthread_equal(id, head->id))
            break;
        head = head->next;
    }
    return head;
}

// threads_cond - 找到空闲的线程, 并返回其信号量 
static pthread_cond_t * threads_cond(struct threads * pool) {
    struct thread * head = pool->thrs;
    while (head) {
        if (head->wait)
            return &head->cond;
        head = head->next;
    }
    return NULL;
}

// THREADS_INT - 开启的线程数是 2 * CPU
#define THREADS_INT        (8)

//
// threads_create - 创建线程池对象
// return   : 创建的线程池对象, NULL 表示失败
//
inline threads_t 
threads_create(void) {
    struct threads * pool = calloc(1, sizeof(struct threads));
    pool->size = THREADS_INT;
    pool->mutx = PTHREAD_MUTEX_INITIALIZER;
    return pool;
}

//
// threads_delete - 异步销毁线程池对象
// pool     : 线程池对象
// return   : void
//
void 
threads_delete(threads_t pool) {
    if (!pool || pool->cancel)
        return;

    // 已经在销毁过程中, 直接返回
    pthread_mutex_lock(&pool->mutx);
    if (pool->cancel) {
        pthread_mutex_unlock(&pool->mutx);
        return;
    }

    // 标识当前线程池正在销毁过程中, 并随后释放任务列表
    pool->cancel = true;
    struct job * head = pool->head;
    while (head) {
        struct job * next = head->next;
        free(head);
        head = next;
    }
    pool->head = pool->tail = NULL;
    pthread_mutex_unlock(&pool->mutx);

    // 再来销毁每个线程
    struct thread * thrs = pool->thrs;
    while (thrs) {
        struct thread * next = thrs->next;
        // 激活每个线程让其主动退出
        pthread_cond_signal(&thrs->cond);
        pthread_join(thrs->id, NULL);
        thrs = next;
    }

    // 销毁自己
    free(pool);
}

// thread_consumer - 消费线程
static void thread_consumer(struct threads * pool) {
    pthread_t id = pthread_self();
    pthread_mutex_t * mutx = &pool->mutx;

    pthread_mutex_lock(mutx);

    struct thread * thrd = threads_get(pool, id);
    assert(thrd);
    pthread_cond_t * cond = &thrd->cond;

    // 使劲循环的主体, 开始消费 or 沉睡
    while (!pool->cancel) {
        if (pool->head) {
            struct job * job = pool->head;
            pool->head = job->next;
            // 队列尾置空监测
            if (pool->tail == job)
                pool->tail = NULL;

            // 解锁, 允许其它消费者线程加锁或生产者添加新任务
            pthread_mutex_unlock(mutx);

            // 回调函数, 后面再去删除任务
            job->frun(job->arg);
            free(job);

            // 新的一轮开始, 需要重新加锁
            pthread_mutex_lock(mutx);
            continue;
        }

        // job 已经为 empty , 开启线程等待
        thrd->wait = true;
        ++pool->idle;

        // 开启等待, 直到线程被激活
        int status = pthread_cond_wait(cond, mutx);
        if (status < 0) {
            pthread_detach(id);
            CERR("pthread_cond_wait error status = %d.", status);
            break;
        }
        thrd->wait = false;
        --pool->idle;
    }

    // 到这里程序出现异常, 线程退出中, 先减少当前线程
    --pool->curr;
    // 去掉这个线程链表 pool->thrs 中对应的数据
    threads_del(pool, cond);

    // 所有线程共用同一把任务锁
    pthread_mutex_unlock(mutx);
}

//
// threads_insert - 线程池中添加待处理的任务
// pool     : 线程池对象
// frun     : node_f 运行的执行体
// arg      : frun 的参数
// return   : void
//
void 
threads_insert(threads_t pool, void * frun, void * arg) {
    if (!frun || !pool || pool->cancel)
        return;

    struct job * job = job_new(frun, arg);
    pthread_mutex_t * mutx = &pool->mutx;

    pthread_mutex_lock(mutx);

    // 线程池中任务队列的插入任务
    if (!pool->head)
        pool->head = job;
    else
        pool->tail->next = job;
    pool->tail = job;

    // 构建线程, 构建完毕直接获取
    if (pool->idle > 0) {
        pthread_cond_t * cond = threads_cond(pool);
        // 先释放锁后发送信号激活线程, 速度快, 缺点丧失线程执行优先级
        pthread_mutex_unlock(mutx);
        // 发送给空闲的线程, 这个信号量一定存在
        pthread_cond_signal(cond);
        return;
    }

    if (pool->curr < pool->size) { // 没有, 那就新建线程去处理
        pthread_t id;
        if (pthread_create(&id, NULL, (start_f)thread_consumer, pool))
            CERR("pthread_create error curr = %zu.", pool->curr);
        else // 添加开启线程的信息
            threads_add(pool, id);
    }

    pthread_mutex_unlock(mutx);
}
