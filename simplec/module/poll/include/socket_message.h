#ifndef _H_SIMPLEC_SOCKET_MESSAGE
#define _H_SIMPLEC_SOCKET_MESSAGE

#include <socket_server.h>

struct sockmsg {
	struct smessage rms;
	uint8_t type;	// see SSERVER_*
};

extern void sockmsg_init(void);
extern void sockmsg_free(void);
extern void sockmsg_exit(void);
extern  int sockmsg_poll(void);

extern int sockmsg_send(int id, void * buffer, int sz);
extern int sockmsg_send_lowpriority(int id, void * buffer, int sz);

extern int sockmsg_listen(uintptr_t opaque, const char * host, uint16_t port);
extern int sockmsg_connect(uintptr_t opaque, const char * host, uint16_t port);
extern int sockmsg_bind(uintptr_t opaque, socket_t fd);

extern void sockmsg_close(uintptr_t opaque, int id);
extern void sockmsg_shutdown(uintptr_t opaque, int id);
extern void sockmsg_start(uintptr_t opaque, int id);
extern void sockmsg_nodelay(int id);

extern int sockmsg_udp(uintptr_t opaque, const char * addr, uint16_t port);
extern int sockmsg_udp_connect(int id, const char * addr, uint16_t port);
extern int sockmsg_udp_send(uintptr_t opaque, int id, const char * address, const void * buffer, int sz);
extern const char * sockmsg_udp_address(struct sockmsg * msg, int * addrsz);

#endif//_H_SIMPLEC_SOCKET_MESSAGE