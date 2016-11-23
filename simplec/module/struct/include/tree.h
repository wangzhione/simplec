#ifndef _H_SIMPLEC_TREE
#define _H_SIMPLEC_TREE

#include <struct.h>

//
// 定义二叉查找树基础结构
// tree_t		: 二叉查找树类型
// tnode		: 定义查找树规则的结构
// _HEAD_TREE	: 这个结构需要放在希望使用二叉排序树规则的结构体第一行
//
typedef struct tree * tree_t;

struct tnode {
	struct tnode * right;
	struct tnode * left;
};

//
// 这个宏必须放在使用的结构体第一行
//
#define _TREE_HEAD	struct tnode $node

//
// 二叉查找树构建函数, 需要注册构建结点规则, 插入, 删除和比较规则, 销毁结点规则
// new		: 结点申请内存用的函数指针, 对映参数中是 特定结构体指针
// acmp		: 用于添加比较
// gdcmp	: 两个结点比较函数,用户查找和删除
// die		: 结点回收函数,第一个参数就是 二叉树中保存的结点地址
// return   : 返回创建好的二叉树结构, 这里是 
//
extern tree_t tree_create(new_f new, cmp_f acmp, cmp_f gdcmp, die_f die);

// 
// 删除这个二叉查找树,会调用 del(foreach) 去删除所有结点.
// root		: 二叉查找树数对象
//
extern void tree_delete(tree_t root);

//
// 二叉树的插入和删除
// root		: 指向tree_t 根结点的指针,
// node		: 待处理的结点对象, 通过cmp(node, foreach) -> new or die
//
extern void tree_insert(tree_t root, void * node);
extern void tree_remove(tree_t root, void * node);

//
// 通过结点数据查找想要的结点返回 
// root		: 二叉树查找树对象
// node		: 待查找结点, 会通过cmp(node, foreach)进行查找
// return	: 返回查找到的结点, 返回NULL表示没有查到
//
extern void * tree_find(tree_t root, const void * node);

//
// 通过结点数据查找想要的结点返回, 也会返回父亲结点数据
// root		: 根结点,查找的总对象
// node		: 查找条件,会通过cmp(node, foreach)去查找
// pparent	: 返回查找到的父亲结点
// return	: 返回查找到的结点对象
//
extern void* tree_get(tree_t root, const void * node, void ** pparent);

#endif // !_H_SIMPLEC_TREE