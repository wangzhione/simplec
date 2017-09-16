#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined (__NetBSD__)

#include <socket_poll.h>
#include <sys/event.h>

//
// sp_create	- 创建一个poll模型
// sp_invalid	- 检查这个poll模型是否有问题, true表示有问题
// sp_delete	- 销毁这个poll模型
//
inline poll_t
sp_create(void) {
    return kqueue();
}

inline bool
sp_invalid(poll_t sp) {
    return 0 > sp;
}

inline void
sp_delete(poll_t sp) {
    close(sp);
}

//
// sp_add		- 添加监测的socket, 并设置读模式, 失败返回true
// sp_del		- 删除监测的socket
// sp_write		- 修改当前socket, 并设置为写模式
//
bool
sp_add(poll_t sp, socket_t sock, void * ud) {
    struct kevent ke;
    EV_SET(&ke, sock, EVFILT_READ, EV_ADD, 0, 0, ud);
    if (kevent(sp, &ke, 1, NULL, 0, NULL) < 0 || ke.flags & EV_ERROR)
	    return true;

    EV_SET(&ke, sock, EVFILT_WRITE, EV_ADD, 0, 0, ud);
    if (kevent(sp, &ke, 1, NULL, 0, NULL) < 0 || ke.flags & EV_ERROR) {
	    EV_SET(&ke, sock, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	    kevent(sp, &ke, 1, NULL, 0, NULL);
	    return true;
    }
    EV_SET(&ke, sock, EVFILT_WRITE, EV_DISABLE, 0, 0, ud);
    if (kevent(sp, &ke, 1, NULL, 0, NULL) < 0 || ke.flags & EV_ERROR) {
	    sp_del(sp, sock);
	    return true;
    }
    return false;
}

inline void
sp_del(poll_t sp, socket_t sock) {
	struct kevent ke;
	EV_SET(&ke, sock, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	kevent(sp, &ke, 1, NULL, 0, NULL);
	EV_SET(&ke, sock, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
	kevent(sp, &ke, 1, NULL, 0, NULL);
}

void
sp_write(poll_t sp, socket_t sock, void * ud, bool enable) {
    struct kevent ke;
    EV_SET(&ke, sock, EVFILT_WRITE, enable ? EV_ENABLE : EV_DISABLE, 0, 0, ud);
    if (kevent(sp, &ke, 1, NULL, 0, NULL) < 0 || ke.flags & EV_ERROR) {
	    // todo: check error
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
    struct kevent ev[max];
    int i, n = kevent(sp, NULL, 0, ev, max, NULL);

    for (i = 0; i < n; ++i) {
	    e[i].s = ev[i].udata;
	    e[i].write = ev[i].filter == EVFILT_WRITE;
	    e[i].read = ev[i].filter == EVFILT_READ;
	    e[i].error = false;	// kevent has not error event
    }

    return n;
}

#endif