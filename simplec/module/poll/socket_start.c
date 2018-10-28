#include "haid.h"
#include <scrunloop.h>
#include <socket_start.h>

#define _INT_CNMAX (4096 - 1)

struct connect {
	int id;	// server socket id
	rsmq_t buffer;
};

struct gate {
	intptr_t opaque;
	int lisid;
	struct haid hash;
	struct connect conn[_INT_CNMAX];
	srl_t mloop;
};

static void _gate_delete(struct gate * g) {
	intptr_t ctx = g->opaque;
	for (int i = 0; i < LEN(g->conn); ++i) {
		struct connect * c = g->conn + i;
		if (c->id >= 0) {
			rsmq_delete(c->buffer);
			server_close(ctx, c->id);
		}
	}
	if (g->lisid >= 0) 
		server_close(ctx, g->lisid);
	haid_clear(&g->hash);
	free(g);
}

static struct gate * _gate_create(const char * host, uint16_t port) {
	struct gate * g = malloc(sizeof(struct gate));
	if (NULL == g)
		RETURN(NULL, "malloc sizeof struct gate error!");
	g->opaque = (intptr_t)g;
	for (int i = 0; i < LEN(g->conn); ++i) {
		g->conn[i].id = -1;
		g->conn[i].buffer = NULL;
	}

	haid_init(&g->hash, LEN(g->conn));
	// 下面开始启动监听操作
	g->lisid = server_listen(g->opaque, host, port);
	if (g->lisid < 0) {
		free(g);
		RETURN(NULL, "server_listen host = %s, port = %hu err.", host, port);
	}
	server_start(g->opaque, g->lisid);
	return g;
}

// socket poll 处理的主体
static void * _server(void * arg) {

	for (;;) {
		int r = server_poll();
		if (r == 0)
			break;
		if (r < 0) {
			// check context is empty need break exit
			continue;
		}
		// awaken block thread 
	}
	
	return NULL;
}

static struct {
	pthread_t tid;
	struct gate * g;
} _ss;

//
// ss_run - 启动单例消息服务器
// host		: 主机名称
// port		: 端口
// run		: 消息解析协议
// return	: void
//
void 
ss_run(const char * host, uint16_t port, void (* run)(msgrs_t)) {
	assert(_ss.g == NULL);

	server_init();
	_ss.g = _gate_create(host, port);
	if (_ss.g == NULL)
		EXIT("gate_create err host = %s, port = %hu.", host, port);

	_ss.g->mloop = srl_create(run, msgrs_delete);

	// 这里开始启动线程跑起来
	if (pthread_create(&_ss.tid, NULL, _server, NULL))
		EXIT("pthread_create is error!");
}

//
// ss_end - 关闭单例的消息服务器
// return	: void
//
void 
ss_end(void) {
	assert(_ss.g != NULL);
	
	server_exit();
	// 等待线程结束
	pthread_join(_ss.tid, NULL);
	srl_delete(_ss.g->mloop);
	_gate_delete(_ss.g);
	server_free();
}

static void _ss_push_data(uintptr_t opaque, struct smsg * sm) {
	int id, r;
	msgrs_t msg;
	struct gate * g = (struct gate *)opaque;
	id = haid_lookup(&g->hash, sm->id);
	if (id > 0) {
		struct connect * c = g->conn + id;
		// 填充数据
		rsmq_push(c->buffer, sm->data, sm->ud);
		// ... 弹出所需要的消息体, 跑上去
		r = rsmq_pop(c->buffer, &msg);
		if (r == ErrParse) {
			CL_ERROR("rsmq_pop parse error %d", sm->id);
			server_close(opaque, sm->id);
		} else if (r == SufBase) {
			// 数据压队列开始处理
			srl_push(g->mloop, msg);
		}

	} else {
		CL_ERROR("Drop unknown connection %d message", sm->id);
		// 岁月给了无尽的伤感, 遗留下的都是莫名的感动~
		server_close(opaque, sm->id);
	}
}

// 
// ss_push - 向socket服务器填入信息
// opaque	: 注入的对象
// sm		: 填入的信息
// return	: void
//
extern void ss_push(uintptr_t opaque, struct smsg * sm) {
	int id;
	struct connect * c;
	struct gate * g = (struct gate *)opaque;
	assert(opaque > 0 && sm);
	switch(sm->type) {
	case SERVER_DATA	:
		_ss_push_data(opaque, sm);
		break;
	case SERVER_CONNECT	:
		// start listening
		if (sm->id == g->lisid)
			break;
		id = haid_lookup(&g->hash, sm->id);
		if (id < 0) {
			CL_ERROR("Close unknown connection %d", sm->id);
			server_close(opaque, sm->id);
		}
		break;
	case SERVER_CLOSE	:
	case SERVER_ERROR	:
		id = haid_remove(&g->hash, sm->id);
		if (id >= 0) {
			c = g->conn + id;
			rsmq_delete(c->buffer);
			c->buffer = NULL;
			c->id = -1;
		}
		break;
	case SERVER_ACCEPT	:
		assert(g->lisid == sm->id);
		if(haid_full(&g->hash)) {
			CL_INFOS("full(%d) hash %d.", sm->id, sm->ud);
			server_close(opaque, sm->id);
		} else {
			id = haid_insert(&g->hash, sm->ud);
			c = g->conn + id;
			c->id = sm->ud;
			//sm + 1 -> data print
			assert(c->buffer == NULL);
			c->buffer = rsmq_create();
			server_start(opaque, c->id);
		}
		break;
	case SERVER_UDP		:
		// 目前UDP功能没有启用, 暂时没有考虑, 其实考虑起来更简单(安全的UDP可以替代TCP)
		break;
	case SERVER_WARNING	:
		CL_ERROR("fd (%d) send buffer (%d)K", sm->id, sm->ud);
		break;
	}

	free(sm->data);
	free(sm);
}