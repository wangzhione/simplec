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

#if defined(_MSC_VER)

//
// usleep - 毫秒级别等待函数
// usec		: 等待的毫秒
// return	: The usleep() function returns 0 on success.  On error, -1 is returned.
//
int 
usleep(unsigned usec) {
	int rt = Error_Base;
	// Convert to 100 nanosecond interval, negative value indicates relative time
	LARGE_INTEGER ft = { .QuadPart = -10ll * usec };

	HANDLE timer = CreateWaitableTimer(NULL, TRUE, NULL);
	if (timer) {
		// 负数以100ns为单位等待, 正数以标准FILETIME格式时间
		SetWaitableTimer(timer, &ft, 0, NULL, NULL, FALSE);
		WaitForSingleObject(timer, INFINITE);
		if (GetLastError() == ERROR_SUCCESS)
			rt = Success_Base;
		CloseHandle(timer);
	}

	return rt;
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

// 判断是大端序还是小端序,大端序返回true
inline bool
sh_isbig(void) {
	static union {
		uint16_t us;
		uint8_t  uc;
	} _u = { 1 };
	return 0 == _u.uc;
}