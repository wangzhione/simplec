#include "rbtree.h"

/*
 * 操作辅助宏, 得到红黑树中具体父结点, 颜色. 包括详细设置信息
 * r	: 头结点
 * p	: 父结点新值
 * c	: 当前颜色
 */
#define rb_parent(r)		((struct $rbnode *)((r)->parent_color & ~3))
#define rb_color(r)			((r)->parent_color & 1)
#define rb_is_red(r)		(!rb_color(r))
#define rb_is_black(r)		rb_color(r)
#define rb_set_black(r)		(r)->parent_color |= 1
#define rb_set_red(r)		(r)->parent_color &= ~1

static inline void rb_set_parent(struct $rbnode * r, struct $rbnode * p) {
     r->parent_color = (r->parent_color & 3) | (uintptr_t)p;
}

static inline void rb_set_color(struct $rbnode * r, int color) {
     r->parent_color = (r->parent_color & ~1) | (1 & color);
}

static inline int _rb_cmp(const void * ln, const void * rn) {
	return (int)((const char *)ln - (const char *)rn);
}

 /*
  * 创建一颗红黑树头结点
  * new		: 注册创建结点的函数
  * cmp		: 注册比较的函数
  * die		: 注册程序销毁函数
  *			: 返回创建好的红黑树结点
  */
rbtree_t 
rb_create(new_f new, cmp_f cmp, die_f die) {
	rbtree_t tree = malloc(sizeof(*tree));
	if(NULL == tree) {
		CERR("rb_new malloc is error!");
		return NULL;	
	}
	
	tree->root = NULL;
	tree->new = new;
	tree->cmp = cmp ? cmp : _rb_cmp;
	tree->die = die;

	return tree;
}

static inline struct $rbnode * _rb_new(rbtree_t tree, void * pack) {
	struct $rbnode * node = tree->new ? tree->new(pack) : pack;
	memset(node, 0, sizeof(struct $rbnode));
	return node;
}

/* 
 * 对红黑树的节点(x)进行左旋转
 *
 * 左旋示意图(对节点x进行左旋)：
 *      px                              px
 *     /                               /
 *    x                               y                
 *   /  \      --(左旋)-->           / \                #
 *  lx   y                          x  ry     
 *     /   \                       /  \
 *    ly   ry                     lx  ly  
 *
 */
static void _rbtree_left_rotate(rbtree_t tree, struct $rbnode * x) {
    // 设置x的右孩子为y
    struct $rbnode * y = x->right;
	struct $rbnode * xparent = rb_parent(x);

    // 将 “y的左孩子” 设为 “x的右孩子”；
    x->right = y->left;
	// 如果y的左孩子非空，将 “x” 设为 “y的左孩子的父亲”
    if (y->left != NULL)
		rb_set_parent(y->left, x);

    // 将 “x的父亲” 设为 “y的父亲”
	rb_set_parent(y, xparent);

    if (xparent == NULL)
        tree->root = y;            // 如果 “x的父亲” 是空节点，则将y设为根节点
    else {
        if (xparent->left == x)
            xparent->left = y;     // 如果 x是它父节点的左孩子，则将y设为“x的父节点的左孩子”
        else
            xparent->right = y;    // 如果 x是它父节点的左孩子，则将y设为“x的父节点的左孩子”
    }
    
    // 将 “x” 设为 “y的左孩子”
    y->left = x;
    // 将 “x的父节点” 设为 “y”
	rb_set_parent(x, y);
}

/* 
 * 对红黑树的节点(y)进行右旋转
 *
 * 右旋示意图(对节点y进行左旋)：
 *            py                               py
 *           /                                /
 *          y                                x                  
 *         /  \      --(右旋)-->            /  \                     #
 *        x   ry                           lx   y  
 *       / \                                   / \                   #
 *      lx  rx                                rx  ry
 * 
 */
