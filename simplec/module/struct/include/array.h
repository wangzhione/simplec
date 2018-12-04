#ifndef _H_SIMPLEC_ARRAY
#define _H_SIMPLEC_ARRAY

#include <struct.h>

struct array {
    void *      as;     /* 存储数组具体内容首地址 */
    unsigned    len;    /* 当前数组的长度 */
    unsigned    size;   /* 当前数组容量大小 */
    size_t      alloc;  /* 每个元素字节大小 */
};

// 定义可变数组类型 对象
typedef struct array * array_t;

/*
 * 在栈上创建对象var
 * var		: 创建对象名称
 * size		: 创建对象总长度
 * alloc	: 每个元素分配空间大小
 */
#define ARRAY_NEW(var, size, alloc) \
	struct array var[1] = { { NULL, 0, 0, alloc } }; \
	array_init(var, size)
#define ARRAY_DIE(var) \
	free(var->as)

/*
 * 返回创建数组对象
 * size		: 创建数组的总大小个数, 0表示走默认值创建
 * alloc	: 数组中每个元素的字节数, need > 1 否则行为未知
 *			: 返回创建的数组对象
 */
extern array_t array_new(unsigned size, size_t alloc);

/*
 * 销毁这个创建的数组对象
 * a		: 创建的数组对象
 */
extern void array_die(array_t a);

/*
 * 重新构建一个数组对象
 * a		: 可变数组对象
 * size		: 新可变数组总长度
 */
extern void array_init(array_t a, unsigned size);

/*
 * 为可变数组插入一个元素, 并返回这个元素的首地址
 * a		: 可变数组对象
 *			: 返回创建对象位置
 */
extern void * array_push(array_t a);

/*
 * 弹出一个数组元素
 * a		: 可变数组对象
 *			: 返回弹出数组元素节点
 */
extern void * array_pop(array_t a);

/*
 * 按照索引得到数组元素
 * a		: 可变数组对象
 * idx		: 索引位置
 *			: 返回查询到数据
 */
extern void * array_get(array_t a, unsigned idx);

/*
 * 得到节点elem在数组中索引
 * a		: 可变数组对象
 * elem		: 查询元素
 *			: 返回查询到位置
 */
extern unsigned array_idx(array_t a, void * elem);

/*
 * 得到数组顶的元素
 * a		: 可变数组对象
 *			: 返回得到元素
 */
extern void * array_top(array_t a);

/*
 * 两个数组进行交换
 * a		: 数组a
 * b		: 数组b
 */
extern void array_swap(array_t a, array_t b);

/*
 * 数组进行排序
 * a		: 数组对象
 * compare	: 比对规则
 */
extern void array_sort(array_t a, cmp_f compare);

/*
 * 数组进行遍历
 * a        : 可变数组对象
 * func     : 执行每个结点函数, typedef int (* each_f)(void * node, void * arg);
 * arg      : 附加参数
 * return   : 返回操作结果状态码
 */
int array_each(array_t a, each_f func, void * arg);

#endif//_H_SIMPLEC_ARRAY