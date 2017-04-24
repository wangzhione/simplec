#include <scpthread.h>

//全局计时器,存在锁问题
static int _old;

//简单的线程打印函数
static void _ppt(const char * str) {
	printf("%d => %s\n", ++_old, str);
}

//另一个线程测试函数
static  void _doc(void* arg) {
	printf("p = %d, 技术不决定项目的成败!我老大哭了\n", ++_old);
}

// 测试开启线程量集
#define _INT_THS (10000)

void test_scpthread(void) {	
	int i;
	//创建线程池
	threadpool_t pool = sp_NEW();
	
	//添加任务到线程池中
	for(i=0; i<_INT_THS; ++i){
		sp_ADD(pool, _ppt, "你为你负责的项目拼命过吗.流过泪吗");
		sp_ADD(pool, _doc, NULL);
	}
	
	//等待5s 再结束吧
	sh_sleep(5000);
	//清除当前线程池资源, 实战上线程池是常驻内存,不要清除.
	sp_del(&pool);
	
#ifdef __GNUC__
	exit(EXIT_SUCCESS);
#endif
}
