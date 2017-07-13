#include <socket_server.h>
#include <scatom.h>

#define MAX_INFO 128
#define MAX_EVENT 64
#define MIN_READ_BUFFER 64

#define SOCKET_TYPE_INVALID 0
#define SOCKET_TYPE_RESERVE 1
#define SOCKET_TYPE_PLISTEN 2
#define SOCKET_TYPE_LISTEN 3
#define SOCKET_TYPE_CONNECTING 4
#define SOCKET_TYPE_CONNECTED 5
#define SOCKET_TYPE_HALFCLOSE 6
#define SOCKET_TYPE_PACCEPT 7
#define SOCKET_TYPE_BIND 8

#define PRIORITY_HIGH 0
#define PRIORITY_LOW 1

// MAX_SOCKET will be 2 ^ [x]
#define MAX_SOCKET (1 << 16)
#define HASH_ID(id) (((unsigned)(id)) % MAX_SOCKET)

#define PROTOCOL_TCP 0
#define PROTOCOL_UDP 1
#define PROTOCOL_UDPv6 2

// ipv6 128bit + port 16bit + 1 byte type
#define UDP_ADDRESS_SIZE 19	

#define MAX_UDP_PACKAGE 65535

#define WARNING_SIZE (1024 * 1024)


// EAGAIN and EWOULDBLOCK may be not the same value.
#if (SOCKET_EAGAIN != SOCKET_EWOULDBOCK)
#define AGAIN_WOULDBLOCK SOCKET_EAGAIN : case SOCKET_EWOULDBOCK
#else
#define AGAIN_WOULDBLOCK SOCKET_EAGAIN
#endif

struct write_buffer {
	struct write_buffer * next;
	void *buffer;
	char *ptr;
	int sz;
	bool userobject;
	uint8_t udp_address[UDP_ADDRESS_SIZE];
};

#define SIZEOF_TCPBUFFER (offsetof(struct write_buffer, udp_address))
#define SIZEOF_UDPBUFFER (sizeof(struct write_buffer))

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
	int checkctrl;
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
	int port;
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
	int fd;
	uintptr_t opaque;
	char host[1];
};

struct request_bind {
	int id;
	int fd;
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
	int fd;
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
	void (*free_func)(void *);
};

#define MALLOC malloc
#define FREE free

static inline bool
send_object_init(sserver_t ss, struct send_object *so, void *object, int sz) {
	if (sz < 0) {
		so->buffer = ss->soi.buffer(object);
		so->sz = ss->soi.size(object);
		so->free_func = ss->soi.free;
		return true;
	} else {
		so->buffer = object;
		so->sz = sz;
		so->free_func = FREE;
		return false;
	}
}

static inline void
write_buffer_free(sserver_t ss, struct write_buffer *wb) {
	if (wb->userobject) {
		ss->soi.free(wb->buffer);
	} else {
		FREE(wb->buffer);
	}
	FREE(wb);
}

static void
socket_keepalive(int fd) {
	int keepalive = 1;
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive));  
}

static int
reserve_id(sserver_t ss) {
	int i;
	for (i=0;i<MAX_SOCKET;i++) {
		int id = ATOM_INC(ss->alloc_id);
		if (id < 0) {
			id = ATOM_AND(ss->alloc_id, 0x7fffffff);
		}
		struct socket *s = &ss->slot[HASH_ID(id)];
		if (s->type == SOCKET_TYPE_INVALID) {
			if (ATOM_CAS(s->type, SOCKET_TYPE_INVALID, SOCKET_TYPE_RESERVE)) {
				s->id = id;
				s->fd = -1;
				return id;
			} else {
				// retry
				--i;
			}
		}
	}
	return -1;
}

static inline void
clear_wb_list(struct wb_list *list) {
	list->head = NULL;
	list->tail = NULL;
}

sserver_t  
sserver_create() {
	int i;
	socket_t fd[2];
	poll_t efd = sp_create();
	if (sp_invalid(efd)) {
		fprintf(stderr, "socket-server: create event pool failed.\n");
		return NULL;
	}
	if (pipe(fd)) {
		sp_delete(efd);
		fprintf(stderr, "socket-server: create socket pair failed.\n");
		return NULL;
	}
	if (sp_add(efd, fd[0], NULL)) {
		// add recvctrl_fd to event poll
		fprintf(stderr, "socket-server: can't add server fd to event pool.\n");
		socket_close(fd[0]);
		socket_close(fd[1]);
		sp_delete(efd);
		return NULL;
	}

	sserver_t ss = MALLOC(sizeof(*ss));
	ss->event_fd = efd;
	ss->recvctrl_fd = fd[0];
	ss->sendctrl_fd = fd[1];
	ss->checkctrl = 1;

	for (i=0;i<MAX_SOCKET;i++) {
		struct socket *s = &ss->slot[i];
		s->type = SOCKET_TYPE_INVALID;
		clear_wb_list(&s->high);
		clear_wb_list(&s->low);
	}
	ss->alloc_id = 0;
	ss->event_n = 0;
	ss->event_index = 0;
	memset(&ss->soi, 0, sizeof(ss->soi));
	FD_ZERO(&ss->rfds);
	assert(ss->recvctrl_fd < FD_SETSIZE);

	return ss;
}

