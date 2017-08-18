#include <scatom.h>
#include <socket_server.h>

#define SOCKET_TYPE_INVALID		(0)
#define SOCKET_TYPE_RESERVE		(1)
#define SOCKET_TYPE_PLISTEN		(2)
#define SOCKET_TYPE_LISTEN		(3)
#define SOCKET_TYPE_CONNECTING	(4)
#define SOCKET_TYPE_CONNECTED	(5)
#define SOCKET_TYPE_HALFCLOSE	(6)
#define SOCKET_TYPE_PACCEPT		(7)
#define SOCKET_TYPE_BIND		(8)

#define MAX_INFO				(128)
#define MAX_EVENT				(64)
#define MIN_READ_BUFFER			(64)

#define PRIORITY_HIGH			(0)
#define PRIORITY_LOW			(1)
#define WARNING_SIZE			(1024*1024)

// MAX_SOCKET will be 2 ^ x
#define MAX_SOCKET				(1 << 16)
#define HASH_ID(id)				(((unsigned)id) % MAX_SOCKET)
// IPPROTO_UDP 的伪造补充协议
#define IPPROTO_UDPv6			(IPPROTO_UDP + 1)
// ipv6 128bit + port 16bit + type 1bit | ipv4 32bit + port 16bit + type 1bit
#define SADDRUDPv6_SIZE			(1 + 2 + 16)
#define SADDRUDP_SIZE			(1 + 2 +  4)

struct write_buffer {
	struct write_buffer * next;
	void * buffer;
	char * ptr;
	int sz;
	bool userobject;
	uint8_t udp_address[SADDRUDPv6_SIZE];
};

#define SIZEOF_TCPBUFFER		(offsetof(struct write_buffer, udp_address))
#define SIZEOF_UDPBUFFER		(sizeof(struct write_buffer))

struct wb_list {
	struct write_buffer * head;
	struct write_buffer * tail;
};

struct socket {
	uintptr_t opaque;
	struct wb_list high;
	struct wb_list low;
	int64_t wb_size;
	socket_t fd;
	int id;
	uint8_t protocol;
	uint8_t type;
	uint16_t udpconnecting;
	int64_t warn_size;
	union {
		int size;
		uint8_t udp_address[SADDRUDPv6_SIZE];
	} p;
	int dw_lock;
	int dw_offset;
	const void * dw_buffer;
	size_t dw_size;
};

struct sserver {
	socket_t recvctrl_fd;
	socket_t sendctrl_fd;
	bool checkctrl;
	poll_t event_fd;
	int alloc_id;
	int event_n;
	int event_index;
	struct sinterface soi;
	struct event ev[MAX_EVENT];
	struct socket slot[MAX_SOCKET];
	char buffer[MAX_INFO];
	uint8_t udpbuffer[USHRT_MAX];
	fd_set rfds;
};

struct request_open {
	int id;
	uint16_t port;
	uintptr_t opaque;
	char host[1];
};

struct request_send {
	int id;
	int sz;
	char * buffer;
};

struct request_send_udp {
	struct request_send send;
	uint8_t address[SADDRUDPv6_SIZE];
};

struct request_setudp {
	int id;
	uint8_t address[SADDRUDPv6_SIZE];
};

struct request_close {
	int id;
	int shutdown;
	uintptr_t opaque;
};

struct request_listen {
	int id;
	socket_t fd;
	uintptr_t opaque;
	char host[1];
};

struct request_bind {
	int id;
	socket_t fd;
	uintptr_t opaque;
};

struct request_start {
	int id;
	uintptr_t opaque;
};

struct request_setopt {
	int id;
	int what;
	int value;
};

struct request_udp {
	int id;
	socket_t fd;
	int family;
	uintptr_t opaque;
};

/*
	The first byte is TYPE
	
	S Start socket
	B Bind socket
	L Listen socket
	K Close socket
	O Connect to (Open)
	X Exit
	D Send package (high)
	P Send package (low)
	A Send UDP package
	T Set opt
	U Create UDP socket
	C set udp address
 */
struct request_package {
	uint8_t header[8];	// 6 bytes dummy
	union {
		char buffer[256];
		struct request_open open;
		struct request_send send;
		struct request_send_udp send_udp;
		struct request_close close;
		struct request_listen listen;
		struct request_bind bind;
		struct request_start start;
		struct request_setopt setopt;
		struct request_udp udp;
		struct request_setudp set_udp;
	} u;
	uint8_t dummy[256];
};

union sockaddr_all {
	struct sockaddr s;
	struct sockaddr_in v4;
	struct sockaddr_in6 v6;
};

struct send_object {
	void * buffer;
	int sz;
	void (* free_func)(void * uobj);
};

struct socket_lock {
	int * lock;
	int count;
};

static inline void
socket_lock_init(struct socket * s, struct socket_lock * sl) {
	sl->lock = &s->dw_lock;
	sl->count = 0;
}

static inline void
socket_lock(struct socket_lock * sl) {
	if (sl->count == 0)
		ATOM_LOCK(*sl->lock);
	++sl->count;
}

static inline bool
socket_trylock(struct socket_lock * sl) {
	if (sl->count == 0)
		if (!ATOM_TRYLOCK(*sl->lock))
			return false;	// lock failed
	++sl->count;
	return true;
}

static inline void
socket_unlock(struct socket_lock * sl) {
	--sl->count;
	if (sl->count <= 0) {
		assert(sl->count == 0);
		ATOM_UNLOCK(*sl->lock);
	}
}

static inline bool
send_object_init(struct sserver * ss, struct send_object * so, void * object, int sz) {
	if (sz < 0) {
		so->buffer = ss->soi.buffer(object);
		so->sz = ss->soi.size(object);
		so->free_func = ss->soi.free;
		return true;
	}
	so->buffer = object;
	so->sz = sz;
	so->free_func = free;
	return false;
}

static inline void
write_buffer_free(struct sserver * ss, struct write_buffer * wb) {
	if (wb->userobject)
		ss->soi.free(wb->buffer);
	else
		free(wb->buffer);
	free(wb);
}

static int
reserve_id(struct sserver * ss) {
	for (int i = 0; i < LEN(ss->slot); i++) {
		int id = ATOM_INC(ss->alloc_id);
		if (id < 0)
			id = ATOM_AND(ss->alloc_id, INT_MAX);

		struct socket * s = &ss->slot[HASH_ID(id)];
		if (s->type == SOCKET_TYPE_INVALID) {
			if (ATOM_CAS(s->type, SOCKET_TYPE_INVALID, SOCKET_TYPE_RESERVE)) {
				s->id = id;
				// sserver_udp_connect may inc s->udpconncting directly (from other thread, before new_fd), 
				// so reset it to 0 here rather than in new_fd.
				s->udpconnecting = 0;
				s->fd = -1;
				return id;
			}
			// retry
			--i;
		}
	}
	return -1;
}

