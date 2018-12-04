#include <rtree.h>

/*
 * 操作辅助宏, 得到红黑树中具体父结点, 颜色. 包括详细设置信息
 * r	: 头结点
 * p	: 父结点新值
 * c	: 当前颜色
 */
#define rb_parent(r)        ((struct $rnode *)((r)->parent_color & ~3))
#define rb_color(r)         ((r)->parent_color & 1)
#define rb_is_red(r)        (!rb_color(r))
#define rb_is_black(r)      rb_color(r)
#define rb_set_red(r)       (r)->parent_color &= ~1
#define rb_set_black(r)     (r)->parent_color |= 1

static inline void rb_set_parent(struct $rnode * r, struct $rnode * p) {
     r->parent_color = (r->parent_color & 3) | (uintptr_t)p;
}

static inline void rb_set_color(struct $rnode * r, int color) {
     r->parent_color = (r->parent_color & ~1) | (1 & color);
}

static inline void * _rb_dnew(void * node) { return node; }
static inline void _rb_ddie(void * node) { }
static inline int _rb_dcmp(const void * ln, const void * rn) { 
	return (int)((intptr_t)ln - (intptr_t)rn); 
}

/*
 * 创建一颗红黑树头结点 
 * new		: 注册创建结点的函数
 * die		: 注册程序销毁函数
 * cmp		: 注册比较的函数
 * return	: 返回创建好的红黑树结点
 */
inline rtree_t 
rb_create(vnew_f new, node_f die, cmp_f cmp) {
	rtree_t tree = malloc(sizeof(*tree));
	if(NULL == tree) {
		RETURN(NULL, "rb_new malloc is error!");
	}
	
	tree->root = NULL;
	tree->new = new ? new : _rb_dnew;
	tree->die = die ? die : _rb_ddie;
	tree->cmp = cmp ? cmp : _rb_dcmp;

	return tree;
}

