#if !defined(_H_SIMPLEC_SCOROUTINE$LINUX) && defined(__GNUC__)
#define _H_SIMPLEC_SCOROUTINE$LINUX

#include "scoroutine.h"
#include <ucontext.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

// 默认协程栈大小 和 初始化协程数量
#define _INT_STACK		(256 * 1024)
#define _INT_COROUTINE	(16)

// 声明协程结构 和 协程管理器结构
struct sco {
	char * stack;			// 当前协程栈指针
	ucontext_t ctx;			// 当前协程运行的上下文环境
	ptrdiff_t cap;			// 当前栈的容量
	ptrdiff_t cnt;			// 当前栈的大小

	sco_f func;				// 协程体执行
	void * arg;				// 用户输入的参数
	int status;				// 当前协程运行状态 _SCO_*
};

struct scomng {
	char stack[_INT_STACK];	// 当前协程中开辟的栈对象
	ucontext_t main;		// 当前协程上下文对象

	int running;			// 当前协程中运行的协程id

	struct sco ** cos;		// 协程对象集, 循环队列
	int cap;				// 协程对象集容量
	int idx;				// 当前协程集中轮询到的索引
	int cnt;				// 当前存在的协程个数
};

/*
 * 开启协程系统函数, 并返回创建的协程管理器
 *			: 返回创建的协程对象
 */
inline void *
sco_open(void) {
	struct scomng * comng = malloc(sizeof(struct scomng));
	assert(NULL != comng);
	comng->running = -1;
	comng->cos = calloc(_INT_COROUTINE, sizeof(struct sco *));
	comng->cap = _INT_COROUTINE;
	comng->idx = 0;
	comng->cnt = 0;
	assert(NULL != comng->cos);
	return comng;
}

// 销毁一个协程对象
static inline void _sco_delete(struct sco * co) {
	free(co->stack);
	free(co);
}

/*
 * 关闭已经开启的协程系统函数
 *	sco		: sco_oepn 返回的当前协程中协程管理器
 */
void
sco_close(void * sco) {
	int i = -1;
	struct scomng * comng = sco;
	while (++i < comng->cap) {
		struct sco * co = comng->cos[i];
		if (co) {
			_sco_delete(co);
			comng->cos[i] = NULL;
		}
	}

	free(comng->cos);
	comng->cos = NULL;
	free(comng);
}

// 构建 struct sco 协程对象
static inline struct sco * _sco_new(sco_f func, void * arg) {
	struct sco * co = malloc(sizeof(struct sco));
	assert(co && func);
	co->func = func;
	co->arg = arg;
	co->status = _SCO_READY;

	co->stack = NULL;
	co->cap = 0;
	co->cnt = 0;

	return co;
}

/*
 * 创建一个协程, 此刻是就绪态
 *  sco		: 协程管理器
 *	func	: 协程体执行的函数体
 *  arg		: 协程体中传入的参数
 *			: 返回创建好的协程id
 */
int
sco_create(void * sco, sco_f func, void * arg) {
	struct sco * co = _sco_new(func, arg);
	struct scomng * comng = sco;
	struct sco ** cos = comng->cos;
	int cap = comng->cap;
	// 下面开始寻找, 如果数据足够的话
	if (comng->cnt < comng->cap) {
		// 当循环队列去查找
		int idx = comng->idx;
		do {
			if (NULL == cos[idx]) {
				cos[idx] = co;
				++comng->cnt;
				++comng->idx;
				return idx;
			}
			idx = (idx + 1) % cap;
		} while (idx != comng->idx);

		assert(idx == comng->idx);
		return -1;
	}

	// 这里需要重新构建空间
	cos = realloc(cos, sizeof(struct sco *) * cap * 2);
	assert(NULL != cos);
	memset(cos + cap, 0, sizeof(struct sco *) * cap);
	comng->cos = cos;
	comng->cap = cap << 1;
	++comng->cnt;
	cos[comng->idx] = co;
	return comng->idx++;
}

