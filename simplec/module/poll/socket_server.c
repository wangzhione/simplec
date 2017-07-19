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

#define PRIORITY_HIGH			(0)
#define PRIORITY_LOW			(1)

// ipv6 128bit + port 16bit + 1 byte type
#define UDP_ADDRESS_SIZE		(19)
// 应用层伪装 UDPv6协议, 基于 UDP协议 + IPv6地址
#define IPPROTO_UDPv6			(IPPROTO_UDP + 1)

#define MAX_INFO				(128)
#define MAX_EVENT				(64)
// MAX_SOCKET will be 2 ^ [x]
#define MAX_SOCKET				(1 << 16)
#define HASH_ID(id)				(((unsigned)(id)) % MAX_SOCKET)
#define MAX_UDP_PACKAGE			(65535)

#define MIN_READ_BUFFER			(64)

#define WARNING_SIZE			(1024 * 1024)

struct write_buffer {
	struct write_buffer * next;
	void * buffer;
	char * ptr;
	int sz;
	bool userobject;
	uint8_t udp_address[UDP_ADDRESS_SIZE];
};

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
	uint16_t protocol;
	uint16_t type;
	int64_t warn_size;
	union {
		int size;
		uint8_t udp_address[UDP_ADDRESS_SIZE];
	} p;
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
	uint8_t udpbuffer[MAX_UDP_PACKAGE];
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
	uint8_t address[UDP_ADDRESS_SIZE];
};

struct request_setudp {
	int id;
	uint8_t address[UDP_ADDRESS_SIZE];
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
	int sz;
	void * buffer;
	void (* free_func)(void * uobj);
};

static inline bool
send_object_init(sserver_t ss, struct send_object * so, void * uobj, int sz) {
	if (sz < 0) {
		so->sz = ss->soi.size(uobj);
		so->buffer = ss->soi.buffer(uobj);
		so->free_func = ss->soi.free;
		return true;
	}

	so->sz = sz;
	so->buffer = uobj;
	so->free_func = free;
	return false;
}

static inline void
write_buffer_free(sserver_t ss, struct write_buffer * wb) {
	if (wb->userobject)
		ss->soi.free(wb->buffer);
	else
		free(wb->buffer);

	free(wb);
}

