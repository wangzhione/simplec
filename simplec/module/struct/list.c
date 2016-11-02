#include <list.h>

/*
 *  采用头查法插入结点, 第一使用需要 list_t head = NULL;
 *返回 _RT_OK 表示成功!
 * ph		: 指向头结点的指针
 * node		: 待插入的结点对象
 */
int 
list_add(list_t* ph, void* node)
{
	if (ph == NULL || node == NULL){
		CERR("list_add 检查到(pal == NULL || node == NULL)!");
		return RT_ErrorParam;
	}

	list_next(node) = *ph;
	*ph = node;

	return RT_SuccessBase;
}

/*
 * 链表中查找函数,查找失败返回NULL,查找成功直接返回那个结点,推荐不要乱改,否则就崩了.
 *如果需要改的话,推荐 用 list_findpop, 找到并弹出
 * h		: 链表头结点
 * cmp		: 查找的比较函数
 * left		: cmp(left, right) 用的左结点
 *			: 返回查找的结点对象
 */
void* 
list_find(list_t h, cmp_f cmp, const void* left)
{
	struct __lnode* head;
	if(cmp == NULL || left == NULL){
		CERR("list_find 检查到(cmp == NULL || left == NULL)!");
		return NULL;
	}
	//找到结果直接结束
	for(head = h; head; head = head->next)
		if(cmp(left, head) == 0)
			break;
	return head;
}

/*
 * 查找到要的结点,并弹出,需要你自己回收
 * ph		: 指向头结点的指针
 * cmp		: 比较函数,将left同 *ph中对象按个比较
 * left		: cmp(left, x) 比较返回 0 >0 <0
 *			: 找到了退出/返回结点, 否则返回NULL
 */
void* 
list_findpop(list_t *ph, cmp_f cmp, const void* left)
{
	struct __lnode *head, *tmp;
	if((!ph) || (!cmp) || (!left) || !(head = *ph)){
		CERR("check find {(!ph) || (!cmp) || (!left) || !(head = *ph)}!");
		return NULL;
	}
	//头部检测
	if(cmp(left, head) == 0){
		*ph = head->next;
		return head;
	}
	//后面就是普通的
	while((tmp = head->next)){
		if(cmp(left, tmp) == 0){
			head->next = tmp->next;
			break;
		}
		head = tmp;
	}
	
	return tmp; //仍然没有找见
}

/*
 * 这里获取当前链表长度, 推荐调用一次就记住len
 * h		: 当前链表的头结点
 *			: 返回 链表长度 >=0
 */
int list_len(list_t h)
{
	int len = 0;
	while(h){
		++len;
		h = list_next(h);
	}
	return len;
}

/*
 * 查找索引位置为idx的结点,找不见返回NULL
 * h		: 当前结点
 * idx		: 查找的索引值[0,len)
 *			: 返回查到的结点,如果需要删除的推荐调用 list_pop(&h, idx);
 */
void* 
list_get(list_t h, int idx)
{
	if(h==NULL || idx < 0){
		CERR("check is {h==NULL || idx < 0}");
		return NULL;
	}
	//主要查找函数,代码还是比较精简的还是值得学习的
	while(h){
		if(idx-- == 0)
			return h;
		h = list_next(h);
	}
	
	if(idx > 0)
		CERR("check is idx >= length!, idx-length=%d.", idx);
	return NULL;
}

/*
 * 按照索引弹出并返回结点, 需要自己回收这个结点 推荐 free(list_pop...);
 * ph		: 指向链表结点的指针
 * idx		: 弹出的索引
 * return	: 无效的弹出,返回NULL
 */
void* 
list_pop(list_t* ph, int idx)
{
	struct __lnode *head, *front;//第一个是要找的结点,后面是它的前驱结点
	if((!ph) || (idx<0) || !(head=*ph)){
		CERR("check is {(!ph) || (idx<0) || !(head=*ph)}");
		return NULL;
	}
	
	for(front = NULL; head && idx>0; --idx){
		front = head;
		head = head->next;
		--idx;
	}
	
	if(idx>0){
		CERR("check is idx>length, idx-length = %d.", idx);
		return NULL;
	}
	//下面就是找到的请况,返回结果
	if(front == NULL)
		*ph = head->next;
	else
		front->next = head->next;
	return head;
}

/*
 * 返回结点node 的上一个结点,如果node = NULL, 返回最后一个结点
 * h		: 当前链表结点
 * node		: 待查找的结点信息
 * return	: 返回查找到的结点,不存在返回NULL
 */
void* 
list_front(list_t h, void* node)
{
	struct __lnode* head = h; //直接跑到崩溃同strcpy
	while(head->next && head->next != node)
		head = head->next;
	return head->next == node ? head : NULL;
}

/*
 * 和 list_add 功能相似,但是插入位置在尾巴那
 * ph		: 待插入结点的指针
 * node		: 待插入的当前结点
 */ 
int 
list_addlast(list_t* ph, void* node)
{
	struct __lnode* head;
	if(!ph || !node){
		CERR("check is {!ph || !node}! not nothing in it!");
		return RT_ErrorParam;
	}
	
	list_next(node) = NULL;//将这个结点的置空
	if(!(head=*ph)){ //插入的是头结点直接返回
		*ph = node;
		return RT_SuccessBase;
	}
	
	while(head->next)
		head = head->next;
	head->next = node;
	return RT_SuccessBase;
}

/*
 * 在链表的第idx索引处插入结点,也必须需要 list_t head = NULL; 在idx过大的时候
 *插入尾巴处,如果<0直接返回 _RT_EP. 成功了返回 _RT_OK
 * ph		: 指向头结点的指针
 * idx		: 结点的索引处
 * node		: 待插入的结点
*/
int 
list_addidx(list_t* ph, int idx, void* node)
{
	struct __lnode* head;
	if(!ph || idx<0 || !node){ //以后可能加入 idx < 0的尾巴插入细则
		CERR("check is {!ph || idx<0 || !node}! Don't naughty again!");
		return RT_ErrorParam;
	}
	//插入做为头结点
	if(!(head=*ph) || idx == 0){
		list_next(node) = *ph;
		*ph = node;
		return RT_SuccessBase;
	}
	
	while(head->next && idx>1){
		--idx;
		head = head->next;
	}
	list_next(node) = head->next;
	head->next = node;
	return RT_SuccessBase;
}

/*
 * 这里的销毁函数,只有这些数据都是栈上的才推荐这么做,会自动让其指向NULL
 * ph 		: 指向当前链表结点的指针
 */
void 
list_destroy(list_t* ph)
{
	struct __lnode *head, *next;
	if((!ph) || !(head=*ph))
		return;
	do{ //do 循环可以省略一次判断,但是有点丑陋
		next = head->next;
		free(head);
	}while((head=next));
	
	*ph = NULL;
}