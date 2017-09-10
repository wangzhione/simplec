#ifndef _H_SIMPLEC_RSMQ
#define _H_SIMPLEC_RSMQ

#include <schead.h>

//
// recv msg : 
//  这仅仅是一个是处理网络序列接收端的解析库. 通过 len [网络字节序, sizeof uint32] -> data
//
// need send msg :
//	one send sizeof uint32_t + data
//

typedef struct {
	// uint8_t type + uint8_t check + uint16_t len
	uint32_t sz;
	char data[];
} * msgrs_t ;

typedef struct rsmq * rsmq_t;

//
// RSMQ_TYPE - 得到当前消息8bit字节
// RSMQ_SIZE - 得到当前消息长度 0x00 + 2 字节 sz
// RSMQ_SZ	- 8bit type + 24 bit len -> uint32_t
//
#define MSGRS_TYPE(sz)		(uint8_t)((uint32_t)(sz) >> 24)
#define MSGRS_LEN(sz)		((uint32_t)(sz) & 0xffffff)
#define MSGRS_SZ(type, len)	(((uint32_t)((uint8_t)type) << 24) | (uint32_t)(len))

//
// msgrs_create - msgrs构建函数, 客户端发送 write(fd, msg->data, msg->sz);
// msg		: 待填充的消息体
// data		: 客户端待发送的消息体
// sz		: data 的长度
// type		: 发送的消息类型, 默认0是 RSMQ_TYPE_INFO
// return	: 创建好的消息体
//
extern msgrs_t msgrs_create(const void * data, uint32_t sz);
extern void msgrs_delete(msgrs_t msg);

extern rsmq_t rsmq_create(void);
extern void rsmq_delete(rsmq_t q);

//
// rsmq_pop - 数据队列中弹出一个解析好的消息体
// q		: 数据队列对象, rsmq_create 创建
// pmsg		: 返回的消息体对象指针
// return	: ErrParse 协议解析错误, ErrEmpty 数据不完整, SufBase 解析成功
//
extern int rsmq_pop(rsmq_t q, msgrs_t * pmsg);
extern void rsmq_push(rsmq_t q, const void * data, uint32_t sz);

#endif//_H_SIMPLEC_RSMQ
