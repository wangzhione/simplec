#ifndef _H_SIMPLEC_SOCKET_START
#define _H_SIMPLEC_SOCKET_START

#include <rsmq.h>
#include <socket_msg.h>

//
// ss_run - 启动单例消息服务器
// host		: 主机名称
// port		: 端口
// run		: 消息解析协议
// return	: void
//
extern void ss_run(const char * host, uint16_t port, void (* run)(msgrs_t));

//
// ss_end - 关闭单例的消息服务器
// return	: void
//
extern void ss_end(void);

#endif//_H_SIMPLEC_SOCKET_START