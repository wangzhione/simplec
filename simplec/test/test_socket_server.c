#include <socket_server.h>

static void * _poll(void * ud) {
	sserver_t ss = ud;
	struct smessage result;
	for (;;) {
		int type = sserver_poll(ss, &result, NULL);
		// DO NOT use any ctrl command (sserver_close , etc. ) in this thread.
		switch (type) {
		case SSERVER_EXIT:
			return NULL;
		case SSERVER_DATA:
			printf("message(%"PRIdPTR") [id=%d] size=%d\n", result.opaque, result.id, result.ud);
			free(result.data);
			break;
		case SSERVER_CLOSE:
			printf("close(%"PRIdPTR") [id=%d]\n", result.opaque, result.id);
			break;
		case SSERVER_OPEN:
			printf("open(%"PRIdPTR") [id=%d] %s\n", result.opaque, result.id, result.data);
			break;
		case SSERVER_ERR:
			printf("error(%"PRIdPTR") [id=%d] %s\n", result.opaque, result.id, result.data);
			break;
		case SSERVER_ACCEPT:
			printf("accept(%"PRIdPTR") [id=%d %s] from [%d]\n", result.opaque, result.ud, result.data, result.id);
			break;
		}
	}
	return ss;
}

//
// socket_server.h 单元测试
//		-> 它 * 天阶功法 * 真强
//
void test_socket_server(void) {
	int c, l;
	pthread_t pid;
	socket_t s = socket_dgram();
	sserver_t ss = sserver_create();
	pthread_create(&pid, NULL, _poll, ss);

	c = sserver_connect(ss, 100, "127.0.0.1", 8088);
	printf("connecting(100) %d\n", c);

	l = sserver_listen(ss, 200, "127.0.0.1", 8888);
	printf("listening(200) %d\n", l);
	sserver_start(ss, 201, l);
	int b = sserver_bind(ss, 300, s);
	printf("binding(300) new socket %d\n", b);

	for (int i = 0; i < 100; ++i)
		sserver_connect(ss, 400 + i, "127.0.0.1", 8888);

	sh_msleep(500000);
	sserver_exit(ss);

	pthread_join(pid, NULL);
	sserver_delete(ss);
	socket_close(s);
}