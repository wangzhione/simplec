#ifndef _H_SIMPLEC_SOCKET_SERVER
#define _H_SIMPLEC_SOCKET_SERVER

#include <scpipe.h>
#include <socket_poll.h>

#define SSERVER_DATA			(0)
#define SSERVER_CLOSE			(1)
#define SSERVER_OPEN			(2)
#define SSERVER_ACCEPT			(3)
#define SSERVER_ERR				(4)
#define SSERVER_EXIT			(5)
#define SSERVER_UDP				(6)
#define SSERVER_WARNING			(7)

typedef struct sserver * sserver_t;

struct smessage {
	int id;
	uintptr_t opaque;
	// for accept, ud is new connection id ; for data, ud is size of data 
	int ud;
	char * data;
};

typedef struct smessage * smessage_t;

extern sserver_t sserver_create(void);
extern void sserver_delete(sserver_t ss);
extern int sserver_poll(sserver_t ss, smessage_t result, int * more);

extern void sserver_exit(sserver_t ss);
extern void sserver_close(sserver_t ss, uintptr_t opaque, int id);
extern void sserver_shutdown(sserver_t ss, uintptr_t opaque, int id);
extern void sserver_start(sserver_t ss, uintptr_t opaque, int id);

// return -1 when error
extern int sserver_send(sserver_t ss, int id, const void * buffer, int sz);
extern int sserver_send_lowpriority(sserver_t ss, int id, const void * buffer, int sz);

// ctrl command below returns id
extern int sserver_listen(sserver_t ss, uintptr_t opaque, const char * addr, uint16_t port);
extern int sserver_connect(sserver_t ss, uintptr_t opaque, const char * addr, uint16_t port);
extern int sserver_bind(sserver_t ss, uintptr_t opaque, socket_t fd);

// for tcp
extern void sserver_nodelay(sserver_t ss, int id);


typedef uint8_t * udpaddr_t;

// create an udp socket handle, attach opaque with it . udp socket don't need call sserver_start to recv message
// if port != 0, bind the socket . if addr == NULL, bind ipv4 0.0.0.0 . If you want to use ipv6, addr can be "::" and port 0.
extern int sserver_udp(sserver_t ss, uintptr_t opaque, const char * addr, uint16_t port);
// set default dest address, return 0 when success
extern int sserver_udp_connect(sserver_t ss, int id, const char * addr, uint16_t port);
// If the socket_udp_address is NULL, use last call sserver_udp_connect address instead
// You can also use sserver_send 
extern int sserver_udp_send(sserver_t ss, int id, const udpaddr_t udpaddr, const void * buffer, int sz);
// extract the address of the message, smessage_t  should be SSERVER_UDP
extern const udpaddr_t sserver_udp_address(sserver_t ss, smessage_t msg, int * addrsz);

struct sinterface {
	int (* size)(void * uobj);
	void (* free)(void * uobj);
	void * (* buffer)(const void * uobj);
};

// if you send package sz == -1, use soi.
extern void sserver_userobject(sserver_t ss, struct sinterface * soi);

#endif // !_H_SIMPLEC_SOCKET_SERVER
