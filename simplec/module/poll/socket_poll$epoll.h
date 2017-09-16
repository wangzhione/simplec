#if defined(__linux__)

#include <socket_poll.h>
#include <sys/epoll.h>

//
// sp_create	- 创建一个poll模型
// sp_invalid	- 检查这个poll模型是否有问题, true表示有问题
// sp_delete	- 销毁这个poll模型
//
inline poll_t 
sp_create(void) {
    return epoll_create1(0);
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
inline bool 
sp_add(poll_t sp, socket_t sock, void * ud) {
    struct epoll_event ev = { EPOLLIN };
    ev.data.ptr = ud;
    return epoll_ctl(sp, EPOLL_CTL_ADD, sock, &ev) < 0;
}

inline void 
sp_del(poll_t sp, socket_t sock) {
    epoll_ctl(sp, EPOLL_CTL_DEL, sock, NULL);
}

inline void 
sp_write(poll_t sp, socket_t sock, void * ud, bool enable) {
    struct epoll_event ev;
    ev.events = EPOLLIN | (enable ? EPOLLOUT : 0);
    ev.data.ptr = ud;
    epoll_ctl(sp, EPOLL_CTL_MOD, sock, &ev);
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
    struct epoll_event ev[max];
    int i, n = epoll_wait(sp, ev, max, -1);

    for (i = 0; i < n; ++i) {
	    uint32_t flag = ev[i].events;
	    e[i].s = ev[i].data.ptr;
	    e[i].write = flag & EPOLLOUT;
	    e[i].read = flag & (EPOLLIN | EPOLLHUP);
	    e[i].error = flag & EPOLLERR;
    }

    return n;
}

#endif