#include <scpipe.h>

#ifdef __GNUC__

struct scpipe {
	int recv;
	int send;
};

scpipe_t 
scpipe_create(void) {
	scpipe_t spie = malloc(sizeof(struct scpipe));
	assert(NULL != spie);

	if (pipe((int *)spie) < 0) {
		free(spie);
		RETURN(NULL, "pipe is error!");
	}

	return spie;
}


void 
scpipe_delete(scpipe_t spie) {
	if (spie) {
		close(spie->recv);
		close(spie->send);
		free(spie);
	}
}

int 
scpipe_recv(scpipe_t spie, void * data, size_t len) {
	ssize_t lrecv = read(spie->recv, data, len);
	if (lrecv < 0) {
		// 不考虑信号中断
		RETURN(ErrBase, "read is error = %ld", lrecv);
	}
	return (int)lrecv;
}

int 
scpipe_send(scpipe_t spie, const void * data, size_t len) {
	ssize_t lsend = write(spie->send, data, len);
	if (lsend < 0) {
		RETURN(ErrBase, "write is error = %ld", lsend);
	}
	return (int)lsend;
}

#elif _MSC_VER

//
// pipe - 移植 linux函数, 通过 WinSock
// pipefd	: 索引0表示read, 1表示write
// return	: zero is error, -1 is returned
// 
int
pipe(socket_t pipefd[2]) {
	socket_t s;
	sockaddr_t name = { AF_INET };
	socklen_t nlen = sizeof name;
	name.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	// 开启一个固定 socket
	if ((s = socket_stream()) == INVALID_SOCKET) 
        return ErrBase;

	if (bind(s, (struct sockaddr *)&name, nlen))
        return socket_close(s), ErrBase;
	if (listen(s, SOMAXCONN)) 
        return socket_close(s), ErrBase;

	// 得到绑定的数据
	if (getsockname(s, (struct sockaddr *)&name, &nlen))
        return socket_close(s), ErrBase;

	// 开始构建互相通信的socket
	if ((pipefd[0] = socket_stream()) == INVALID_SOCKET)
        return socket_close(s), ErrBase;

	if (socket_connect(pipefd[0], &name))
        return socket_close(s), ErrBase;

	// 可以继续添加, 通信协议来避免一些意外
	if ((pipefd[1] = socket_accept(s, &name)) == INVALID_SOCKET)
        return socket_close(pipefd[0]), socket_close(s), ErrBase;

    return socket_close(s), SufBase;
}

struct scpipe {
	HANDLE recv;
	HANDLE send;
};

//
// scpipe_create    - 得到一个父子进程间通信的管道
// scpipe_delete    - 关闭打开的管道
// scpipe_recv      - 管道接收数据, 128K
// scpipe_send      - 管道发送数据, 推荐 BUFSIZ 
//
scpipe_t 
scpipe_create(void) {
	scpipe_t spie = malloc(sizeof (struct scpipe));
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	assert(NULL != spie);
	if (!CreatePipe(&spie->recv, &spie->send, &sa, 0)) {
		free(spie);
		RETURN(NULL, "CreatePipe error!");
	}
	return spie;
}

inline void 
scpipe_delete(scpipe_t spie) {
	if (spie) {
		CloseHandle(spie->recv);
		CloseHandle(spie->send);
		free(spie);
	}
}

int 
scpipe_recv(struct scpipe * spie, void * data, size_t len) {
	DWORD lrecv = 0;
	BOOL ret = PeekNamedPipe(spie->recv, NULL, 0, NULL, &lrecv, NULL);
	if (!ret || lrecv <= 0) {
		RETURN(ErrEmpty, "PeekNamedPipe recv empty error!");
	}

	// 开始读取数据
	ret = ReadFile(spie->recv, data, len, &lrecv, NULL);
	if (!ret) {
		RETURN(ErrBase, "ReadFile is error!");
	}

	return (int)lrecv;
}

int 
scpipe_send(struct scpipe * spie, const void * data, size_t len) {
	DWORD lsend = 0;
	if (!WriteFile(spie->send, data, len, &lsend, NULL)) {
		RETURN(ErrBase, "WriteFile is error!");
	}
	return (int)lsend;
}

#else
#	error "error : Currently only supports the Best New CL and GCC!"
#endif