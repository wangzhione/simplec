#include <plog.h>
#include <pthread.h>

// 参数值
static int _cnt = 0;

// 需要处理的函数操作, 写数据进去
static void * _write(void * arg) {

	for (int i = 0; i < 1024; ++i) {
		PL_ERROR("_write for cnt = %d.", _cnt++);
		PL_INFO("_write for cnt = %d.", _cnt++);
		PL_ERROR("_write for cnt = %d.", _cnt++);
	}

	sh_sleep(2000);

	PL_ERROR("_write cnt = %d.", _cnt++);
	PL_INFO("_write cnt = %d.", _cnt++);
	PL_ERROR("_write cnt = %d.", _cnt++);

	return arg;
}

//
// test plog 日志系统, 这个系统真的比较复杂. 
// 但愿跑起来带感
//
void test_plog(void) {
	// pthread_t th;
	// 启动 plog 日志系统
	pl_start();

	for (int i = 0; i < 1024; ++i) {
		PL_ERROR("_write for cnt = %d.", _cnt++);
		PL_INFO("_write for cnt = %d.", _cnt++);
		PL_ERROR("_write for cnt = %d.", _cnt++);
	}

	sh_sleep(2000);

	PL_ERROR("_write cnt = %d.", _cnt++);
	PL_INFO("_write cnt = %d.", _cnt++);
	PL_ERROR("_write cnt = %d.", _cnt++);

	// 开启线程, 跑起来测试
	//if (pthread_create(&th, NULL, _write, NULL) < 0) {
	//	cerr_exit("pthread_create is _run error!");
	//}

	//// 开始奔跑等待了
	//pthread_join(th, NULL);

#ifdef __GUNC__
	exit(EXIT_SUCCESS);
#endif
}