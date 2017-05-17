#include <objs.h>

// 默认对象池大小
#define _UINT_SIZE		(128u)

//
// objs_create - 构建一个对象池
// alloc	: 对象大小, sizeof(type)
// size		: 对象池容量, 0 就是默认大小
// return	: NULL表示构建失败, 默认都是成功 
//
objs_t 
objs_create(size_t alloc, size_t size) {
	objs_t p;

	// 默认的对象池大小
	if (size <= 0) size = _UINT_SIZE;
	p = malloc(sizeof(struct objs) + sizeof(void *) * size);
	if (NULL == p)
		RETURN(NULL, "malloc struct objsvoid * error size = %zd.", size);

	p->size = size;
	p->alloc = alloc;
	p->len = 0u;

	return p;
}

//
// objs_delete - 销毁一个对象池
// pool		: 对象池对象
// return	: void
//
void 
objs_delete(objs_t pool) {
	if (pool) {
		if (pool->as) {
			size_t i = 0;
			while (i < pool->len)
				free(pool->as[i++]);
		}
		free(pool);
	}
}

//
// objs_malloc - 从对象池中创建对象, 内存是置0的
// pool		: 对象池对象
// return	: void
//
inline void * 
objs_malloc(objs_t pool) {
	if (pool->len <= 0)
		return calloc(1, pool->alloc);
	return pool->as[--pool->len];
}

//
// objs_free - 将对象返还给对象池
// pool		: 对象池对象
// return	: void
//
void 
objs_free(objs_t pool, void * obj) {
	if (pool->len >= pool->size)
		free(obj);
	else {
		pool->as[pool->len++] = obj;
		memset(obj, 0, pool->alloc);
	}
}