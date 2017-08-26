#include <scthreads.h>

// 测试开启线程量集
#define _INT_THS (8192)

//全局计时器,存在锁问题
static int _old;

static inline void _run(void * arg) {
	printf("_run arg = %p | %p.\n", _run, arg);
}

//简单的线程打印函数
static inline void _ppt(const char * str) {
	printf("%d => %s\n", ++_old, str);
}

//另一个线程测试函数
static inline void _doc(void * arg) {
	printf("p = %d, 技术不决定项目的成败!我老大哭了\n", ++_old);
}

void test_scthreads(void) {
	//创建线程池
	threads_t pool = threads_create();

	async_run(_run, test_scthreads);

	//添加任务到线程池中
	for (int i = 0; i<_INT_THS; ++i) {
		threads_insert(pool, _ppt, "你为你负责的项目拼命过吗.流过泪吗");
		threads_insert(pool, _doc, NULL);
	}

	//等待5s 再结束吧
	sh_msleep(5000);
	//清除当前线程池资源, 实战上线程池是常驻内存,不要清除.
	threads_delete(pool);
}