static void
free_wb_list(sserver_t ss, struct wb_list *list) {
	struct write_buffer *wb = list->head;
	while (wb) {
		struct write_buffer *tmp = wb;
		wb = wb->next;
		write_buffer_free(ss, tmp);
	}
	list->head = NULL;
	list->tail = NULL;
}

static void
force_close(sserver_t ss, struct socket *s, smessage_t result) {
	result->id = s->id;
	result->ud = 0;
	result->data = NULL;
	result->opaque = s->opaque;
	if (s->type == SOCKET_TYPE_INVALID) {
		return;
	}
	assert(s->type != SOCKET_TYPE_RESERVE);
	free_wb_list(ss,&s->high);
	free_wb_list(ss,&s->low);
	if (s->type != SOCKET_TYPE_PACCEPT && s->type != SOCKET_TYPE_PLISTEN) {
		sp_del(ss->event_fd, s->fd);
	}
	if (s->type != SOCKET_TYPE_BIND) {
		if (socket_close(s->fd) < 0) {
			perror("socket_close socket:");
		}
	}
	s->type = SOCKET_TYPE_INVALID;
}

void 
sserver_release(sserver_t ss) {
	int i;
	struct smessage dummy;
	for (i=0;i<MAX_SOCKET;i++) {
		struct socket *s = &ss->slot[i];
		if (s->type != SOCKET_TYPE_RESERVE) {
			force_close(ss, s , &dummy);
		}
	}
	socket_close(ss->sendctrl_fd);
	socket_close(ss->recvctrl_fd);
	sp_delete(ss->event_fd);
	FREE(ss);
}

static inline void
check_wb_list(struct wb_list *s) {
	assert(s->head == NULL);
	assert(s->tail == NULL);
}

static struct socket *
new_fd(sserver_t ss, int id, int fd, int protocol, uintptr_t opaque, bool add) {
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
	check_wb_list(&s->high);
	check_wb_list(&s->low);
	return s;
}

// return -1 when connecting
static int
open_socket(sserver_t ss, struct request_open * request, smessage_t result) {
	int id = request->id;
	result->opaque = request->opaque;
	result->id = id;
	result->ud = 0;
	result->data = NULL;
	struct socket *ns;
	int status;
	struct addrinfo ai_hints;
	struct addrinfo *ai_list = NULL;
	struct addrinfo *ai_ptr = NULL;
	char port[16];
	sprintf(port, "%d", request->port);
	memset(&ai_hints, 0, sizeof( ai_hints ) );
	ai_hints.ai_family = AF_UNSPEC;
	ai_hints.ai_socktype = SOCK_STREAM;
	ai_hints.ai_protocol = IPPROTO_TCP;

	status = getaddrinfo( request->host, port, &ai_hints, &ai_list );
	if ( status != 0 ) {
		result->data = (void *)gai_strerror(status);
		goto _failed;
	}
	int sock= -1;
	for (ai_ptr = ai_list; ai_ptr != NULL; ai_ptr = ai_ptr->ai_next ) {
		sock = socket( ai_ptr->ai_family, ai_ptr->ai_socktype, ai_ptr->ai_protocol );
		if ( sock < 0 ) {
			continue;
		}
		socket_keepalive(sock);
		socket_set_nonblock(sock);
		status = connect( sock, ai_ptr->ai_addr, ai_ptr->ai_addrlen);
		if ( status != 0 && socket_errno != SOCKET_EINPROGRESS) {
			socket_close(sock);
			sock = -1;
			continue;
		}
		break;
	}

	if (sock < 0) {
		result->data = sys_strerror(socket_errno);
		goto _failed;
	}

	ns = new_fd(ss, id, sock, PROTOCOL_TCP, request->opaque, true);
	if (ns == NULL) {
		socket_close(sock);
		result->data = "reach skynet socket number limit";
		goto _failed;
	}

	if(status == 0) {
		ns->type = SOCKET_TYPE_CONNECTED;
		struct sockaddr * addr = ai_ptr->ai_addr;
		void * sin_addr = (ai_ptr->ai_family == AF_INET) ? (void*)&((struct sockaddr_in *)addr)->sin_addr : (void*)&((struct sockaddr_in6 *)addr)->sin6_addr;
		if (inet_ntop(ai_ptr->ai_family, sin_addr, ss->buffer, sizeof(ss->buffer))) {
			result->data = ss->buffer;
		}
		freeaddrinfo( ai_list );
		return SSERVER_OPEN;
	} else {
		ns->type = SOCKET_TYPE_CONNECTING;
		sp_write(ss->event_fd, ns->fd, ns, true);
	}

	freeaddrinfo( ai_list );
	return -1;
_failed:
	freeaddrinfo( ai_list );
	ss->slot[HASH_ID(id)].type = SOCKET_TYPE_INVALID;
	return SSERVER_ERR;
}

