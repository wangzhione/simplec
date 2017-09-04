#include <scthreads.h>

// 任务链表 结构 和 构造
struct job {
    struct job * next;          // 指向下一个任务结点
    node_f run;                 // 任务结点执行的函数体
    void * arg;	
};

static inline struct job * _job_new(node_f run, void * arg) {
	struct job * job = malloc(sizeof(struct job));
	if (NULL == job)
		CERR_EXIT("malloc sizeof(struct job) is error!");
	job->next = NULL;
	job->run = run;
	job->arg = arg;
	return job;
}

// 线程结构体, 每个线程一个信号量, 定点触发
struct thread {
    struct thread * next;       // 下一个线程对象
    bool wait;                  // true 表示当前线程被挂起
    pthread_t tid;              // 当前线程id
    pthread_cond_t cond;        // 线程条件变量
};

// 定义线程池(线程集)定义
struct threads {
    size_t size;                // 线程池大小, 最大线程结构体数量
    size_t curr;                // 当前线程池中总的线程数
    size_t idle;                // 当前线程池中空闲的线程数
    volatile bool cancel;       // true表示当前线程池正在 delete
    pthread_mutex_t mutx;       // 线程互斥量
    struct thread * thrs;       // 线程结构体对象集
    struct job * head;          // 线程任务链表的链头, 队列结构
    struct job * tail;          // 线程任务队列的表尾, 后插入后执行
};

// 线程池中添加线程集
static void _threads_add(struct threads * pool, pthread_t tid) {
	struct thread * thrd = malloc(sizeof(struct thread));
	if (NULL == thrd)
		CERR_EXIT("malloc sizeof(struct thread) error!");
	
	thrd->wait = false;
	thrd->tid = tid;
	pthread_cond_init(&thrd->cond, NULL);

	thrd->next = pool->thrs;
	pool->thrs = thrd;

	++pool->curr;
}

// 根据cond 内存地址熟悉, 删除pool->thrs 中指定数据
static void _threads_del(struct threads * pool, pthread_cond_t * cond) {
	struct thread * head = pool->thrs, * front = NULL;
	
	while (head) {
		// 找见了, 否则开始记录前置位置
		if (cond == &head->cond)
			break;
		front = head;
		head = head->next;
	}

	if (head) {
		if (front)
			front->next = head->next;
		else
			pool->thrs = head->next;

		// head 结点就是我们要找的
		pthread_cond_destroy(&head->cond);
		free(head);
	}
}

// 找到线程tid 对应的条件变量地址
static struct thread * _threads_get(struct threads * pool, pthread_t tid) {
	struct thread * head = pool->thrs;
	while (head) {
		if (pthread_equal(tid, head->tid))
			return head;
		head = head->next;
	}
	return NULL;
}

// 找到空闲的线程, 并返回起信号量 
static pthread_cond_t * _threads_getcont(struct threads * pool) {
	struct thread * head = pool->thrs;
	while (head) {
		if (head->wait)
			return &head->cond;
		head = head->next;
	}
	return NULL;
}

// 开启的线程数
#define _UINT_THREADS		(4u)

//
// threads_create - 创建一个线程池处理对象
// return	: 返回创建好的线程池对象, NULL表示失败
//
threads_t 
threads_create(void) {
	struct threads * pool = calloc(1, sizeof(struct threads));
	if (NULL == pool) {
		RETURN(NULL, "calloc sizeof(struct threads) is error!");
	}

	pool->size = _UINT_THREADS;
	pthread_mutex_init(&pool->mutx, NULL);

	return pool;
}

//
// threads_delete - 异步销毁一个线程池对象
// pool		: 线程池对象
// return	: void
//
void 
threads_delete(threads_t pool) {
	struct job * head;
	struct thread * thrs;

	if (!pool || pool->cancel) 
		return;
	// 标识当前线程池正在销毁过程中
	pool->cancel = true;

	pthread_mutex_lock(&pool->mutx);

	// 先来释放任务列表
	head = pool->head;
	while (head) {
		struct job * next = head->next;
		free(head);
		head = next;
	}
	pool->head = pool->tail = NULL;

	pthread_mutex_unlock(&pool->mutx);

	// 再来销毁每个线程
	thrs = pool->thrs;
	while (thrs) {
		struct thread * next = thrs->next;
		// 激活每个线程让其主动退出
		pthread_cond_signal(&thrs->cond);
		pthread_join(thrs->tid, NULL);
		thrs = next;
	}

	// 销毁自己
	pthread_mutex_destroy(&pool->mutx);
	free(pool);
}

// 线程运行的时候执行函数, 消费者线程
static void _consumer(struct threads * pool) {
	int status;
	struct thread * thrd;
	pthread_cond_t * cond;
	pthread_t tid = pthread_self();
	pthread_mutex_t * mutx = &pool->mutx;

	pthread_mutex_lock(mutx);

	thrd = _threads_get(pool, tid);
	assert(thrd);
	cond = &thrd->cond;

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
			job->run(job->arg);
			free(job);

			// 新的一轮开始, 需要重新加锁
			pthread_mutex_lock(mutx);
			continue;
		}

		// job 已经为empty , 开启线程等待
		thrd->wait = true;
		++pool->idle;

		// 开启等待, 直到线程被激活
		status = pthread_cond_wait(cond, mutx);
		if (status < 0) {
			pthread_detach(tid);
			CERR("pthread_cond_wait error status = %d.", status);
			break;
		}
		thrd->wait = false;
		--pool->idle;
	}

	// 到这里程序出现异常, 线程退出中, 先减少当前线程
	--pool->curr;
	// 去掉这个线程链表pool->thrs中对应的数据
	_threads_del(pool, cond);

	// 一个线程一把锁, 退出中
	pthread_mutex_unlock(mutx);
}

//
// threads_insert_ - 线程池中添加要处理的任务
// pool		: 线程池对象
// run		: 运行的执行题
// arg		: run的参数
// return	: void
//
void 
threads_insert_(threads_t pool, node_f run, void * arg) {
	struct job * job;
	pthread_mutex_t * mutx;
	// 无意义的添加直接滚蛋
	if (!run || !pool || pool->cancel)
		return;
	mutx = &pool->mutx;
	job = _job_new(run, arg);

	pthread_mutex_lock(mutx);

	// 处理线程池中任务队列的插入
	if (!pool->head)
		pool->head = job;
	else
		pool->tail->next = job;
	pool->tail = job;

	// 构建线程, 构建完毕直接获取
	if (pool->idle > 0) {
		pthread_cond_t * cond = _threads_getcont(pool);
		// 先释放锁后发送信号激活线程, 速度快, 缺点丧失线程执行优先级
		pthread_mutex_unlock(mutx);
		// 发送给空闲的线程, 这个信号量一定存在
		pthread_cond_signal(cond);
		return;
	}

	if (pool->curr < pool->size) { // 没有那就新建线程去处理
		pthread_t tid;
		if (pthread_create(&tid, NULL, (start_f)_consumer, pool))
			CERR("pthread_create error curr = %zu.", pool->curr);
		else // 添加开启线程的信息
			_threads_add(pool, tid);
	}

	pthread_mutex_unlock(mutx);
}