#include <array.h>

#define _INT_ARRAY_SIZE		(16)		// 数组的默认大小

/*
 * 返回创建数组对象
 * size		: 创建数组的总大小个数
 * alloc	: 数组中每个元素的字节数
 *			: 返回创建的数组对象
 */
array_t 
array_new(unsigned size, size_t alloc) {
	struct array * a = malloc(sizeof(struct array));
	assert(NULL != a);
	// 指定默认size大小
	size = size ? size : _INT_ARRAY_SIZE;
	a->as = malloc(size * alloc);
	assert(NULL != a->as);
	a->len = 0;
	a->size = size;
	a->alloc = alloc;

	return a;
}

/*
 * 销毁这个创建的数组对象
 * a		: 创建的数组对象
 */
inline void 
array_die(array_t a) {
	if (a) {
		free(a->as);
		free(a);
	}
}

/*
 * 重新构建一个数组对象
 * a		: 可变数组对象
 * size		: 新可变数组总长度
 */
inline void
array_init(array_t a, unsigned size) {
	assert(NULL != a);
	a->as = realloc(a->as, size * a->alloc);
	assert(NULL != a->as);
	if (a->len > size)
		a->len = size;
	a->size = size;
}

/*
 * 为可变数组插入一个元素, 并返回这个元素的首地址
 * a		: 可变数组对象
 *			: 返回创建对象位置
 */
inline void * 
array_push(array_t a) {
	assert(NULL != a);

	if (a->len == a->size) {
		/* the array is full; allocate new array */
		a->size <<= 1;
		a->as = realloc(a->as, a->size * a->alloc);
		assert(NULL != a->as);
	}

	return (unsigned char *)a->as + a->alloc * a->len++;
}

/*
 * 弹出一个数组元素
 * a		: 可变数组对象
 *			: 返回弹出数组元素节点
 */
inline void * 
array_pop(array_t a) {
	assert(NULL != a && 0 != a->len);
	--a->len;
	return (unsigned char *)a->as + a->alloc * a->len;
}

/*
 * 按照索引得到数组元素
 * a		: 可变数组对象
 * idx		: 索引位置
 *			: 返回查询到数据
 */
inline void * 
array_get(array_t a, unsigned idx) {
	assert(NULL != a && idx < a->len);
	return (unsigned char *)a->as + a->alloc * idx;
}

/*
 * 得到节点elem在数组中索引
 * a		: 可变数组对象
 * elem		: 查询元素
 *			: 返回查询到位置
 */
inline unsigned 
array_idx(array_t a, void * elem) {
	unsigned char * p, * q;
	unsigned off;

	assert(NULL != a && elem >= a->as);

	p = a->as;
	q = elem;
	off = (unsigned)(q - p);

	assert(off % (unsigned)a->alloc == 0);

	return off / (unsigned)a->alloc;
}

/*
 * 得到数组顶的元素
 * a		: 可变数组对象
 *			: 返回得到元素
 */
inline void * 
array_top(array_t a) {
	assert(NULL != a && 0 != a->len);
	return (unsigned char *)a->as + a->alloc * (a->len - 1);
}

/*
 * 两个数组进行交换
 * a		: 数组a
 * b		: 数组b
 */
inline 
void array_swap(array_t a, array_t b) {
	struct array t = *a;
	*a = *b;
	*b = t;
}

/*
 * 数组进行排序
 * a		: 数组对象
 * compare	: 比对规则
 */
inline void 
array_sort(array_t a, cmp_f compare) {
	assert(NULL != a && 0 != a->len && NULL != compare);
	qsort(a->as, a->len, a->alloc, (int (*)(const void *, const void *))compare);
}

/*
 * 数组进行遍历
 * a        : 可变数组对象
 * func     : 执行每个结点函数, typedef int (* each_f)(void * node, void * arg);
 * arg      : 附加参数
 * return   : 返回操作结果状态码
 */
int 
array_each(array_t a, each_f func, void * arg) {
	int rt;
	unsigned char * s, * e;

	assert(NULL != a && NULL != func);

	s = a->as;
	e = s + a->alloc * a->len;
	while (s < e) {
		rt = func(s, arg);
		if (SufBase != rt)
			return rt;
		s += a->alloc;
	}

	return SufBase;
}