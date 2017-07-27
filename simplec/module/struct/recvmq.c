#include <recvmq.h>
#include <scatom.h>

#define _INT_RECVMQ	(1 << 9)

//
// tail == -1 ( head = 0 ) -> queue empty
// push head == tail + 1 -> queue full
//
struct recvmq {
	int lock;
	int cap;
	int head;
	int tail;
	char * buff;
	// buffq.sz is msg header body size length
	uint32_t sz;
};

inline recvmq_t
recvmq_create(void) {
	struct recvmq * buff = malloc(sizeof(struct recvmq));
	buff->lock = 0;
	buff->cap = _INT_RECVMQ;
	buff->head = 0;
	buff->tail = -1;
	buff->buff = malloc(buff->cap);
	buff->sz = 0;
	return buff;
}

inline void 
recvmq_delete(recvmq_t buff) {
	if (buff) {
		free(buff->buff);
		free(buff);
	}
}

static inline int _recvmq_len(recvmq_t buff) {
	int tail = buff->tail;
	if (tail < 0)
		return 0;

	tail -= buff->head - 1;
	return tail > 0 ? tail : tail + buff->cap;
}

static inline void _recvmq_expand(recvmq_t buff, int sz) {
	// 确定是够需要扩充内存
	int cap = buff->cap;
	int head = buff->head;
	int len = _recvmq_len(buff);
	if (len + sz <= cap)
		return;

	// 开始构建所需内存
	do cap <<= 1; while (len + sz > cap);
	char * nbuf = malloc(cap);
	assert(NULL != nbuf);

	if (len <= 0)
		buff->tail = -1;
	else {
		if (head + len <= cap)
			memcpy(nbuf, buff->buff + head, len);
		else {
			memcpy(nbuf, buff->buff + head, buff->cap - head);
			memcpy(nbuf + buff->cap - head, buff->buff, len + head - buff->cap);
		}
		buff->tail = len;
	}

	// 数据定型操作
	free(buff->buff);
	buff->cap = cap;
	buff->head = 0;
	buff->buff = nbuf;
}

void 
recvmq_push(recvmq_t buff, const void * data, uint32_t sz) {
	int tail, cap, len = (int)sz;
	ATOM_LOCK(buff->lock);

	// 开始检测一下内存是否足够
	_recvmq_expand(buff, len);

	cap = buff->cap;
	tail = buff->tail + 1;
	// 分布填充数据
	if (tail + len < cap)
		memcpy(buff->buff + tail, data, len);
	else {
		memcpy(buff->buff + tail, data, cap - tail);
		memcpy(buff->buff, (const char *)data + cap - tail, tail + len - cap);
	}
	buff->tail = (buff->tail + len) & (cap - 1);

	ATOM_UNLOCK(buff->lock);
}

static void _recvmq_pop_dn(recvmq_t buff, void * d, int len) {
	char * nbuf = d;
	int head = buff->head, cap = buff->cap;

	if (head + len <= cap)
		memcpy(nbuf, buff->buff + head, len);
	else {
		memcpy(nbuf, buff->buff + head, buff->cap - head);
		memcpy(nbuf + buff->cap - head, buff->buff, len + head - buff->cap);
	}

	buff->head = (head + len) & (cap - 1);
	// 这是empty,情况, 重置
	if (_recvmq_len(buff) == buff->cap) {
		buff->tail = -1;
		buff->head = 0;
	}
}

static inline void _recvmq_pop_sz(recvmq_t buff) {
	_recvmq_pop_dn(buff, &buff->sz, sizeof(buff->sz));
	buff->sz = sh_ntoh(buff->sz);
}

//
// recvmq_pop - 数据队列中弹出一个解析好的消息体
// buff		: 数据队列对象, buffq_create 创建
// psz		: 返回对象长度, -> len
// return	: NULL 表示没有完整数据 -> data
//
void * 
recvmq_pop(recvmq_t buff, uint32_t * psz) {
	int len;
	char * data = NULL;
	ATOM_LOCK(buff->lock);

	len = _recvmq_len(buff);

	// step 1 : 报文长度 buffq.sz check
	if (buff->sz <= 0)
	{
		if (len < sizeof(uint32_t))
			goto __rpop;
		// 得到报文长度, 小端网络字节转成本地字节
		_recvmq_pop_sz(buff);
	}

	// step 2 : buffq.sz > 0 继续看是否有需要的报文内容
	if (buff->sz <= 0 || (int)buff->sz > len)
		goto __rpop;

	// 索要的报文长度存在, 构建好给弹出去
	data = malloc(buff->sz);
	assert(NULL != data);
	_recvmq_pop_dn(buff, data, (int)buff->sz);
	if (psz)
		*psz = buff->sz;
	buff->sz = 0;

__rpop:
	ATOM_UNLOCK(buff->lock);
	return data;
}