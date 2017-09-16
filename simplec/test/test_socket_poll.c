#include <socket_poll.h>

#define _USHORT_PORT	(8088u)

//
// 参照云风的思路自己设计了 window 部分代码
// https://github.com/cloudwu/skynet/blob/master/skynet-src/socket_poll.h
//
void test_socket_poll(void) {
    int n;
    poll_t poll;
    socket_t sock;
    struct event evs[FD_SETSIZE];

    // 开始构建一个 socket
    sock = socket_tcp(NULL, _USHORT_PORT);
    if (sock == INVALID_SOCKET)
	    return;

    poll = sp_create();
    assert(!sp_invalid(poll));

    if (sp_add(poll, sock, NULL))
	    CERR("sp_add sock = is error!");
    else {
	    // 开始等待数据
	    printf("sp_wait [127.0.0.1:%hu] listen ... \n", _USHORT_PORT);
	    n = sp_wait(poll, evs, LEN(evs));
	    printf("sp_wait n = %d. 一切都是那么意外!\n", n);
    }

    sp_delete(poll);
    socket_close(sock);
}