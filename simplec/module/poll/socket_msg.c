#include <socket_msg.h>

static sserver_t _server;

inline void 
server_init(void) {
	_server = sserver_create();
}

inline void 
server_free(void) {
	sserver_delete(_server);
	_server = NULL;
}

inline void 
server_exit(void) {
	sserver_exit(_server);
}

// mainloop thread
static void _server_msg(int type, bool padding, struct smessage * result) {
	struct smsg * sm;
	size_t sz = sizeof(struct smsg);
	if (padding) {
		if (result->data)
			sz += strlen(result->data);
		else
			result->data = "";
	}

	if ((sm = malloc(sz)) == NULL)
		EXIT("malloc error sz = %zu.", sz);

	sm->type = type;
	sm->id = result->id;
	sm->ud = result->ud;
	if (padding) {
		sm->data = NULL;
		memcpy(sm + 1, result->data, sz - sizeof(struct smsg));
	}
	else {
		sm->data = result->data;
	}

	//
	// 这里数据数据需要插入指定的消息队列中, 等待处理.
	//
	// ...
	EXTERN_RUN(ss_push, result->opaque, sm);
}

int
server_poll(void) {
	int more = 1;
	struct smessage result;
	int type = sserver_poll(_server, &result, &more);
	switch (type) {
	case SSERVER_EXIT:
		return 0;
	case SSERVER_DATA:
		_server_msg(SERVER_DATA, false, &result);
		break;
	case SSERVER_CLOSE:
		_server_msg(SERVER_CLOSE, false, &result);
		break;
	case SSERVER_OPEN:
		_server_msg(SERVER_CONNECT, true, &result);
		break;
	case SSERVER_ERR:
		_server_msg(SERVER_ERROR, true, &result);
		break;
	case SSERVER_ACCEPT:
		_server_msg(SERVER_ACCEPT, true, &result);
		break;
	case SSERVER_UDP:
		_server_msg(SERVER_UDP, false, &result);
		break;
	case SSERVER_WARNING:
		_server_msg(SERVER_WARNING, false, &result);
		break;
	default:
		RETURN(-1, "Unknown socket message type %d.", type);
	}
	return more ? -1 : 1;
}

inline int 
server_send(int id, void * buffer, int sz) {
	return sserver_send(_server, id, buffer, sz);
}

inline int 
server_send_lowpriority(int id, void * buffer, int sz) {
	return sserver_send_lowpriority(_server, id, buffer, sz);
}

inline int 
server_listen(uintptr_t opaque, const char * host, uint16_t port) {
	return sserver_listen(_server, opaque, host, port);
}

inline int 
server_connect(uintptr_t opaque, const char * host, uint16_t port) {
	return sserver_connect(_server, opaque, host, port);
}

inline int 
server_bind(uintptr_t opaque, socket_t fd) {
	return sserver_bind(_server, opaque, fd);
}

inline void 
server_close(uintptr_t opaque, int id) {
	sserver_close(_server, opaque, id);
}

inline void 
server_shutdown(uintptr_t opaque, int id) {
	sserver_shutdown(_server, opaque, id);
}

inline void 
server_start(uintptr_t opaque, int id) {
	sserver_start(_server, opaque, id);
}

inline void 
server_nodelay(int id) {
	sserver_nodelay(_server, id);
}

inline int 
server_udp(uintptr_t opaque, const char * addr, uint16_t port) {
	return sserver_udp(_server, opaque, addr, port);
}

inline int 
server_udp_connect(int id, const char * addr, uint16_t port) {
	return sserver_udp_connect(_server, id, addr, port);
}

inline int 
server_udp_send(uintptr_t opaque, int id, const char * address, const void * buffer, int sz) {
	return sserver_udp_send(_server, id, (const udpaddr_t)address, buffer, sz);
}

inline const char *
server_udp_address(struct smsg * msg, int * addrsz) {
	if (msg->type == SSERVER_UDP) {
		struct smessage sm = { msg->id, 0, msg->ud, msg->data };
		return (const char *)sserver_udp_address(_server, &sm, addrsz);
	}
	return NULL;
}