static void _rbtree_right_rotate(rbtree_t tree, struct $rbnode * y) {
    // 设置x是当前节点的左孩子。
    struct $rbnode * x = y->left;
	struct $rbnode * yparent = rb_parent(y);

    // 将 “x的右孩子” 设为 “y的左孩子”；
	y->left = x->right;
    // 如果"x的右孩子"不为空的话，将 “y” 设为 “x的右孩子的父亲”
    if (x->right != NULL)
		rb_set_parent(x->right, y);

    // 将 “y的父亲” 设为 “x的父亲”
    rb_set_parent(x, yparent);
    if (yparent == NULL) 
        tree->root = x;				// 如果 “y的父亲” 是空节点，则将x设为根节点
    else {
        if (y == yparent->right)
            yparent->right = x;		// 如果 y是它父节点的右孩子，则将x设为“y的父节点的右孩子”
        else
            yparent->left = x;		// (y是它父节点的左孩子) 将x设为“x的父节点的左孩子”
    }

    // 将 “y” 设为 “x的右孩子”
    x->right = y;
    // 将 “y的父节点” 设为 “x”
	rb_set_parent(y, x);
}

/*
 * 红黑树插入修正函数
 *
 * 在向红黑树中插入节点之后(失去平衡)，再调用该函数；
 * 目的是将它重新塑造成一颗红黑树。
 *
 * 参数说明：
 *     tree 红黑树的根
 *     node 插入的结点        // 对应《算法导论》中的z
 */
static void _rbtree_insert_fixup(rbtree_t tree, struct $rbnode * node) {
    struct $rbnode * parent, * gparent, * uncle;

    // 若“父节点存在，并且父节点的颜色是红色”
    while ((parent = rb_parent(node)) && rb_is_red(parent)) {
        gparent = rb_parent(parent);

        //若“父节点”是“祖父节点的左孩子”
        if (parent == gparent->left) {
            // Case 1条件：叔叔节点是红色
            uncle = gparent->right;
            if (uncle && rb_is_red(uncle)) {
                rb_set_black(uncle);
                rb_set_black(parent);
                rb_set_red(gparent);
                node = gparent;
                continue;
            }

            // Case 2条件：叔叔是黑色，且当前节点是右孩子
            if (parent->right == node) {
                _rbtree_left_rotate(tree, parent);
                uncle = parent;
                parent = node;
                node = uncle;
            }

            // Case 3条件：叔叔是黑色，且当前节点是左孩子。
            rb_set_black(parent);
            rb_set_red(gparent);
            _rbtree_right_rotate(tree, gparent);
        } 
        else { //若“z的父节点”是“z的祖父节点的右孩子”
            // Case 1条件：叔叔节点是红色
            uncle = gparent->left;
            if (uncle && rb_is_red(uncle)) {
                rb_set_black(uncle);
                rb_set_black(parent);
                rb_set_red(gparent);
                node = gparent;
                continue;
            }

            // Case 2条件：叔叔是黑色，且当前节点是左孩子
            if (parent->left == node) {
                _rbtree_right_rotate(tree, parent);
                uncle = parent;
                parent = node;
                node = uncle;
            }

            // Case 3条件：叔叔是黑色，且当前节点是右孩子。
            rb_set_black(parent);
            rb_set_red(gparent);
            _rbtree_left_rotate(tree, gparent);
        }
    }

    // 将根节点设为黑色
    rb_set_black(tree->root);
}

/*
 * 插入一个结点, 会插入 new(pack)
 * tree		: 红黑树头结点
 * pack		: 待插入的结点当cmp(x, pack) 右结点
 */
