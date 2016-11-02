#include <schead.h>

//简单通用的等待函数
inline void
sh_pause(void) {
	rewind(stdin);
	printf(_STR_PAUSEMSG);
	sh_getch();
}

//12.0 判断是大端序还是小端序,大端序返回true
inline bool
sh_isbig(void) {
	static union {
		unsigned short _s;
		unsigned char _c;
	} __u = { 1 };
	return __u._c == 0;
}

/*
 *	简单的释放内存函数. 对free封装了一层.
 *释放后会置空, 可以简易的避免野指针.
 * pobj		: 指向待释放内存的指针(void*)
 * return		: void
 */
inline void
sh_free(void ** pobj) {
	if (pobj == NULL || *pobj == NULL)
		return;
	free(*pobj);
	*pobj = NULL;
}

// 为linux扩展一些功能
#if defined(__GNUC__)

/*
 * 得到用户输入的一个字符
 *		: 返回得到字符
 */
int 
sh_getch(void) {
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