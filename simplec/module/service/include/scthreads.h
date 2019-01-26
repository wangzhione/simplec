#ifndef _SIMPLEC_SCTHREADS_H
#define _SIMPLEC_SCTHREADS_H

#include <schead.h>

//
// 简易的线程池的库
//
typedef struct threads * threads_t;

//
// threads_create - 创建线程池对象
// return   : 创建的线程池对象, NULL 表示失败
//
extern threads_t threads_create(void);

//
// threads_delete - 异步销毁线程池对象
// pool     : 线程池对象
// return   : void
//
extern void threads_delete(threads_t pool);

//
// threads_insert - 线程池中添加待处理的任务
// pool     : 线程池对象
// frun     : node_f 运行的执行体
// arg      : frun 的参数
// return   : void
//
extern void threads_insert(threads_t pool, void * frun, void * arg);

#endif // !_SIMPLEC_SCTHREADS_H