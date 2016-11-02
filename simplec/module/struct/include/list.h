#ifndef _H_LIST
#define _H_LIST

#include <schead.h>

/*
 *	这个万能单链表库 前提所有结点都是堆上分配的,设计的比较老了,能用
 *注意
 *	1.使用的时候,需要加上 _LIST_HEAD; 宏
 *	2.创建的第一句话就是 list head = NULL; 开始从空链表开始list的生涯
 */

struct __lnode {
	struct __lnode* next;
};

// 不多说了一定放在想使用链表结构的结构体头部
#define _LIST_HEAD \
	struct __lnode __ln;

// 简单链表结构, 当你使用这个链表的时候 需要 list_t head = NULL; 开始使用之旅
typedef void* list_t;

/*
 *  采用头查法插入结点, 第一使用需要 list_t head = NULL;
 *返回 _RT_OK 表示成功!
 * ph		: 指向头结点的指针
 * node		: 待插入的结点对象
 */
extern int list_add(list_t* ph, void* node);

/*
 * 链表中查找函数,查找失败返回NULL,查找成功直接返回那个结点,推荐不要乱改,否则就崩了.
 *如果需要改的话,推荐 用 list_findpop, 找到并弹出
 * h		: 链表头结点
 * cmp		: 查找的比较函数
 * left		: cmp(left, right) 用的左结点
 *			: 返回查找的结点对象
 */
extern void* list_find(list_t h, cmp_f cmp, const void* left);

/*
 * 查找到要的结点,并弹出,需要你自己回收
 * ph		: 指向头结点的指针
 * cmp		: 比较函数,将left同 *ph中对象按个比较
 * left		: cmp(left, x) 比较返回 0 >0 <0
 *			: 找到了退出/返回结点, 否则返回NULL
 */
extern void* list_findpop(list_t *ph, cmp_f cmp, const void* left);

/*
 * 这里获取当前链表长度, 推荐调用一次就记住len
 * h		: 当前链表的头结点
 *			: 返回 链表长度 >=0
 */
extern int list_len(list_t h);

/*
 * 查找索引位置为idx的结点,找不见返回NULL
 * h		: 当前结点
 * idx		: 查找的索引值[0,len)
 *			: 返回查到的结点,如果需要删除的推荐调用 list_pop(&h, idx);
 */
extern void* list_get(list_t h, int idx);

/*
 * 按照索引弹出并返回结点, 需要自己回收这个结点 推荐 free(list_pop...);
 * ph		: 指向链表结点的指针
 * idx		: 弹出的索引
 * return	: 无效的弹出,返回NULL
 */
void* list_pop(list_t* ph, int idx);

/*
 * 返回结点node 的上一个结点,如果node = NULL, 返回最后一个结点
 * h		: 当前链表结点
 * node		: 待查找的结点信息
 * return	: 返回查找到的结点,不存在返回NULL
 */
void* list_front(list_t h, void* node);

/*
 * 这个宏推荐不使用, 主要返回结点n的下一个结点
 * 第一种使用方法 node->next = (void*)list_node(n), 另一种是 list_node(n) = node;
 * n		: 当前结点
 */
#define list_next(n) \
	(((struct __lnode*)n)->next)

/*
 * 和 list_add 功能相似,但是插入位置在尾巴那
 * ph		: 待插入结点的指针
 * node		: 待插入的当前结点
 */ 
int list_addlast(list_t* ph, void* node);

/*
 * 在链表的第idx索引处插入结点,也必须需要 list_t head = NULL; 在idx过大的时候
 *插入尾巴处,如果<0直接返回 _RT_EP. 成功了返回 _RT_OK
 * ph		: 指向头结点的指针
 * idx		: 结点的索引处
 * node		: 待插入的结点
*/
int list_addidx(list_t* ph, int idx, void* node);

/*
 * 这里的销毁函数,只有这些数据都是栈上的才推荐这么做,会自动让其指向NULL
 * ph 		: 指向当前链表结点的指针
 */
void list_destroy(list_t* ph);

#endif // !_H_LIST