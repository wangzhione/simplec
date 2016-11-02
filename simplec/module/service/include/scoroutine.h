#ifndef _H_SIMPLEC_SCOROUTINE
#define _H_SIMPLEC_SCOROUTINE

#define _SCO_DEAD		(0)		// 协程死亡状态
#define _SCO_READY		(1)		// 协程已经就绪
#define _SCO_RUNNING	(2)		// 协程正在运行
#define _SCO_SUSPEND	(3)		// 协程暂停等待

/*
 * 注册的协程体
 * sco		: 创建开启的协程总对象
 * arg		: 用户创建协程的时候传入的参数
 */
typedef void(* sco_f)(void * sco, void * arg);

/*
 * 开启协程系统函数, 并返回创建的协程管理器
 *			: 返回创建的协程对象
 */
extern void * sco_open(void);

/*
 * 关闭已经开启的协程系统函数
 *	sco		: sco_oepn 返回的当前协程中协程管理器
 */
extern void sco_close(void * sco);

/*
 * 创建一个协程, 此刻是就绪态
 *  sco		: 协程管理器
 *	func	: 协程体执行的函数体
 *  arg		: 协程体中传入的参数
 *			: 返回创建好的协程id
 */
extern int sco_create(void * sco, sco_f func, void * arg);

/*
 * 通过协程id激活协程
 *	sco		: 协程系统管理器
 *	id		: 具体协程id, sco_create 返回的协程id
 */
extern void sco_resume(void * sco, int id);

/*
 * 关闭当前正在运行的协程, 让协程处理暂停状态
 *	sco		: 协程系统管理器
 */
extern void sco_yield(void * sco);

/*
 * 得到当前协程状态
 *	sco		: 协程系统管理器
 *	id		: 协程id
 *			: 返回 _SCO_* 相关的协程状态信息
 */
extern int sco_status(void * sco, int id);

/*
 * 当前协程系统中运行的协程id
 *	sco		: 协程系统管理器
 *			: 返回 < 0 表示没有协程在运行
 */
extern int sco_running(void * sco);

#endif // !_H_SIMPLEC_SCOROUTINE