#ifndef _H_SIMPLEC_SOCKET_MSG
#define _H_SIMPLEC_SOCKET_MSG

#include <socket_server.h>

#define SERVER_DATA		(1)
#define SERVER_CONNECT	(2)
#define SERVER_CLOSE	(3)
#define SERVER_ACCEPT	(4)
#define SERVER_ERROR	(5)
#define SERVER_UDP		(6)
#define SERVER_WARNING	(7)

struct smsg {
	uint8_t type;
	int id;
	int ud; // for accept, ud is new connection id ; for data, ud is size of data 
	char * data;
};

extern void server_init(void);
extern void server_free(void);
extern void server_exit(void);
extern  int server_poll(void);

extern int server_send(int id, void * buffer, int sz);
extern int server_send_lowpriority(int id, void * buffer, int sz);

extern int server_listen(uintptr_t opaque, const char * host, uint16_t port);
extern int server_connect(uintptr_t opaque, const char * host, uint16_t port);
extern int server_bind(uintptr_t opaque, socket_t fd);

extern void server_close(uintptr_t opaque, int id);
extern void server_shutdown(uintptr_t opaque, int id);
extern void server_start(uintptr_t opaque, int id);
extern void server_nodelay(int id);

extern int server_udp(uintptr_t opaque, const char * addr, uint16_t port);
extern int server_udp_connect(int id, const char * addr, uint16_t port);
extern int server_udp_send(uintptr_t opaque, int id, const char * address, const void * buffer, int sz);
extern const char * server_udp_address(struct smsg * msg, int * addrsz);

#endif//_H_SIMPLEC_SOCKET_MSG