void 
rb_insert(rbtree_t tree, void * pack) {
	cmp_f cmp;
	struct $rbnode * node, * x, * y;
	if((!tree) || (!pack) || !(node = _rb_new(tree, pack))) {
		CERR("rb_insert param is empty! tree = %p, pack = %p.\n", tree, pack);
		return;	
	}
	
	cmp = tree->cmp;
	// 开始走插入工作
	y = NULL;
	x = tree->root;

	// 1. 将红黑树当作一颗二叉查找树，将节点添加到二叉查找树中。从小到大
	while (x != NULL) {
		y = x;
		if (cmp(x, node) > 0)
			x = x->left;
		else
			x = x->right;
	}
	rb_set_parent(node, y);

	if (y != NULL) {
		if (cmp(y, node) > 0)
			y->left = node;             // 情况2：若“node所包含的值” < “y所包含的值”，则将node设为“y的左孩子”
		else
			y->right = node;            // 情况3：(“node所包含的值” >= “y所包含的值”)将node设为“y的右孩子” 
	}
	else
		tree->root = node;              // 情况1：若y是空节点，则将node设为根

	// 2. 设置节点的颜色为红色
	rb_set_red(node);

	// 3. 将它重新修正为一颗二叉查找树
	_rbtree_insert_fixup(tree, node);
}

/*
 * 红黑树删除修正函数
 *
 * 在从红黑树中删除插入节点之后(红黑树失去平衡)，再调用该函数；
 * 目的是将它重新塑造成一颗红黑树。
 *
 * 参数说明：
 *     tree 红黑树的根
 *     node 待修正的节点
 */
static void _rbtree_delete_fixup(rbtree_t tree, struct $rbnode * node, struct $rbnode * parent) {
    struct $rbnode * other;

    while ((!node || rb_is_black(node)) && node != tree->root) {
        if (parent->left == node) {
            other = parent->right;
            if (rb_is_red(other)) {
                // Case 1: x的兄弟w是红色的  
                rb_set_black(other);
                rb_set_red(parent);
                _rbtree_left_rotate(tree, parent);
                other = parent->right;
            }
            if ((!other->left || rb_is_black(other->left)) &&
                (!other->right || rb_is_black(other->right))) {
                // Case 2: x的兄弟w是黑色，且w的俩个孩子也都是黑色的  
                rb_set_red(other);
                node = parent;
                parent = rb_parent(node);
            }
            else {
                if (!other->right || rb_is_black(other->right)) {
                    // Case 3: x的兄弟w是黑色的，并且w的左孩子是红色，右孩子为黑色。  
                    rb_set_black(other->left);
                    rb_set_red(other);
                    _rbtree_right_rotate(tree, other);
                    other = parent->right;
                }
                // Case 4: x的兄弟w是黑色的；并且w的右孩子是红色的，左孩子任意颜色。
                rb_set_color(other, rb_color(parent));
                rb_set_black(parent);
                rb_set_black(other->right);
                _rbtree_left_rotate(tree, parent);
                node = tree->root;
                break;
            }
        }
        else {
            other = parent->left;
            if (rb_is_red(other)) {
                // Case 1: x的兄弟w是红色的  
                rb_set_black(other);
                rb_set_red(parent);
                _rbtree_right_rotate(tree, parent);
                other = parent->left;
            }
            if ((!other->left || rb_is_black(other->left)) &&
                (!other->right || rb_is_black(other->right))) {
                // Case 2: x的兄弟w是黑色，且w的俩个孩子也都是黑色的  
                rb_set_red(other);
                node = parent;
                parent = rb_parent(node);
            }
            else {
                if (!other->left || rb_is_black(other->left)) {
                    // Case 3: x的兄弟w是黑色的，并且w的左孩子是红色，右孩子为黑色。  
                    rb_set_black(other->right);
                    rb_set_red(other);
                    _rbtree_left_rotate(tree, other);
                    other = parent->left;
                }
                // Case 4: x的兄弟w是黑色的；并且w的右孩子是红色的，左孩子任意颜色。
                rb_set_color(other, rb_color(parent));
                rb_set_black(parent);
                rb_set_black(other->left);
                _rbtree_right_rotate(tree, parent);
                node = tree->root;
                break;
            }
        }
    }
    if (node)
        rb_set_black(node);
}

