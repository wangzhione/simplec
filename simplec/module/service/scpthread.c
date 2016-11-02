#include <scpthread.h>
#include <pthread.h>

// 线程任务链表
struct threadjob {
	die_f run;					//当前任务中要执行函数体,注册的事件
	void* arg;					//任务中待执行事件的参数	
	struct threadjob* next;		//指向下一个线程任务链表
};

// struct threadjob 结构对象创建
static inline struct threadjob* _new_threadjob(die_f run, void* arg)
{
	struct threadjob* job = malloc(sizeof(struct threadjob));
	if(!job)
		CERR_EXIT("malloc struct threadjob is NULL!");
	job->run = run;
	job->arg = arg;
	job->next = NULL;
	return job;
}

// 线程结构体,每个线程一个信号量
struct thread {
	pthread_t tid;				//运行的线程id, 在释放的时候用
	pthread_cond_t cond;		//当前线程的条件变量
	struct thread* next;		//下一个线程
};

// 线程池类型定义
struct threadpool {
	int size;					//线程池大小,最大线程数限制
	int curr;					//当前线程池中总的线程数
	int idle;					//当前线程池中空闲的线程数
	pthread_mutex_t mutex;		//线程互斥锁
	struct thread* threads;		//线程条件变量,依赖mutex线程互斥锁
	struct threadjob* head;		//线程任务链表的表头, head + tail就是一个队列结构
	struct threadjob* tail;		//线程任务链表的表尾,这个量是为了后插入的后执行
};

// 添加一个等待的 struct thread 对象到 线程池pool中
static void _thread_add(struct threadpool* pool, pthread_t tid)
{
	struct thread* thread = malloc(sizeof(struct thread));
	if(!thread)
		CERR_EXIT("malloc sizeof(struct thread) is error!");
	thread->tid = tid;
	pthread_cond_init(&thread->cond, NULL);
	thread->next = pool->threads;
	pool->threads = thread;
}

// 依据cnd内存地址属性, 删除pool->threads 中指定数据
static void _thread_del(struct threadpool* pool, pthread_cond_t* cnd)
{
	struct thread* head = pool->threads;
	if(cnd == &head->cond){
		pool->threads = head->next;
		pthread_cond_destroy(&head->cond);
		free(head);
		return;
	}
	// 下面是处理非头结点删除
	while(head->next){
		struct thread* tmp = head->next;
		if(cnd == &tmp->cond){ //找见了,删掉退出
			head->next = tmp->next;
			pthread_cond_destroy(&tmp->cond);
			free(tmp);
			break;
		}
		head = tmp;
	}
}

// 使用了栈内存比较函数,返回对应线程的cond
static pthread_cond_t* _thread_get(struct threadpool* pool, pthread_t tid)
{
	struct thread* head = pool->threads;

	while (head) {
		if (pthread_equal(tid, head->tid))
			break;
		head = head->next;
	}
	return &head->cond;
}

/*
 * 通过这个接口创建线程池对象.后面就可以使用了.
 * max		: 当前线程池中最大的线程个数
 *			: 返回创建好的线程池值.创建失败返回NULL
 */
threadpool_t 
sp_new(int size)
{
	struct threadpool* pool;
	// 错误判断,有点丑陋, 申请内存并初始化
	if((size <= 0) || !(pool = calloc(1, sizeof(struct threadpool)))){
		CERR("struct threadpool calloc is error!");
		return NULL;
	}
	pool->size = size;
	pthread_mutex_init(&pool->mutex, NULL);
	
	return pool;
}

// 线程运行的时候执行函数
static void* _consumer(struct threadpool* pool)
{
	struct threadjob* job;
	int status;
	pthread_t tid = pthread_self();
	pthread_mutex_t* mtx = &pool->mutex;
	pthread_cond_t* cnd;

	//设置线程属性, 默认线程属性 允许退出线程 
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); //设置立即取消 
	pthread_detach(tid); //设置线程分离,自销毁
	
	// 改消费者线程加锁, 并且得到当前线程的条件变量,die_f为了linux上消除警告
	pthread_cleanup_push((die_f)pthread_mutex_unlock, mtx);
	pthread_mutex_lock(mtx);

	cnd = _thread_get(pool, tid);
