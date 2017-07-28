#include <recvmq.h>

//
// 测试接受队列
//
void test_recvmq(void) {
	struct recvmsg msg = { 0 };
	recvmq_t rq = recvmq_create();

	// 构建数据填充进去
	char * str = "simplc";
	uint32_t len = (int)strlen(str) + 1;
	uint32_t x = RECVMSG_SZ(1, len);
	x = sh_hton(x);

	// send阶段, 先发送字符长度
	recvmq_push(rq, &x, sizeof(uint32_t));

	int rt = recvmq_pop(rq, &msg);
	printf("rt = %d, type = %d, sz = %d, data = %s.\n", 
		rt, RECVMSG_TYPE(msg.sz), RECVMSG_LEN(msg.sz), (char *)msg.data);

	// send阶段, 后发送详细数据
	recvmq_push(rq, str, len);

	rt = recvmq_pop(rq, &msg);
	printf("rt = %d, type = %d, sz = %d, data = %s.\n",
		rt, RECVMSG_TYPE(msg.sz), RECVMSG_LEN(msg.sz), (char *)msg.data);

	free(msg.data);
	recvmq_delete(rq);
}