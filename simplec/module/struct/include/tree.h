#ifndef _H_TREE
#define _H_TREE

#include <schead.h>

/*
*  这里是简单二叉查找树封装的基库,封装库的库
*  需要用的的一些辅助结构,主要是通用结构和申请释放的函数指针
*/
typedef struct tree * tree_t;


// __开头一般意思是不希望你使用,私有的,系统使用
struct __tnode {
	struct __tnode * lc;
	struct __tnode * rc;
};
/*
*   这个宏必须放在使用的结构体开头,如下
*  struct persion {
		_TREE_HEAD;
		char* name;
		int age;
		...
*  }
*
*/
#define _TREE_HEAD \
	struct __tnode __tn


/*
* new   : 结点申请内存用的函数指针, 对映参数中是 特定结构体指针
* acmp  : 用于添加比较 
* gdcmp : 两个结点比较函数,用户查找和删除
* del   : 结点回收函数,第一个参数就是 二叉树中保存的结点地址
* ret   : 返回创建好的二叉树结构, 这里是 tree_t 结构
*/
extern tree_t tree_create(new_f new, cmp_f acmp, cmp_f gdcmp, die_f del);

/*
* proot  : 指向tree_t 根结点的指针,
* node	 : 待处理的结点对象, 会调用new(node) 创建新结点
* ret    : proot 即是输入参数也是返回参数,返回根结点返回状况
*/
extern void tree_add(tree_t* proot, void* node);

/*
* proot  : 输入和输出参数,指向根结点的指针
* node   : 删除结点,这里会调用 cmp(node 左参数, foreach) 找见,通过del(find) 删除
*/
extern void tree_del(tree_t* proot, void* node);

/*
* root   : 根结点,查找的总对象
* node   : 查找条件,会通过cmp(node, foreach)去查找
* parent : 返回查找到的父亲结点
* ret	 : 返回查找到的结点对象
*/
extern void* tree_get(tree_t root, const void* node, void** parent);

/*
* proot  : 指向二叉树数结点指针
* 会调用 del(foreach) 去删除所有结点,并将所有还原到NULL
*/
extern void tree_destroy(tree_t* proot);

#endif // !_H_TREE
