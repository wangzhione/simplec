#ifndef _H_SIMPLEC_SCALLOC
#define _H_SIMPLEC_SCALLOC

#include <stdlib.h>

// 释放sm_malloc_和sm_realloc_申请的内存, 必须配套使用
void sm_free_(void * ptr, const char * file, int line, const char * func);
// 返回申请的一段干净的内存
void * sm_malloc_(size_t sz, const char * file, int line, const char * func);
// 返回重新申请的内存, 只能和sm_malloc_配套使用
void * sm_realloc_(void * ptr, size_t sz, const char * file, int line, const char * func);

/*
 * 释放申请的内存
 * ptr	: 申请的内存
 */
#define sm_free(ptr)		sm_free_(ptr, __FILE__, __LINE__, __func__)
/*
 * 返回申请的内存, 并且是填充'\0'
 * sz	: 申请内存的长度
 */
#define sm_malloc(sz)		sm_malloc_(sz, __FILE__, __LINE__, __func__)
/*
 * 返回申请到num*sz长度内存, 并且是填充'\0'
 * num	: 申请的数量
 * sz	: 申请内存的长度
 */
#define sm_calloc(num, sz)	sm_malloc_(num*sz, __FILE__, __LINE__, __func__)
/*
 * 返回重新申请的内存
 * ptr	: 申请的内存
 * sz	: 申请内存的长度
 */
#define sm_realloc(ptr, sz)	sm_realloc_(ptr, sz, __FILE__, __LINE__, __func__)

// 定义全局内存使用宏, 替换原有的malloc系列函数
#ifndef _SIMPLEC_ALLOC_CLOSE
#	define free			sm_free
#	define malloc		sm_malloc
#	define calloc		sm_calloc
#	define realloc		sm_realloc
#endif

#endif // !_H_SIMPLEC_SCALLOC
