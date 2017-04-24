#ifndef _H_SIMPLEC_SCTIMER
#define _H_SIMPLEC_SCTIMER

#include <schead.h>

/*
 *  简单的定时器代码.跨平台,线程安全
 *关键是使用简单.
 *例如
 *	1. 启动一次,不要求多线程, 1s后执行
 *		st_add(1, 1, 0, timer, arg, 0)
 *  2. 启动轮询事件, 要求多线程,立即启动,并且每隔200ms执行一次
 *		st_add(0, -1, 200, timer, arg, 1)
 *
 *这些参数具体含义,讲述的很清楚. 你看明白后再用.或者把你常用的封装好
 */

/*
 *  添加定时器事件,虽然设置的属性有点多但是都是必要的 .
 * start	: 延迟启动的时间, 0表示立即启动, 单位是毫秒
 * cnt		: 表示执行次数, 0表示永久时间, 一次就为1
 * intval	: 每次执行的时间间隔, 单位是毫秒
 * timer	: 定时器执行函数
 * arg		: 定时器参数指针
 * fb		: 0表示不启用多线程, 1表示启用多线程
 * return	: 返回这个定时器的 唯一id
 */
extern int st_add(int start, int cnt, int intval, die_f timer, void * arg, bool fb);

/*
 * 删除指定事件
 * st		: st_add 返回的定时器id
 */
extern void st_del(int st);

#endif // !_H_SIMPLEC_SCTIMER
