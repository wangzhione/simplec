#include <stdio.h>
#include <scpipe.h>

//
// test simplec pipe 
//
void test_scpipe(void) {
	char data[] = "我爱中国, I love 主席.";
	scpipe_t spie = scpipe_create();
	
	puts(data);
	scpipe_send(spie, data, sizeof(data));

	scpipe_recv(spie, data, sizeof(data));
	puts(data);

	scpipe_delete(spie);
}