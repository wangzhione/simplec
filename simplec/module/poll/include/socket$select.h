#if defined(_MSC_VER)

#include <socket_poll.h>

struct sevent {
	void * ud;
	bool write;
	socket_t fd;
};

struct select_poll {
	fd_set rd;
	fd_set wt;
	uint16_t n;
	struct sevent evs[FD_SETSIZE];
};

//
// sp_create	- 创建一个poll模型
// sp_invalid	- 检查这个poll模型是否有问题, true表示有问题
// sp_delete	- 销毁这个poll模型
//
inline poll_t 
sp_create(void) {
	return calloc(1, sizeof(struct select_poll));
}

inline bool 
sp_invalid(poll_t sp) {
	return NULL == sp;
}

inline void 
sp_delete(poll_t sp) {
	free(sp);
}

//
// sp_add		- 添加监测的socket, 并设置读模式, 失败返回true
// sp_del		- 删除监测的socket
// sp_write		- 修改当前socket, 并设置为写模式
//
bool 
sp_add(poll_t sp, socket_t sock, void * ud) {
	struct sevent * sev, * eev;
	if (sp->n >= FD_SETSIZE)
		return true;

	sev = sp->evs;
	eev = sp->evs + sp->n;
	while (sev < eev) {
		if (sev->fd == sock)
			break;
		++sev;
	}
	if (sev == eev) {
		++sp->n;
		sev->fd = sock;
	}

	sev->ud = ud;
	sev->write = false;
	return false;
}

void 
sp_del(poll_t sp, socket_t sock) {
	struct sevent * sev = sp->evs, * eev = sp->evs + sp->n;
	while (sev < eev) {
		if (sev->fd == sock) {
			--sp->n;
			while (++sev < eev)
				sev[-1] = sev[0];
			break;
		}
		++sev;
	}
}

void 
sp_write(poll_t sp, socket_t sock, void * ud, bool enable) {
	struct sevent * sev = sp->evs, * eev = sp->evs + sp->n;
	while (sev < eev) {
		if (sev->fd == sock) {
			sev->ud = ud;
			sev->write = enable;
			break;
		}
		++sev;
	}
}

//
// sp_wait		- poll 的 wait函数, 等待别人自投罗网
// sp		: poll 模型
// e		: 返回的操作事件集
// max		: e 的最大长度
// return	: 返回待操作事件长度, <= 0 表示失败
//
int 
sp_wait(poll_t sp, struct event e[], int max) {
	int i, n, retn;
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
		RETURN(n, "select n = %d", n);

	for (retn = i = 0; i < sp->n && retn < max && retn < n; ++i) {
		struct sevent * sev = sp->evs + i;
		e[retn].read = FD_ISSET(sev->fd, &sp->rd);
		e[retn].write = sev->write && FD_ISSET(sev->fd, &sp->wt);
		if (e[retn].read || e[retn].write) {
			e[retn].s = sev->ud;
			e[retn].error = false;
			++retn;
		}
	}
	return retn;
}

#endif