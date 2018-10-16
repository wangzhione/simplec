#ifndef _H_SCRUNLOOP_SIMPLEC
#define _H_SCRUNLOOP_SIMPLEC

#include "schead.h"

typedef struct srl * srl_t;

//
// srl_delete - 销毁轮询对象,回收资源
// s        : 轮询对象
// return   : void 
//
extern void srl_delete(srl_t srl);

//
// srl_push - 将消息压入到轮询器中
// s        : 轮询对象
// msg      : 待加入的消息地址
// return   : void
// 
extern void srl_push(srl_t s, void * msg);

//
// srl_create - 创建轮询服务对象
// frun     : 轮序处理每条消息体, 弹出消息体的时候执行
// fdie     : srl_push msg 销毁函数
// return   : srl_t 轮询器对象 
//
#define srl_create(frun, fdie)                      \
srl_create_((node_f)(frun), (node_f)(fdie))
extern srl_t srl_create_(node_f frun, node_f fdie);

#endif//_H_SCRUNLOOP_SIMPLEC
