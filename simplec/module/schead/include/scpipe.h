#ifndef _H_SIMPLEC_SCPIPE
#define _H_SIMPLEC_SCPIPE

typedef struct scpipe * scpipe_t;

//
// scpipe_create	- 得到一个父子进程间通信的管道
// scpipe_delete	- 关闭打开的管道
// scpipe_recv		- 管道接收数据, 128K
// scpipe_send		- 管道发送数据, 推荐 BUFSIZ 
//
extern scpipe_t scpipe_create(void);
extern void scpipe_delete(scpipe_t spie);
extern int scpipe_recv(scpipe_t spie, char * data, int len);
extern int scpipe_send(scpipe_t spie, const char * data, int len);

#endif // !_H_SIMPLEC_SCPIPE