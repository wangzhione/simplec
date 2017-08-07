#ifndef _H_SIMPLEC_SCHEAD
#define _H_SIMPLEC_SCHEAD

#include <clog.h>
#include <scrand.h>
#include <struct.h>

//
//  一切的宏都是, 跨平台丑陋的证明~ 宏就是C的金字塔最底层
//  __clang__ => clang 平台特殊操作
//  __GNUC__  => gcc 平台特殊操作
//  __MSC_VER => cl 平台特殊操作
//
#ifdef __GNUC__

#include <termio.h>
#include <sys/stat.h>
#include <sys/types.h>

/*
 * 屏幕清除宏, 依赖系统脚本
 *  return	: void
 */
#define sh_cls() \
		printf("\ec")

//
// getch - 立即得到用户输入的一个字符, linux实现
// return	: 返回得到字符
//
inline int getch(void) {
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

//
// sh_mkdir - 通用的单层目录创建宏 等同于 shell> mkdir path
// path		: 目录路径加名称
// return	: 0表示成功, -1表示失败, 失败原因都在 errno
// 
#define sh_mkdir(path) \
	mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)

#elif _MSC_VER

#include <direct.h> 
#include <conio.h>

#define sh_cls() \
		system("cls")

#define sh_mkdir(path) \
	mkdir(path)

#else
#	error "error : Currently only supports the Best New CL and GCC!"
#endif

/* 栈上辅助操作宏 */
#if !defined(_H_ARRHELP)

// 添加双引号的宏 
#define _STR(v) #v
#define CSTR(v)	_STR(v)

// 获取数组长度,只能是数组类型或""字符串常量,后者包含'\0'
#define LEN(a) (sizeof(a) / sizeof(*(a)))

// 置空操作, v必须是个变量
#define BZERO(v) \
	memset(&(v), 0, sizeof(v))

// 得到bit位数
#define BITN(type) (sizeof(type) * 8)

/*
 * 比较两个结构体栈上内容是否相等,相等返回true,不等返回false
 * a	: 第一个结构体值
 * b	: 第二个结构体值
 *		: 相等返回true, 否则false
 */
#define STRUCTCMP(a, b) \
	(!memcmp(&a, &b, sizeof(a)))

#define _H_ARRHELP
#endif//_H_ARRHELP

#ifndef _H_CODEHELP

//
// EXTERN_RUN - 简单的声明, 并立即使用的宏
// test		: 需要执行的函数名称
//
#define EXTERN_RUN(test, ...)	\
	do {						\
		extern void test();		\
		test(##__VA_ARGS__);	\
	} while(0)

// scanf 健壮的多次输入宏
#define SSCANF(scanf_code, ...)										\
		while (printf(##__VA_ARGS__), scanf_code){					\
			rewind(stdin);											\
			puts("Input error, please according to the prompt!");	\
		}															\
		rewind(stdin);												\
	} while (0)

// 简单的time时间记录宏
#define TIME_PRINT(code)												\
	do {																\
		clock_t $st, $et;												\
		$st = clock();													\
		code															\
		$et = clock();													\
		printf("The current code block running time:%lf seconds.\n",	\
		 (0.0 + $et - $st) / CLOCKS_PER_SEC);							\
	} while (0)

#define _H_CODEHELP
#endif//_H_CODEHELP

// 等待的宏 是个单线程没有加锁 | "请按任意键继续. . ."
inline void sh_pause(void) {
	rewind(stdin);
	printf("Press any key to continue . . .");
	getch();
}

#ifndef SH_PAUSE

#	ifdef _DEBUG
#		define SH_PAUSE() atexit(sh_pause)
#	else
#		define SH_PAUSE() /* 别说了,都重新开始吧 */
#	endif

#endif // !INIT_PAUSE

//
// sh_isbig - 判断是大端序还是小端序,大端序返回true
// sh_hton - 将本地四字节数据转成'小端'网络字节
// sh_ntoh - 将'小端'网络四字节数值转成本地数值
//
inline bool sh_isbig(void) {
	union { uint16_t i; uint8_t c; } u = { 1 };
	return 0 == u.c;
}

inline uint32_t sh_hton(uint32_t x) {
	if (sh_isbig()) {
		uint8_t t;
		union { uint32_t i; uint8_t s[sizeof(uint32_t)]; } u = { x };
		t = u.s[0], u.s[0] = u.s[sizeof(u) - 1], u.s[sizeof(u) - 1] = t;
		t = u.s[1], u.s[1] = u.s[sizeof(u) - 1 - 1], u.s[sizeof(u) - 1 - 1] = t;
		return u.i;
	}
	return x;
}

inline uint32_t sh_ntoh(uint32_t x) {
	return sh_hton(x);
}

#endif//_H_SIMPLEC_SCHEAD