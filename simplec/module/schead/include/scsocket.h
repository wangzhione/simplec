#ifndef _H_SIMPLEC_SCSOCKET
#define _H_SIMPLEC_SCSOCKET

#include <schead.h>

//
//	- 跨平台的丑陋从这里开始, 封装一些共用实现
//  __GNUC		=> linux 平台特殊操作		-> gcc
//  __MSC_VER	=> winds 平台特殊操作		-> vs
//
#ifdef __GNUC__

#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/select.h>
#include <sys/resource.h>

//
// This is used instead of -1, since the. by WinSock
//
#define INVALID_SOCKET			(~0)
#define SOCKET_ERROR            (-1)

#define socket_errno			errno
#define SOCKET_EINTR			EINTR
#define SOCKET_EAGAIN			EAGAIN
#define SOCKET_EINVAL			EINVAL
#define SOCKET_EINPROGRESS		EINPROGRESS

#if defined(EWOULDBOCK)
#define SOCKET_EWOULDBOCK		EWOULDBOCK
#else
#define SOCKET_EWOULDBOCK		EAGAIN
#endif

#define IGNORE_SIGNAL(sig)			signal(sig, SIG_IGN)

#define SET_RLIMIT_NOFILE(num) \
	do {\
		struct rlimit $r = { num, num }; \
		setrlimit(RLIMIT_NOFILE, &$r);\
	} while(0)

typedef int socket_t;

#elif _MSC_VER

#include <ws2tcpip.h>

#define IGNORE_SIGNAL(sig)
#define SET_RLIMIT_NOFILE(num)	

#define socket_errno			WSAGetLastError()
#define SOCKET_EINTR			WSAEINTR
#define SOCKET_EAGAIN			WSAEWOULDBLOCK
#define SOCKET_EINVAL			WSAEINVAL
#define SOCKET_EWOULDBOCK		WSAEWOULDBLOCK
#define SOCKET_EINPROGRESS		WSAEINPROGRESS

typedef int socklen_t;
typedef SOCKET socket_t;

#define write(fd, buf, count) \
	send(fd, buf, count, 0)
#define read(fd, buf, count) \
	recv(fd, buf, count, 0)

#else
#	error "error : Currently only supports the Best New CL and GCC!"
#endif

//
// IGNORE_SIGPIPE - 管道破裂,忽略SIGPIPE信号
//
#define IGNORE_SIGPIPE()		IGNORE_SIGNAL(SIGPIPE)

// 目前通用的tcp udp v4地址
typedef struct sockaddr_in sockaddr_t;

//
// MAKE_TIMEVAL - 毫秒数转成 timeval 变量
// tv		: struct timeval 变量
// msec		: 毫秒数
//
#define MAKE_TIMEVAL(tv, msec) \
	do {\
		if((msec) > 0) {\
			(tv).tv_sec = (msec) / _INT_STOMS;\
			(tv).tv_usec = ((msec) % _INT_STOMS) * _INT_STOMS;\
		}\
		else {\
			(tv).tv_sec = 0;\
			(tv).tv_usec = 0;\
		}\
	} while(0)

//
// socket_start	- 单例启动socket库的初始化方法
// socket_addr	- 通过ip, port 得到 ipv4 地址信息
// 
extern void socket_start(void);
extern int socket_addr(const char * ip, uint16_t port, sockaddr_t * addr);

//
// socket_dgram		- 创建UDP socket
// socket_stream	- 创建TCP socket
// socket_close		- 关闭上面创建后的句柄
// socket_bind		- socket绑定操作
//
extern socket_t socket_dgram(void);
extern socket_t socket_stream(void);
extern int socket_close(socket_t s);
extern int socket_bind(socket_t s, const char * ip, uint16_t port);

//
// socket_set_block		- 设置套接字是阻塞
// socket_set_nonblock	- 设置套接字是非阻塞
// socket_set_reuseaddr	- 开启地址复用
// socket_set_recvtimeo	- 设置接收数据毫秒超时时间
// socket_set_sendtimeo	- 设置发送数据毫秒超时时间
//
extern int socket_set_block(socket_t s);
extern int socket_set_nonblock(socket_t s);
extern int socket_set_reuseaddr(socket_t s);
extern int socket_set_recvtimeo(socket_t s, int ms);
extern int socket_set_sendtimeo(socket_t s, int ms);

//
// socket_recv		- socket接受信息
// socket_recvn		- socket接受len个字节进来
// socket_send		- socket发送消息
// socket_sendn		- socket发送len个字节出去
// socket_recvfrom	- recvfrom接受函数
// socket_sendto	- udp发送函数, 通过socket_udp 搞的完全可以 socket_send发送
//
extern int socket_recv(socket_t s, void * buf, int len);
extern int socket_recvn(socket_t s, void * buf, int len);
extern int socket_send(socket_t s, const void * buf, int len);
extern int socket_sendn(socket_t s, const void * buf, int len);
extern int socket_recvfrom(socket_t s, void * buf, int len, int flags, sockaddr_t * in, socklen_t * inlen);
extern int socket_sendto(socket_t s, const void * buf, int len, int flags, const sockaddr_t * to, socklen_t tolen);

//
// socket_tcp			- 创建TCP详细的套接字
// socket_udp			- 创建UDP详细套接字
// socket_connect		- connect操作
// socket_connects		- 通过socket地址连接
// socket_connecto		- connect连接超时判断
// socket_connectos		- connect连接客户端然后返回socket_t句柄
// socket_accept		- accept 链接函数
//
extern socket_t socket_tcp(const char * host, uint16_t port);
extern socket_t socket_udp(const char * host, uint16_t port);
extern int socket_connect(socket_t s, const sockaddr_t * addr);
extern int socket_connects(socket_t s, const char * ip, uint16_t port);
extern int socket_connecto(socket_t s, const sockaddr_t * addr, int ms);
extern socket_t socket_connectos(const char * host, uint16_t port, int ms);
extern socket_t socket_accept(socket_t s, sockaddr_t * addr, socklen_t * len);

#endif // !_H_SIMPLEC_SCSOCKET
