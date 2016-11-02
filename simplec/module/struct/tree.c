#include "tree.h"

//内部使用的主要结构
struct tree {
	//保存二叉树的头结点
	struct __tnode* root;

	//构建,释放,删除操作的函数指针
	new_f new;
	cmp_f acmp;
	cmp_f gdcmp;
	die_f del;
};


/*
 * new   : 结点申请内存用的函数指针, 对映参数中是 特定结构体指针
 * acmp  : 用于添加比较
 * gdcmp : 两个结点比较函数,用户查找和删除
 * del   : 结点回收函数,第一个参数就是 二叉树中保存的结点地址
 * ret   : 返回创建好的二叉树结构, 这里是 tree_t 结构
 */
tree_t 
tree_create(new_f new, cmp_f acmp, cmp_f gdcmp, die_f del)
{
	tree_t root = malloc(sizeof(struct tree));
	if (NULL == root)
		CERR_EXIT("malloc struct tree error!");

	//初始化挨个操作
	memset(root, 0, sizeof(struct tree));
	root->new = new;
	root->acmp = acmp;
	root->gdcmp = gdcmp;
	root->del = del;

	return root;
}

/*
 * proot  : 指向tree_t 根结点的指针,
 * node	  : 待处理的结点对象, 会调用new(node) 创建新结点
 * ret    : proot 即是输入参数也是返回参数,返回根结点返回状况
 */
void 
tree_add(tree_t* proot, void* node)
{
	tree_t tm;
	struct __tnode *n, *p = NULL;
	cmp_f cmp;
	int tmp = 0;

	if ((!proot) || (!node) || !(tm = *proot)) //参数无效直接返回
		return;
	if (!(n = tm->root)) { //插入的结点为头结点,直接赋值返回
		tm->root = tm->new(node);
		return;
	}
	//下面开始找 待插入结点
	cmp = tm->acmp;
	while (n) {
		if ((tmp = cmp(node, n)) == 0) //这种情况是不允许插入的
			return;
		p = n;
		if (tmp < 0)
			n = n->lc;
		else
			n = n->rc;
	}

	//找见了开始插入结点
	if (tmp < 0)
		p->lc = tm->new(node);
	else
		p->rc = tm->new(node);
}

/*
 * proot  : 输入和输出参数,指向根结点的指针
 * node   : 删除结点,这里会调用 cmp(node 左参数, foreach) 找见,通过del(find) 删除
 */
void 
tree_del(tree_t* proot, void* node)
{
	tree_t tm;
	struct __tnode *n, *p, *t, *tp;
	if ((!proot) || (!node) || !(tm = *proot) || !(tm->root))
		return;
	//查找一下这个结点,如果不存在直接返回
	if (!(n = tree_get(tm, node, (void**)&p)))
		return;
	//第一种删除和操作
	if ((!n->lc || !n->rc) && !(t = n->lc)) 
			t = n->rc;
	else { //第二种情况,将右子树最小结点和当前删除结点交换
		for (tp = n, t = tp->rc; (t->lc); tp = t, t = t->lc)
			; //找见了最小的左子树结点n 和父结点p
		if (tp->lc == t)
			tp->lc = t->rc;
		else
			tp->rc = t->rc;
		//移动孩子关系
		t->lc = n->lc;
		t->rc = n->rc;
	}

	if (!p) //设置新的root结点
		tm->root = t;
	else {
		if (p->lc == n) //调整父亲和孩子关系,需要你理解二叉查找树,否则那就相信我吧
			p->lc = t;
		else
			p->rc = t;
	}
	//这里释放那个结点
	if (tm->del)
		tm->del(n);
}

/*
 * root   : 根结点,查找的总对象
 * node   : 查找条件,会通过cmp(node, foreach)去查找
 * parent : 返回查找到的父亲结点
 * ret	 : 返回查找到的结点对象
 */
void* 
tree_get(tree_t root,const void* node, void** parent)
{
	struct __tnode *n, *p = NULL;
	cmp_f cmp;
	int tmp;

	if(parent) //初始化功能
		*parent = NULL;
	if ((!node) || (!root) || !(n = root->root))
		return NULL;
	//查找结点
	cmp = root->gdcmp;
	while (n) {
		if ((tmp = cmp(node, n)) == 0){ //这种情况是不允许插入的		
			//返回父亲结点,没有就置空
			if (parent)
				*parent = p;	
			break;
		}
		p = n;
		if (tmp < 0)
			n = n->lc;
		else
			n = n->rc;
	}

	return n;
}

//实际的删除函数,采用后续删除
static void __tree_destroy(struct __tnode* root, die_f del)
{
	if (root) {
		__tree_destroy(root->lc, del);
		__tree_destroy(root->rc, del);
		del(root); //结点删除采用注册方法
	}
}

/*
 * proot  : 指向二叉树数结点指针
 * 会调用 del(foreach) 去删除所有结点,并将所有还原到NULL
 */
void 
tree_destroy(tree_t* proot)
{
	tree_t root;
	if ((!proot) || !(root = *proot))
		return;
	if (root->root && root->del)
		__tree_destroy(root->root, root->del);
	free(*proot); //单独释放最外层内容
	*proot = NULL;
}