/*
 * 删除rb_get得到的结点
 * root		: 红黑树结点
 * pack		: 当cmp(x, pack) 右结点
 */
void 
rb_remove(rbtree_t tree, void * pack) {
	struct $rbnode * child, * parent, * node = NULL;
	int color;
	
	if ((!tree) || !(node = (struct $rbnode *)pack)) {
		CERR("rb_remove check is error, tree = %p, node = %p.", tree, node);
		return;
	}

	// 被删除节点的"左右孩子都不为空"的情况。
	if (NULL != node->left && node->right != NULL) {
		// 被删节点的后继节点。(称为"取代节点")
		// 用它来取代"被删节点"的位置，然后再将"被删节点"去掉。
		struct $rbnode * replace = node;

		// 获取后继节点
		replace = replace->right;
		while (replace->left != NULL)
			replace = replace->left;

		// "node节点"不是根节点(只有根节点不存在父节点)
		if ((parent = rb_parent(node))) {
			if (parent->left == node)
				parent->left = replace;
			else
				parent->right = replace;
		} 
		else 
			// "node节点"是根节点，更新根节点。
			tree->root = replace;

		// child是"取代节点"的右孩子，也是需要"调整的节点"。
		// "取代节点"肯定不存在左孩子！因为它是一个后继节点。
		child = replace->right;
		parent = rb_parent(replace);
		// 保存"取代节点"的颜色
		color = rb_color(replace);

		// "被删除节点"是"它的后继节点的父节点"
		if (parent == node)
			parent = replace; 
		else {
			// child不为空
			if (child)
				rb_set_parent(child, parent);
			parent->left = child;

			replace->right = node->right;
			rb_set_parent(node->right, replace);
		}
		
		rb_set_parent(replace, rb_parent(node));
		rb_set_color(replace, rb_color(node));
		replace->left = node->left;
		rb_set_parent(node->left, replace);

		if (color) // 黑色结点重新调整关系
			_rbtree_delete_fixup(tree, child, parent);
		// 结点销毁操作
		if(tree->die)
			tree->die(node);
		return ;
	}

	if (node->left !=NULL)
		child = node->left;
	else 
		child = node->right;

	parent = rb_parent(node);
	// 保存"取代节点"的颜色
	color = rb_color(node);

	if (child)
		rb_set_parent(child, parent);

	// "node节点"不是根节点
	if (parent) {
		if (parent->left == node)
			parent->left = child;
		else
			parent->right = child;
	}
	else
		tree->root = child;

	if (!color)
		_rbtree_delete_fixup(tree, child, parent);
	if(tree->die)
		tree->die(node);
}

/*
 * 得到红黑树中匹配的结点
 * root		: 匹配的结点信息
 * pack		: 当前待匹配结点, cmp(x, pack)当右结点处理
 */
void * 
rb_get(rbtree_t tree, void * pack) {
	cmp_f cmp;
	struct $rbnode * node;
	if((!tree) || !pack) {
		CERR("rb_get param is empty! tree = %p, pack = %p.\n", tree, pack);
		return NULL;	
	}
	
	cmp = tree->cmp;
	node = tree->root;
	while(node) {
		int ct = cmp(node, pack);
		if(ct == 0)
			return node;
		node = ct > 0 ? node->left : node->right;
	}

	return NULL;
}

// 后序遍历删除操作
static void _rb_delete(struct $rbnode * root, die_f die) {
	if(NULL == root)
		return;
	_rb_delete(root->left, die);
	_rb_delete(root->right, die);
	die(root);
}

/*
 * 销毁这颗二叉树
 * root		: 当前红黑树结点
 */
void
rb_delete(rbtree_t tree) {
	if(!tree || !tree->root || !tree->die)
		return;

	// 后续递归删除
	_rb_delete(tree->root, tree->die);

	// 销毁树本身内存
	tree->root = NULL;
	free(tree);
}