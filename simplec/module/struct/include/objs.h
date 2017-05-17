#ifndef _H_SIMPLEC_OBJS
#define _H_SIMPLEC_OBJS

#include <struct.h>

typedef struct objs {
	size_t		alloc;	// 每个对象的字节大小
	size_t		size;   // 对象池容量
	size_t		len;	// 当前可用对象数
	void *		as[];	// 对象池
} * objs_t;

//
// objs_create - 构建一个对象池
// alloc	: 对象大小, sizeof(type)
// size		: 对象池容量, 0 就是默认大小
// return	: NULL表示构建失败, 默认都是成功 
//
extern objs_t objs_create(size_t alloc, size_t size);

//
// objs_delete - 销毁一个对象池
// pool		: 对象池对象
// return	: void
//
extern void objs_delete(objs_t pool);

//
// objs_malloc - 从对象池中创建对象, 内存是置0的
// pool		: 对象池对象
// return	: void
//
extern void * objs_malloc(objs_t pool);

//
// objs_free - 将对象返还给对象池
// pool		: 对象池对象
// return	: void
//
extern void objs_free(objs_t pool, void * obj);

#endif // !_H_SIMPLEC_OBJS
