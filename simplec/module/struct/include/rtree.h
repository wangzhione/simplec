#ifndef _H_SIMPLEC_RTREE
#define _H_SIMPLEC_RTREE

#include <struct.h>

struct $rnode {
	uintptr_t parent_color;
	struct $rnode * right;
	struct $rnode * left;
};

typedef struct {
	struct $rnode * root;
	vnew_f new;
	node_f die;
	cmp_f cmp;
} * rtree_t;

/*
 * 每个想使用红黑树的结构, 需要在头部插入下面宏. 
 * 例如 :
	struct person {
		$RTREE_HEAD;
		... // 自定义信息
	};
 */
#define $RTREE_HEAD struct $rnode $node

/*
 * 创建一颗红黑树头结点 
 * new		: 注册创建结点的函数
 * die		: 注册程序销毁函数
 * cmp		: 注册比较的函数
 * return	: 返回创建好的红黑树结点
 */
extern rtree_t rb_create(vnew_f new, node_f die, cmp_f cmp);

/*
 * 插入一个结点, 会插入 new(pack)
 * root		: 红黑树头结点
 * pack		: 待插入的结点当cmp(x, pack) 右结点
 */
extern void rb_insert(rtree_t tree, void * pack);

/*
 * 删除能和pack匹配的结点
 * root		: 红黑树结点
 * pack		: 当cmp(x, pack) 右结点
 */
extern void rb_remove(rtree_t tree, void * pack);

/*
 * 得到红黑树中匹配的结点
 * root		: 匹配的结点信息
 * pack		: 当前待匹配结点, cmp(x, pack)当右结点处理
 */
extern void * rb_find(rtree_t tree, void * pack);

/*
 * 销毁这颗二叉树
 * root		: 当前红黑树结点
 */
extern void rb_delete(rtree_t tree);

#endif /* _H_SIMPLEC_RTREE */