sserver_t
sserver_create(void) {
	socket_t fd[2];
	poll_t efd = sp_create();
	if (sp_invalid(efd))
		RETURN(NULL, "socket-server: create event pool failed.");

	if (pipe(fd)) {
		sp_delete(efd);
		RETURN(NULL, "socket-server: create socket pair failed.");
	}
	if (sp_add(efd, fd[0], NULL)) {
		// add recvctrl_fd to event poll
		socket_close(fd[0]);
		socket_close(fd[1]);
		sp_delete(efd);
		RETURN(NULL, "socket-server: can't add server fd to event pool.");
	}

	struct sserver * ss = calloc(1, sizeof(struct sserver));
	assert(ss != NULL);
	ss->event_fd = efd;
	ss->recvctrl_fd = fd[0];
	ss->sendctrl_fd = fd[1];
	ss->checkctrl = true;
	assert(ss->recvctrl_fd < FD_SETSIZE);
	return ss;
}

static void
free_wb_list(struct sserver * ss, struct wb_list * list) {
	struct write_buffer * wb = list->head;
	while (wb) {
		struct write_buffer * tmp = wb;
		wb = wb->next;
		write_buffer_free(ss, tmp);
	}
	list->tail = list->head = NULL;
}

static void
free_buffer(struct sserver * ss, const void * buffer, int sz) {
	struct send_object so;
	send_object_init(ss, &so, (void *)buffer, sz);
	so.free_func((void *)buffer);
}

static void
force_close(struct sserver * ss, struct socket * s, struct socket_lock * l, struct smessage * result) {
	result->id = s->id;
	result->ud = 0;
	result->data = NULL;
	result->opaque = s->opaque;
	if (s->type == SOCKET_TYPE_INVALID)
		return;
	assert(s->type != SOCKET_TYPE_RESERVE);

	free_wb_list(ss, &s->high);
	free_wb_list(ss, &s->low);
	if (s->type != SOCKET_TYPE_PACCEPT && s->type != SOCKET_TYPE_PLISTEN)
		sp_del(ss->event_fd, s->fd);

	socket_lock(l);
	if (s->type != SOCKET_TYPE_BIND)
		if (socket_close(s->fd))
			CERR("close socket");

	s->type = SOCKET_TYPE_INVALID;
	if (s->dw_buffer) {
		free_buffer(ss, s->dw_buffer, s->dw_size);
		s->dw_buffer = NULL;
	}
	socket_unlock(l);
}

void
sserver_delete(sserver_t ss) {
	struct smessage dummy;
	for (int i = 0; i < LEN(ss->slot); i++) {
		struct socket * s = &ss->slot[i];
		struct socket_lock l;
		socket_lock_init(s, &l);
		if (s->type != SOCKET_TYPE_RESERVE)
			force_close(ss, s, &l, &dummy);
	}
	sp_delete(ss->event_fd);
	socket_close(ss->sendctrl_fd);
	socket_close(ss->recvctrl_fd);
	free(ss);
}

static struct socket *
new_fd(struct sserver * ss, int id, socket_t fd, uint8_t protocol, uintptr_t opaque, bool add) {
	struct socket * s = &ss->slot[HASH_ID(id)];
	assert(s->type == SOCKET_TYPE_RESERVE);

	if (add && sp_add(ss->event_fd, fd, s)) {
		s->type = SOCKET_TYPE_INVALID;
		return NULL;
	}

	s->id = id;
	s->fd = fd;
	s->protocol = protocol;
	s->p.size = MIN_READ_BUFFER;
	s->opaque = opaque;
	s->wb_size = 0;
	s->warn_size = 0;
	assert(s->high.head == NULL && s->high.tail == NULL);
	assert(s->low.head == NULL && s->low.tail == NULL);
	s->dw_lock = 0;
	s->dw_buffer = NULL;
	s->dw_size = 0;
	return s;
}

// return -1 when connecting
static int
open_socket(struct sserver * ss, struct request_open * request, struct smessage * result) {
	int id = request->id;
	result->opaque = request->opaque;
	result->id = id;
	result->ud = 0;
	result->data = NULL;
	int status;
	struct socket * ns;
	struct addrinfo ai_hints = { 0 };
	struct addrinfo * ai_ptr, * ai_list = NULL;
	char port[16];
	sprintf(port, "%hu", request->port);
	ai_hints.ai_family = AF_UNSPEC;
	ai_hints.ai_socktype = SOCK_STREAM;
	ai_hints.ai_protocol = IPPROTO_TCP;

	status = getaddrinfo(request->host, port, &ai_hints, &ai_list);
	if (status) {
		result->data = (void *)gai_strerror(status);
		goto _failed;
	}
	socket_t sock = INVALID_SOCKET;
	for (ai_ptr = ai_list; ai_ptr != NULL; ai_ptr = ai_ptr->ai_next) {
		sock = socket(ai_ptr->ai_family, ai_ptr->ai_socktype, ai_ptr->ai_protocol);
		if (sock == INVALID_SOCKET)
			continue;

		socket_set_keepalive(sock);
		socket_set_nonblock(sock);
		status = connect(sock, ai_ptr->ai_addr, ai_ptr->ai_addrlen);
		if (status != 0 && errno != ECONNECTED) {
			socket_close(sock);
			sock = INVALID_SOCKET;
			continue;
		}
		break;
	}

	if (sock == INVALID_SOCKET) {
		result->data = strerror(errno);
		goto _failed;
	}

	ns = new_fd(ss, id, sock, IPPROTO_TCP, request->opaque, true);
	if (ns == NULL) {
		socket_close(sock);
		result->data = "reach skynet socket number limit";
		goto _failed;
	}

	if (status == 0) {
		ns->type = SOCKET_TYPE_CONNECTED;
		struct sockaddr * addr = ai_ptr->ai_addr;
		void * sin_addr = ai_ptr->ai_family == AF_INET ? (void*)&((struct sockaddr_in *)addr)->sin_addr : (void*)&((struct sockaddr_in6 *)addr)->sin6_addr;
		if (inet_ntop(ai_ptr->ai_family, sin_addr, ss->buffer, sizeof(ss->buffer)))
			result->data = ss->buffer;
		freeaddrinfo(ai_list);
		return SSERVER_OPEN;
	}
	ns->type = SOCKET_TYPE_CONNECTING;
	sp_write(ss->event_fd, ns->fd, ns, true);

	freeaddrinfo(ai_list);
	return -1;
_failed:
	freeaddrinfo(ai_list);
	ss->slot[HASH_ID(id)].type = SOCKET_TYPE_INVALID;
	return SSERVER_ERR;
}

