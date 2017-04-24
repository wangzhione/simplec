#ifndef _H_SIMPLEC_SCRUNLOOP
#define _H_SIMPLEC_SCRUNLOOP

#include <struct.h>

typedef struct srl * srl_t;

//
// srl_create - 创建轮询服务对象
// run		: 轮序处理每条消息体, 弹出消息体的时候执行
// return	: void 
//
extern srl_t srl_create(die_f run);

//
// srl_delete - 销毁轮询对象,回收资源
// srl		: 轮询对象
// return	: void 
//
extern void srl_delete(srl_t srl);

//
// srl_push - 将消息压入到轮询器中
// msg		: 待加入的消息地址
// return	: void
// 
extern void srl_push(srl_t srl, void * msg);

//
// srl_start - 启动当前轮序器
// srl		: 轮询器对象
// return	: void
//
extern void srl_start(srl_t srl);

//
// srl_stop - 暂停轮序器
// srl		: 轮询器对象
// return	: void
//
extern void srl_stop(srl_t srl);

#endif // !_H_SIMPLEC_SCRUNLOOP