#include <rsmq.h>
#include <scatom.h>

//
// msgrs_create - msgrs构建函数, 客户端发送 write(fd, msg->data, msg->sz);
// msg		: 待填充的消息体
// data		: 客户端待发送的消息体
// sz		: data 的长度
// type		: 发送的消息类型, 默认0是 RSMQ_TYPE_INFO
// return	: 创建好的消息体
//
inline msgrs_t 
msgrs_create(const void * data, uint32_t sz) {
	DEBUG_CODE({
		if (!data || sz <= 0 || sz > USHRT_MAX)
			EXIT("msgrs_create params data = %p, sz = %u", data, sz);
	});
	uint32_t szn = sz + sizeof(uint32_t); 
	msgrs_t msg = malloc(sizeof(*msg) + szn);
	if (NULL == msg)
		EXIT("malloc sizeof uint32_t + %u err!", sz);
	msg->sz = szn;

	// type + sz -> 协议值 -> 网络传输约定值
	szn = MSGRS_SZ(0, sz);
	szn = sh_hton(szn);
	// 开始内存填充
	memcpy(msg->data, &szn, sizeof(uint32_t));
	memcpy((char *)msg->data + sizeof(uint32_t), data, sz);

	return msg;
}

inline void 
msgrs_delete(msgrs_t msg) {
	if (msg) free(msg);
}

#define _INT_RECVMQ	(1 << 8)

//
// tail == -1 ( head = 0 ) -> queue empty
// push head == tail + 1 -> queue full
//
struct rsmq {
	int lock;
	int cap;
	int head;
	int tail;
	char * buff;
	// buffq.sz is msg header body size length
	uint32_t sz;
};

inline rsmq_t
rsmq_create(void) {
	struct rsmq * buff = malloc(sizeof(struct rsmq));
	buff->lock = 0;
	buff->cap = _INT_RECVMQ;
	buff->head = 0;
	buff->tail = -1;
	buff->buff = malloc(buff->cap);
	buff->sz = 0;
	return buff;
}

inline void 
rsmq_delete(rsmq_t q) {
	if (q) {
		free(q->buff);
		free(q);
	}
}

static inline int _rsmq_len(rsmq_t q) {
	int tail = q->tail;
	if (tail < 0)
		return 0;

	tail -= q->head - 1;
	return tail > 0 ? tail : tail + q->cap;
}

static inline void _rsmq_expand(rsmq_t q, int sz) {
	// 确定是够需要扩充内存
	int cap = q->cap;
	int head = q->head;
	int len = _rsmq_len(q);
	if (len + sz <= cap)
		return;

	// 开始构建所需内存
	do cap <<= 1; while (len + sz > cap);
	char * nbuf = malloc(cap);
	assert(NULL != nbuf);

	if (len <= 0)
		q->tail = -1;
	else {
		if (head + len <= cap)
			memcpy(nbuf, q->buff + head, len);
		else {
			memcpy(nbuf, q->buff + head, q->cap - head);
			memcpy(nbuf + q->cap - head, q->buff, len + head - q->cap);
		}
		q->tail = len;
	}

	// 数据定型操作
	free(q->buff);
	q->cap = cap;
	q->head = 0;
	q->buff = nbuf;
}

void 
rsmq_push(rsmq_t q, const void * data, uint32_t sz) {
	int tail, cap, len = (int)sz;
	ATOM_LOCK(q->lock);

	// 开始检测一下内存是否足够
	_rsmq_expand(q, len);

	cap = q->cap;
	tail = q->tail + 1;
	// 分布填充数据
	if (tail + len < cap)
		memcpy(q->buff + tail, data, len);
	else {
		memcpy(q->buff + tail, data, cap - tail);
		memcpy(q->buff, (const char *)data + cap - tail, tail + len - cap);
	}
	q->tail = (q->tail + len) & (cap - 1);

	ATOM_UNLOCK(q->lock);
}

static void _rsmq_pop_dn(rsmq_t q, void * d, int len) {
	char * nbuf = d;
	int head = q->head, cap = q->cap;

	if (head + len <= cap)
		memcpy(nbuf, q->buff + head, len);
	else {
		memcpy(nbuf, q->buff + head, q->cap - head);
		memcpy(nbuf + q->cap - head, q->buff, len + head - q->cap);
	}

	q->head = (head + len) & (cap - 1);
	// 这是empty,情况, 重置
	if (_rsmq_len(q) == q->cap) {
		q->tail = -1;
		q->head = 0;
	}
}

static inline void _rsmq_pop_sz(rsmq_t q) {
	_rsmq_pop_dn(q, &q->sz, sizeof(q->sz));
	q->sz = sh_ntoh(q->sz);
}

//
// rsmq_pop - 数据队列中弹出一个解析好的消息体
// q		: 数据队列对象, rsmq_create 创建
// pmsg		: 返回的消息体对象指针
// return	: ErrParse 协议解析错误, ErrEmpty 数据不完整, SufBase 解析成功
//
int 
rsmq_pop(rsmq_t q, msgrs_t * pmsg) {
	int len, cnt;
	msgrs_t msg;
	ATOM_LOCK(q->lock);

	cnt = _rsmq_len(q);

	// step 1 : 报文长度 buffq.sz check
	if (q->sz <= 0 && cnt >= sizeof(uint32_t)) {
		// 得到报文长度, 小端网络字节转成本地字节
		_rsmq_pop_sz(q);
		cnt -= sizeof(q->sz);
	}

	
	len = MSGRS_LEN(q->sz);
	// step 2 : check data parse is true
	if (len > USHRT_MAX || (q->sz > 0 && len <= 0)) {
		ATOM_UNLOCK(q->lock);
		return ErrParse;
	}

	// step 3 : buffq.sz > 0 继续看是否有需要的报文内容
	if (len <= 0 || len > cnt) {
		ATOM_UNLOCK(q->lock);
		return ErrEmpty;
	}

	// 索要的报文长度存在, 构建好给弹出去
	msg = malloc(sizeof(*msg) + len);
	assert(NULL != msg);
	// 返回数据
	msg->sz = q->sz;
	_rsmq_pop_dn(q, msg->data, len);
	q->sz = 0;
	*pmsg = msg;

	ATOM_UNLOCK(q->lock);
	return SufBase;
}