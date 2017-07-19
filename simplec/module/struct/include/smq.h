#ifndef _H_SIMPLEC_SMQ
#define _H_SIMPLEC_SMQ

#include <struct.h>

typedef struct smq * smq_t;

typedef struct smsg {
	uint32_t source;
	int session;
	void * data;
	size_t sz;
} * smsg_t;

typedef void( * smsgdie_f)(smsg_t m, void * ud);

// type is encoding in smsg.sz high 8bit
#define SMSG_TYPE		(SIZE_MAX >> 8)
#define SMSG_SHIFT		((sizeof(size_t) - 1) * 8)

//
// smq_global_push - 消息队列插入到全局消息队列管理器中
// smq_global_pop - 全局消息队列弹出数据
//
extern void smq_global_push(smq_t q);
extern smq_t smq_global_pop(void);

//
// smq_create - 创建一个消息队列
// smq_release - 添加释放标识, 并放入全局消息队列中
// smq_delete - 试图清除一个消息队列, 没有释放标识会放入全局消息队列中 
//
extern smq_t smq_create(uint32_t handle);
extern void smq_release(smq_t q);
extern void smq_delete(smq_t q, smsgdie_f drop, void * ud);

//
// smq_pop - 弹出消息队列中消息, 返回true表示弹出成功
// smq_push - 消息队列中插入数据
//
extern bool smq_pop(smq_t q, smsg_t m);
extern void smq_push(smq_t q, smsg_t m);

//
// smq_get_handle - 得到注册时候的handle
// smq_get_length - 得到此刻消息队列中长度
// smq_get_overload - 得到此刻消息队列过载量
// 
extern uint32_t smq_get_handle(smq_t q);
extern int smq_get_length(smq_t q);
extern int smq_get_overload(smq_t q);

#endif//!_H_SIMPLEC_SMQ