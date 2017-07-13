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
scpipe_recv(scpipe_t spie, char * data, int len) {
	ssize_t lrecv = read(spie->recv, data, len);
	if (lrecv < 0) {
		// 不考虑信号中断
		RETURN(Error_Base, "read is error = %ld", lrecv);
	}
	return (int)lrecv;
}

int 
scpipe_send(scpipe_t spie, const char * data, int len) {
	ssize_t lsend = write(spie->send, data, len);
	if (lsend < 0) {
		RETURN(Error_Base, "write is error = %ld", lsend);
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
	if ((s = socket_stream()) == INVALID_SOCKET) {
		RETURN(Error_Base, "socket_stream s is error!");
	}
	if (bind(s, (struct sockaddr *)&name, nlen) < 0) {
		socket_close(s);
		RETURN(Error_Base, "bind s is error!");
	}
	if (listen(s, SOMAXCONN) < 0) {
		socket_close(s);
		RETURN(Error_Base, "listen s is error!");
	}
	// 得到绑定的数据
	if (getsockname(s, (struct sockaddr *)&name, &nlen) < 0) {
		socket_close(s);
		RETURN(Error_Base, "getsockname s is error!");
	}

	// 开始构建互相通信的socket
	if ((pipefd[0] = socket_stream()) == INVALID_SOCKET) {
		closesocket(s);
		RETURN(Error_Base, "socket_stream pipefd[0] is error!");
	}

	if (socket_connect(pipefd[0], &name) < 0) {
		socket_close(s);
		RETURN(Error_Base, "socket_connect pipefd[0] is error!");
	}
	// 可以继续添加, 通信协议来避免一些意外
	if ((pipefd[1] = socket_accept(s, &name, &nlen)) == INVALID_SOCKET) {
		socket_close(s);
		socket_close(pipefd[0]);
		RETURN(Error_Base, "socket_accept sendfd is error!");
	}

	socket_close(s);
	return Success_Base;
}

struct scpipe {
	HANDLE recv;
	HANDLE send;
};

//
// scpipe_create	- 得到一个父子进程间通信的管道
// scpipe_delete	- 关闭打开的管道
// scpipe_recv		- 管道接收数据
// scpipe_send		- 管道发送数据 
//
scpipe_t 
scpipe_create(void) {
	scpipe_t spie = malloc(sizeof (struct scpipe));
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	assert(NULL != spie);
	if (!CreatePipe(&spie->recv, &spie->send, &sa, 0)) {
		free(spie);
		RETURN(NULL, "CreatePipe errno = %lu.", GetLastError());
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
scpipe_recv(struct scpipe * spie, char * data, int len) {
	DWORD lrecv = 0;
	BOOL ret = PeekNamedPipe(spie->recv, NULL, 0, NULL, &lrecv, NULL);
	if (!ret || lrecv <= 0) {
		RETURN(Error_Empty, "PeekNamedPipe recv empty or %lu.", GetLastError());
	}

	// 开始读取数据
	ret = ReadFile(spie->recv, data, len, &lrecv, NULL);
	if (!ret) {
		RETURN(Error_Base, "ReadFile is error %lu.", GetLastError());
	}

	return (int)lrecv;
}

int 
scpipe_send(struct scpipe * spie, const char * data, int len) {
	DWORD lsend = 0;
	if (WriteFile(spie->send, data, len, &lsend, NULL)) {
		RETURN(Error_Base, "WriteFile is error %lu.", GetLastError());
	}
	return (int)lsend;
}

#else
#	error "error : Currently only supports the Best New CL and GCC!"
#endif