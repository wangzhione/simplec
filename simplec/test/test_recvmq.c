#include <recvmq.h>

//
// 测试接受队列
//
void test_recvmq(void) {
	recvmq_t rq = recvmq_create();

	// 构建数据填充进去
	char * str = "simplc";
	uint32_t len = strlen(str) + 1;
	uint32_t x = sh_hton(len);

	// send阶段, 先发送字符长度
	recvmq_push(rq, &x, sizeof(uint32_t));

	char * nstr = recvmq_pop(rq, NULL);
	printf("nstr = %s.\n", nstr);

	// send阶段, 后发送详细数据
	recvmq_push(rq, str, len);

	nstr = recvmq_pop(rq, &x);
	printf("nstr = %s, x = %u.\n", nstr, x);

	free(nstr);
	recvmq_delete(rq);
}