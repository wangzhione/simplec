#define FD_SETSIZE	(1024)
#include <scsocket.h>

struct sevent {
	void * ud;
	bool write;
	socket_t fd;
};

struct sselect {
	fd_set rd;
	fd_set wt;
	uint16_t n;
	struct sevent evs[FD_SETSIZE];
};

typedef struct sselect * poll_fd;

struct event {
	void * s;
	bool read;
	bool write;
	bool error;
};

static bool sp_invalid(poll_fd fd) {
	return NULL == fd;
}

static poll_fd sp_create(void) {
	return calloc(1, sizeof(struct sselect));
}

static void sp_release(poll_fd fd) {
	free(fd);
}

static int sp_add(poll_fd fd, socket_t sock, void * ud) {
	struct sevent * sev, * eev;
	if (fd->n >= FD_SETSIZE)
		return 1;

	sev = fd->evs;
	eev = fd->evs + fd->n;
	while (sev < eev) {
		if (sev->fd == sock)
			break;
		++sev;
	}

	if (sev == eev) {
		++fd->n;
		sev->fd = sock;
	}

	sev->ud = ud;
	sev->write = false;

	return 0;
}

static void sp_del(poll_fd fd, socket_t sock) {
	struct sevent * sev = fd->evs, * eev = fd->evs + fd->n;
	while (sev < eev) {
		if (sev->fd == sock) {
			--fd->n;
			while (++sev < eev)
				sev[-1] = sev[0];
			break;
		}
		++sev;
	}
}

static void sp_write(poll_fd fd, socket_t sock, void * ud, bool enable) {
	struct sevent * sev = fd->evs, * eev = fd->evs + fd->n;
	while (sev < eev) {
		if (sev->fd == sock) {
			sev->ud = ud;
			sev->write = enable;
			break;
		}
		++sev;
	}
}

static int sp_wait(poll_fd sp, struct event * e, int max) {
	int i, n;
	FD_ZERO(&sp->rd);
	FD_ZERO(&sp->wt);

	for (i = 0; i < sp->n; ++i) {
		struct sevent * sev = sp->evs + i;
		FD_SET(sev->fd, &sp->rd);
		if (sev->write)
			FD_SET(sev->fd, &sp->wt);
	}

	n = select(0, &sp->rd, &sp->wt, NULL, NULL);
	if (n <= 0)
		return n;

	int retn = 0;
	for (i = 0; i < sp->n && retn < max && retn < n; ++i) {
		struct sevent * sev = sp->evs + i;
		e[retn].read = FD_ISSET(sev->fd, &sp->rd);
		e[retn].write = sev->write && FD_ISSET(sev->fd, &sp->wt);
		if (e[retn].read || e[retn].write) {
			++retn;
			e[retn].s = sev->ud;
			e[retn].error = false;
		}
	}
	return retn;
}

static void sp_nonblocking(socket_t sock) {
	socket_set_nonblock(sock);
}

#define _STR_IPS	"127.0.0.1"
#define _INT_PORT	(8088)

//
// 参照云风的思路自己设计 window 部分代码
// https://github.com/cloudwu/skynet/blob/master/skynet-src/socket_poll.h
//
void test_winselect(void) {
	int n;
	poll_fd poll;
	socket_t sock;
	struct event evs[FD_SETSIZE];

	// 开始构建一个socket
	sock = socket_tcp(_STR_IPS, _INT_PORT);
	if (sock == INVALID_SOCKET)
		return;

	poll = sp_create();
	assert(!sp_invalid(poll));

	if (sp_add(poll, sock, NULL)) {
		CERR("sp_add sock = is error!");
		goto __close;
	}

	// 开始等待数据
	n = sp_wait(poll, evs, LEN(evs));
	
	printf("sp_wait n = %d. 一切都是那么意外!", n);

__close:
	socket_close(sock);
	sp_release (poll);
}