static int
send_list_tcp(struct sserver * ss, struct socket * s, struct wb_list * list, struct socket_lock * l, struct smessage * result) {
	while (list->head) {
		struct write_buffer * tmp = list->head;
		for (;;) {
			int sz = socket_write(s->fd, tmp->ptr, tmp->sz);
			if (sz < 0) {
				switch (errno) {
				case EINTR:
					continue;
				case EAGAIN_WOULDBOCK:
					return -1;
				}
				force_close(ss, s, l, result);
				return SSERVER_CLOSE;
			}
			s->wb_size -= sz;
			if (sz != tmp->sz) {
				tmp->ptr += sz;
				tmp->sz -= sz;
				return -1;
			}
			break;
		}
		list->head = tmp->next;
		write_buffer_free(ss, tmp);
	}
	list->tail = NULL;

	return -1;
}

static socklen_t
udp_socket_address(struct socket * s, const uint8_t udp_address[SADDRUDPv6_SIZE], union sockaddr_all * sa) {
	if (*udp_address != s->protocol)
		return 0;
	uint16_t port = 0;
	memcpy(&port, ++udp_address, sizeof port);
	switch (s->protocol) {
	case IPPROTO_UDP:
		memset(&sa->v4, 0, sizeof(sa->v4));
		sa->s.sa_family = AF_INET;
		sa->v4.sin_port = port;
		// ipv4 address is 32 bits
		memcpy(&sa->v4.sin_addr, udp_address + sizeof(uint16_t), sizeof(sa->v4.sin_addr));
		return sizeof(sa->v4);
	case IPPROTO_UDPv6:
		memset(&sa->v6, 0, sizeof(sa->v6));
		sa->s.sa_family = AF_INET6;
		sa->v6.sin6_port = port;
		// ipv6 address is 128 bits
		memcpy(&sa->v6.sin6_addr, udp_address + sizeof(uint16_t), sizeof(sa->v6.sin6_addr));
		return sizeof(sa->v6);
	}
	return 0;
}

static int
send_list_udp(struct sserver * ss, struct socket * s, struct wb_list * list, struct smessage * result) {
	while (list->head) {
		struct write_buffer * tmp = list->head;
		union sockaddr_all sa;
		socklen_t sasz = udp_socket_address(s, tmp->udp_address, &sa);
		int err = sendto(s->fd, tmp->ptr, tmp->sz, 0, &sa.s, sasz);
		if (err < 0) {
			switch (errno) {
			case EINTR:
			case EAGAIN_WOULDBOCK:
				return -1;
			}
			RETURN(-1, "socket-server : udp (%d) sendto error.", s->id);
		}
		s->wb_size -= tmp->sz;
		list->head = tmp->next;
		write_buffer_free(ss, tmp);
	}
	list->tail = NULL;

	return -1;
}

static int
send_list(struct sserver * ss, struct socket * s, struct wb_list * list, struct socket_lock * l, struct smessage * result) {
	if (s->protocol == IPPROTO_TCP)
		return send_list_tcp(ss, s, list, l, result);
	return send_list_udp(ss, s, list, result);
}

static inline int
list_uncomplete(struct wb_list * s) {
	struct write_buffer * wb = s->head;
	if (wb == NULL)
		return 0;
	return (void *)wb->ptr != wb->buffer;
}

static void
raise_uncomplete(struct socket * s) {
	struct wb_list * low = &s->low;
	struct write_buffer * tmp = low->head;
	low->head = tmp->next;
	if (low->head == NULL)
		low->tail = NULL;

	// move head of low list (tmp) to the empty high list
	struct wb_list * high = &s->high;
	assert(high->head == NULL);

	tmp->next = NULL;
	high->head = high->tail = tmp;
}

static inline int
send_buffer_empty(struct socket * s) {
	return (s->high.head == NULL && s->low.head == NULL);
}

/*
	Each socket has two write buffer list, high priority and low priority.
	
	1. send high list as far as possible.
	2. If high list is empty, try to send low list.
	3. If low list head is uncomplete (send a part before), move the head of low list to empty high list (call raise_uncomplete) .
	4. If two lists are both empty, turn off the event. (call check_close)
 */
static int
send_buffer_(struct sserver * ss, struct socket * s, struct socket_lock * l, struct smessage * result) {
	assert(!list_uncomplete(&s->low));
	// step 1
	if (send_list(ss, s, &s->high, l, result) == SSERVER_CLOSE) 
		return SSERVER_CLOSE;

	if (s->high.head == NULL) {
		// step 2
		if (s->low.head != NULL) {
			if (send_list(ss, s, &s->low, l, result) == SSERVER_CLOSE) {
				return SSERVER_CLOSE;
			}
			// step 3
			if (list_uncomplete(&s->low)) {
				raise_uncomplete(s);
				return -1;
			}
		}
		// step 4
		assert(send_buffer_empty(s) && s->wb_size == 0);
		sp_write(ss->event_fd, s->fd, s, false);

		if (s->type == SOCKET_TYPE_HALFCLOSE) {
			force_close(ss, s, l, result);
			return SSERVER_CLOSE;
		}
		if (s->warn_size > 0) {
			s->warn_size = 0;
			result->opaque = s->opaque;
			result->id = s->id;
			result->ud = 0;
			result->data = NULL;
			return SSERVER_WARNING;
		}
	}

	return -1;
}

static int
send_buffer(struct sserver * ss, struct socket * s, struct socket_lock * l, struct smessage * result) {
	if (!socket_trylock(l))
		return -1;	// blocked by direct write, send later.
	if (s->dw_buffer) {
		// add direct write buffer before high.head
		struct write_buffer * buf = malloc(SIZEOF_TCPBUFFER);
		struct send_object so;
		buf->userobject = send_object_init(ss, &so, (void *)s->dw_buffer, s->dw_size);
		buf->ptr = (char *)so.buffer + s->dw_offset;
		buf->sz = so.sz - s->dw_offset;
		buf->buffer = (void *)s->dw_buffer;
		s->wb_size += buf->sz;
		if (s->high.head == NULL) {
			s->high.head = s->high.tail = buf;
			buf->next = NULL;
		} else {
			buf->next = s->high.head;
			s->high.head = buf;
		}
		s->dw_buffer = NULL;
	}
	int r = send_buffer_(ss, s, l, result);
	socket_unlock(l);

	return r;
}

