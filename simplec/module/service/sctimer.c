#include <sctimer.h>
#include <scatom.h>
#include <pthread.h>

// 使用到的定时器结点
struct stnode {
	int id;						//当前定时器的id
	time_t stime;				//运行的具体时间到秒
	int ms;						//还需要等待的毫秒数
	die_f timer;				//执行的函数事件
	void * arg;					//执行函数参数
	struct stnode * next;		//下一个定时器结点
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
static struct stnode * _stnode_new(int start, die_f timer, void * arg) {
	int s = start / 1000;
	struct stnode * node = malloc(sizeof(struct stnode));
	if (NULL == node)
		RETURN(NULL, "malloc struct stnode is error!");

	// 初始化, 首先初始化当前id
	node->id = ATOM_ADD_FETCH(_st.nowid, 1);
	node->stime = s + time(NULL);
	node->ms = start - s * 1000;
	node->timer = timer;
	node->arg = arg;
	node->next = NULL;

	return node;
}

// 如果stl < str 返回true, 否则返回false
static inline bool _stnode_cmp(struct stnode * stl, struct stnode * str) {
	return (stl->stime < str->stime) || 
		(stl->stime == str->stime && stl->ms < str->ms);
}

// 添加链表对象, 返回true表示插入的是头结点, 当你执行的时候需要全额加锁
static bool _stlist_add(struct stlist * st, struct stnode * node) {
	struct stnode * head = st->head;

	// 插入为头结点直接返回
	if (!head || _stnode_cmp(node, head)) {
		node->next = head;
		st->head = node;
		return true;
	}

	// 中间插入了
	while (head->next){
		if (_stnode_cmp(node, head->next))
			break;
		head = head->next;
	}
	node->next = head->next;
	head->next = node;

	return false;
}

// 得到等待的时间,毫秒, <=0的时候头时间就可以执行了
static inline time_t _sleepms(struct stlist * st) {
	struct timeval tv;
	struct stnode * head = st->head;
	gettimeofday(&tv, NULL);
	return (head->stime - tv.tv_sec) * 1000 + head->ms - tv.tv_usec / 1000;
}

// 重新调整, 只能在 _stlist_loop 后面调用, 线程安全,只加了一把锁
static void _slnode_run(struct stlist * st) {
	struct stnode * sn;

	ATOM_LOCK(st->lock); // 加锁防止调整关系覆盖,可用还是比较重要的
	sn = st->head;
	st->head = sn->next;
	sn->timer(sn->arg);
	free(sn);
	ATOM_UNLOCK(st->lock);
}

// 运行的主loop,基于timer管理器
static void * _stlist_loop(struct stlist * st) {
	// 正常轮询,检测时间
	while (st->head) {
		int nowt = _sleepms(st);
		if (nowt >= 0) {
			sh_sleep(nowt);
			continue;
		}

		_slnode_run(st);
	}

	// 已经运行结束
	st->status = false;
	return NULL;
}

static void _stlist_delete(struct stlist * st) {
	ATOM_LOCK(st->lock);
	while (st->head) {
		struct stnode * next = st->head->next;
		free(st->head);
		st->head = next;
	}
	st->head = NULL;
	ATOM_UNLOCK(st->lock);
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

	_stlist_add(&_st, now);

	// 这个时候重新开启线程
	if(!_st.status){
		int rt = pthread_create(&_st.tid, NULL, (start_f)_stlist_loop, &_st);
		if (rt < 0) {
			_stlist_delete(&_st);
			RETURN(Error_Fd, "pthread_create is error!");
		}
		_st.status = true;
		pthread_detach(_st.tid);
	}

	ATOM_UNLOCK(_st.lock);
	
	return now->id;
}

// 根据id,删除一个timer结点, 返回NULL表示没有找见不处理,多线程安全的
static struct stnode * _stlist_del(struct stlist * st, int id) {
	struct stnode * head = st->head, * tmp = NULL;
	if (!head) return NULL;

	ATOM_LOCK(st->lock);

	// 删除为头结点直接返回
	if (head->id == id) {
		st->head = head->next;
		tmp = head;
	}
	else { // 中间删除那个结点了
		while (head->next) {
			if (head->next->id == id)
				break;
			head = head->next;
		}
		if (head->next) {
			tmp = head->next;
			head->next = tmp->next;
		}
	}

	ATOM_UNLOCK(st->lock);
	return tmp;
}

//
// st_del - 删除指定事件
// st		: st_add 返回的定时器id
// return	: void
//
inline void 
st_del(int st) {
	free(_stlist_del(&_st, st));
}
