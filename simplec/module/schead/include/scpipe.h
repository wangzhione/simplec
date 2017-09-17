#ifndef _H_SIMPLEC_SCPIPE
#define _H_SIMPLEC_SCPIPE

#include <scsocket.h>

#if defined(_MSC_VER)

//
// pipe - 移植 linux函数, 通过 WinSock
// pipefd	: 索引0表示read, 1表示write
// return	: zero is error, -1 is returned
// 
extern int pipe(socket_t pipefd[2]);

#endif

typedef struct scpipe * scpipe_t;

//
// scpipe_create    - 得到一个父子进程间通信的管道
// scpipe_delete    - 关闭打开的管道
// scpipe_recv      - 管道接收数据, 128K
// scpipe_send      - 管道发送数据, 推荐 BUFSIZ 
//
extern scpipe_t scpipe_create(void);
extern void scpipe_delete(scpipe_t spie);
extern int scpipe_recv(scpipe_t spie, void * data, size_t len);
extern int scpipe_send(scpipe_t spie, const void * data, size_t len);

#endif // !_H_SIMPLEC_SCPIPE