static struct write_buffer *
append_sendbuffer_(struct sserver * ss, struct wb_list * s, struct request_send * request, int size) {
	struct write_buffer * buf = malloc(size);
	struct send_object so;
	buf->userobject = send_object_init(ss, &so, request->buffer, request->sz);
	buf->ptr = (char *)so.buffer;
	buf->sz = so.sz;
	buf->buffer = request->buffer;
	buf->next = NULL;
	if (s->head == NULL)
		s->head = s->tail = buf;
	else {
		assert(s->tail != NULL && s->tail->next == NULL);
		s->tail->next = buf;
		s->tail = buf;
	}
	return buf;
}

static inline void
append_sendbuffer_udp(struct sserver * ss, struct socket * s, int priority, struct request_send * request, const uint8_t udp_address[SADDRUDPv6_SIZE]) {
	struct wb_list * wl = priority == PRIORITY_HIGH ? &s->high : &s->low;
	struct write_buffer * buf = append_sendbuffer_(ss, wl, request, SIZEOF_UDPBUFFER);
	memcpy(buf->udp_address, udp_address, sizeof(buf->udp_address));
	s->wb_size += buf->sz;
}

static inline void
append_sendbuffer(struct sserver * ss, struct socket * s, struct request_send * request) {
	struct write_buffer * buf = append_sendbuffer_(ss, &s->high, request, SIZEOF_TCPBUFFER);
	s->wb_size += buf->sz;
}

static inline void
append_sendbuffer_low(struct sserver *ss, struct socket *s, struct request_send * request) {
	struct write_buffer * buf = append_sendbuffer_(ss, &s->low, request, SIZEOF_TCPBUFFER);
	s->wb_size += buf->sz;
}


/*
	When send a package , we can assign the priority : PRIORITY_HIGH or PRIORITY_LOW
	
	If socket buffer is empty, write to fd directly.
	If write a part, append the rest part to high list. (Even priority is PRIORITY_LOW)
	Else append package to high (PRIORITY_HIGH) or low (PRIORITY_LOW) list.
 */
static int
send_socket(struct sserver * ss, struct request_send * request, struct smessage * result, int priority, const uint8_t * udp_address) {
	int id = request->id;
	struct socket * s = &ss->slot[HASH_ID(id)];
	struct send_object so;
	send_object_init(ss, &so, request->buffer, request->sz);
	if (s->type == SOCKET_TYPE_INVALID || s->id != id
		|| s->type == SOCKET_TYPE_HALFCLOSE
		|| s->type == SOCKET_TYPE_PACCEPT) {
		so.free_func(request->buffer);
		return -1;
	}
	if (s->type == SOCKET_TYPE_PLISTEN || s->type == SOCKET_TYPE_LISTEN) {
		so.free_func(request->buffer);
		RETURN(-1, "socket-server: write to listen fd %d.", id);
	}
	if (send_buffer_empty(s) && s->type == SOCKET_TYPE_CONNECTED) {
		if (s->protocol == IPPROTO_TCP)
			append_sendbuffer(ss, s, request);	// add to high priority list, even priority == PRIORITY_LOW
		else {
			// udp
			if (udp_address == NULL)
				udp_address = s->p.udp_address;
			union sockaddr_all sa;
			socklen_t sasz = udp_socket_address(s, udp_address, &sa);
			int n = sendto(s->fd, so.buffer, so.sz, 0, &sa.s, sasz);
			if (n != so.sz)
				append_sendbuffer_udp(ss, s, priority, request, udp_address);
			else {
				so.free_func(request->buffer);
				return -1;
			}
		}
		sp_write(ss->event_fd, s->fd, s, true);
	} else {
		if (s->protocol == IPPROTO_TCP) {
			if (priority == PRIORITY_LOW)
				append_sendbuffer_low(ss, s, request);
			else
				append_sendbuffer(ss, s, request);
		} else {
			if (udp_address == NULL)
				udp_address = s->p.udp_address;
			append_sendbuffer_udp(ss, s, priority, request, udp_address);
		}
	}
	if (s->wb_size >= WARNING_SIZE && s->wb_size >= s->warn_size) {
		s->warn_size = s->warn_size == 0 ? WARNING_SIZE * 2 : s->warn_size * 2;
		result->opaque = s->opaque;
		result->id = s->id;
		result->ud = (int)(s->wb_size / 1024) + !!(s->wb_size % 1024);
		result->data = NULL;
		return SSERVER_WARNING;
	}
	return -1;
}

static int
listen_socket(struct sserver * ss, struct request_listen * request, struct smessage * result) {
	int id = request->id;
	int listen_fd = request->fd;
	struct socket * s = new_fd(ss, id, listen_fd, IPPROTO_TCP, request->opaque, false);
	if (s == NULL)
		goto _failed;

	s->type = SOCKET_TYPE_PLISTEN;
	return -1;
_failed:
	socket_close(listen_fd);
	result->opaque = request->opaque;
	result->id = id;
	result->ud = 0;
	result->data = "reach skynet socket number limit";
	ss->slot[HASH_ID(id)].type = SOCKET_TYPE_INVALID;

	return SSERVER_ERR;
}

static int
close_socket(struct sserver * ss, struct request_close * request, struct smessage * result) {
	int id = request->id;
	struct socket * s = &ss->slot[HASH_ID(id)];
	if (s->type == SOCKET_TYPE_INVALID || s->id != id) {
		result->id = id;
		result->opaque = request->opaque;
		result->ud = 0;
		result->data = NULL;
		return SSERVER_CLOSE;
	}
	struct socket_lock l;
	socket_lock_init(s, &l);
	if (!send_buffer_empty(s)) {
		int type = send_buffer(ss, s, &l, result);
		// type : -1 or SSERVER_WARNING or SSERVER_CLOSE, SSERVER_WARNING means send_buffer_empty
		if (type != -1 && type != SSERVER_WARNING)
			return type;
	}
	if (request->shutdown || send_buffer_empty(s)) {
		force_close(ss, s, &l, result);
		result->id = id;
		result->opaque = request->opaque;
		return SSERVER_CLOSE;
	}
	s->type = SOCKET_TYPE_HALFCLOSE;

	return -1;
}

static int
bind_socket(struct sserver * ss, struct request_bind * request, struct smessage * result) {
	result->id = request->id;
	result->opaque = request->opaque;
	result->ud = 0;
	struct socket * s = new_fd(ss, request->id, request->fd, IPPROTO_TCP, request->opaque, true);
	if (s == NULL) {
		result->data = "reach skynet socket number limit";
		return SSERVER_ERR;
	}
	socket_set_nonblock(request->fd);
	s->type = SOCKET_TYPE_BIND;
	result->data = "binding";
	return SSERVER_OPEN;
}

