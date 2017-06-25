#include <sctimer.h>
#include <scatom.h>
#include <list.h>
#include <pthread.h>

// 使用到的定时器结点
struct stnode {
	_LIST_HEAD;

	int id;						//当前定时器的id
	struct timespec tv;			//运行的具体时间
	die_f timer;				//执行的函数事件
	void * arg;					//执行函数参数
};

// 当前链表对象管理器
struct stlist {
	int lock;					//加锁用的
	int nowid;					//当前使用的最大timer id
	bool status;				//false表示停止态, true表示主线程loop运行态
	pthread_t tid;				//主循环线程id, 0表示没有启动
	struct stnode * head;		//定时器链表的头结点
};

// 定时器对象的单例, 最简就是最复杂
static struct stlist _st;

// 先创建链表对象处理函数
static struct stnode * _stnode_new(int s, die_f timer, void * arg) {
	struct stnode * node = malloc(sizeof(struct stnode));
	if (NULL == node)
		RETURN(NULL, "malloc struct stnode is error!");

	// 初始化, 首先初始化当前id
	node->id = ATOM_ADD_FETCH(_st.nowid, 1);
	timespec_get(&node->tv, TIME_UTC);
	node->tv.tv_sec += s / _INT_STOMS;
	node->tv.tv_nsec += (s % _INT_STOMS) * _INT_MSTONS;
	node->timer = timer;
	node->arg = arg;
	node->$node.next = NULL;

	return node;
}

// 得到等待的微秒时间, <=0的时候头时间就可以执行了
static inline int _sleepus(struct stlist * st) {
	struct timespec tv[1], * th = &st->head->tv;
	timespec_get(tv, TIME_UTC);
	return (unsigned)((th->tv_sec - tv->tv_sec) * _INT_MSTONS
		+ (th->tv_nsec - tv->tv_nsec) / _INT_STOMS);
}

// 重新调整, 只能在 _stlist_loop 后面调用, 线程安全,只加了一把锁
static void _slnode_run(struct stlist * st) {
	struct stnode * sn;

	ATOM_LOCK(st->lock); // 加锁防止调整关系覆盖,可用还是比较重要的
	sn = st->head;
	st->head = (struct stnode *)list_next(sn);
	ATOM_UNLOCK(st->lock);

	sn->timer(sn->arg);
	free(sn);
}

// 运行的主loop,基于timer管理器
static void * _stlist_loop(struct stlist * st) {
	// 正常轮询,检测时间
	while (st->head) {
		int nowt = _sleepus(st);
		if (nowt > 0) {
			usleep(nowt);
			continue;
		}

		_slnode_run(st);
	}

	// 已经运行结束
	st->status = false;
	return NULL;
}

// st < sr 返回 < 0, == 返回 0, > 返回 > 0
static inline int _stnode_cmptime(struct stnode * sl, struct stnode * sr) {
	if (sl->tv.tv_sec != sr->tv.tv_sec)
		return (int)(sl->tv.tv_sec - sr->tv.tv_sec);
	return sl->tv.tv_nsec - sr->tv.tv_nsec;
}

//
// st_add - 添加定时器事件,虽然设置的属性有点多但是都是必要的
// intval	: 执行的时间间隔, <=0 表示立即执行, 单位是毫秒
// timer	: 定时器执行函数
// arg		: 定时器参数指针
// return	: 返回这个定时器的唯一id
//
int 
st_add(int intval, die_f timer, void * arg) {
	struct stnode * now;

	// 各种前戏操作
	if (intval <= 0) {
		timer(arg);
		return Success_Base;
	}

	now = _stnode_new(intval, timer, arg);
	if (NULL == now) {
		RETURN(Error_Alloc, "_new_stnode is error intval = %d.", intval);
	}

	ATOM_LOCK(_st.lock); //核心添加模块 要等, 添加到链表, 看线程能否取消等

	list_add((list_t *)&_st.head, _stnode_cmptime, now);

	// 这个时候重新开启线程
	if(!_st.status){
		int rt = pthread_create(&_st.tid, NULL, (start_f)_stlist_loop, &_st);
		if (rt < 0) {
			list_destroy(&_st.head);
			RETURN(Error_Fd, "pthread_create is error!");
		}
		_st.status = true;
		pthread_detach(_st.tid);
	}

	ATOM_UNLOCK(_st.lock);
	
	return now->id;
}

// 通过id开始查找
static inline int _stnode_cmpid(intptr_t id, struct stnode * sr) {
	return id - sr->id;
}

//
// st_del - 删除指定事件
// id		: st_add 返回的定时器id
// return	: void
//
void 
st_del(int id) {
	struct stnode * node;
	if (!_st.head) return;

	ATOM_LOCK(_st.lock);
	node = list_findpop((list_t *)&_st.head, _stnode_cmpid, (const void *)(intptr_t)id);
	ATOM_UNLOCK(_st.lock);
	
	free(node);
}