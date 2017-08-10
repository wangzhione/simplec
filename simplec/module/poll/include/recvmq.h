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

struct recvmsg {
	// uint8_t type + uint8_t check + uint16_t len
	uint32_t sz;
	void * data;
};

typedef struct recvmq * recvmq_t;

//
// RECVMSG_TYPE - 得到当前消息8bit字节
// RECVMSG_SIZE - 得到当前消息长度 0x00 + 2 字节 sz
// RECVMSG_SZ	- 8bit type + 8 bit check + 16 bit len -> uint32_t
//
#define RECVMSG_TYPE(sz)		(uint8_t)((uint32_t)(sz) >> 24)
#define RECVMSG_LEN(sz)			(int)((uint32_t)(sz) & 0xffffff)
#define RECVMSG_SZ(type, len)	(((uint32_t)(type) << 24) | (uint32_t)(len))

extern recvmq_t recvmq_create(void);
extern void recvmq_delete(recvmq_t buff);
//
// recvmq_pop - 数据队列中弹出一个解析好的消息体
// buff		: 数据队列对象, recvmq_create 创建
// msg		: 返回的消息体对象
// return	: ErrParse 协议解析错误, ErrEmpty 数据不完整, SufBase 解析成功
//
extern int recvmq_pop(recvmq_t buff, struct recvmsg * msg);
extern void recvmq_push(recvmq_t buff, const void * data, uint32_t sz);

#endif//_H_SIMPLEC_RECVMQ
