#include <socket_poll.h>

#define _STR_IPS	"127.0.0.1"
#define _INT_PORT	(8088)

//
// 参照云风的思路自己设计了 window 部分代码
// https://github.com/cloudwu/skynet/blob/master/skynet-src/socket_poll.h
//
void test_poll(void) {
	int n;
	poll_t poll;
	socket_t sock;
	struct event evs[FD_SETSIZE];

	// 开始构建一个 socket
	sock = socket_tcp(_STR_IPS, _INT_PORT);
	if (sock == INVALID_SOCKET)
		return;

	poll = sp_create();
	assert(!sp_invalid(poll));

	if (sp_add(poll, sock, NULL)) {
		CERR("sp_add sock = is error!");
		goto __close;
	}

	// 开始等待数据
	printf("sp_wait [%s:%d] connect ... \n", _STR_IPS, _INT_PORT);
	n = sp_wait(poll, evs, LEN(evs));
	printf("sp_wait n = %d. 一切都是那么意外!\n", n);

__close:
	socket_close(sock);
	sp_delete(poll);
}