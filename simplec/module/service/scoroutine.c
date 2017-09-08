// Compiler Foreplay
#if !defined(_MSC_VER) && !defined(__GNUC__)
#	error "error : Currently only supports the Best New CL and GCC!"
#endif

#include <string.h>
#include <stdlib.h>
#include <assert.h>

// 默认协程栈大小 和 初始化协程数量
#define _INT_STACK		(256 * 1024)
#define _INT_COROUTINE	(16)

#include "scoroutine$winds.h"
#include "scoroutine$linux.h"

//
// sco_status - 得到当前协程状态
// sco      : 协程系统管理器
// id       : 协程id
// return   : 返回 SCO_* 相关的协程状态信息
//
inline int
sco_status(scomng_t sco, int id) {
	assert(sco && id >= 0 && id < sco->cap);
	return sco->cos[id] ? sco->cos[id]->status : SCO_DEAD;
}

//
// sco_running - 当前协程系统中运行的协程id
// sco      : 协程系统管理器
// return   : 返回 < 0 表示没有协程在运行
//
inline int
sco_running(scomng_t sco) {
	return sco->running;
}