static int
start_socket(struct sserver * ss, struct request_start * request, struct smessage * result) {
	int id = request->id;
	result->id = id;
	result->opaque = request->opaque;
	result->ud = 0;
	result->data = NULL;
	struct socket * s = &ss->slot[HASH_ID(id)];
	if (s->type == SOCKET_TYPE_INVALID || s->id != id) {
		result->data = "invalid socket";
		return SSERVER_ERR;
	}
	struct socket_lock l;
	socket_lock_init(s, &l);
	if (s->type == SOCKET_TYPE_PACCEPT || s->type == SOCKET_TYPE_PLISTEN) {
		if (sp_add(ss->event_fd, s->fd, s)) {
			int error = errno;
			force_close(ss, s, &l, result);
			result->data = strerror(error);
			return SSERVER_ERR;
		}
		s->type = (s->type == SOCKET_TYPE_PACCEPT) ? SOCKET_TYPE_CONNECTED : SOCKET_TYPE_LISTEN;
		s->opaque = request->opaque;
		result->data = "start";
		return SSERVER_OPEN;
	}
	else if (s->type == SOCKET_TYPE_CONNECTED) {
		// todo: maybe we should send a message SOCKET_TRANSFER to s->opaque
		s->opaque = request->opaque;
		result->data = "transfer";
		return SSERVER_OPEN;
	}
	// if s->type == SOCKET_TYPE_HALFCLOSE , SSERVER_CLOSE message will send later
	return -1;
}

static void
setopt_socket(struct sserver * ss, struct request_setopt * request) {
	int id = request->id;
	struct socket * s = &ss->slot[HASH_ID(id)];
	if (s->type == SOCKET_TYPE_INVALID || s->id != id)
		return;
	setsockopt(s->fd, IPPROTO_TCP, request->what, (void *)&request->value, sizeof(request->value));
}

static void
block_readpipe(socket_t pipefd, void * buffer, int sz) {
	for (;;) {
		int n = socket_read(pipefd, buffer, sz);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			RETURN(NIL, "socket-server : read pipe error.");
		}
		// must atomic read from a pipe
		assert(n == sz);
		break;
	}
}

static bool
has_cmd(struct sserver * ss) {
	struct timeval tv = { 0,0 };
	FD_SET(ss->recvctrl_fd, &ss->rfds);
	return 1 == select(ss->recvctrl_fd + 1, &ss->rfds, NULL, NULL, &tv);
}

static void
add_udp_socket(struct sserver * ss, struct request_udp * udp) {
	int id = udp->id;
	uint8_t protocol = udp->family == AF_INET6 ? IPPROTO_UDPv6 : IPPROTO_UDP;
	struct socket * ns = new_fd(ss, id, udp->fd, protocol, udp->opaque, true);
	if (ns == NULL) {
		socket_close(udp->fd);
		ss->slot[HASH_ID(id)].type = SOCKET_TYPE_INVALID;
		return;
	}
	ns->type = SOCKET_TYPE_CONNECTED;
	memset(ns->p.udp_address, 0, sizeof(ns->p.udp_address));
}

static int
set_udp_address(struct sserver * ss, struct request_setudp * request, struct smessage * result) {
	int id = request->id;
	struct socket * s = &ss->slot[HASH_ID(id)];
	if (s->type == SOCKET_TYPE_INVALID || s->id != id)
		return -1;
	int type = request->address[0];
	if (type != s->protocol) {
		// protocol mismatch
		result->opaque = s->opaque;
		result->id = s->id;
		result->ud = 0;
		result->data = "protocol mismatch";

		return SSERVER_ERR;
	}
	// 1 type, 2 port, 4 ipv4 | 1 type, 2 port, 16 ipv6
	memcpy(s->p.udp_address, request->address, type == IPPROTO_UDP ? SADDRUDP_SIZE : SADDRUDPv6_SIZE);
	ATOM_DEC(s->udpconnecting);
	return -1;
}

// return type
static int
ctrl_cmd(struct sserver * ss, struct smessage * result) {
	socket_t fd = ss->recvctrl_fd;
	// the length of message is one byte, so 256 + 8 buffer size is enough.
	uint8_t buffer[256];
	uint8_t header[2];
	block_readpipe(fd, header, sizeof(header));
	int type = header[0];
	int len = header[1];
	block_readpipe(fd, buffer, len);
	// ctrl command only exist in local fd, so don't worry about endian.
	switch (type) {
	case 'S':
		return start_socket(ss, (struct request_start *)buffer, result);
	case 'B':
		return bind_socket(ss, (struct request_bind *)buffer, result);
	case 'L':
		return listen_socket(ss, (struct request_listen *)buffer, result);
	case 'K':
		return close_socket(ss, (struct request_close *)buffer, result);
	case 'O':
		return open_socket(ss, (struct request_open *)buffer, result);
	case 'X':
		memset(result, 0, sizeof(struct smessage));
		return SSERVER_EXIT;
	case 'D':
		return send_socket(ss, (struct request_send *)buffer, result, PRIORITY_HIGH, NULL);
	case 'P':
		return send_socket(ss, (struct request_send *)buffer, result, PRIORITY_LOW, NULL);
	case 'A': {
		struct request_send_udp * rsu = (struct request_send_udp *)buffer;
		return send_socket(ss, &rsu->send, result, PRIORITY_HIGH, rsu->address);
	}
	case 'C':
		return set_udp_address(ss, (struct request_setudp *)buffer, result);
	case 'T':
		setopt_socket(ss, (struct request_setopt *)buffer);
		return -1;
	case 'U':
		add_udp_socket(ss, (struct request_udp *)buffer);
		return -1;
	default:
		CERR("socket - server: Unknown ctrl %c.", type);
	};

	return -1;
}

// return -1 (ignore) when error
static int
forward_message_tcp(struct sserver * ss, struct socket * s, struct socket_lock * l, struct smessage * result) {
	int error, sz = s->p.size;
	char * buffer = malloc(sz);
	int n = socket_read(s->fd, buffer, sz);
	if (n < 0) {
		free(buffer);
		switch ((error = errno)) {
		case EINTR:
			break;
		case EAGAIN_WOULDBOCK:
			CERR("socket - server: EAGAIN capture.");
			break;
		default:
			// close when error
			force_close(ss, s, l, result);
			result->data = strerror(error);
			return SSERVER_ERR;
		}
		return -1;
	}
	if (n == 0) {
		free(buffer);
		force_close(ss, s, l, result);
		return SSERVER_CLOSE;
	}

	if (s->type == SOCKET_TYPE_HALFCLOSE) {
		// discard recv data
		free(buffer);
		return -1;
	}

	if (n == sz)
		s->p.size *= 2;
	else if (sz > MIN_READ_BUFFER && n * 2 < sz)
		s->p.size /= 2;

	result->opaque = s->opaque;
	result->id = s->id;
	result->ud = n;
	result->data = buffer;
	return SSERVER_DATA;
}

