#ifndef _H_SIMPLEC_SCTIMER
#define _H_SIMPLEC_SCTIMER

#include <schead.h>

//
// st_add - 添加定时器事件,虽然设置的属性有点多但是都是必要的
// intval	: 执行的时间间隔, <=0 表示立即执行, 单位是毫秒
// timer	: 定时器执行函数
// arg		: 定时器参数指针
// return	: 返回这个定时器的唯一id
//
extern int st_add_(int intval, node_f timer, void * arg);
#define st_add(intval, timer, arg) \
        st_add_(intval, (node_f)timer, (void *)(intptr_t)arg)

//
// st_del - 删除指定事件
// id		: st_add 返回的定时器id
// return	: void
//
extern void st_del(int id);

#endif // !_H_SIMPLEC_SCTIMER
