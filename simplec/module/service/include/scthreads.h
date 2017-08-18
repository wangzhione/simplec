#ifndef _H_SIMPLEC_SCTHREADS
#define _H_SIMPLEC_SCTHREADS

#include <schead.h>

//
// 这是个线程池的库. 支持异步取消 也加过一些线程帮助库
//

typedef struct threads * threads_t;

//
// threads_create - 创建一个线程池处理对象
// return	: 返回创建好的线程池对象, NULL表示失败
//
extern threads_t threads_create(void);

//
// threads_delete - 异步销毁一个线程池对象
// pool		: 线程池对象
// return	: void
//
extern void threads_delete(threads_t pool);

//
// threads_add - 线程池中添加要处理的任务
// pool		: 线程池对象
// run		: 运行的执行题
// arg		: run的参数
// return	: void
//
extern void threads_add(threads_t pool, node_f run, void * arg);

#endif // !_H_SIMPLEC_SCTHREADS