static int
send_list_tcp(sserver_t ss, struct socket *s, struct wb_list *list, smessage_t result) {
	while (list->head) {
		struct write_buffer * tmp = list->head;
		for (;;) {
			int sz = write(s->fd, tmp->ptr, tmp->sz);
			if (sz < 0) {
				switch (socket_errno) {
				case SOCKET_EINTR:
					continue;
				case AGAIN_WOULDBLOCK:
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
		write_buffer_free(ss,tmp);
	}
	list->tail = NULL;

	return -1;
}

static socklen_t
udp_socket_address(struct socket *s, const uint8_t udp_address[UDP_ADDRESS_SIZE], union sockaddr_all *sa) {
	int type = (uint8_t)udp_address[0];
	if (type != s->protocol)
		return 0;
	uint16_t port = 0;
	memcpy(&port, udp_address+1, sizeof(uint16_t));
	switch (s->protocol) {
	case PROTOCOL_UDP:
		memset(&sa->v4, 0, sizeof(sa->v4));
		sa->s.sa_family = AF_INET;
		sa->v4.sin_port = port;
		memcpy(&sa->v4.sin_addr, udp_address + 1 + sizeof(uint16_t), sizeof(sa->v4.sin_addr));	// ipv4 address is 32 bits
		return sizeof(sa->v4);
	case PROTOCOL_UDPv6:
		memset(&sa->v6, 0, sizeof(sa->v6));
		sa->s.sa_family = AF_INET6;
		sa->v6.sin6_port = port;
		memcpy(&sa->v6.sin6_addr, udp_address + 1 + sizeof(uint16_t), sizeof(sa->v6.sin6_addr)); // ipv6 address is 128 bits
		return sizeof(sa->v6);
	}
	return 0;
}

static int
send_list_udp(sserver_t ss, struct socket *s, struct wb_list *list, smessage_t result) {
	while (list->head) {
		struct write_buffer * tmp = list->head;
		union sockaddr_all sa;
		socklen_t sasz = udp_socket_address(s, tmp->udp_address, &sa);
		int err = sendto(s->fd, tmp->ptr, tmp->sz, 0, &sa.s, sasz);
		if (err < 0) {
			switch (socket_errno) {
			case SOCKET_EINTR:
			case AGAIN_WOULDBLOCK:
				return -1;
			}
			fprintf(stderr, "socket-server : udp (%d) sendto error %s.\n", s->id, sys_strerror(socket_errno));
			return -1;
			/*			// ignore udp sendto error

			result->opaque = s->opaque;
			result->id = s->id;
			result->ud = 0;
			result->data = NULL;

			return SOCKET_ERR;
			*/
		}

		s->wb_size -= tmp->sz;
		list->head = tmp->next;
		write_buffer_free(ss,tmp);
	}
	list->tail = NULL;

	return -1;
}

static int
send_list(sserver_t ss, struct socket *s, struct wb_list *list, smessage_t result) {
	if (s->protocol == PROTOCOL_TCP) {
		return send_list_tcp(ss, s, list, result);
	} else {
		return send_list_udp(ss, s, list, result);
	}
}

static inline int
list_uncomplete(struct wb_list *s) {
	struct write_buffer *wb = s->head;
	if (wb == NULL)
		return 0;
	
	return (void *)wb->ptr != wb->buffer;
}

static void
raise_uncomplete(struct socket * s) {
	struct wb_list *low = &s->low;
	struct write_buffer *tmp = low->head;
	low->head = tmp->next;
	if (low->head == NULL) {
		low->tail = NULL;
	}

	// move head of low list (tmp) to the empty high list
	struct wb_list *high = &s->high;
	assert(high->head == NULL);

	tmp->next = NULL;
	high->head = high->tail = tmp;
}

static inline int
send_buffer_empty(struct socket *s) {
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
send_buffer(sserver_t ss, struct socket *s, smessage_t result) {
	assert(!list_uncomplete(&s->low));
	// step 1
	if (send_list(ss,s,&s->high,result) == SSERVER_CLOSE) {
		return SSERVER_CLOSE;
	}
	if (s->high.head == NULL) {
		// step 2
		if (s->low.head != NULL) {
			if (send_list(ss,s,&s->low,result) == SSERVER_CLOSE) {
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
append_sendbuffer_(sserver_t ss, struct wb_list *s, struct request_send * request, int size, int n) {
	struct write_buffer * buf = MALLOC(size);
	struct send_object so;
	buf->userobject = send_object_init(ss, &so, request->buffer, request->sz);
	buf->ptr = (char*)so.buffer+n;
	buf->sz = so.sz - n;
	buf->buffer = request->buffer;
	buf->next = NULL;
	if (s->head == NULL) {
		s->head = s->tail = buf;
	} else {
		assert(s->tail != NULL);
		assert(s->tail->next == NULL);
		s->tail->next = buf;
		s->tail = buf;
	}
	return buf;
}

static inline void
append_sendbuffer_udp(sserver_t ss, struct socket *s, int priority, struct request_send * request, const uint8_t udp_address[UDP_ADDRESS_SIZE]) {
	struct wb_list *wl = (priority == PRIORITY_HIGH) ? &s->high : &s->low;
	struct write_buffer *buf = append_sendbuffer_(ss, wl, request, SIZEOF_UDPBUFFER, 0);
	memcpy(buf->udp_address, udp_address, UDP_ADDRESS_SIZE);
	s->wb_size += buf->sz;
}

static inline void
append_sendbuffer(sserver_t ss, struct socket *s, struct request_send * request, int n) {
	struct write_buffer *buf = append_sendbuffer_(ss, &s->high, request, SIZEOF_TCPBUFFER, n);
	s->wb_size += buf->sz;
}

static inline void
append_sendbuffer_low(sserver_t ss,struct socket *s, struct request_send * request) {
	struct write_buffer *buf = append_sendbuffer_(ss, &s->low, request, SIZEOF_TCPBUFFER, 0);
	s->wb_size += buf->sz;
}


/*
	When send a package , we can assign the priority : PRIORITY_HIGH or PRIORITY_LOW

	If socket buffer is empty, write to fd directly.
		If write a part, append the rest part to high list. (Even priority is PRIORITY_LOW)
	Else append package to high (PRIORITY_HIGH) or low (PRIORITY_LOW) list.
 */
static int
send_socket(sserver_t ss, struct request_send * request, smessage_t result, int priority, const uint8_t *udp_address) {
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
		fprintf(stderr, "socket-server: write to listen fd %d.\n", id);
		so.free_func(request->buffer);
		return -1;
	}
	if (send_buffer_empty(s) && s->type == SOCKET_TYPE_CONNECTED) {
		if (s->protocol == PROTOCOL_TCP) {
			int n = write(s->fd, so.buffer, so.sz);
			if (n<0) {
				switch (socket_errno) {
				case SOCKET_EINTR:
				case AGAIN_WOULDBLOCK:
					n = 0;
					break;
				default:
					fprintf(stderr, "socket-server: write to %d (fd=%d) error :%s.\n", id, s->fd, sys_strerror(socket_errno));
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
		} else {
			// udp
			if (udp_address == NULL) {
				udp_address = s->p.udp_address;
			}
			union sockaddr_all sa;
			socklen_t sasz = udp_socket_address(s, udp_address, &sa);
			int n = sendto(s->fd, so.buffer, so.sz, 0, &sa.s, sasz);
			if (n != so.sz) {
				append_sendbuffer_udp(ss,s,priority,request,udp_address);
			} else {
				so.free_func(request->buffer);
				return -1;
			}
		}
		sp_write(ss->event_fd, s->fd, s, true);
	} else {
		if (s->protocol == PROTOCOL_TCP) {
			if (priority == PRIORITY_LOW) {
				append_sendbuffer_low(ss, s, request);
			} else {
				append_sendbuffer(ss, s, request, 0);
			}
		} else {
			if (udp_address == NULL) {
				udp_address = s->p.udp_address;
			}
			append_sendbuffer_udp(ss,s,priority,request,udp_address);
		}
	}
	if (s->wb_size >= WARNING_SIZE && s->wb_size >= s->warn_size) {
		s->warn_size = s->warn_size == 0 ? WARNING_SIZE *2 : s->warn_size*2;
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
	int listen_fd = request->fd;
	struct socket *s = new_fd(ss, id, listen_fd, PROTOCOL_TCP, request->opaque, false);
	if (s == NULL) {
		goto _failed;
	}
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
close_socket(sserver_t ss, struct request_close *request, smessage_t result) {
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
		int type = send_buffer(ss,s,result);
		// type : -1 or SOCKET_WARNING or SOCKET_CLOSE, SOCKET_WARNING means send_buffer_empty
		if (type != -1 && type != SSERVER_WARNING)
			return type;
	}
	if (request->shutdown || send_buffer_empty(s)) {
		force_close(ss,s,result);
		result->id = id;
		result->opaque = request->opaque;
		return SSERVER_CLOSE;
	}
	s->type = SOCKET_TYPE_HALFCLOSE;

	return -1;
}

static int
bind_socket(sserver_t ss, struct request_bind *request, smessage_t result) {
	int id = request->id;
	result->id = id;
	result->opaque = request->opaque;
	result->ud = 0;
	struct socket *s = new_fd(ss, id, request->fd, PROTOCOL_TCP, request->opaque, true);
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
start_socket(sserver_t ss, struct request_start *request, smessage_t result) {
	int id = request->id;
	result->id = id;
	result->opaque = request->opaque;
	result->ud = 0;
	result->data = NULL;
	struct socket *s = &ss->slot[HASH_ID(id)];
	if (s->type == SOCKET_TYPE_INVALID || s->id !=id) {
		result->data = "invalid socket";
		return SSERVER_ERR;
	}
	if (s->type == SOCKET_TYPE_PACCEPT || s->type == SOCKET_TYPE_PLISTEN) {
		if (sp_add(ss->event_fd, s->fd, s)) {
			force_close(ss, s, result);
			result->data = sys_strerror(socket_errno);
			return SSERVER_ERR;
		}
		s->type = (s->type == SOCKET_TYPE_PACCEPT) ? SOCKET_TYPE_CONNECTED : SOCKET_TYPE_LISTEN;
		s->opaque = request->opaque;
		result->data = "start";
		return SSERVER_OPEN;
	} else if (s->type == SOCKET_TYPE_CONNECTED) {
		// todo: maybe we should send a message SOCKET_TRANSFER to s->opaque
		s->opaque = request->opaque;
		result->data = "transfer";
		return SSERVER_OPEN;
	}
	// if s->type == SOCKET_TYPE_HALFCLOSE , SOCKET_CLOSE message will send later
	return -1;
}

static void
setopt_socket(sserver_t ss, struct request_setopt *request) {
	int id = request->id;
	struct socket *s = &ss->slot[HASH_ID(id)];
	if (s->type == SOCKET_TYPE_INVALID || s->id !=id) {
		return;
	}
	int v = request->value;
	setsockopt(s->fd, IPPROTO_TCP, request->what, (void *)&v, sizeof(v));
}

static void
block_readpipe(int pipefd, void *buffer, int sz) {
	for (;;) {
		int n = read(pipefd, buffer, sz);
		if (n<0) {
			if (socket_errno == SOCKET_EINTR)
				continue;
			fprintf(stderr, "socket-server : read pipe error %s.\n", sys_strerror(socket_errno));
			return;
		}
		// must atomic read from a pipe
		assert(n == sz);
		return;
	}
}

static int
has_cmd(sserver_t ss) {
	struct timeval tv = {0,0};
	int retval;

	FD_SET(ss->recvctrl_fd, &ss->rfds);

	retval = select(ss->recvctrl_fd+1, &ss->rfds, NULL, NULL, &tv);
	if (retval == 1) {
		return 1;
	}
	return 0;
}

static void
add_udp_socket(sserver_t ss, struct request_udp *udp) {
	int id = udp->id;
	int protocol;
	if (udp->family == AF_INET6) {
		protocol = PROTOCOL_UDPv6;
	} else {
		protocol = PROTOCOL_UDP;
	}
	struct socket *ns = new_fd(ss, id, udp->fd, protocol, udp->opaque, true);
	if (ns == NULL) {
		socket_close(udp->fd);
		ss->slot[HASH_ID(id)].type = SOCKET_TYPE_INVALID;
		return;
	}
	ns->type = SOCKET_TYPE_CONNECTED;
	memset(ns->p.udp_address, 0, sizeof(ns->p.udp_address));
}

static int
set_udp_address(sserver_t ss, struct request_setudp *request, smessage_t result) {
	int id = request->id;
	struct socket *s = &ss->slot[HASH_ID(id)];
	if (s->type == SOCKET_TYPE_INVALID || s->id !=id) {
		return -1;
	}
	int type = request->address[0];
	if (type != s->protocol) {
		// protocol mismatch
		result->opaque = s->opaque;
		result->id = s->id;
		result->ud = 0;
		result->data = "protocol mismatch";

		return SSERVER_ERR;
	}
	if (type == PROTOCOL_UDP) {
		memcpy(s->p.udp_address, request->address, 1+2+4);	// 1 type, 2 port, 4 ipv4
	} else {
		memcpy(s->p.udp_address, request->address, 1+2+16);	// 1 type, 2 port, 16 ipv6
	}
	return -1;
}

// return type
static int
ctrl_cmd(sserver_t ss, smessage_t result) {
	int fd = ss->recvctrl_fd;
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
		return start_socket(ss,(struct request_start *)buffer, result);
	case 'B':
		return bind_socket(ss,(struct request_bind *)buffer, result);
	case 'L':
		return listen_socket(ss,(struct request_listen *)buffer, result);
	case 'K':
		return close_socket(ss,(struct request_close *)buffer, result);
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
	default:
		fprintf(stderr, "socket-server: Unknown ctrl %c.\n",type);
		return -1;
	};

	return -1;
}

// return -1 (ignore) when error
static int
forward_message_tcp(sserver_t ss, struct socket *s, smessage_t  result) {
	int sz = s->p.size;
	char * buffer = MALLOC(sz);
	int n = read(s->fd, buffer, sz);
	if (n<0) {
		FREE(buffer);
		switch(socket_errno) {
		case SOCKET_EINTR:
			break;
		case AGAIN_WOULDBLOCK:
			fprintf(stderr, "socket-server: EAGAIN capture.\n");
			break;
		default:
			// close when error
			force_close(ss, s, result);
			result->data = sys_strerror(socket_errno);
			return SSERVER_ERR;
		}
		return -1;
	}
	if (n==0) {
		FREE(buffer);
		force_close(ss, s, result);
		return SSERVER_CLOSE;
	}

	if (s->type == SOCKET_TYPE_HALFCLOSE) {
		// discard recv data
		FREE(buffer);
		return -1;
	}

	if (n == sz) {
		s->p.size *= 2;
	} else if (sz > MIN_READ_BUFFER && n*2 < sz) {
		s->p.size /= 2;
	}

	result->opaque = s->opaque;
	result->id = s->id;
	result->ud = n;
	result->data = buffer;
	return SSERVER_DATA;
}

static int
gen_udp_address(int protocol, union sockaddr_all *sa, uint8_t * udp_address) {
	int addrsz = 1;
	udp_address[0] = (uint8_t)protocol;
	if (protocol == PROTOCOL_UDP) {
		memcpy(udp_address+addrsz, &sa->v4.sin_port, sizeof(sa->v4.sin_port));
		addrsz += sizeof(sa->v4.sin_port);
		memcpy(udp_address+addrsz, &sa->v4.sin_addr, sizeof(sa->v4.sin_addr));
		addrsz += sizeof(sa->v4.sin_addr);
	} else {
		memcpy(udp_address+addrsz, &sa->v6.sin6_port, sizeof(sa->v6.sin6_port));
		addrsz += sizeof(sa->v6.sin6_port);
		memcpy(udp_address+addrsz, &sa->v6.sin6_addr, sizeof(sa->v6.sin6_addr));
		addrsz += sizeof(sa->v6.sin6_addr);
	}
	return addrsz;
}

static int
forward_message_udp(sserver_t ss, struct socket *s, smessage_t  result) {
	union sockaddr_all sa;
	socklen_t slen = sizeof(sa);
	int n = recvfrom(s->fd, ss->udpbuffer,MAX_UDP_PACKAGE,0,&sa.s,&slen);
	if (n<0) {
		switch(socket_errno) {
		case SOCKET_EINTR:
		case AGAIN_WOULDBLOCK:
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
		if (s->protocol != PROTOCOL_UDP)
			return -1;
		data = MALLOC(n + 1 + 2 + 4);
		gen_udp_address(PROTOCOL_UDP, &sa, data + n);
	} else {
		if (s->protocol != PROTOCOL_UDPv6)
			return -1;
		data = MALLOC(n + 1 + 2 + 16);
		gen_udp_address(PROTOCOL_UDPv6, &sa, data + n);
	}
	memcpy(data, ss->udpbuffer, n);

	result->opaque = s->opaque;
	result->id = s->id;
	result->ud = n;
	result->data = (char *)data;

	return SSERVER_UDP;
}

static int
report_connect(sserver_t ss, struct socket *s, smessage_t result) {
	int error;
	socklen_t len = sizeof(error);  
	int code = getsockopt(s->fd, SOL_SOCKET, SO_ERROR, (void *)&error, &len);  
	if (code < 0 || error) {  
		force_close(ss,s, result);
		if (code >= 0)
			result->data = sys_strerror(error);
		else
			result->data = sys_strerror(errno);
		return SSERVER_ERR;
	} else {
		s->type = SOCKET_TYPE_CONNECTED;
		result->opaque = s->opaque;
		result->id = s->id;
		result->ud = 0;
		if (send_buffer_empty(s)) {
			sp_write(ss->event_fd, s->fd, s, false);
		}
		union sockaddr_all u;
		socklen_t slen = sizeof(u);
		if (getpeername(s->fd, &u.s, &slen) == 0) {
			void * sin_addr = (u.s.sa_family == AF_INET) ? (void*)&u.v4.sin_addr : (void *)&u.v6.sin6_addr;
			if (inet_ntop(u.s.sa_family, sin_addr, ss->buffer, sizeof(ss->buffer))) {
				result->data = ss->buffer;
				return SSERVER_OPEN;
			}
		}
		result->data = NULL;
		return SSERVER_OPEN;
	}
}

// return 0 when failed, or -1 when file limit
static int
report_accept(sserver_t ss, struct socket *s, smessage_t result) {
	union sockaddr_all u;
	socklen_t len = sizeof(u);
	int client_fd = accept(s->fd, &u.s, &len);
	if (client_fd < 0) {
		if (socket_errno == EMFILE || socket_errno == ENFILE) {
			result->opaque = s->opaque;
			result->id = s->id;
			result->ud = 0;
			result->data = sys_strerror(socket_errno);
			return -1;
		} else {
			return 0;
		}
	}
	int id = reserve_id(ss);
	if (id < 0) {
		socket_close(client_fd);
		return 0;
	}
	socket_keepalive(client_fd);
	socket_set_nonblock(client_fd);
	struct socket *ns = new_fd(ss, id, client_fd, PROTOCOL_TCP, s->opaque, false);
	if (ns == NULL) {
		socket_close(client_fd);
		return 0;
	}
	ns->type = SOCKET_TYPE_PACCEPT;
	result->opaque = s->opaque;
	result->id = s->id;
	result->ud = id;
	result->data = NULL;

	void * sin_addr = (u.s.sa_family == AF_INET) ? (void*)&u.v4.sin_addr : (void *)&u.v6.sin6_addr;
	int sin_port = ntohs((u.s.sa_family == AF_INET) ? u.v4.sin_port : u.v6.sin6_port);
	char tmp[INET6_ADDRSTRLEN];
	if (inet_ntop(u.s.sa_family, sin_addr, tmp, sizeof(tmp))) {
		snprintf(ss->buffer, sizeof(ss->buffer), "%s:%d", tmp, sin_port);
		result->data = ss->buffer;
	}

	return 1;
}

static inline void 
clear_closed_event(sserver_t ss, smessage_t  result, int type) {
	if (type == SSERVER_CLOSE || type == SSERVER_ERR) {
		int id = result->id;
		int i;
		for (i=ss->event_index; i<ss->event_n; i++) {
			struct event *e = &ss->ev[i];
			struct socket *s = e->s;
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
sserver_poll(sserver_t ss, smessage_t  result, int * more) {
	for (;;) {
		if (ss->checkctrl) {
			if (has_cmd(ss)) {
				int type = ctrl_cmd(ss, result);
				if (type != -1) {
					clear_closed_event(ss, result, type);
					return type;
				} else
					continue;
			} else {
				ss->checkctrl = 0;
			}
		}
		if (ss->event_index == ss->event_n) {
			ss->event_n = sp_wait(ss->event_fd, ss->ev, MAX_EVENT);
			ss->checkctrl = 1;
			if (more) {
				*more = 0;
			}
			ss->event_index = 0;
			if (ss->event_n <= 0) {
				ss->event_n = 0;
				if (socket_errno == SOCKET_EINTR) {
					continue;
				}
				return -1;
			}
		}
		struct event *e = &ss->ev[ss->event_index++];
		struct socket *s = e->s;
		if (s == NULL) {
			// dispatch pipe message at beginning
			continue;
		}
		switch (s->type) {
		case SOCKET_TYPE_CONNECTING:
			return report_connect(ss, s, result);
		case SOCKET_TYPE_LISTEN: {
			int ok = report_accept(ss, s, result);
			if (ok > 0) {
				return SSERVER_ACCEPT;
			} if (ok < 0 ) {
				return SSERVER_ERR;
			}
			// when ok == 0, retry
			break;
		}
		case SOCKET_TYPE_INVALID:
			fprintf(stderr, "socket-server: invalid socket\n");
			break;
		default:
			if (e->read) {
				int type;
				if (s->protocol == PROTOCOL_TCP) {
					type = forward_message_tcp(ss, s, result);
				} else {
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
			if (e->error) {
				// close when error
				int error;
				socklen_t len = sizeof(error);  
				int code = getsockopt(s->fd, SOL_SOCKET, SO_ERROR, (void *)&error, &len);  
				if (code < 0) {
					result->data = sys_strerror(socket_errno);
				} else if (error != 0) {
					result->data = strerror(error);
				} else {
					result->data = "Unknown error";
				}
				force_close(ss, s, result);
				return SSERVER_ERR;
			}
			break;
		}
	}
}

static void
send_request(sserver_t ss, struct request_package *request, char type, int len) {
	request->header[6] = (uint8_t)type;
	request->header[7] = (uint8_t)len;
	for (;;) {
		int n = write(ss->sendctrl_fd, &request->header[6], len + 2);
		if (n<0) {
			if (socket_errno != SOCKET_EINTR) {
				fprintf(stderr, "socket-server : send ctrl command error %s.\n", sys_strerror(socket_errno));
			}
			continue;
		}
		assert(n == len+2);
		return;
	}
}

static int
open_request(sserver_t ss, struct request_package *req, uintptr_t opaque, const char *addr, int port) {
	int len = strlen(addr);
	if (len + sizeof(req->u.open) >= 256) {
		fprintf(stderr, "socket-server : Invalid addr %s.\n",addr);
		return -1;
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
sserver_connect(sserver_t ss, uintptr_t opaque, const char * addr, int port) {
	struct request_package request;
	int len = open_request(ss, &request, opaque, addr, port);
	if (len < 0)
		return -1;
	send_request(ss, &request, 'O', sizeof(request.u.open) + len);
	return request.u.open.id;
}

static void
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

void
sserver_exit(sserver_t ss) {
	struct request_package request;
	send_request(ss, &request, 'X', 0);
}

void
sserver_close(sserver_t ss, uintptr_t opaque, int id) {
	struct request_package request;
	request.u.close.id = id;
	request.u.close.shutdown = 0;
	request.u.close.opaque = opaque;
	send_request(ss, &request, 'K', sizeof(request.u.close));
}


void
sserver_shutdown(sserver_t ss, uintptr_t opaque, int id) {
	struct request_package request;
	request.u.close.id = id;
	request.u.close.shutdown = 1;
	request.u.close.opaque = opaque;
	send_request(ss, &request, 'K', sizeof(request.u.close));
}

// return -1 means failed
// or return AF_INET or AF_INET6
static int
do_bind(const char *host, int port, int protocol, int *family) {
	int fd;
	int status;
	int reuse = 1;
	struct addrinfo ai_hints;
	struct addrinfo *ai_list = NULL;
	char portstr[16];
	if (host == NULL || host[0] == 0) {
		host = "0.0.0.0";	// INADDR_ANY
	}
	sprintf(portstr, "%d", port);
	memset( &ai_hints, 0, sizeof( ai_hints ) );
	ai_hints.ai_family = AF_UNSPEC;
	if (protocol == IPPROTO_TCP) {
		ai_hints.ai_socktype = SOCK_STREAM;
	} else {
		assert(protocol == IPPROTO_UDP);
		ai_hints.ai_socktype = SOCK_DGRAM;
	}
	ai_hints.ai_protocol = protocol;

	status = getaddrinfo( host, portstr, &ai_hints, &ai_list );
	if ( status != 0 ) {
		return -1;
	}
	*family = ai_list->ai_family;
	fd = socket(*family, ai_list->ai_socktype, 0);
	if (fd < 0) {
		goto _failed_fd;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(int))==-1) {
		goto _failed;
	}
	status = bind(fd, (struct sockaddr *)ai_list->ai_addr, ai_list->ai_addrlen);
	if (status != 0)
		goto _failed;

	freeaddrinfo( ai_list );
	return fd;
_failed:
	socket_close(fd);
_failed_fd:
	freeaddrinfo( ai_list );
	return -1;
}

static int
do_listen(const char * host, int port, int backlog) {
	int family = 0;
	int listen_fd = do_bind(host, port, IPPROTO_TCP, &family);
	if (listen_fd < 0) {
		return -1;
	}
	if (listen(listen_fd, backlog) == -1) {
		socket_close(listen_fd);
		return -1;
	}
	return listen_fd;
}

int 
sserver_listen(sserver_t ss, uintptr_t opaque, const char * addr, int port, int backlog) {
	int fd = do_listen(addr, port, backlog);
	if (fd < 0) {
		return -1;
	}
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
sserver_bind(sserver_t ss, uintptr_t opaque, int fd) {
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

void 
sserver_start(sserver_t ss, uintptr_t opaque, int id) {
	struct request_package request;
	request.u.start.id = id;
	request.u.start.opaque = opaque;
	send_request(ss, &request, 'S', sizeof(request.u.start));
}

void
sserver_nodelay(sserver_t ss, int id) {
	struct request_package request;
	request.u.setopt.id = id;
	request.u.setopt.what = TCP_NODELAY;
	request.u.setopt.value = 1;
	send_request(ss, &request, 'T', sizeof(request.u.setopt));
}

void 
sserver_userobject(sserver_t ss, struct sinterface * soi) {
	ss->soi = *soi;
}

// UDP

int 
sserver_udp(sserver_t ss, uintptr_t opaque, const char * addr, int port) {
	int fd;
	int family;
	if (port != 0 || addr != NULL) {
		// bind
		fd = do_bind(addr, port, IPPROTO_UDP, &family);
		if (fd < 0) {
			return -1;
		}
	} else {
		family = AF_INET;
		fd = socket(family, SOCK_DGRAM, 0);
		if (fd < 0) {
			return -1;
		}
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
sserver_udp_send(sserver_t ss, int id, const saddrudp_t addr, const void *buffer, int sz) {
	struct socket * s = &ss->slot[HASH_ID(id)];
	if (s->id != id || s->type == SOCKET_TYPE_INVALID) {
		free_buffer(ss, buffer, sz);
		return -1;
	}

	struct request_package request;
	request.u.send_udp.send.id = id;
	request.u.send_udp.send.sz = sz;
	request.u.send_udp.send.buffer = (char *)buffer;

	const uint8_t *udp_address = (const uint8_t *)addr;
	int addrsz;
	switch (udp_address[0]) {
	case PROTOCOL_UDP:
		addrsz = 1+2+4;		// 1 type, 2 port, 4 ipv4
		break;
	case PROTOCOL_UDPv6:
		addrsz = 1+2+16;	// 1 type, 2 port, 16 ipv6
		break;
	default:
		free_buffer(ss, buffer, sz);
		return -1;
	}

	memcpy(request.u.send_udp.address, udp_address, addrsz);	

	send_request(ss, &request, 'A', sizeof(request.u.send_udp.send)+addrsz);
	return 0;
}

int
sserver_udp_connect(sserver_t ss, int id, const char * addr, int port) {
	int status;
	struct addrinfo ai_hints;
	struct addrinfo *ai_list = NULL;
	char portstr[16];
	sprintf(portstr, "%d", port);
	memset( &ai_hints, 0, sizeof( ai_hints ) );
	ai_hints.ai_family = AF_UNSPEC;
	ai_hints.ai_socktype = SOCK_DGRAM;
	ai_hints.ai_protocol = IPPROTO_UDP;

	status = getaddrinfo(addr, portstr, &ai_hints, &ai_list );
	if ( status != 0 ) {
		return -1;
	}
	struct request_package request;
	request.u.set_udp.id = id;
	int protocol;

	if (ai_list->ai_family == AF_INET) {
		protocol = PROTOCOL_UDP;
	} else if (ai_list->ai_family == AF_INET6) {
		protocol = PROTOCOL_UDPv6;
	} else {
		freeaddrinfo( ai_list );
		return -1;
	}

	int addrsz = gen_udp_address(protocol, (union sockaddr_all *)ai_list->ai_addr, request.u.set_udp.address);

	freeaddrinfo( ai_list );

	send_request(ss, &request, 'C', sizeof(request.u.set_udp) - sizeof(request.u.set_udp.address) +addrsz);

	return 0;
}

const saddrudp_t
sserver_udp_address(sserver_t ss, smessage_t msg, int *addrsz) {
	uint8_t * address = (uint8_t *)(msg->data + msg->ud);
	int type = address[0];
	switch(type) {
	case PROTOCOL_UDP:
		*addrsz = 1+2+4;
		break;
	case PROTOCOL_UDPv6:
		*addrsz = 1+2+16;
		break;
	default:
		return NULL;
	}
	return address;
}