#ifndef _H_SIMPLEC_SCPTHREAD
#define _H_SIMPLEC_SCPTHREAD

#include <schead.h>

/*
 *   这是个基于 pthread.h 的线程池. 简单方便高效.
 * 这里使用了头文件 schead.h 也可以省掉,这里只使用了cdel_f 的类型.
 * typedef void (*die_f)(void* arg);
 * 也自定义了一个类型 threadpool_t 线程池指针类型,也叫作不完整(全)类型.
 * 只声明不写实现.也是个常用技巧
 */
typedef struct threadpool* threadpool_t;

/*
 * 通过这个接口创建线程池对象.后面就可以使用了.
 * size		: 当前线程池中最大的线程个数
 *			: 返回创建好的线程池值
 */
extern threadpool_t sp_new(int size);

/*
 * 在当前线程池中添加待处理的线程对象.
 * pool		: 线程池对象, sp_new 创建的那个
 * run		: 运行的函数体, 返回值void, 参数void*
 * arg		: 传入运行的参数
 *			: 没有返回值
 */
extern void sp_add(threadpool_t pool, die_f run, void* arg);

/*
 * 优化扩展宏,简化操作.唯一恶心的是宏调试难
 * _INT_THREADPOOL 是一个简单的大小设置,控制线程池中线程多少
 *
 * sp_CREATE 同样在上面宏帮助下, 少些一个参数. 认为是函数重载
 * 
 * sp_ADD 是一个开发技巧,帮助我们 把 void (*)(type* pi) => void (*)(void* pi), 
 * 这样我们写函数定义的时候更方便随意.
 */
#define _INT_THREADPOOL	(128)

#define sp_NEW() \
	sp_new(_INT_THREADPOOL)

#define sp_ADD(pool, run, arg) \
	sp_add(pool, (die_f)run, arg)

/*
 * 销毁当前线程池,释放内存,并尝试停止线程池中线程.
 * ppopl		: 指向 sp_new创建的对象的指针
 *				: 没有返回值
 */
extern void sp_del(threadpool_t* ppool);

#endif // !_H_SIMPLEC_SCPTHREAD
