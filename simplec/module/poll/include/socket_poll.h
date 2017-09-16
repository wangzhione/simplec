#ifndef _H_SIMPLEC_SOCKET_POLL
#define _H_SIMPLEC_SOCKET_POLL

#include <scsocket.h>

#ifdef _MSC_VER
typedef struct select_poll * poll_t;
#else
typedef int poll_t;		// * 也就大师背后, 拾人牙慧
#endif

struct event {
    void * s;
    bool read;
    bool write;
    bool error;
};

//
// sp_create	- 创建一个poll模型
// sp_invalid	- 检查这个poll模型是否有问题, true表示有问题
// sp_delete	- 销毁这个poll模型
//
extern poll_t sp_create(void);
extern bool sp_invalid(poll_t sp);
extern void sp_delete(poll_t sp);

//
// sp_add		- 添加监测的socket, 并设置读模式, 失败返回true
// sp_del		- 删除监测的socket
// sp_write		- 修改当前socket, 并设置为写模式
//
extern bool sp_add(poll_t sp, socket_t sock, void * ud);
extern void sp_del(poll_t sp, socket_t sock);
extern void sp_write(poll_t sp, socket_t sock, void * ud, bool enable);

//
// sp_wait		- poll 的 wait函数, 等待别人自投罗网
// sp		: poll 模型
// e		: 返回的操作事件集
// max		: e 的最大长度
// return	: 返回待操作事件长度, <= 0 表示失败
//
extern int sp_wait(poll_t sp, struct event e[], int max);

#endif // !_H_SIMPLEC_SOCKET_POLL
