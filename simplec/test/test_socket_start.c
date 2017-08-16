#include <socket_start.h>

static void _run(msgrs_t msg) {
	// 完整的包直接抛到消息队列中
	// ...
	// 
	uint8_t type = MSGRS_TYPE(msg->sz);
	uint32_t len = MSGRS_LEN(msg->sz);
	// 构建报文抛出去

	if (type == 0) {
		// infos msg

	} else {
		// check msg

	}

	//
	// ... 简单测试 ...
	//
	printf("type = %d, len = %d, data = ", type, len);
	for (uint32_t i = 0; i < len; ++i)
		putchar(msg->data[i]);
	putchar('\n');
	//
	// ... 业务而定 ...
	//
}

#define _STR_HOST		"127.0.0.1"
#define _USHORT_PORT	(8088)

void test_socket_start(void) {
	cl_start("simplec.log");

	// 开启一个socket双线程服务, 进行监听
	ss_run(_STR_HOST, _USHORT_PORT, _run);

	// 这里给上面socket发送消息
	printf("%s:%hu run ...\n", _STR_HOST, _USHORT_PORT);
	sh_msleep(1000);

	// 开始发送消息了, 先链接, 后按照协议发送
	socket_t cs = socket_connectos(_STR_HOST, _USHORT_PORT, 3000);
	if (cs == INVALID_SOCKET) {
		ss_end();
		RETURN(NIL, "socket_connectos %s:%hu is error!", _STR_HOST, _USHORT_PORT);
	}

	// 构建一个通信的消息包发送过去
	const char * data = "你好吗. 还行.. 那就好... by simplec ";
	uint32_t sz = (uint32_t)strlen(data);
	msgrs_t msg = msgrs_create(data, sz);
	socket_sendn(cs, msg->data, msg->sz);
	msgrs_delete(msg);

	sh_msleep(2000);

	ss_end();
	
	// 关闭打开的socket
	socket_close(cs);
}