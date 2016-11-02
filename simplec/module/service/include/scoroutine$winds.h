#if !defined(_H_SIMPLEC_SCOROUTINE$WINDS) && defined(_MSC_VER)
#define _H_SIMPLEC_SCOROUTINE$WINDS

#include "scoroutine.h"
#include <Windows.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

// 默认协程栈大小 和 初始化协程数量
#define _INT_STACK		(256 * 1024)
#define _INT_COROUTINE	(16)

// 声明协程结构 和 协程管理器结构
struct sco {
	PVOID ctx;				// 当前协程运行的环境
	sco_f func;				// 协程体执行
	void * arg;				// 用户输入的参数
	int status;				// 当前协程运行状态 _SCO_*
};

struct scomng {
	PVOID main;				// 当前主协程记录运行环境
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
	// 在当前线程环境中开启Window协程
	comng->main = ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH);
	return comng;
}

// 销毁一个协程对象
static inline void _sco_delete(struct sco * co) {
	DeleteFiber(co->ctx);
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
	// 切换当前协程系统变回默认的主线程, 关闭协程系统
	ConvertFiberToThread();
}

// 构建 struct sco 协程对象
static inline struct sco * _sco_new(sco_f func, void * arg) {
	struct sco * co = malloc(sizeof(struct sco));
	assert(co && func);
	co->func = func;
	co->arg = arg;
	co->status = _SCO_READY;
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

static inline VOID WINAPI _sco_main(LPVOID ptr) {
	struct scomng * comng = ptr;
	int id = comng->running;
	struct sco * co = comng->cos[id];
	// 执行协程体
	co->func(comng, co->arg);
	co = comng->cos[id];
	co->status = _SCO_DEAD;
	// 跳转到主纤程体中销毁
	SwitchToFiber(comng->main);
}
/*
 * 通过协程id激活协程
 *	sco		: 协程系统管理器
 *	id		: 具体协程id, sco_create 返回的协程id
 */
void 
sco_resume(void * sco, int id) {
	struct sco * co;
	struct scomng * comng = sco;
	int running;

	assert(comng && id >= 0 && id < comng->cap);

	// _SCO_DEAD 状态协程, 完全销毁其它协程操作
	running = comng->running;
	if (running != -1) {
		co = comng->cos[running];
		assert(co && co->status == _SCO_DEAD);
		comng->cos[running] = NULL;
		--comng->cnt;
		comng->idx = running;
		comng->running = -1;
		_sco_delete(co);
		if (running == id)
			return;
	}

	// 下面是协程 _SCO_READY 和 _SCO_SUSPEND 处理
	co = comng->cos[id];
	if ((!co) || (co->status != _SCO_READY && co->status != _SCO_SUSPEND))
		return;

	// Window特性创建纤程, 并保存当前上下文环境, 切换到创建的纤程环境中
	if (co->status == _SCO_READY)
		co->ctx = CreateFiberEx(_INT_STACK, 0, FIBER_FLAG_FLOAT_SWITCH, _sco_main, comng);

	co->status = _SCO_RUNNING;
	comng->running = id;
	comng->main = GetCurrentFiber();
	// 正常逻辑切换到创建的子纤程中
	SwitchToFiber(co->ctx);
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
	co->status = _SCO_SUSPEND;
	comng->running = -1;
	co->ctx = GetCurrentFiber();
	SwitchToFiber(comng->main);
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

#endif // !_H_SIMPLEC_SCOROUTINE$WINDS