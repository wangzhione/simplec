#include <mq.h>
#include <scatom.h>

// 2 的 幂
#define _INT_MQ				(1 << 6)

//
// pop empty	<=> tail == -1 ( head = 0 )
// push full	<=> head + 1 == tail
//
struct mq {
    int lock;           // 消息队列锁
    int cap;            // 消息队列容量, 必须是2的幂
    int head;           // 消息队列头索引
    int tail;           // 消息队列尾索引
    void ** queue;      // 具体的使用消息

    volatile bool fee;  // true表示销毁退出
};

//
// mq_create - 创建一个消息队列类型
// die		: 删除push进来的结点
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
	assert(q->queue);
	q->fee = false;
	return q;
}

//
// mq_delete - 删除创建消息队列, 并回收资源
// mq		: 消息队列对象
// die		: 删除push进来的结点
// return	: void
//
void 
mq_delete(mq_t mq, node_f die) {
	if (!mq || mq->fee) return;
	ATOM_LOCK(mq->lock);
	mq->fee = true;
	// 销毁所有对象
	if (mq->tail >= 0 && die) {
		for(;;) {
			die(mq->queue[mq->head]);
			if (mq->tail == mq->head)
				break;
			mq->head = (mq->head + 1) & (mq->cap - 1);
		}
	}
	free(mq->queue);
	ATOM_UNLOCK(mq->lock);
	free(mq);
}

// add two cap memory, memory is do not have assert
static void _mq_expand(struct mq * mq) {
	int i, cap = mq->cap << 1;
	void ** nqueue = malloc(sizeof(void *) * cap);
	assert(nqueue);
	
	for (i = 0; i < mq->cap; ++i)
		nqueue[i] = mq->queue[(mq->head + i) & (mq->cap - 1)];
	
	mq->head = 0;
	mq->tail = mq->cap;
	mq->cap  = cap;
	free(mq->queue);
	mq->queue = nqueue;
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
	if (!mq || mq->fee || !msg) return;
	ATOM_LOCK(mq->lock);

	tail = (mq->tail + 1) & (mq->cap - 1);
	// 队列为full的时候申请内存
	if (tail == mq->head && mq->tail >= 0)
		_mq_expand(mq);
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
void * 
mq_pop(mq_t mq) {
	void * msg = NULL;
	if (!mq || mq->fee) return NULL;

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
	if (!mq || mq->fee) return 0;

	ATOM_LOCK(mq->lock);
	tail = mq->tail;
	if (tail < 0) {
		ATOM_UNLOCK(mq->lock);
		return 0;
	}
	cap = mq->cap;
	head = mq->head;
	ATOM_UNLOCK(mq->lock);

	tail -= head - 1;
	return tail > 0 ? tail : tail + cap;
}