static int
gen_udp_address(uint8_t protocol, union sockaddr_all * sa, uint8_t * udp_address) {
	int addrsz = 1;
	udp_address[0] = protocol;
	if (protocol == IPPROTO_UDP) {
		memcpy(udp_address + addrsz, &sa->v4.sin_port, sizeof(sa->v4.sin_port));
		addrsz += sizeof(sa->v4.sin_port);
		memcpy(udp_address + addrsz, &sa->v4.sin_addr, sizeof(sa->v4.sin_addr));
		addrsz += sizeof(sa->v4.sin_addr);
	} else {
		memcpy(udp_address + addrsz, &sa->v6.sin6_port, sizeof(sa->v6.sin6_port));
		addrsz += sizeof(sa->v6.sin6_port);
		memcpy(udp_address + addrsz, &sa->v6.sin6_addr, sizeof(sa->v6.sin6_addr));
		addrsz += sizeof(sa->v6.sin6_addr);
	}
	return addrsz;
}

static int
forward_message_udp(struct sserver * ss, struct socket * s, struct socket_lock * l, struct smessage * result) {
	union sockaddr_all sa;
	socklen_t slen = sizeof sa;
	int n = recvfrom(s->fd, (void *)ss->udpbuffer, LEN(ss->udpbuffer), 0, &sa.s, &slen);
	if (n < 0) {
		int error = errno;
		switch (error) {
		case EINTR:
		case EAGAIN_WOULDBOCK:
			break;
		default:
			// close when error
			force_close(ss, s, l, result);
			result->data = strerror(error);
			return SSERVER_ERR;
		}
		return -1;
	}
	uint8_t * data;
	if (slen == sizeof(sa.v4)) {
		if (s->protocol != IPPROTO_UDP)
			return -1;
		data = malloc(n + SADDRUDP_SIZE);
		gen_udp_address(IPPROTO_UDP, &sa, data + n);
	} else {
		if (s->protocol != IPPROTO_UDPv6)
			return -1;
		data = malloc(n + SADDRUDPv6_SIZE);
		gen_udp_address(IPPROTO_UDPv6, &sa, data + n);
	}
	memcpy(data, ss->udpbuffer, n);

	result->opaque = s->opaque;
	result->id = s->id;
	result->ud = n;
	result->data = (char *)data;

	return SSERVER_UDP;
}

static int
report_connect(struct sserver * ss, struct socket * s, struct socket_lock * l, struct smessage * result) {
	int error = socket_get_error(s->fd);
	if (error) {
		force_close(ss, s, l, result);
		result->data = strerror(error);
		return SSERVER_ERR;
	}
	s->type = SOCKET_TYPE_CONNECTED;
	result->opaque = s->opaque;
	result->id = s->id;
	result->ud = 0;
	if (send_buffer_empty(s))
		sp_write(ss->event_fd, s->fd, s, false);

	union sockaddr_all u;
	socklen_t slen = sizeof(u);
	if (getpeername(s->fd, &u.s, &slen) == 0) {
		void * sin_addr = u.s.sa_family == AF_INET ? (void *)&u.v4.sin_addr : (void *)&u.v6.sin6_addr;
		if (inet_ntop(u.s.sa_family, sin_addr, ss->buffer, sizeof(ss->buffer))) {
			result->data = ss->buffer;
			return SSERVER_OPEN;
		}
	}
	result->data = NULL;
	return SSERVER_OPEN;
}

// return 0 when failed, or -1 when file limit
static int
report_accept(struct sserver * ss, struct socket * s, struct smessage * result) {
	union sockaddr_all u;
	socklen_t len = sizeof(u);
	socket_t client_fd = accept(s->fd, &u.s, &len);
	if (client_fd == INVALID_SOCKET) {
		int error = errno;
		if (error == EMFILE || error == ENFILE) {
			result->opaque = s->opaque;
			result->id = s->id;
			result->ud = 0;
			result->data = strerror(error);
			return -1;
		}
		return 0;
	}
	int id = reserve_id(ss);
	if (id < 0) {
		socket_close(client_fd);
		return 0;
	}
	socket_set_keepalive(client_fd);
	socket_set_nonblock(client_fd);
	struct socket * ns = new_fd(ss, id, client_fd, IPPROTO_TCP, s->opaque, false);
	if (ns == NULL) {
		socket_close(client_fd);
		return 0;
	}
	ns->type = SOCKET_TYPE_PACCEPT;
	result->opaque = s->opaque;
	result->id = s->id;
	result->ud = id;
	result->data = NULL;

	void * sin_addr = u.s.sa_family == AF_INET ? (void *)&u.v4.sin_addr : (void *)&u.v6.sin6_addr;
	uint16_t sin_port = ntohs(u.s.sa_family == AF_INET ? u.v4.sin_port : u.v6.sin6_port);
	char tmp[INET6_ADDRSTRLEN];
	if (inet_ntop(u.s.sa_family, sin_addr, tmp, sizeof(tmp))) {
		snprintf(ss->buffer, sizeof(ss->buffer), "%s:%hu", tmp, sin_port);
		result->data = ss->buffer;
	}

	return 1;
}

static inline void
clear_closed_event(struct sserver * ss, struct smessage * result, int type) {
	if (type == SSERVER_CLOSE || type == SSERVER_ERR) {
		int id = result->id;
		for (int i = ss->event_index; i < ss->event_n; ++i) {
			struct event * e = &ss->ev[i];
			struct socket * s = e->s;
			if (s) {
				if (s->type == SOCKET_TYPE_INVALID && s->id == id) {
					e->s = NULL;
					break;
				}
			}
		}
	}
}