static inline struct $rnode * _rb_new(rtree_t tree, void * pack) {
	struct $rnode * node = tree->new(pack);
	memset(node, 0, sizeof(struct $rnode));
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
static void _rtree_left_rotate(rtree_t tree, struct $rnode * x) {
    // 设置x的右孩子为y
    struct $rnode * y = x->right;
	struct $rnode * xparent = rb_parent(x);

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
 *       / \                                   / \                  #
 *      lx  rx                                rx  ry
 * 
 */
static void _rtree_right_rotate(rtree_t tree, struct $rnode * y) {
    // 设置x是当前节点的左孩子。
    struct $rnode * x = y->left;
	struct $rnode * yparent = rb_parent(y);

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
static void _rtree_insert_fixup(rtree_t tree, struct $rnode * node) {
    struct $rnode * parent, * gparent, * uncle;

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
                _rtree_left_rotate(tree, parent);
                uncle = parent;
                parent = node;
                node = uncle;
            }

            // Case 3条件：叔叔是黑色，且当前节点是左孩子。
            rb_set_black(parent);
            rb_set_red(gparent);
            _rtree_right_rotate(tree, gparent);
        } else { //若“z的父节点”是“z的祖父节点的右孩子”
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
                _rtree_right_rotate(tree, parent);
                uncle = parent;
                parent = node;
                node = uncle;
            }

            // Case 3条件：叔叔是黑色，且当前节点是右孩子。
            rb_set_black(parent);
            rb_set_red(gparent);
            _rtree_left_rotate(tree, gparent);
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
rb_insert(rtree_t tree, void * pack) {
	cmp_f cmp;
	struct $rnode * node, * x, * y;
	if((!tree) || (!pack) || !(node = _rb_new(tree, pack))) {
		RETURN(NIL, "rb_insert param is empty! tree = %p, pack = %p.\n", tree, pack);
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

	if (NULL == y)
		tree->root = node;              // 情况1：若y是空节点，则将node设为根
	else {
		if (cmp(y, node) > 0)
			y->left = node;             // 情况2：若“node所包含的值” < “y所包含的值”，则将node设为“y的左孩子”
		else
			y->right = node;            // 情况3：(“node所包含的值” >= “y所包含的值”)将node设为“y的右孩子” 
	}

	// 2. 设置节点的颜色为红色
	rb_set_red(node);

	// 3. 将它重新修正为一颗二叉查找树
	_rtree_insert_fixup(tree, node);
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
static void _rtree_delete_fixup(rtree_t tree, struct $rnode * node, struct $rnode * parent) {
    struct $rnode * other;

    while ((!node || rb_is_black(node)) && node != tree->root) {
        if (parent->left == node) {
            other = parent->right;
            if (rb_is_red(other)) {
                // Case 1: x的兄弟w是红色的  
                rb_set_black(other);
                rb_set_red(parent);
                _rtree_left_rotate(tree, parent);
                other = parent->right;
            }
            if ((!other->left || rb_is_black(other->left)) &&
                (!other->right || rb_is_black(other->right))) {
                // Case 2: x的兄弟w是黑色，且w的俩个孩子也都是黑色的  
                rb_set_red(other);
                node = parent;
                parent = rb_parent(node);
            } else {
                if (!other->right || rb_is_black(other->right)) {
                    // Case 3: x的兄弟w是黑色的，并且w的左孩子是红色，右孩子为黑色。  
                    rb_set_black(other->left);
                    rb_set_red(other);
                    _rtree_right_rotate(tree, other);
                    other = parent->right;
                }
                // Case 4: x的兄弟w是黑色的；并且w的右孩子是红色的，左孩子任意颜色。
                rb_set_color(other, rb_color(parent));
                rb_set_black(parent);
                rb_set_black(other->right);
                _rtree_left_rotate(tree, parent);
                node = tree->root;
                break;
            }
        } else {
            other = parent->left;
            if (rb_is_red(other)) {
                // Case 1: x的兄弟w是红色的  
                rb_set_black(other);
                rb_set_red(parent);
                _rtree_right_rotate(tree, parent);
                other = parent->left;
            }
            if ((!other->left || rb_is_black(other->left)) &&
                (!other->right || rb_is_black(other->right))) {
                // Case 2: x的兄弟w是黑色，且w的俩个孩子也都是黑色的  
                rb_set_red(other);
                node = parent;
                parent = rb_parent(node);
            } else {
                if (!other->left || rb_is_black(other->left)) {
                    // Case 3: x的兄弟w是黑色的，并且w的左孩子是红色，右孩子为黑色。  
                    rb_set_black(other->right);
                    rb_set_red(other);
                    _rtree_left_rotate(tree, other);
                    other = parent->left;
                }
                // Case 4: x的兄弟w是黑色的；并且w的右孩子是红色的，左孩子任意颜色。
                rb_set_color(other, rb_color(parent));
                rb_set_black(parent);
                rb_set_black(other->left);
                _rtree_right_rotate(tree, parent);
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
rb_remove(rtree_t tree, void * pack) {
	struct $rnode * child, * parent, * node = NULL;
	int color;
	
	if ((!tree) || !(node = (struct $rnode *)pack)) {
		RETURN(NIL, "rb_remove check is error, tree = %p, node = %p.", tree, node);
	}

	// 被删除节点的"左右孩子都不为空"的情况。
	if (NULL != node->left && node->right != NULL) {
		// 被删节点的后继节点。(称为"取代节点")
		// 用它来取代"被删节点"的位置，然后再将"被删节点"去掉。
		struct $rnode * replace = node;

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
		} else 
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
			_rtree_delete_fixup(tree, child, parent);
		// 结点销毁操作
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
	if (!parent)
		tree->root = child;
	else {
		if (parent->left == node)
			parent->left = child;
		else
			parent->right = child;
	}

	if (!color)
		_rtree_delete_fixup(tree, child, parent);
	tree->die(node);
}

/*
 * 得到红黑树中匹配的结点
 * root		: 匹配的结点信息
 * pack		: 当前待匹配结点, cmp(x, pack)当右结点处理
 */
void * 
rb_find(rtree_t tree, void * pack) {
	cmp_f cmp;
	struct $rnode * node;
	if((!tree) || !pack) {
		RETURN(NULL, "rb_get param is empty! tree = %p, pack = %p.\n", tree, pack);	
	}
	
	cmp = tree->cmp;
	node = tree->root;
	while(node) {
		int ct = cmp(node, pack);
		if(ct == 0)
			break;
		node = ct > 0 ? node->left : node->right;
	}

	return node;
}

// 后序遍历删除操作
static void _rb_delete(struct $rnode * root, node_f die) {
	if(NULL == root) return;
	_rb_delete(root->left, die);
	_rb_delete(root->right, die);
	die(root);
}

/*
 * 销毁这颗二叉树
 * root		: 当前红黑树结点
 */
inline void
rb_delete(rtree_t tree) {
	if(!tree) return;

    // 后续递归删除
    if (tree->root && tree->die == _rb_ddie)
	    _rb_delete(tree->root, tree->die);

	// 销毁树本身内存
	tree->root = NULL;
	free(tree);
}
