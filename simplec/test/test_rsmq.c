#include <rsmq.h>

//
// 测试接受队列
//
void test_rsmq(void) {
	int rt;
	msgrs_t msg = NULL;
	rsmq_t rq = rsmq_create();

	// 构建数据填充进去
	char * str = "simplc";
	uint32_t len = (int)strlen(str) + 1;
	uint32_t x = MSGRS_SZ(1, len);
	x = sh_hton(x);

	// send阶段, 先发送字符长度
	rsmq_push(rq, &x, sizeof(uint32_t));
	rt = rsmq_pop(rq, &msg);
	if (rt == SufBase)
		printf("rt = %d, type = %d, sz = %d, data = %s.\n", 
			rt, MSGRS_TYPE(msg->sz), MSGRS_LEN(msg->sz), (char *)msg->data);

	// send阶段, 后发送详细数据
	rsmq_push(rq, str, len);
	rt = rsmq_pop(rq, &msg);
	printf("rt = %d, type = %d, sz = %d, data = %s.\n",
		rt, MSGRS_TYPE(msg->sz), MSGRS_LEN(msg->sz), (char *)msg->data);
	msgrs_delete(msg);

	// 开始测试客户端给服务器发送消息
	msgrs_t m = msgrs_create(str, len);
	rsmq_push(rq, m->data, m->sz);
	rt = rsmq_pop(rq, &msg);
	printf("rt = %d, type = %d, sz = %d, data = %s.\n",
		rt, MSGRS_TYPE(msg->sz), MSGRS_LEN(msg->sz), (char *)msg->data);
	msgrs_delete(m);

	rsmq_delete(rq);
}