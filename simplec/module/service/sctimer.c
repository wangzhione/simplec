#include <sctimer.h>
#include <scatom.h>
#include <pthread.h>

// 使用到的定时器结点
struct stnode {
	int id;						//当前定时器的id
	time_t stime;				//运行的具体时间到秒
	int ms;						//还需要等待的毫秒数
	int cnt;					//循环执行次数, -1表示一直执行
	int intval;					//下一次轮询的时间间隔
	int type;					//0表示不开启多线程, 1表示开启多线程
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
static struct stnode * _new_stnode(int start, int cnt, int intval, die_f timer, void* arg, bool fb) {
	int s = start / 1000;
	struct stnode * node = malloc(sizeof(struct stnode));
	if (NULL == node)
		CERR_EXIT("_new_stnode malloc node is error!");

	// 初始化, 首先初始化当前id
	node->id = ATOM_ADD_FETCH(_st.nowid, 1);
	node->stime = s + time(NULL);
	node->ms = start - s * 1000;
	node->cnt = cnt > 0 ? cnt + 1 : 0; // 执行到1的时候停止,并且兼容永久时间0
	node->intval = intval;
	node->type = fb;
	node->timer = timer;
	node->arg = arg;
	node->next = NULL;

	return node;
}

// 如果stl < str 返回true, 否则返回false
inline static bool _stnode_cmp(struct stnode * stl, struct stnode * str) {
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

// 得到等待的时间,毫秒, <=0的时候头时间就可以执行了
inline static int _sleeptime(struct stlist * st) {
	struct stnode * head = st->head;
    struct timeval tv;
	gettimeofday(&tv, NULL);
	return (int)(1000 * (head->stime - tv.tv_sec) + head->ms - tv.tv_usec / 1000);
}

// timer线程执行的函数
static void * _slnode_timer(struct stnode * sn) {
	pthread_detach(pthread_self()); //设置线程分离,自销毁
	sn->timer(sn->arg);
	return NULL;
}

// 重新调整, 只能在 _stlist_loop 后面调用, 线程安全,只加了一把锁
static void _slnode_again_run(struct stlist * st) {
	int s, v;
	pthread_t tid;
	struct stnode * sn;

	ATOM_LOCK(st->lock); // 加锁防止调整关系覆盖,可用还是比较重要的
	sn = st->head;
	st->head = sn->next;
	if (sn->cnt == 1){ //这时候不需要了,才开始删除
		ATOM_UNLOCK(st->lock);
		free(sn); 
		return;
	}
	
	//这里需要重新组织数据
	sn->cnt = sn->cnt ? sn->cnt - 1 : 0;
	s = sn->intval + sn->ms;
	v = s / 1000;
	sn->stime += v;
	sn->ms = s - v * 1000;
	
	if (sn->type) // 开始处理,先处理异步模式
		pthread_create(&tid, NULL, (void* (*)(void *))_slnode_timer, sn);
	else //同步模式
		sn->timer(sn->arg);
	_stlist_add(st, sn);
	ATOM_UNLOCK(st->lock);
}

// 运行的主loop,基于timer管理器
static void * _stlist_loop(struct stlist * st) {
	int nowt;
	
	//设置线程分离,自销毁, 默认线程属性 允许退出线程, 设置立即取消 
	pthread_detach(pthread_self());
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	
	// 正常轮询,检测时间
	while (st->head) {
		pthread_testcancel(); //添加测试取消点
		nowt = _sleeptime(st);
		if(nowt <= 0 || st->head->cnt == 1)
			_slnode_again_run(st); //重新调整关系并且开始执行
		else //没有人到这那就继续等待
			sh_sleep(nowt);
	}

	// 已经运行结束
	st->status = false;
	return NULL;
}

/*
 *  添加定时器事件,虽然设置的属性有点多但是都是必要的 .
 * start	: 延迟启动的时间, 0表示立即启动, 单位是毫秒
 * cnt		: 表示执行次数, 0表示永久时间, 一次就为1
 * intval	: 每次执行的时间间隔, 单位是毫秒
 * timer	: 定时器执行函数
 * arg		: 定时器参数指针
 * fb		: 0表示不启用多线程, 1表示启用多线程
 *			: 返回这个定时器的 唯一id
 */
int 
st_add(int start, int cnt, int intval, die_f timer, void * arg, bool fb) {
	struct stnode * now;
	DEBUG_CODE({
		if(start < 0 || cnt < 0 || intval < 0 || !timer)
			CERR_EXIT("debug start,cut,intval,timer => %d,%d,%d,%p.", start, cnt, intval, timer);
	});

	// 这里开始创建对象往 线程队列中添加
	now = _new_stnode(start, cnt, intval, timer, arg, fb);
	
	ATOM_LOCK(_st.lock); //核心添加模块 要等, 添加到链表, 看线程能否取消等

	_stlist_add(&_st, now);
	// 看是否需要取消线程
	if(_st.status && _sleeptime(&_st) < 0) {
		pthread_cancel(_st.tid);
		_st.status = false;
	}
	// 这个时候重新开启线程
	if(_st.status){
		pthread_create(&_st.tid, NULL, (start_f)_stlist_loop, &_st);
		_st.status = true; //延迟真实运行态
	}
	ATOM_UNLOCK(_st.lock);
	
	return now->id;
}

/*
 * 删除指定事件, 删除是临时加上的存在临界的意外.
 * st		: st_add 返回的定时器id
 */
inline void 
st_del(int st) {
	struct stnode * sn = _stlist_del(&_st, st);
	if (sn) {
		free(sn);
	}
}
