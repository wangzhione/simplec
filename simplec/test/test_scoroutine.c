#include <stdio.h>
#include <stdlib.h>
#include <scoroutine.h>

#define _INT_TEST	(5)

struct args {
	int n;
};

static void _foo(scomng_t sco, void * arg) {
	struct args * as = arg;
	int start = as->n;
	int i = -1;

	while (++i < _INT_TEST) {
		printf("coroutine %d : %d.\n", sco_running(sco), start + i);
		sco_yield(sco);
	}
}

static void _test(void * sco) {
	struct args argo = { 000 };
	struct args argt = { 100 };

	int coo = sco_create(sco, _foo, &argo);
	int cot = sco_create(sco, _foo, &argt);

	puts("********************_test start********************");
	while (sco_status(sco, coo) && sco_status(sco, cot)) {
		sco_resume(sco, coo);
		sco_resume(sco, cot);
	}
	puts("********************_test e n d********************");
}

/*
 * 测试主函数, 主要测试协程使用
 * 书写要求
 *	0. [文件名].c
 *	1. void [文件名]()(void) { ... }
 *	2. 在上面函数的最后一行加上 exit(EXIT_SUCCESS);
 */
void test_scoroutine(void) {

	void * sco = sco_open();

	puts("--------------------突然想起了什么,--------------------\n");
	_test(sco);

	// 再来测试一下, 纤程切换问题
	struct args arg = { 222 };
	int co = sco_create(sco, _foo, &arg);
	for (int i = -1; i < _INT_TEST; ++i)
		sco_resume(sco, co);

	puts("\n--------------------笑了笑, 我自己.--------------------");
	sco_close(sco);
}