// return type
int
sserver_poll(sserver_t ss, smessage_t result, int * more) {
	for (;;) {
		if (ss->checkctrl) {
			if (has_cmd(ss)) {
				int type = ctrl_cmd(ss, result);
				if (type != -1) {
					clear_closed_event(ss, result, type);
					return type;
				}
				continue;
			}
			ss->checkctrl = false;
		}
		if (ss->event_index == ss->event_n) {
			ss->event_n = sp_wait(ss->event_fd, ss->ev, LEN(ss->ev));
			ss->checkctrl = true;
			if (more)
				*more = 0;

			ss->event_index = 0;
			if (ss->event_n <= 0) {
				ss->event_n = 0;
				if (errno == EINTR)
					continue;
				return -1;
			}
		}
		struct event * e = &ss->ev[ss->event_index++];
		struct socket * s = e->s;
		// dispatch pipe message at beginning
		if (s == NULL)
			continue;

		struct socket_lock l;
		socket_lock_init(s, &l);
		switch (s->type) {
		case SOCKET_TYPE_CONNECTING:
			return report_connect(ss, s, &l, result);
		case SOCKET_TYPE_LISTEN: {
			int ok = report_accept(ss, s, result);
			if (ok > 0)
				return SSERVER_ACCEPT;
			if (ok < 0)
				return SSERVER_ERR;
			// when ok == 0, retry
			break;
		}
		case SOCKET_TYPE_INVALID:
			CERR("socket-server: invalid socket");
			break;
		default:
			if (e->read) {
				int type;
				if (s->protocol == IPPROTO_TCP)
					type = forward_message_tcp(ss, s, &l, result);
				else {
					type = forward_message_udp(ss, s, &l, result);
					if (type == SSERVER_UDP) {
						// try read again
						--ss->event_index;
						return SSERVER_UDP;
					}
				}
				if (e->write && type != SSERVER_CLOSE && type != SSERVER_ERR) {
					// Try to dispatch write message next step if write flag set.
					e->read = false;
					--ss->event_index;
				}
				if (type == -1)
					break;
				return type;
			}
			if (e->write) {
				int type = send_buffer(ss, s, &l, result);
				if (type == -1)
					break;
				return type;
			}
			if (e->error) {
				// close when error
				int error = socket_get_error(s->fd);
				force_close(ss, s, &l, result);
				result->data = error ? strerror(error) : "Unknown error";
				return SSERVER_ERR;
			}
			break;
		}
	}
}

static void
send_request(struct sserver * ss, struct request_package * request, uint8_t type, int len) {
	request->header[6] = type;
	request->header[7] = (uint8_t)len;
	for (;;) {
		int n = socket_write(ss->sendctrl_fd, &request->header[6], len + 2);
		if (n < 0) {
			if (errno != EINTR)
				CERR("socket-server : send ctrl command error.");
			continue;
		}
		assert(n == len + 2);
		return;
	}
}

static int
open_request(struct sserver * ss, struct request_package * req, uintptr_t opaque, const char * addr, uint16_t port) {
	int len = strlen(addr);
	if (len + sizeof(req->u.open) >= 256) {
		RETURN(-1, "socket-server : Invalid addr %s.", addr);
	}
	int id = reserve_id(ss);
	if (id < 0)
		return -1;
	req->u.open.opaque = opaque;
	req->u.open.id = id;
	req->u.open.port = port;
	memcpy(req->u.open.host, addr, len);
	req->u.open.host[len] = '\0';

	return len;
}

int
sserver_connect(sserver_t ss, uintptr_t opaque, const char * addr, uint16_t port) {
	struct request_package request;
	int len = open_request(ss, &request, opaque, addr, port);
	if (len < 0)
		return -1;
	send_request(ss, &request, 'O', sizeof(request.u.open) + len);
	return request.u.open.id;
}

static inline int
can_direct_write(struct socket * s, int id) {
	return s->id == id && send_buffer_empty(s) && s->type == SOCKET_TYPE_CONNECTED && s->dw_buffer == NULL && s->udpconnecting == 0;
}

// return -1 when error, 0 when success
int
sserver_send(sserver_t ss, int id, const void * buffer, int sz) {
	struct socket * s = &ss->slot[HASH_ID(id)];
	if (s->id != id || s->type == SOCKET_TYPE_INVALID) {
		free_buffer(ss, buffer, sz);
		return -1;
	}

	struct socket_lock l;
	socket_lock_init(s, &l);

	if (can_direct_write(s, id) && socket_trylock(&l)) {
		// may be we can send directly, double check
		if (can_direct_write(s, id)) {
			// send directly
			struct send_object so;
			send_object_init(ss, &so, (void *)buffer, sz);
			int n;
			if (s->protocol == IPPROTO_TCP)
				n = socket_write(s->fd, so.buffer, so.sz);
			else {
				union sockaddr_all sa;
				socklen_t sasz = udp_socket_address(s, s->p.udp_address, &sa);
				n = sendto(s->fd, so.buffer, so.sz, 0, &sa.s, sasz);
			}
			// ignore error, let socket thread try again
			if (n < 0)
				n = 0;

			if (n == so.sz) {
				// write done
				socket_unlock(&l);
				so.free_func((void *)buffer);
				return 0;
			}
			// write failed, put buffer into s->dw_* , and let socket thread send it. see send_buffer()
			s->dw_buffer = buffer;
			s->dw_size = sz;
			s->dw_offset = n;

			sp_write(ss->event_fd, s->fd, s, true);

			socket_unlock(&l);
			return 0;
		}
		socket_unlock(&l);
	}

	struct request_package request;
	request.u.send.id = id;
	request.u.send.sz = sz;
	request.u.send.buffer = (char *)buffer;

	send_request(ss, &request, 'D', sizeof(request.u.send));
	return 0;
}

// return -1 when error, 0 when success
int
sserver_send_lowpriority(sserver_t ss, int id, const void * buffer, int sz) {
	struct socket * s = &ss->slot[HASH_ID(id)];
	if (s->id != id || s->type == SOCKET_TYPE_INVALID) {
		free_buffer(ss, buffer, sz);
		return -1;
	}

	struct request_package request;
	request.u.send.id = id;
	request.u.send.sz = sz;
	request.u.send.buffer = (char *)buffer;

	send_request(ss, &request, 'P', sizeof(request.u.send));
	return 0;
}

inline void
sserver_exit(sserver_t ss) {
	struct request_package request;
	send_request(ss, &request, 'X', 0);
}

inline void
sserver_close(sserver_t ss, uintptr_t opaque, int id) {
	struct request_package request;
	request.u.close.id = id;
	request.u.close.shutdown = 0;
	request.u.close.opaque = opaque;
	send_request(ss, &request, 'K', sizeof(request.u.close));
}


inline void
sserver_shutdown(sserver_t ss, uintptr_t opaque, int id) {
	struct request_package request;
	request.u.close.id = id;
	request.u.close.shutdown = 1;
	request.u.close.opaque = opaque;
	send_request(ss, &request, 'K', sizeof(request.u.close));
}

