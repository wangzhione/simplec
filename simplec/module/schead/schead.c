#include <schead.h>

// 为linux扩展一些功能
#if defined(__GNUC__)

//
// getch - 立即得到用户输入的一个字符, linux实现
// return	: 返回得到字符
//
int
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

#define _STR_PAUSEMSG "Press any key to continue . . ."
// 简单通用的等待函数
inline void
sh_pause(void) {
	rewind(stdin);
	printf(_STR_PAUSEMSG);
	getch();
}

union uaglin {
	uint32_t i;
	uint8_t  c;
	uint8_t  s[sizeof(uint32_t)];
};

// 判断是大端序还是小端序,大端序返回true
inline bool
sh_isbig(void) {
	static union uaglin _u = { 1 };
	return 0 == _u.c;
}

//
// sh_hton - 将本地四字节数据转成'小端'网络字节
// sh_ntoh - 将'小端'网络四字节数值转成本地数值
//
inline uint32_t 
sh_hton(uint32_t x) {
	if (sh_isbig()) {
		uint8_t t;
		union uaglin u = { x };
		t = u.s[0], u.s[0] = u.s[sizeof(u) - 1], u.s[sizeof(u) - 1] = t;
		t = u.s[1], u.s[1] = u.s[sizeof(u) - 1 - 1], u.s[sizeof(u) - 1 - 1] = t;
		return u.i;
	}
	return x;
}

inline uint32_t 
sh_ntoh(uint32_t x) {
	return sh_hton(x);
}