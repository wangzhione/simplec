#include <smq.h>
#include <scatom.h>

#define _INT_DQSZ		(64)
#define _INT_OVERLOAD	(1024)

// in_global = true 表示在全局队列的队列中
struct smq {
	int lock;
	uint32_t handle;
	int cap;
	int head;
	int tail;
	bool release;
	bool in_global;
	int overload;
	int overload_threshold;
	struct smsg * queue;
	struct smq * next;
};

struct gsmq {
	int lock;
	struct smq * head;
	struct smq * tail;
};

static struct gsmq _q;

//
// smq_global_push - 消息队列插入到全局消息队列管理器中
// smq_global_pop - 全局消息队列弹出数据
//
void 
smq_global_push(smq_t q) {
	ATOM_LOCK(_q.lock);
	assert(q->next == NULL);
	if (_q.tail)
		_q.tail->next = q;
	else
		_q.head = q;
	_q.tail = q;
	ATOM_UNLOCK(_q.lock);
}

smq_t 
smq_global_pop(void) {
	ATOM_LOCK(_q.lock);
	struct smq * q = _q.head;
	if (q) {
		_q.head = q->next;
		if (NULL == _q.head) {
			assert(q == _q.tail);
			_q.tail = NULL;
		}
		q->next = NULL;
	}
	ATOM_UNLOCK(_q.lock);
	return q;
}

//
// smq_create - 创建一个消息队列
// smq_release - 添加释放标识, 并放入全局消息队列中
// smq_delete - 试图清除一个消息队列, 没有释放标识会放入全局消息队列中 
//
smq_t 
smq_create(uint32_t handle) {
	struct smq * q = calloc(1, sizeof(struct smq));
	assert(NULL != q);
	q->handle = handle;
	q->cap = _INT_DQSZ;
	q->in_global = true;
	q->overload_threshold = _INT_OVERLOAD;
	q->queue = malloc(sizeof(struct smsg) * _INT_DQSZ);
	assert(NULL != q->queue);
	return q;
}

void 
smq_release(smq_t q) {
	ATOM_LOCK(q->lock);
	assert(q->release == false);
	q->release = true;
	if (!q->in_global)
		smq_global_push(q);
	ATOM_UNLOCK(q->lock);
}

static void _smq_drop(struct smq * q, smsgdie_f drop, void * ud) {
	if (drop) {
		struct smsg msg;
		while (smq_pop(q, &msg))
			drop(&msg, ud);
	}

	assert(q->next == NULL);
	free(q->queue);
	free(q);
}

void 
smq_delete(smq_t q, smsgdie_f drop, void * ud) {
	ATOM_LOCK(q->lock);
	if (q->release) {
		ATOM_UNLOCK(q->lock);
		_smq_drop(q, drop, ud);
	}
	else {
		smq_global_push(q);
		ATOM_UNLOCK(q->lock);
	}
}

//
// smq_pop - 弹出消息队列中消息, 返回true表示弹出成功
// smq_push - 消息队列中插入数据
//
bool 
smq_pop(smq_t q, smsg_t m) {
	ATOM_LOCK(q->lock);
	bool succ = q->head != q->tail;
	if (succ) {
		*m = q->queue[q->head++];

		if (q->head >= q->cap)
			q->head = 0;
		int len = q->tail - q->head;
		if (len < 0)
			len += q->cap;
		while (len > q->overload_threshold) {
			q->overload = len;
			q->overload_threshold *= 2;
		}
	}
	else {
		// reset overload_threshold when queue is empty
		q->overload_threshold = _INT_OVERLOAD;
		q->in_global = false;
	}
	ATOM_UNLOCK(q->lock);
	return succ;
}

static void _smq_expand(struct smq * q) {
	struct smsg * nqueue = malloc(sizeof(struct smsg) * q->cap * 2);
	for (int i = 0; i < q->cap; ++i)
		nqueue[i] = q->queue[(q->head + i) % q->cap];
	q->head = 0;
	q->tail = q->cap;
	q->cap *= 2;

	free(q->queue);
	q->queue = nqueue;
}

void 
smq_push(smq_t q, smsg_t m) {
	assert(q && m);
	ATOM_LOCK(q->lock);

	q->queue[q->tail] = *m;
	if (++q->tail >= q->cap)
		q->tail = 0;
	if (q->head == q->tail)
		_smq_expand(q);

	if (!q->in_global) {
		q->in_global = true;
		smq_global_push(q);
	}

	ATOM_UNLOCK(q->lock);
}

//
// smq_get_handle - 得到注册时候的handle
// smq_get_length - 得到此刻消息队列中长度
// smq_get_overload - 得到此刻消息队列中容量
// 
inline uint32_t 
smq_get_handle(smq_t q) {
	return q->handle;
}

int 
smq_get_length(smq_t q) {
	int head, tail, cap;
	ATOM_LOCK(q->lock);
	head = q->head;
	tail = q->tail;
	cap = q->cap;
	ATOM_UNLOCK(q->lock);

	return tail - head + (head <= tail ? 0 : cap);
}

inline int 
smq_get_overload(smq_t q) {
	int overload = q->overload;
	q->overload = 0;
	return overload;
}