// 协程运行的主体
static inline void _sco_main(uint32_t low32, uint32_t hig32) {
	uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hig32 << 32);
	struct scomng * comng = (struct scomng *)ptr;
	int id = comng->running;
	struct sco * co = comng->cos[id];
	// 执行协程体
	co->func(comng, co->arg);
	co = comng->cos[id];
	co->status = _SCO_DEAD;
	_sco_delete(co);
	comng->cos[id] = NULL;
	--comng->cnt;
	comng->idx = id;
	comng->running = -1;
}
/*
 * 通过协程id激活协程
 *	sco		: 协程系统管理器
 *	id		: 具体协程id, sco_create 返回的协程id
 */
void
sco_resume(void * sco, int id) {
	uintptr_t ptr;
	struct sco * co;
	struct scomng * comng = sco;
	int status;
	int running = comng->running;
	assert(running == -1 && id >= 0 && id < comng->cap);

	// 下面是协程 _SCO_READY 和 _SCO_SUSPEND 处理
	co = comng->cos[id];
	if ((!co) || (status = co->status) == _SCO_DEAD)
		return;

	comng->running = id;
	co->status = _SCO_RUNNING;
	switch (status) {
	case _SCO_READY:
		// 兼容x64指针通过makecontext传入
		ptr = (uintptr_t)comng;
		// 构建栈和运行链
		getcontext(&co->ctx);
		co->ctx.uc_stack.ss_sp = comng->stack;
		co->ctx.uc_stack.ss_size = _INT_STACK;
		co->ctx.uc_link = &comng->main;
		makecontext(&co->ctx, (void(*)())_sco_main, 2, (uint32_t)ptr, (uint32_t)(ptr >> 32));
		// 保存当前运行状态到comng->main, 然后跳转到 co->ctx运行环境中
		swapcontext(&comng->main, &co->ctx);
		break;
	case _SCO_SUSPEND:
		// stack add is high -> low
		memcpy(comng->stack + _INT_STACK - co->cnt, co->stack, co->cnt);
		swapcontext(&comng->main, &co->ctx);
		break;
	default:
		assert(co->status && 0);
	}
}

// 保存当前运行的堆栈信息
static void _sco_savestack(struct sco * co, char * top) {
	char dummy = 0;
	ptrdiff_t size = top - &dummy;
	assert(size <= _INT_STACK);
	if (co->cap < size) {
		free(co->stack);
		co->cap = size;
		co->stack = malloc(size);
	}
	co->cnt = size;
	memcpy(co->stack, &dummy, size);
}

/*
 * 关闭当前正在运行的协程, 让协程处理暂停状态
 *	sco		: 协程系统管理器
 */
void
sco_yield(void * sco) {
	struct sco * co;
	struct scomng * comng = sco;
	int id = comng->running;
	if ((id < 0 || id >= comng->cap) || !(co = comng->cos[id]))
		return;
	assert((char *)&co > comng->stack);
	_sco_savestack(co, comng->stack + _INT_STACK);
	co->status = _SCO_SUSPEND;
	comng->running = -1;
	swapcontext(&co->ctx, &comng->main);
}

/*
* 得到当前协程状态
*	sco		: 协程系统管理器
*	id		: 协程id
*			: 返回 _SCO_* 相关的协程状态信息
*/
inline int
sco_status(void * sco, int id) {
	struct scomng * comng = sco;
	assert(comng && id >= 0 && id < comng->cap);
	return comng->cos[id] ? comng->cos[id]->status : _SCO_DEAD;
}

/*
* 当前协程系统中运行的协程id
*	sco		: 协程系统管理器
*			: 返回 < 0 表示没有协程在运行
*/
inline int
sco_running(void * sco) {
	return ((struct scomng *)sco)->running;
}

#endif // !_H_SIMPLEC_SCOROUTINE$LINUX
