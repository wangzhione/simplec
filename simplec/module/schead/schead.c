#include <schead.h>

#if defined(__GNUC__)

//
// getch - 立即得到用户输入的一个字符, linux实现
// return	: 返回得到字符
//
inline int 
getch(void) {
	int cr;
	struct termios nts, ots;
	if (tcgetattr(0, &ots) < 0) // 得到当前终端(0表示标准输入)的设置
		return EOF;

	nts = ots;
	cfmakeraw(&nts); // 设置终端为Raw原始模式，该模式下所有的输入数据以字节为单位被处理
	if (tcsetattr(0, TCSANOW, &nts) < 0) // 设置上更改之后的设置
		return EOF;

	cr = getchar();
	if (tcsetattr(0, TCSANOW, &ots) < 0) // 设置还原成老的模式
		return EOF;
	return cr;
}

#endif

// 等待的宏 是个单线程没有加锁 | "请按任意键继续. . ."

inline void 
sh_pause(void) {
	rewind(stdin);
	fflush(stderr); fflush(stdout);
	printf("Press any key to continue . . .");
	getch();
}

//
// sh_isbe - 判断是大端序还是小端序,大端序返回true
// sh_hton - 将本地四字节数据转成'大端'网络字节
// sh_ntoh - 将'大端'网络四字节数值转成本地数值
//
inline bool 
sh_isbe(void) {
	static union { uint16_t i; uint8_t c; } _u = { 1 };
	return 0 == _u.c;
}

inline uint32_t 
sh_hton(uint32_t x) {
	if (!sh_isbe()) {
		uint8_t t;
		union { uint32_t i; uint8_t s[sizeof(uint32_t)]; } u = { x };
		t = u.s[0], u.s[0] = u.s[sizeof(u) - 1], u.s[sizeof(u) - 1] = t;
		t = u.s[1], u.s[1] = u.s[sizeof(u) - 2], u.s[sizeof(u) - 2] = t;
		return u.i;
	}
	return x;
}

inline uint32_t 
sh_ntoh(uint32_t x) {
	return sh_hton(x);
}

//
// async_run - 开启一个自销毁的线程 运行 run
// run		: 运行的主体
// arg		: run的参数
// return	: >= SufBase 表示成功
//
inline int 
async_run_(node_f run, void * arg) {
	pthread_t tid;
	pthread_attr_t attr;

	// 构建pthread 线程奔跑起来
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&tid, &attr, (start_f)run, arg) < 0) {
		pthread_attr_destroy(&attr);
		RETURN(ErrBase, "pthread_create error run, arg = %p | %p.", run, arg);
	}

	pthread_attr_destroy(&attr);
	return SufBase;
}