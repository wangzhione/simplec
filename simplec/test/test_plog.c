#include <plog.h>
#include <pthread.h>

#define _INT_TEST		(1024)

// 参数值
static int _cnt;

// 需要处理的函数操作, 写数据进去
static void * _write(void * arg) {

	for (int i = 0; i < _INT_TEST; ++i) {
		PL_ERROR("_write for cnt = %d.", _cnt++);
		PL_INFOS("_write for cnt = %d.", _cnt++);
		PL_DEBUG("_write for cnt = %d.", _cnt++);
	}

	sh_msleep(_INT_TEST);

	PL_ERROR("_write cnt = %d.", _cnt++);
	PL_INFOS("_write cnt = %d.", _cnt++);
	PL_DEBUG("_write cnt = %d.", _cnt++);

	return arg;
}

//
// test plog 日志系统, 这个系统真的比较复杂. 
// 但愿跑起来带感
//
void test_plog(void) {
	pthread_t th;
	// 启动 plog 日志系统
	pl_start();

	PL_ERROR("_write cnt = %d.", _cnt++);
	PL_INFOS("_write cnt = %d.", _cnt++);
	PL_DEBUG("_write cnt = %d.", _cnt++);

	// 开启线程, 跑起来测试
	if (pthread_create(&th, NULL, _write, NULL) < 0) {
		CERR_EXIT("pthread_create is _run error!");
	}

	for (int i = 0; i < _INT_TEST; ++i) {
		PL_ERROR("_write for cnt = %d.", _cnt++);
		PL_INFOS("_write for cnt = %d.", _cnt++);
		PL_DEBUG("_write for cnt = %d.", _cnt++);
	}

	// 开始奔跑等待了
	pthread_join(th, NULL);
}