int
sserver_listen(sserver_t ss, uintptr_t opaque, const char * addr, uint16_t port) {
	socket_t fd = socket_tcp(addr, port);
	if (fd == INVALID_SOCKET)
		return -1;

	struct request_package request;
	int id = reserve_id(ss);
	if (id < 0) {
		socket_close(fd);
		return id;
	}
	request.u.listen.opaque = opaque;
	request.u.listen.id = id;
	request.u.listen.fd = fd;
	send_request(ss, &request, 'L', sizeof(request.u.listen));
	return id;
}

int
sserver_bind(sserver_t ss, uintptr_t opaque, socket_t fd) {
	struct request_package request;
	int id = reserve_id(ss);
	if (id < 0)
		return -1;
	request.u.bind.opaque = opaque;
	request.u.bind.id = id;
	request.u.bind.fd = fd;
	send_request(ss, &request, 'B', sizeof(request.u.bind));
	return id;
}

inline void
sserver_start(sserver_t ss, uintptr_t opaque, int id) {
	struct request_package request;
	request.u.start.id = id;
	request.u.start.opaque = opaque;
	send_request(ss, &request, 'S', sizeof(request.u.start));
}

inline void
sserver_nodelay(sserver_t ss, int id) {
	struct request_package request;
	request.u.setopt.id = id;
	request.u.setopt.what = TCP_NODELAY;
	request.u.setopt.value = 1;
	send_request(ss, &request, 'T', sizeof(request.u.setopt));
}

inline void
sserver_userobject(sserver_t ss, struct sinterface * soi) {
	ss->soi = *soi;
}

// UDP

int
sserver_udp(sserver_t ss, uintptr_t opaque, const char * addr, uint16_t port) {
	socket_t fd;
	int family;
	if (port != 0 || addr != NULL) {
		// bind
		fd = socket_bind(addr, port, IPPROTO_UDP, &family);
		if (fd == INVALID_SOCKET)
			return -1;
	} else {
		family = AF_INET;
		fd = socket(family, SOCK_DGRAM, 0);
		if (fd == INVALID_SOCKET)
			return -1;
	}
	socket_set_nonblock(fd);

	int id = reserve_id(ss);
	if (id < 0) {
		socket_close(fd);
		return -1;
	}

	struct request_package request;
	request.u.udp.id = id;
	request.u.udp.fd = fd;
	request.u.udp.opaque = opaque;
	request.u.udp.family = family;

	send_request(ss, &request, 'U', sizeof(request.u.udp));
	return id;
}

int
sserver_udp_send(sserver_t ss, int id, const udpaddr_t udpaddr, const void * buffer, int sz) {
	struct socket * s = &ss->slot[HASH_ID(id)];
	if (s->id != id || s->type == SOCKET_TYPE_INVALID) {
		free_buffer(ss, buffer, sz);
		return -1;
	}

	int addrsz;
	switch (udpaddr[0]) {
	case IPPROTO_UDP:
		addrsz = SADDRUDP_SIZE;		// 1 type, 2 port, 4 ipv4
		break;
	case IPPROTO_UDPv6:
		addrsz = SADDRUDPv6_SIZE;	// 1 type, 2 port, 16 ipv6
		break;
	default:
		free_buffer(ss, buffer, sz);
		return -1;
	}

	struct socket_lock l;
	socket_lock_init(s, &l);

	if (can_direct_write(s, id) && socket_trylock(&l)) {
		// may be we can send directly, double check
		if (can_direct_write(s, id)) {
			// send directly
			struct send_object so;
			send_object_init(ss, &so, (void *)buffer, sz);
			union sockaddr_all sa;
			socklen_t sasz = udp_socket_address(s, udpaddr, &sa);
			int n = sendto(s->fd, so.buffer, so.sz, 0, &sa.s, sasz);
			if (n >= 0) {
				// sendto succ
				socket_unlock(&l);
				so.free_func((void *)buffer);
				return 0;
			}
		}
		socket_unlock(&l);
		// let socket thread try again, udp doesn't care the order
	}

	struct request_package request;
	request.u.send_udp.send.id = id;
	request.u.send_udp.send.sz = sz;
	request.u.send_udp.send.buffer = (char *)buffer;
	memcpy(request.u.send_udp.address, udpaddr, addrsz);
	send_request(ss, &request, 'A', sizeof(request.u.send_udp.send) + addrsz);
	return 0;
}

int
sserver_udp_connect(sserver_t ss, int id, const char * addr, uint16_t port) {
	struct socket * s = &ss->slot[HASH_ID(id)];
	if (s->id != id || s->type == SOCKET_TYPE_INVALID)
		return -1;

	struct socket_lock l;
	socket_lock_init(s, &l);
	socket_lock(&l);
	if (s->id != id || s->type == SOCKET_TYPE_INVALID) {
		socket_unlock(&l);
		return -1;
	}
	ATOM_INC(s->udpconnecting);
	socket_unlock(&l);

	int status;
	struct addrinfo ai_hints = { 0 };
	struct addrinfo * ai_list = NULL;
	char portstr[16];
	sprintf(portstr, "%hu", port);
	ai_hints.ai_family = AF_UNSPEC;
	ai_hints.ai_socktype = SOCK_DGRAM;
	ai_hints.ai_protocol = IPPROTO_UDP;
	status = getaddrinfo(addr, portstr, &ai_hints, &ai_list);
	if (status != 0)
		return -1;

	uint8_t protocol;
	if (ai_list->ai_family == AF_INET)
		protocol = IPPROTO_UDP;
	else if (ai_list->ai_family == AF_INET6)
		protocol = IPPROTO_UDPv6;
	else {
		freeaddrinfo(ai_list);
		return -1;
	}

	struct request_package request;
	request.u.set_udp.id = id;
	int addrsz = gen_udp_address(protocol, (union sockaddr_all *)ai_list->ai_addr, request.u.set_udp.address);
	freeaddrinfo(ai_list);
	send_request(ss, &request, 'C', sizeof(request.u.set_udp) - sizeof(request.u.set_udp.address) + addrsz);

	return 0;
}

inline const udpaddr_t
sserver_udp_address(sserver_t ss, smessage_t msg, int * addrsz) {
	uint8_t * address = (uint8_t *)(msg->data + msg->ud);
	switch (address[0]) {
	case IPPROTO_UDP:
		*addrsz = SADDRUDP_SIZE;
		break;
	case IPPROTO_UDPv6:
		*addrsz = SADDRUDPv6_SIZE;
		break;
	default:
		return NULL;
	}
	return address;
}