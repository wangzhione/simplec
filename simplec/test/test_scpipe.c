#include <scpipe.h>

//
// test simplec pipe 
//
void test_scpipe(void) {
	socket_t fd[2];
	char data[] = "我爱中国, I support 主席.";
	scpipe_t spie = scpipe_create();
	
	puts(data);
	scpipe_send(spie, data, sizeof data);

	scpipe_recv(spie, data, sizeof data);
	puts(data);

	scpipe_delete(spie);

	// 这里继续测试 pipe 管道移植版本的兼容性
	IF(pipe(fd) < 0);

	socket_send(fd[1], data, sizeof data);
	socket_recv(fd[0], data, sizeof data);
	puts(data);

	socket_close(fd[0]);
	socket_close(fd[1]);
}