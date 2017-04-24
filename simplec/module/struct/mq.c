#include <assert.h>
#include <scatom.h>
#include <mq.h>

#define _INT_MQ				(64)

//
// 栈empty	<=> tail == -1 ( head = 0 )
// 栈full	<=> head == cap
//
struct mq {
	int lock;			// 消息队列锁
	int cap;			// 消息队列容量, 必须是2的幂
	int head;			// 消息队列头索引
	int tail;			// 消息队列尾索引
	void ** queue;		// 具体的使用消息
};

//
// mq_create - 创建一个消息队列类型
// return	: 返回创建好的消息队列对象, NULL表示失败
//
inline mq_t 
mq_create(void) {
	struct mq * q = malloc(sizeof(struct mq));
	assert(q);
	q->lock = 0;
	q->cap = _INT_MQ;
	q->head = 0;
	q->tail = -1;
	q->queue = malloc(sizeof(void *) * _INT_MQ);
	return q;
}

//
// mq_delete - 删除创建消息队列, 并回收资源
// mq		: 消息队列对象
// return	: void
//
inline void 
mq_delete(mq_t mq) {
	if (mq) {
		free(mq->queue);
		free(mq);
	}
}

// add two cap memory, return 0 is error
static int
_expand_queue(struct mq * mq) {
	int i, j, cap = mq->cap << 1;
	void ** nqueue = realloc(mq->queue, sizeof(void *) * cap);
	if (!nqueue) return -1;
	
	// 开始移动内存位置
	for (i = 0; i < mq->head; ++i) {
		void * tmp = mq->queue[i];
		for (j = i; j < mq->cap; j += mq->head) 
			mq->queue[j] = mq->queue[(mq->head + j) & (mq->cap - 1)];
		mq->queue[j & (mq->cap - 1)] = tmp;
	}
	
	mq->head = 0;
	mq->tail = mq->cap;
	mq->cap  = cap;
	mq->queue = nqueue;
	return 0;
}

//
// mq_push - 消息队列中压入数据
// mq		: 消息队列对象
// msg		: 压入的消息
// return	: void
// 
void 
mq_push(mq_t mq, void * msg) {
	int tail;
	assert(mq && msg);
	ATOM_LOCK(mq->lock);

	tail = (mq->tail + 1) & (mq->cap - 1);
	// 队列为full的时候申请内存
	if (mq->tail >= 0 && tail == mq->head) {
		if (_expand_queue(mq)) return;
	}
	else
		mq->tail = tail;

	mq->queue[mq->tail] = msg;

	ATOM_UNLOCK(mq->lock);
}

//
// mq_pop - 消息队列中弹出消息,并返回
// mq		: 消息队列对象
// return	: 返回队列尾巴, 队列为empty返回NULL
//
void * mq_pop(mq_t mq) {
	void * msg = NULL;
	assert(mq);

	ATOM_LOCK(mq->lock);

	if (mq->tail >= 0) {
		msg = mq->queue[mq->head];
		if(mq->tail != mq->head)
			mq->head = (mq->head + 1) & (mq->cap - 1);
		else {
			// 这是empty,情况, 重置
			mq->tail = -1;
			mq->head = 0;
		}
	}

	ATOM_UNLOCK(mq->lock);

	return msg;
}

//
// mq_len - 得到消息队列的长度,并返回
// mq		: 消息队列对象
// return	: 返回消息队列长度
// 
int 
mq_len(mq_t mq) {
	int head, tail, cap;
	assert(mq);

	ATOM_LOCK(mq->lock);

	cap = mq->cap;
	head = mq->head;
	tail = mq->tail;

	ATOM_UNLOCK(mq->lock);

	tail -= head - 1;
	return tail < 0 ? tail + cap : tail;
}