__loop:
	if(pool->head != NULL) {
		// 有多线程任务,取出数据从下面处理
		job = pool->head;
		pool->head = job->next;
		if(pool->tail == job)
			pool->tail = NULL;
		
		// 解锁, 允许其它消费者线程加锁或生产线程添加新任务
		pthread_mutex_unlock(mtx);
		// 回调函数,后面再去删除这个任务
		job->run(job->arg);
		free(job);
		
		// 新的一轮开始需要重新加锁
		pthread_mutex_lock(mtx);
		goto __loop;
	}
	// 这里相当于 if 的 else, pool->first == NULL
	++pool->idle;
	// 调用pthread_cond_wait 等待线程条件变量被通知且自动解锁
	status = pthread_cond_wait(cnd, mtx);
	--pool->idle;
	if(status == 0) //等待成功了,那就开始轮序处理任务
		goto __loop;
	
	//到这里是程序出现异常, 进程退出中, 先减少当前线程
	--pool->curr;
	//去掉这个线程链表pool->threads中对应数据
	_thread_del(pool, cnd);

	pthread_mutex_unlock(mtx);
	pthread_cleanup_pop(0);

	return NULL;
}

/*
 * 在当前线程池中添加待处理的线程对象.
 * pool		: 线程池对象, sp_new 创建的那个
 * run		: 运行的函数体, 返回值void, 参数void*
 * arg		: 传入运行的参数
 *			: 不需要返回值
 */
void 
sp_add(threadpool_t pool, die_f run, void* arg)
{
	struct threadjob* job = _new_threadjob(run, arg);
	pthread_mutex_t* mtx = &pool->mutex;
	
	pthread_mutex_lock(mtx);
	if(!pool->head) //线程池中没有线程头,那就设置线程头
		pool->head = job;
	else
		pool->tail->next = job;
	pool->tail = job;
	
	// 有空闲线程,添加到处理任务队列中,直接返回
	if(pool->idle > 0){
		pthread_mutex_unlock(mtx);
		// 这是一种算法, 先释放锁后发送信号激活线程,速度快,缺点丧失线程执行优先级
		pthread_cond_signal(&pool->threads->cond);
	}
	else if(pool->curr < pool->size){ // 没有那就新建线程, 条件不满足那就等待
		pthread_t tid;
		if(pthread_create(&tid, NULL, (void* (*)(void*))_consumer, pool) == 0)
			++pool->curr;
		//添加开启线程的信息
		_thread_add(pool, tid);
		pthread_mutex_unlock(mtx);
	}
}

/*
 * 销毁当前线程池,释放内存,并尝试停止线程池中线程.
 * ppopl		: 指向 sp_new创建的对象的指针
 *				: 没有返回值
 */
void 
sp_del(threadpool_t* ppool)
{
	struct threadpool* pool;
	struct thread* thread;
	struct threadjob* head;
	if((!ppool) || !(pool = *ppool)) return;
	
	//加锁,等待完全占有锁的时候再去释放资源
	pthread_mutex_lock(&pool->mutex);
	
	//先释放线程
	thread = pool->threads;
	while(thread){
		struct thread* next = thread->next;
		pthread_cancel(thread->tid);
		pthread_cond_destroy(&thread->cond);
		free(thread);
		thread = next;
	}
	//再来释放任务列表
	head = pool->head;
	while(head) {
		struct threadjob* next = head->next;
		free(head);
		head = next;
	}
	pthread_mutex_unlock(&pool->mutex);
	
	//最后要销毁这个使用的线程锁对象
	pthread_mutex_destroy(&pool->mutex);	
	*ppool = NULL;
}