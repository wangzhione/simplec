#include <schead.h>
#include <scconf.h>
#include <sciconv.h>
#include <scsocket.h>

static void _server(const char * host, uint16_t port) {
	puts("simplec udp server start ... ");
	// 启动须知
	printf("%s:%hu is start ...\n", host, port);

	socket_t sup = socket_udp(host, port);
	if (sup == INVALID_SOCKET) {
		CL_ERROR("socket_udp host, port is error = %s | %hu.", host, port);
		return;
	}

	// 这里 socket udp server 已经启动起来
	for (;;) {
		sockaddr_t in;
		char ip[64];
		char buf[128];

		puts("sleep udp msg ...");
		int rt = socket_recvfrom(sup, buf, sizeof buf, 0, &in);
		if (rt < 0) {
			CL_ERROR("socket_recvfrom is error = %s.", strerror(rt));
			continue;
		}
		
		if (!inet_ntop(in.sin_family, &in.sin_addr.s_addr, ip, sizeof ip)) {
			CL_ERROR("inet_ntop is error = %s", strerror(errno));
			continue;
		}

		// 开始输出数据
		buf[rt > 0 ? rt - 1 : 0] = '\0';
		if (si_isutf8(buf))
			si_utf8togbks(buf);

		printf(")%s:%hu( -> )%s(\n", ip, ntohs(in.sin_port), buf);
		CL_INFOS(")%s:%hu( -> )%s(\n", ip, ntohs(in.sin_port), buf);
	}

	socket_close(sup);
}

static void _client(const char * host, uint16_t port) {
	// 发送报文给服务器
	char buf[] = "期待下一次, 再见 ~";
	// 还是用 gbk 吧, 毕竟在 china red
	if (si_isutf8(buf))
		si_utf8togbks(buf);

	sockaddr_t to = { AF_INET };
	to.sin_port = htons(port);
	int rt = inet_pton(AF_INET, host, &to.sin_addr);
	if (rt <= 0) {
		RETURN(NIL, "inet_pton is error host = %s.", host);
	}

	socket_t cu = socket_dgram();
	if (cu == INVALID_SOCKET) {
		RETURN(NIL, "socket_dgram is error!");
	}

	// 发送个消息给 udp 服务器
	rt = socket_sendto(cu, buf, sizeof buf, 0, &to);
	if (rt < 0) {
		RETURN(NIL, "socket_sendto host = %s, port = %hu is error!", host, port);
	}

	socket_close(cu);
}

/*
 * simple c 框架业务层启动的代码
 */
void simplec_main(void) {
#if defined(_MSC_VER) && defined(_DEBUG)
	// 开始简单的测试
	EXTERN_RUN(simplec_test);
#endif

	// 开始动画部分
	EXTERN_RUN(simplec_go);

	/*
	 * Hero later is your world 
	 * . . .
	 */

	puts("simplec thinks you ...");

	const char * host = cnf_get("ServerHost");
	const char * pors = cnf_get("ServerPort");
	if (!host || !pors) {
		CL_ERROR("config mcnf_get ServerHost ServerPort is empty!");
		return;
	}
	uint16_t port = (uint16_t)atoi(pors);

	const char * types = cnf_get("ServerType");
	bool isser = types && !strcmp(types, "Server");
	if (isser) 
		_server(host, port);
	else
		_client(host, port);

}

// 第一次见面的函数
void 
simplec_go(void) {
    // 通过配置版本信息, 简单打印
    puts(cnf_get("Image"));
}

#if defined(_MSC_VER) && defined(_DEBUG)

/*
 * simple c 单元测试主函数
 * return   : void
 */
void 
simplec_test(void) {
    //
    // run test ... ... 
    //
    EXTERN_RUN(test_scthreads);

    exit(EXIT_SUCCESS);
}

#endif
