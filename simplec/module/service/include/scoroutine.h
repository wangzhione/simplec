#ifndef _H_SIMPLEC_SCOROUTINE
#define _H_SIMPLEC_SCOROUTINE

#define SCO_DEAD        (0) // 协程死亡状态
#define SCO_READY       (1) // 协程已经就绪
#define SCO_RUNNING     (2) // 协程正在运行
#define SCO_SUSPEND     (3) // 协程暂停等待

// 协程管理器
typedef struct scomng * scomng_t;

//
// 注册的协程体
// sco		: 创建开启的协程总对象
// arg		: 用户创建协程的时候传入的参数
//
typedef void (* sco_f)(scomng_t sco, void * arg);

//
// sco_open - 开启协程系统函数, 并返回创建的协程管理器
//			: 返回创建的协程对象
//
extern scomng_t sco_open(void);

//
// sco_close - 关闭已经开启的协程系统函数
// sco		: sco_oepn 返回的当前协程中协程管理器
//
extern void sco_close(scomng_t sco);

//
// sco_create - 创建一个协程, 此刻是就绪态
// sco		: 协程管理器
// func		: 协程体执行的函数体
// arg		: 协程体中传入的参数
// return	: 返回创建好的协程id
//
extern int sco_create(scomng_t sco, sco_f func, void * arg);

//
// sco_resume - 通过协程id激活协程
// sco		: 协程系统管理器
// id		: 具体协程id, sco_create 返回的协程id
//
extern void sco_resume(scomng_t sco, int id);

//
// sco_yield - 关闭当前正在运行的协程, 让协程处理暂停状态
// sco		: 协程系统管理器
//
extern void sco_yield(scomng_t sco);

//
// sco_status - 得到当前协程状态
// sco		: 协程系统管理器
// id		: 协程id
//			: 返回 _SCO_* 相关的协程状态信息
//
extern int sco_status(scomng_t sco, int id);

//
// sco_running - 当前协程系统中运行的协程id
// sco		: 协程系统管理器
//			: 返回 < 0 表示没有协程在运行
//
extern int sco_running(scomng_t sco);

#endif // !_H_SIMPLEC_SCOROUTINE