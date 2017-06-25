#include <sclog.h>
#include <pthread.h>

static void * test_one(void * arg) {
	sl_init("test_one", "8.8.8.8");

	SL_TRACE("test_one log test start!");
	for (int i = 0; i < 100; ++i) {
		SL_FATAL("pthread test one fatal is at %d, It's %s.",i, "OK");
		SL_WARNG("pthread test one warning is at %d, It's %s.", i, "OK");
		SL_INFOS("pthread test one info is at %d, It's %s.", i, "OK");
		SL_DEBUG("pthread test one debug is at %d, It's %s.", i, "OK");

		sh_msleep(2); //等待2ms
	}
	SL_TRACE("test_one log test end!");

	return NULL;
}

// 线程二测试函数
static void * test_two(void * arg) {
	sl_init("test_two", "8.8.8.8");
	//线程分离,自回收
	pthread_detach(pthread_self());

	SL_TRACE("test_two log test start!");
	for (int i = 0; i < 3; ++i) {
		SL_FATAL("pthread test two fatal is at %d, It's %s.", i, "OK");
		SL_WARNG("pthread test two warning is at %d, It's %s.", i, "OK");
		SL_INFOS("pthread test two info is at %d, It's %s.", i, "OK");
		SL_DEBUG("pthread test two debug is at %d, It's %s.", i, "OK");

		sh_msleep(1000); //等待1s
	}
	SL_TRACE("test_two SL_TRACE test end!");

	return NULL;
}

void test_sclog(void) {
	pthread_t tone, ttwo;

	sl_start();
	SL_NOTIE("main log test start!");

	pthread_create(&tone, NULL, test_one, NULL);
	pthread_create(&ttwo, NULL, test_two, NULL);

	pthread_join(tone, NULL);

	SL_NOTIE("main log test end!");
}
