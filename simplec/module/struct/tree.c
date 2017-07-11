#include <tree.h>

//
// 二叉查找树构建函数, 需要注册构建结点规则, 插入, 删除和比较规则, 销毁结点规则
// new		: 结点申请内存用的函数指针, 对映参数中是 特定结构体指针
// acmp		: 用于添加比较
// gdcmp	: 两个结点比较函数,用户查找和删除
// die		: 结点回收函数,第一个参数就是 二叉树中保存的结点地址
// return   : 返回创建好的二叉树结构, 这里是 
//
inline tree_t
tree_create(new_f new, cmp_f acmp, cmp_f gdcmp, die_f die) {
	tree_t root = malloc(sizeof(*root));
	if (NULL == root)
		CERR_EXIT("malloc struct tree error!");

	// 挨个初始化数据, 最终返回结果
	root->root = NULL;
	root->new = new;
	root->acmp = acmp;
	root->gdcmp = gdcmp;
	root->die = die;

	return root;
}

// 具体的核心删除操作
static void _tree_delete(struct $tnode * root, die_f die) {
	if (root) {
		_tree_delete(root->left, die);
		_tree_delete(root->right, die);
		die(root);
	}
}

// 
// 删除这个二叉查找树,会调用 del(foreach) 去删除所有结点.
// root		: 二叉查找树数对象
//
inline void
tree_delete(tree_t root) {
	if (!root || !root->root)
		return;

	// 没有执行置空操作
	if (root->die)
		_tree_delete(root->root, root->die);
	free(root);
}

//
// 二叉树的插入和删除
// root		: 指向tree_t 根结点的指针,
// node		: 待处理的结点对象, 通过cmp(node, foreach) -> new or die
//
void
tree_insert(tree_t root, void * node) {
	int tmp;
	cmp_f cmp;
	struct $tnode * tnode, *next, *parent;

	if (!root || !node) {
		RETURN(NIL, "check is root || node!");
	}

	// 构建结点并等待插入时机
	tnode = root->new(node);
	tnode->left = tnode->right = NULL;

	// 头结点插入的话直接返回
	if (!(next = root->root)) {
		root->root = tnode;
		return;
	}

	tmp = 0;
	parent = NULL;
	cmp = root->acmp;
	do {
		if ((tmp = cmp(node, next)) == 0) {
			RETURN(NIL, "tmp cmp node = %p, next = %p, tmp = 0 error.", node, next);
		}

		parent = next;
		next = tmp < 0 ? next->left : next->right;
	} while (next);

	// 默认左结点放大值, 右结点放小值
	if (tmp < 0)
		parent->left = tnode;
	else
		parent->right = tnode;
}

void
tree_remove(tree_t root, void * node) {
	struct $tnode * next, * parent, * tnode, * tparent;
	// 简单检查一下这个结点, 开始处理
	if (!root || !node || !root->root) {
		RETURN(NIL, "check is !root || !node || !root->root.");
	}

	// 没有这个结点直接返回结果
	if (!(next = tree_get(root, node, (void **)&parent))) {
		RETURN(NIL, "tree_get node = %p is not find.", node);
	}

	// 第一种情况, 待删除结点只有一个叶子结点, 保存到tnode中
	if (!next->left || !next->right)
		tnode = next->left ? next->left : next->right;
	else {
		// 第二种情况, 将右子树最小结点和当前结点交换
		for (tparent = next, tnode = tparent->right;
			!!(tnode->left);
			tparent = tnode, tnode = tnode->left)
			;

		// 让右子树最左结点父亲结点重新指向它的儿子结点
		if (tparent->left == tnode)
			tparent->left = tnode->right;
		else
			tparent->right = tnode->right;

		// 移动孩子关系
		tnode->left = next->left;
		tnode->right = next->right;
	}

	if (NULL == parent)
		root->root = tnode;
	else {
		// 调整新的父子关系
		if (parent->left == next)
			parent->left = tnode;
		else
			parent->right = tnode;
	}

	if (root->die)
		root->die(node);
}

//
// 通过结点数据查找想要的结点返回 
// root		: 二叉树查找树对象
// node		: 待查找结点, 会通过cmp(node, foreach)进行查找
// return	: 返回查找到的结点, 返回NULL表示没有查到
//
void *
tree_find(tree_t root, const void * node) {
	int tmp;
	cmp_f cmp;
	struct $tnode * next;

	// 简单检查一下这个结点, 开始处理
	if ((!root) || (!node) || !(next = root->root)) {
		RETURN(NULL, "check is !root || !node || !root->root.");
	}

	// 查找数据查找到了直接返回结果
	cmp = root->gdcmp;
	do {
		if ((tmp = cmp(node, next)) == 0)
			break;
		next = tmp < 0 ? next->left : next->right;
	} while (next);
	return next;
}

//
// 通过结点数据查找想要的结点返回, 也会返回父亲结点数据
// root		: 根结点,查找的总对象
// node		: 查找条件,会通过cmp(node, foreach)去查找
// pparent	: 返回查找到的父亲结点
// return	: 返回查找到的结点对象
//
void *
tree_get(tree_t root, const void * node, void ** pparent) {
	int tmp;
	cmp_f cmp;
	struct $tnode * next, *parent;

	if (pparent)
		*pparent = NULL;

	// 简单检查一下这个结点, 开始处理
	if ((!root) || (!node) || !(next = root->root)) {
		RETURN(NULL, "check is !root || !node || !root->root.");
	}

	// 查找数据查找到了直接返回结果
	cmp = root->gdcmp;
	parent = NULL;

	do {
		if ((tmp = cmp(node, next)) == 0) {
			if (pparent)
				*pparent = parent;
			break;
		}
		parent = next;
		next = tmp < 0 ? next->left : next->right;
	} while (next);

	return next;
}