static int
reserve_id(sserver_t ss) {
	for (int i = 0; i < LEN(ss->slot); ++i) {
		int id = ATOM_INC(ss->alloc_id);
		if (id < 0)
			id = ATOM_AND(ss->alloc_id, INT_MAX);
		struct socket * s = &ss->slot[HASH_ID(id)];
		if (s->type == SOCKET_TYPE_INVALID) {
			if (ATOM_CAS(s->type, SOCKET_TYPE_INVALID, SOCKET_TYPE_RESERVE)) {
				s->id = id;
				s->fd = INVALID_SOCKET;
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
		sp_delete(efd);
		socket_close(fd[0]);
		socket_close(fd[1]);
		RETURN(NULL, "socket-server: can't add server fd to event pool.");
	}

	sserver_t ss = calloc(1, sizeof(struct sserver));
	assert(ss && fd[0] < FD_SETSIZE);
	ss->event_fd = efd;
	ss->recvctrl_fd = fd[0];
	ss->sendctrl_fd = fd[1];
	ss->checkctrl = true;
	return ss;
}

static void
free_wb_list(sserver_t ss, struct wb_list * list) {
	struct write_buffer * wb = list->head;
	while (wb) {
		struct write_buffer * tmp = wb;
		wb = wb->next;
		write_buffer_free(ss, tmp);
	}
	list->head = list->tail = NULL;
}

static void
force_close(sserver_t ss, struct socket * s, smessage_t result) {
	result->ud = 0;
	result->id = s->id;
	result->data = NULL;
	result->opaque = s->opaque;
	if (s->type == SOCKET_TYPE_INVALID)
		return;

	assert(s->type != SOCKET_TYPE_RESERVE);
	free_wb_list(ss, &s->high);
	free_wb_list(ss, &s->low);
	if (s->type != SOCKET_TYPE_PACCEPT && s->type != SOCKET_TYPE_PLISTEN)
		sp_del(ss->event_fd, s->fd);

	if (s->type != SOCKET_TYPE_BIND)
		if (socket_close(s->fd) < 0)
			CERR("socket_close error");

	s->type = SOCKET_TYPE_INVALID;
}

void 
sserver_delete(sserver_t ss) {
	struct smessage dummy;
	for (int i = 0; i < LEN(ss->slot); ++i) {
		struct socket * s = &ss->slot[i];
		if (s->type != SOCKET_TYPE_RESERVE)
			force_close(ss, s , &dummy);
	}
	socket_close(ss->sendctrl_fd);
	socket_close(ss->recvctrl_fd);
	sp_delete(ss->event_fd);
	free(ss);
}

static struct socket *
new_fd(sserver_t ss, int id, socket_t fd, int protocol, uintptr_t opaque, bool add) {
	struct socket * s = &ss->slot[HASH_ID(id)];
	assert(s->type == SOCKET_TYPE_RESERVE);

	if (add) {
		if (sp_add(ss->event_fd, fd, s)) {
			s->type = SOCKET_TYPE_INVALID;
			return NULL;
		}
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
	return s;
}

// return -1 when connecting
static int
open_socket(sserver_t ss, struct request_open * request, smessage_t result) {
	char port[16];
	struct socket * ns;
	int status, id = request->id;
	socket_t sock = INVALID_SOCKET;
	struct addrinfo * ai_ptr = NULL;
	struct addrinfo * ai_list = NULL;
	struct addrinfo ai_hints = { 0 };

	result->ud = 0;
	result->id = id;
	result->data = NULL;
	result->opaque = request->opaque;

	sprintf(port, "%hu", request->port);
	ai_hints.ai_family = AF_UNSPEC;
	ai_hints.ai_socktype = SOCK_STREAM;
	ai_hints.ai_protocol = IPPROTO_TCP;

	status = getaddrinfo(request->host, port, &ai_hints, &ai_list);
	if (status != 0) {
		result->data = (void *)gai_strerror(status);
		goto __failed;
	}

	for (ai_ptr = ai_list; NULL != ai_ptr; ai_ptr = ai_ptr->ai_next) {
		sock = socket(ai_ptr->ai_family, ai_ptr->ai_socktype, ai_ptr->ai_protocol);
		if (INVALID_SOCKET == sock)
			continue;

		socket_set_keepalive(sock);
		socket_set_nonblock(sock);
		status = connect(sock, ai_ptr->ai_addr, ai_ptr->ai_addrlen);
		if (status != 0 && socket_errno != SOCKET_CONNECTED) {
			socket_close(sock);
			sock = INVALID_SOCKET;
			continue;
		}
		break;
	}

	if (INVALID_SOCKET == sock) {
		result->data = (void *)sys_strerror(socket_errno);
		goto __failed;
	}

	ns = new_fd(ss, id, sock, IPPROTO_TCP, request->opaque, true);
	if (ns == NULL) {
		socket_close(sock);
		result->data = "reach skynet socket number limit";
		goto __failed;
	}

	if(status == 0) {
		ns->type = SOCKET_TYPE_CONNECTED;
		struct sockaddr * addr = ai_ptr->ai_addr;
		void * sin_addr = ai_ptr->ai_family == AF_INET ? (void *)&((struct sockaddr_in *)addr)->sin_addr : (void *)&((struct sockaddr_in6 *)addr)->sin6_addr;
		if (inet_ntop(ai_ptr->ai_family, sin_addr, ss->buffer, sizeof(ss->buffer)))
			result->data = ss->buffer;

		freeaddrinfo(ai_list);
		return SSERVER_OPEN;
	}
	freeaddrinfo(ai_list);
	ns->type = SOCKET_TYPE_CONNECTING;
	sp_write(ss->event_fd, ns->fd, ns, true);
	return -1;

__failed:
	freeaddrinfo(ai_list);
	ss->slot[HASH_ID(id)].type = SOCKET_TYPE_INVALID;
	return SSERVER_ERR;
}

static int
send_list_tcp(sserver_t ss, struct socket * s, struct wb_list * list, smessage_t result) {
	while (list->head) {
		struct write_buffer * tmp = list->head;
		for (;;) {
			int sz = socket_write(s->fd, tmp->ptr, tmp->sz);
			if (sz < 0) {
				switch (socket_errno) {
				case SOCKET_EINTR:
					continue;
				case SOCKET_WAGAIN:
					return -1;
				}
				force_close(ss, s, result);
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
udp_socket_address(struct socket * s, const uint8_t udp_address[UDP_ADDRESS_SIZE], union sockaddr_all * sa) {
	uint16_t port;
	if (udp_address[0] != s->protocol)
		return 0;

	memcpy(&port, udp_address + 1, sizeof(uint16_t));
	switch (s->protocol) {
	case IPPROTO_UDP:	// ipv4 address is 32 bits
		memset(&sa->v4, 0, sizeof(sa->v4));
		sa->s.sa_family = AF_INET;
		sa->v4.sin_port = port;
		memcpy(&sa->v4.sin_addr, udp_address + 1 + sizeof(uint16_t), sizeof(sa->v4.sin_addr));
		return sizeof(sa->v4);
	case IPPROTO_UDPv6:	// ipv6 address is 128 bits
		memset(&sa->v6, 0, sizeof(sa->v6));
		sa->s.sa_family = AF_INET6;
		sa->v6.sin6_port = port;
		memcpy(&sa->v6.sin6_addr, udp_address + 1 + sizeof(uint16_t), sizeof(sa->v6.sin6_addr));
		return sizeof(sa->v6);
	}
	return 0;
}

static int
send_list_udp(sserver_t ss, struct socket * s, struct wb_list * list, smessage_t result) {
	while (list->head) {
		union sockaddr_all sa;
		struct write_buffer * tmp = list->head;
		socklen_t sasz = udp_socket_address(s, tmp->udp_address, &sa);
		int err = sendto(s->fd, tmp->ptr, tmp->sz, 0, &sa.s, sasz);
		if (err < 0) {
			switch (socket_errno) {
			case SOCKET_EINTR:
			case SOCKET_WAGAIN:
				return -1;
			}
			RETURN(-1, "socket-server : udp (%d) sendto error!", s->id);
		}

		s->wb_size -= tmp->sz;
		list->head = tmp->next;
		write_buffer_free(ss, tmp);
	}
	list->tail = NULL;

	return -1;
}

static int
send_list(sserver_t ss, struct socket * s, struct wb_list * list, smessage_t result) {
	if (s->protocol == IPPROTO_TCP)
		return send_list_tcp(ss, s, list, result);
	return send_list_udp(ss, s, list, result);
}

static inline bool
list_uncomplete(struct wb_list * s) {
	struct write_buffer * wb = s->head;
	if (NULL == wb)
		return false;
	return (void *)wb->ptr != wb->buffer;
}

static inline void
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

static inline bool
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
send_buffer(sserver_t ss, struct socket * s, smessage_t result) {
	assert(!list_uncomplete(&s->low));
	// step 1
	if (send_list(ss, s, &s->high, result) == SSERVER_CLOSE)
		return SSERVER_CLOSE;

	if (s->high.head == NULL) {
		// step 2
		if (s->low.head != NULL) {
			if (send_list(ss, s, &s->low, result) == SSERVER_CLOSE)
				return SSERVER_CLOSE;

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
				force_close(ss, s, result);
				return SSERVER_CLOSE;
		}
		if(s->warn_size > 0){
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

static struct write_buffer *
append_sendbuffer_(sserver_t ss, struct wb_list * s, struct request_send * request, int size, int n) {
	struct send_object so;
	struct write_buffer * buf = malloc(size);
	buf->userobject = send_object_init(ss, &so, request->buffer, request->sz);
	buf->ptr = (char *)so.buffer + n;
	buf->sz = so.sz - n;
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
append_sendbuffer_udp(sserver_t ss, struct socket * s, int priority, struct request_send * request, const uint8_t udp_address[UDP_ADDRESS_SIZE]) {
	struct wb_list * wl = priority == PRIORITY_HIGH ? &s->high : &s->low;
	struct write_buffer * buf = append_sendbuffer_(ss, wl, request, sizeof(struct write_buffer), 0);
	memcpy(buf->udp_address, udp_address, UDP_ADDRESS_SIZE);
	s->wb_size += buf->sz;
}

static inline void
append_sendbuffer(sserver_t ss, struct socket * s, struct request_send * request, int n) {
	struct write_buffer * buf = append_sendbuffer_(ss, &s->high, request, offsetof(struct write_buffer, udp_address), n);
	s->wb_size += buf->sz;
}

static inline void
append_sendbuffer_low(sserver_t ss, struct socket * s, struct request_send * request) {
	struct write_buffer * buf = append_sendbuffer_(ss, &s->low, request, offsetof(struct write_buffer, udp_address), 0);
	s->wb_size += buf->sz;
}

/*
	When send a package , we can assign the priority : PRIORITY_HIGH or PRIORITY_LOW

	If socket buffer is empty, write to fd directly.
		If write a part, append the rest part to high list. (Even priority is PRIORITY_LOW)
	Else append package to high (PRIORITY_HIGH) or low (PRIORITY_LOW) list.
 */
static int
send_socket(sserver_t ss, struct request_send * request, smessage_t result, int priority, const uint8_t * udp_address) {
	int id = request->id;
	struct send_object so;
	struct socket * s = &ss->slot[HASH_ID(id)];
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
		if (s->protocol == IPPROTO_TCP) {
			int n = socket_write(s->fd, so.buffer, so.sz);
			if (n < 0) {
				switch (socket_errno) {
				case SOCKET_EINTR:
				case SOCKET_WAGAIN:
					n = 0;
					break;
				default:
					CERR("socket-server: write to %d (fd=%d) error!", id, s->fd);
					force_close(ss, s, result);
					so.free_func(request->buffer);
					return SSERVER_CLOSE;
				}
			}
			if (n == so.sz) {
				so.free_func(request->buffer);
				return -1;
			}
			append_sendbuffer(ss, s, request, n);	// add to high priority list, even priority == PRIORITY_LOW
		}
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
	} 
	else {
		if (s->protocol == IPPROTO_TCP) {
			if (priority == PRIORITY_LOW)
				append_sendbuffer_low(ss, s, request);
			else
				append_sendbuffer(ss, s, request, 0);
		} 
		else {
			if (udp_address == NULL)
				udp_address = s->p.udp_address;
			append_sendbuffer_udp(ss, s, priority, request, udp_address);
		}
	}
	if (s->wb_size >= WARNING_SIZE && s->wb_size >= s->warn_size) {
		s->warn_size = s->warn_size ? s->warn_size * 2 : WARNING_SIZE * 2;
		result->opaque = s->opaque;
		result->id = s->id;
		result->ud = (int)(s->wb_size / 1024) + !!(s->wb_size % 1024);
		result->data = NULL;
		return SSERVER_WARNING;
	}
	return -1;
}

static int
listen_socket(sserver_t ss, struct request_listen * request, smessage_t result) {
	int id = request->id;
	socket_t listen_fd = request->fd;
	struct socket * s = new_fd(ss, id, listen_fd, IPPROTO_TCP, request->opaque, false);
	if (s) {
		s->type = SOCKET_TYPE_PLISTEN;
		return -1;
	}

	socket_close(listen_fd);
	result->opaque = request->opaque;
	result->id = id;
	result->ud = 0;
	result->data = "reach skynet socket number limit";
	ss->slot[HASH_ID(id)].type = SOCKET_TYPE_INVALID;

	return SSERVER_ERR;
}

static int
close_socket(sserver_t ss, struct request_close * request, smessage_t result) {
	int id = request->id;
	struct socket * s = &ss->slot[HASH_ID(id)];
	if (s->type == SOCKET_TYPE_INVALID || s->id != id) {
		result->id = id;
		result->opaque = request->opaque;
		result->ud = 0;
		result->data = NULL;
		return SSERVER_CLOSE;
	}
	if (!send_buffer_empty(s)) { 
		int type = send_buffer(ss, s, result);
		// type : -1 or SOCKET_WARNING or SOCKET_CLOSE, SOCKET_WARNING means send_buffer_empty
		if (type != -1 && type != SSERVER_WARNING)
			return type;
	}
	if (request->shutdown || send_buffer_empty(s)) {
		force_close(ss, s, result);
		result->id = id;
		result->opaque = request->opaque;
		return SSERVER_CLOSE;
	}
	s->type = SOCKET_TYPE_HALFCLOSE;

	return -1;
}

static int
bind_socket(sserver_t ss, struct request_bind * request, smessage_t result) {
	int id = request->id;
	result->id = id;
	result->opaque = request->opaque;
	result->ud = 0;
	struct socket * s = new_fd(ss, id, request->fd, IPPROTO_TCP, request->opaque, true);
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
start_socket(sserver_t ss, struct request_start * request, smessage_t result) {
	int id = request->id;
	result->id = id;
	result->opaque = request->opaque;
	result->ud = 0;
	result->data = NULL;
	struct socket * s = &ss->slot[HASH_ID(id)];
	if (s->type == SOCKET_TYPE_INVALID || s->id !=id) {
		result->data = "invalid socket";
		return SSERVER_ERR;
	}
	if (s->type == SOCKET_TYPE_PACCEPT || s->type == SOCKET_TYPE_PLISTEN) {
		if (sp_add(ss->event_fd, s->fd, s)) {
			force_close(ss, s, result);
			result->data = (void *)sys_strerror(socket_errno);
			return SSERVER_ERR;
		}
		s->type = s->type == SOCKET_TYPE_PACCEPT ? SOCKET_TYPE_CONNECTED : SOCKET_TYPE_LISTEN;
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
	// if s->type == SOCKET_TYPE_HALFCLOSE , SOCKET_CLOSE message will send later
	return -1;
}

static inline void
setopt_socket(sserver_t ss, struct request_setopt * request) {
	int id = request->id;
	struct socket * s = &ss->slot[HASH_ID(id)];
	if (s->type == SOCKET_TYPE_INVALID || s->id !=id)
		return;

	setsockopt(s->fd, IPPROTO_TCP, request->what, (const void *)&request->value, sizeof(request->value));
}

static void
block_readpipe(socket_t pipefd, void * buffer, int sz) {
	while(sz > 0) {
		// winds and linux read sz = zero, before is block, after is nonblock
		int n = socket_read(pipefd, buffer, sz);
		if (n < 0) {
			if (socket_errno == SOCKET_EINTR)
				continue;
			RETURN(NIL, "socket-server : read pipe error!");
		}
		// must atomic read from a pipe
		assert(n == sz);
		break;
	}
}

static inline bool
has_cmd(sserver_t ss) {
	struct timeval tv = { 0, 0 };
	FD_SET(ss->recvctrl_fd, &ss->rfds);
	return 1 == select(ss->recvctrl_fd + 1, &ss->rfds, NULL, NULL, &tv);
}

static void
add_udp_socket(sserver_t ss, struct request_udp * udp) {
	int id = udp->id;
	int protocol = udp->family == AF_INET6 ? IPPROTO_UDPv6 : IPPROTO_UDP;
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
set_udp_address(sserver_t ss, struct request_setudp * request, smessage_t result) {
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
	memcpy(s->p.udp_address, request->address, type == IPPROTO_UDP ? 1 + 2 + 4 : 1 + 2 + 16);
	return -1;
}

// return type
static int
ctrl_cmd(sserver_t ss, smessage_t result) {
	socket_t fd = ss->recvctrl_fd;
	// the length of message is one byte, so 256+8 buffer size is enough.
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
		result->opaque = 0;
		result->id = 0;
		result->ud = 0;
		result->data = NULL;
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
	};

	RETURN(-1, "socket-server: Unknown ctrl %c | %d.", type, type);
}

// return -1 (ignore) when error
static int
forward_message_tcp(sserver_t ss, struct socket * s, smessage_t  result) {
	int sz = s->p.size;
	char * errs, * buffer = malloc(sz);
	int n = socket_read(s->fd, buffer, sz);
	if (n < 0) {
		free(buffer);
		switch(socket_errno) {
		case SOCKET_EINTR:
			break;
		case SOCKET_WAGAIN:
			CERR("socket-server: EAGAIN capture.");
			break;
		default: 			
			// close when error
			errs = (void *)sys_strerror(socket_errno);
			force_close(ss, s, result);
			result->data = errs;
			return SSERVER_ERR;
		}
		return -1;
	}
	if (n == 0) {
		free(buffer);
		force_close(ss, s, result);
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
gen_udp_address(int protocol, union sockaddr_all * sa, uint8_t * udp_address) {
	int addrsz = 1;
	udp_address[0] = (uint8_t)protocol;
	if (protocol == IPPROTO_UDP) {
		memcpy(udp_address + addrsz, &sa->v4.sin_port, sizeof(sa->v4.sin_port));
		addrsz += sizeof(sa->v4.sin_port);
		memcpy(udp_address + addrsz, &sa->v4.sin_addr, sizeof(sa->v4.sin_addr));
		addrsz += sizeof(sa->v4.sin_addr);
	} 
	else {
		memcpy(udp_address + addrsz, &sa->v6.sin6_port, sizeof(sa->v6.sin6_port));
		addrsz += sizeof(sa->v6.sin6_port);
		memcpy(udp_address + addrsz, &sa->v6.sin6_addr, sizeof(sa->v6.sin6_addr));
		addrsz += sizeof(sa->v6.sin6_addr);
	}
	return addrsz;
}

static int
forward_message_udp(sserver_t ss, struct socket * s, smessage_t  result) {
	union sockaddr_all sa;
	socklen_t slen = sizeof(sa);
	int n = recvfrom(s->fd, (void *)ss->udpbuffer, MAX_UDP_PACKAGE, 0, &sa.s, &slen);
	if (n < 0) {
		switch(socket_errno) {
		case SOCKET_EINTR:
		case SOCKET_WAGAIN:
			break;
		default:
			// close when error
			force_close(ss, s, result);
			result->data = strerror(socket_errno);
			return SSERVER_ERR;
		}
		return -1;
	}
	uint8_t * data;
	if (slen == sizeof(sa.v4)) {
		if (s->protocol != IPPROTO_UDP)
			return -1;
		data = malloc(n + 1 + 2 + 4);
		gen_udp_address(IPPROTO_UDP, &sa, data + n);
	} 
	else {
		if (s->protocol != IPPROTO_UDPv6)
			return -1;
		data = malloc(n + 1 + 2 + 16);
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
report_connect(sserver_t ss, struct socket * s, smessage_t result) {
	int error = socket_get_error(s->fd);
	if (error) {
		force_close(ss, s, result);
		result->data = (void *)sys_strerror(error);
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
report_accept(sserver_t ss, struct socket * s, smessage_t result) {
	union sockaddr_all u;
	socklen_t len = sizeof(u);
	socket_t client_fd = accept(s->fd, &u.s, &len);
	if (client_fd == INVALID_SOCKET) {
		if (socket_errno == SOCKET_EMFILE || socket_errno == SOCKET_ENFILE) {
			result->opaque = s->opaque;
			result->id = s->id;
			result->ud = 0;
			result->data = (void *)sys_strerror(socket_errno);
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

static void 
clear_closed_event(sserver_t ss, smessage_t result, int type) {
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
				if (socket_errno == SOCKET_EINTR)
					continue;
				return -1;
			}
		}
		struct event * e = &ss->ev[ss->event_index++];
		struct socket * s = e->s;
		// dispatch pipe message at beginning
		if (s == NULL)
			continue;

		switch (s->type) {
		case SOCKET_TYPE_CONNECTING:
			return report_connect(ss, s, result);
		case SOCKET_TYPE_LISTEN: {
			int ok = report_accept(ss, s, result);
			// when ok == 0, retry
			if(ok == 0)
				break;
			return ok > 0 ? SSERVER_ACCEPT : SSERVER_ERR;
		}
		case SOCKET_TYPE_INVALID:
			CERR("socket-server: invalid socket");
			break;
		default:
			if (e->error) {
				// close when error
				int error = socket_get_error(s->fd);
				force_close(ss, s, result);
				result->data = error ? (void *)sys_strerror(error) : "Unknown error";
				return SSERVER_ERR;
			}
			if (e->read) {
				int type;
				if (s->protocol == IPPROTO_TCP)
					type = forward_message_tcp(ss, s, result);
				else {
					type = forward_message_udp(ss, s, result);
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
				int type = send_buffer(ss, s, result);
				if (type == -1)
					break;
				return type;
			}
		}
	}
}

static void
send_request(sserver_t ss, struct request_package * request, char type, int len) {
	request->header[6] = (uint8_t)type;
	request->header[7] = (uint8_t)len;
	for (;;) {
		int n = socket_write(ss->sendctrl_fd, &request->header[6], len + 2);
		if (n < 0) {
			if (socket_errno != SOCKET_EINTR)
				CERR("socket-server : send ctrl command error!");
			continue;
		}
		assert(n == len + 2);
		break;
	}
}

static int
open_request(sserver_t ss, struct request_package * req, uintptr_t opaque, const char * addr, uint16_t port) {
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
	memcpy(req->u.open.host, addr, len + 1);

	return len;
}

inline int
sserver_connect(sserver_t ss, uintptr_t opaque, const char * addr, uint16_t port) {
	struct request_package request;
	int len = open_request(ss, &request, opaque, addr, port);
	if (len < 0)
		return -1;
	send_request(ss, &request, 'O', sizeof(request.u.open) + len);
	return request.u.open.id;
}

static inline void
free_buffer(sserver_t ss, const void * buffer, int sz) {
	struct send_object so;
	send_object_init(ss, &so, (void *)buffer, sz);
	so.free_func((void *)buffer);
}

// return -1 when error, 0 when success
int 
sserver_send(sserver_t ss, int id, const void * buffer, int sz) {
	struct socket * s = &ss->slot[HASH_ID(id)];
	if (s->id != id || s->type == SOCKET_TYPE_INVALID) {
		free_buffer(ss, buffer, sz);
		return -1;
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
	socket_t fd = socket_listen(addr, port);
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

inline int
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
	int family;
	socket_t fd;
	if (port != 0 || addr != NULL) // bind
		fd = socket_bind(addr, port, IPPROTO_UDP, &family);
	else {
		family = PF_INET;
		fd = socket(family, SOCK_DGRAM, IPPROTO_UDP);
	}
	if (fd == INVALID_SOCKET)
		return -1;

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
sserver_udp_send(sserver_t ss, int id, const saddrudp_t addr, const void * buffer, int sz) {
	struct socket * s = &ss->slot[HASH_ID(id)];
	if (s->id != id || s->type == SOCKET_TYPE_INVALID) {
		free_buffer(ss, buffer, sz);
		return -1;
	}

	struct request_package request;
	request.u.send_udp.send.id = id;
	request.u.send_udp.send.sz = sz;
	request.u.send_udp.send.buffer = (char *)buffer;

	int addrsz;
	switch (addr[0]) {
	case IPPROTO_UDP:
		addrsz = 1 + 2 + 4;		// 1 type, 2 port, 4 ipv4
		break;
	case IPPROTO_UDPv6:
		addrsz = 1 + 2 + 16;	// 1 type, 2 port, 16 ipv6
		break;
	default:
		free_buffer(ss, buffer, sz);
		return -1;
	}

	memcpy(request.u.send_udp.address, addr, addrsz);
	send_request(ss, &request, 'A', sizeof(request.u.send_udp.send) + addrsz);
	return 0;
}

int
sserver_udp_connect(sserver_t ss, int id, const char * addr, uint16_t port) {
	int status;
	char portstr[16];
	struct addrinfo ai_hints = { 0 };
	struct addrinfo * ai_list = NULL;
	sprintf(portstr, "%hu", port);
	ai_hints.ai_family = AF_UNSPEC;
	ai_hints.ai_socktype = SOCK_DGRAM;
	ai_hints.ai_protocol = IPPROTO_UDP;

	status = getaddrinfo(addr, portstr, &ai_hints, &ai_list);
	if (status != 0)
		return -1;

	struct request_package request;
	request.u.set_udp.id = id;
	int protocol;
	if (ai_list->ai_family == AF_INET)
		protocol = IPPROTO_UDP;
	else if (ai_list->ai_family == AF_INET6)
		protocol = IPPROTO_UDPv6;
	else {
		freeaddrinfo(ai_list);
		return -1;
	}

	int addrsz = gen_udp_address(protocol, (union sockaddr_all *)ai_list->ai_addr, request.u.set_udp.address);
	freeaddrinfo(ai_list);
	send_request(ss, &request, 'C', sizeof(request.u.set_udp) - sizeof(request.u.set_udp.address) + addrsz);
	return 0;
}

inline const saddrudp_t
sserver_udp_address(sserver_t ss, smessage_t msg, int * addrsz) {
	uint8_t * address = (uint8_t *)(msg->data + msg->ud);
	switch(address[0]) {
	case IPPROTO_UDP:
		*addrsz = 1 + 2 + 4;
		break;
	case IPPROTO_UDPv6:
		*addrsz = 1 + 2 + 16;
		break;
	default:
		return NULL;
	}
	return address;
}