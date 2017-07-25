#include <socket_message.h>

static sserver_t _server;

inline void 
sockmsg_init(void) {
	_server = sserver_create();
}

inline void 
sockmsg_free(void) {
	sserver_delete(_server);
	_server = NULL;
}

inline void 
sockmsg_exit(void) {
	sserver_exit(_server);
}

// mainloop thread
static void
forward_message(int type, bool padding, struct smessage * result) {
	size_t sz = sizeof(struct sockmsg);
	if (padding) {
		if (result->data) {
			size_t msg_sz = strlen(result->data);
			if (msg_sz > 128) {
				msg_sz = 128;
			}
			sz += msg_sz;
		}
		else {
			result->data = "";
		}
	}
	struct sockmsg * sm = malloc(sz);
	sm->type = type;
	sm->rms.id = result->id;
	if (padding) {
		sm->rms.ud = sz - sizeof(struct sockmsg);
		sm->rms.data = NULL;
		memcpy(sm + 1, result->data, sz - sizeof(struct sockmsg));
	}
	else {
		sm->rms.ud = result->ud;
		sm->rms.data = result->data;
	}

	//
	// 这里数据数据需要插入指定的消息队列中, 等待处理.
	//
	// ...
}

int
sockmsg_poll(void) {
	int more = 1;
	struct smessage result;
	assert(_server != NULL);
	int type = sserver_poll(_server, &result, &more);
	switch (type) {
	case SSERVER_EXIT:
		return 0;
	case SSERVER_DATA:
	case SSERVER_CLOSE:
	case SSERVER_UDP:
	case SSERVER_WARNING:
		forward_message(type, false, &result);
		break;
	case SSERVER_OPEN:
	case SSERVER_ACCEPT:
	case SSERVER_ERR:
		forward_message(type, true, &result);
		break;
	default:
		RETURN(-1, "Unknown socket message type %d.", type);
	}
	return more ? -1 : 1;
}

inline int 
sockmsg_send(int id, void * buffer, int sz) {
	return sserver_send(_server, id, buffer, sz);
}

inline int 
sockmsg_send_lowpriority(int id, void * buffer, int sz) {
	return sserver_send_lowpriority(_server, id, buffer, sz);
}

inline int 
sockmsg_listen(uintptr_t opaque, const char * host, uint16_t port) {
	return sserver_listen(_server, opaque, host, port);
}

inline int 
sockmsg_connect(uintptr_t opaque, const char * host, uint16_t port) {
	return sserver_connect(_server, opaque, host, port);
}

inline int 
sockmsg_bind(uintptr_t opaque, socket_t fd) {
	return sserver_bind(_server, opaque, fd);
}

inline void 
sockmsg_close(uintptr_t opaque, int id) {
	sserver_close(_server, opaque, id);
}

inline void 
sockmsg_shutdown(uintptr_t opaque, int id) {
	sserver_shutdown(_server, opaque, id);
}

inline void 
sockmsg_start(uintptr_t opaque, int id) {
	sserver_start(_server, opaque, id);
}

inline void 
sockmsg_nodelay(int id) {
	sserver_nodelay(_server, id);
}

inline int 
sockmsg_udp(uintptr_t opaque, const char * addr, uint16_t port) {
	return sserver_udp(_server, opaque, addr, port);
}

inline int 
sockmsg_udp_connect(int id, const char * addr, uint16_t port) {
	return sserver_udp_connect(_server, id, addr, port);
}

inline int 
sockmsg_udp_send(uintptr_t opaque, int id, const char * address, const void * buffer, int sz) {
	return sserver_udp_send(_server, id, (const udpaddr_t)address, buffer, sz);
}

inline const char *
sockmsg_udp_address(struct sockmsg * msg, int * addrsz) {
	if (msg->type == SSERVER_UDP) {
		struct smessage sm = msg->rms;
		return (const char *)sserver_udp_address(_server, &sm, addrsz);
	}
	return NULL;
}