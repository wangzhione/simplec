#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

// 测试线程数量
#define _INT_THS	(3)

struct threadarg {
	sem_t sids[_INT_THS];
	pthread_t tids[_INT_THS];
};

// 简单运行函数
static void * _run(void * arg) {
	int i = -1, j = -1;
	struct threadarg * ths = arg;
	pthread_t tid = pthread_self();
	pthread_detach(tid);

	// 确定这是第几个线程
	while (++i < _INT_THS)
		if (pthread_equal(tid, ths->tids[i]))
			break;
	
	// 循环个特定遍数就结束吧
	while (++j < _INT_THS) {
		// 第 i 个线程, 等待 第 i - 1 个线程, 输出 'A' + i 
		sem_wait(ths->sids + (i - 1 + _INT_THS) % _INT_THS);
		putchar('A' + i);
		// 第 i 个线程, 激活 第 i 个信号量
		sem_post(ths->sids + i);
	}

	return NULL;
}

//
// 写个测试线程信号量代码
// 开启 _INT_THS 个线程, 顺序打印数据 A->B->C->...->A->B->....
//
void test_semaphore(void) {
	// 开始初始化了
	int i, j;
	struct threadarg targ;
	
	// 先初始化信号量,后初始化线程
	for (i = 0; i < _INT_THS; ++i) {
		// 0 : 表示局部信号量当前可用; 0 : 当前信号量值为0
		if (sem_init(targ.sids + i, 0, 0) < 0)
			goto __faild;
	}

	// 开启线程
	for (j = 0; j < _INT_THS; ++j) {
		// 开启三个线程
		if (pthread_create(targ.tids + j, NULL, _run, &targ) < 0)
			goto __faild;
	}

	// 激活第一个线程, 输出 'A' 开头
	sem_post(targ.sids + _INT_THS - 1);

	// 中间等待一些时间吧
	getchar();

__faild:
	// 注意的是, 假如信号量释放了, 线程还在跑, 会异常
	for (j = 0; j < i; ++j)
		sem_destroy(targ.sids + j);
}