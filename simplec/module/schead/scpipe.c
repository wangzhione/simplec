#include <scpipe.h>
#include <struct.h>

#ifdef __GNUC__

#include <unistd.h>

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

#include <Windows.h>

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