#ifndef _H_SIMPLEC_SCRUNLOOP
#define _H_SIMPLEC_SCRUNLOOP

#include <struct.h>

typedef struct srl * srl_t;

//
// srl_create - 创建轮询服务对象
// run		: 轮序处理每条消息体, 弹出消息体的时候执行
// die		: srl_push msg 的销毁函数
// return	: void 
//
srl_t srl_create_(node_f run, node_f die);
#define srl_create(run, die) \
        srl_create_((node_f)(run), (node_f)(die))

//
// srl_delete - 销毁轮询对象,回收资源
// s		: 轮询对象
// return	: void 
//
extern void srl_delete(srl_t srl);

//
// srl_push - 将消息压入到轮询器中
// s		: 轮询对象
// msg		: 待加入的消息地址
// return	: void
// 
extern void srl_push(srl_t s, void * msg);

#endif // !_H_SIMPLEC_SCRUNLOOP