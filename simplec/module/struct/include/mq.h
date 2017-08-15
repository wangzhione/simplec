#ifndef _H_SIMPLEC_MQ
#define _H_SIMPLEC_MQ

#include <struct.h>

typedef struct mq * mq_t;

//
// mq_create - 创建一个消息队列类型
// return	: 返回创建好的消息队列对象, NULL表示失败
//
extern mq_t mq_create(void);

//
// mq_delete - 删除创建消息队列, 并回收资源
// mq		: 消息队列对象
// die		: 删除push进来的结点
// return	: void
//
extern void mq_delete(mq_t mq, node_f die);

//
// mq_push - 消息队列中压入数据
// mq		: 消息队列对象
// msg		: 压入的消息
// return	: void
// 
extern void mq_push(mq_t mq, void * msg);

//
// mq_pop - 消息队列中弹出消息,并返回
// mq		: 消息队列对象
// return	: 返回队列尾巴, 队列为empty返回NULL
//
extern void * mq_pop(mq_t mq);

//
// mq_len - 得到消息队列的长度,并返回
// mq		: 消息队列对象
// return	: 返回消息队列长度
// 
extern int mq_len(mq_t mq);

#endif // !_H_SIMPLEC_MQ