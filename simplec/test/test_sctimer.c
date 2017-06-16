#include <sctimer.h>

static void _timer(void * arg) {
	static int _sm;

	stime_t tstr;
	stu_getntstr(tstr);
	printf("%p + %d => %s\n", arg, ++_sm, tstr);
}

// 连环施法
static void _timertwo(void * arg) {
	printf("2s after _timer arg = %p.\n", arg);

	st_add(2000, _timer, arg);
}

void test_sctimer(void) {
	int tid;
	
	st_add(0, _timer, (void *)1);
	st_add(3000, _timer, (void *)2);
	st_add(4000, _timer, (void *)3);

	// 开启一个多线程的永久异步方法
	tid = st_add(1000, _timer, (void *)4);

	// 等待5秒后关闭 上面永久的定时器事件
	sh_sleep(5000);
	st_del(tid);

	// 测试一个连环施法
	st_add(1000, _timertwo, (void *)5);
}
