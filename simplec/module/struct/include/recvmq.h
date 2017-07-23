#ifndef _H_SIMPLEC_RECVMQ
#define _H_SIMPLEC_RECVMQ

#include <schead.h>

//
// recv msg : 
//  这仅仅是一个是处理网络序列接收端的解析库. 通过 len [统一小端网络字节, sizeof uint32] -> data
//
// need send msg :
//	one send sizeof uint32_t + data
//

typedef struct recvmq * recvmq_t;

extern recvmq_t recvmq_create(void);
extern void recvmq_delete(recvmq_t buff);

extern void recvmq_push(recvmq_t buff, const void * data, uint32_t sz);

//
// recvmq_pop - 数据队列中弹出一个解析好的消息体
// buff		: 数据队列对象, buffq_create 创建
// psz		: 返回对象长度, -> len
// return	: NULL 表示没有完整数据 -> data
//
extern void * recvmq_pop(recvmq_t buff, uint32_t * psz);

#endif//_H_SIMPLEC_RECVMQ
