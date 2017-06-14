#include <sctimer.h>

static void _timer(void * arg) {
	static int _sm;

	stime_t tstr;
	stu_getntstr(tstr);
	printf("%p + %d => %s\n", arg, ++_sm, tstr);
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

	// 再注册一个方法, 这个是永久执行, 没有被 st_del的话, 将会和系统共存亡
	st_add(100, _timer, (void *)5);
}
