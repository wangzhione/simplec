#ifdef _MSC_VER

#include <socket_poll.h>

//
// sp_create	- 创建一个poll模型
// sp_invalid	- 检查这个poll模型是否有问题, true表示有问题
// sp_delete	- 销毁这个poll模型
//
poll_t 
sp_create(void) {
	return NULL;
}

bool 
sp_invalid(poll_t sp) {
	return true;
}

void 
sp_delete(poll_t sp) {

}

//
// sp_add		- 添加监测的socket, 并设置读模式, 成功返回true
// sp_del		- 删除监测的socket
// sp_write		- 修改当前socket, 并设置为写模式
//
bool 
sp_add(poll_t sp, socket_t sock, void * ud) {
	return false;
}

void 
sp_del(poll_t sp, socket_t sock) {

}

void 
sp_write(poll_t sp, socket_t sock, void * ud, bool enable) {

}

//
// sp_wait		- poll 的 wait函数, 等待别人自投罗网
// sp		: poll 模型
// e		: 返回的操作事件集
// max		: e 的最大长度
// return	: 返回待操作事件长度, <= 0 表示失败
//
int 
sp_wait(poll_t sp, struct event e, size_t max) {

	return Error_Base;
}

#endif