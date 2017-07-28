#ifndef _H_SIMPLEC_SCHEAD
#define _H_SIMPLEC_SCHEAD

#include <clog.h>
#include <scrand.h>

//
//  跨平台的丑陋从这里开始, 封装一些共用实现
//  __GNUC__	= > linux GCC 平台特殊操作
//  __MSC_VER	= > winds  CL 平台特殊操作
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
extern int getch(void);

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
#define CSTR(a)	_STR(a)
#define _STR(a) #a

// 获取数组长度,只能是数组类型或""字符串常量,后者包含'\0'
#define LEN(arr) (sizeof(arr) / sizeof(*(arr)))

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

/* 范围内比较大小辅助宏 */
#if !defined(_H_EQUAL)

// 浮点数据判断宏帮助, __开头表示不希望你使用的宏
#define _DIFF(x, y)			((x)-(y))				//两个表达式做差宏
#define _IF_X(x, c)			((x)<c && (x)>-c)		//判断宏,z必须是宏常量
#define EQC(x, y, c)		_IF_X(_DIFF(x, y), c)	//判断x和y是否在误差范围内相等

// float判断定义的宏
#define EQ_FLOAT_ZERO(x)	_IF_X(x, FLT_MIN)		//float 判断x是否为零是返回true
#define EQ_FLOAT(x, y)		EQC(x, y, FLT_MIN)		//判断表达式x与y是否相等

// double判断定义的宏
#define EQ_DOUBLE_ZERO(x)	_IF_X(x, DBL_MIN)		//double 判断x是否为零是返回true
#define EQ_DOUBLE(x,y)		EQC(x, y, DBL_MIN)		//判断表达式x与y是否相等

#define _H_EQUAL
#endif//_H_EQUAL

#ifndef _H_CODEHELP

//
// EXTERN_RUN - 简单的声明, 并立即使用的宏
// test		: 需要执行的函数名称
//
#define EXTERN_RUN(test, ...) \
	do { \
		extern void test(); \
		test(##__VA_ARGS__); \
	} while(0)

// scanf 健壮的多次输入宏
#define _STR_SSCANF "Input error, please according to the prompt!"
#define SSCANF(scanf_code, ...) \
		while (printf(##__VA_ARGS__), scanf_code){ \
			rewind(stdin); \
			puts(_STR_SSCANF); \
		} \
		rewind(stdin); \
	} while (0)

// 简单的time时间记录宏
#define _STR_TPRINT "The current code block running time:%lf seconds.\n"
#define TIME_PRINT(code) \
	do { \
		clock_t $st, $et; \
		$st = clock(); \
		code \
		$et = clock(); \
		printf(_STR_TPRINT, (0.0 + $et - $st) / CLOCKS_PER_SEC); \
	} while (0)

#define _H_CODEHELP
#endif//_H_CODEHELP

// 等待的宏 是个单线程没有加锁 | "请按任意键继续. . ."
extern void sh_pause(void);

#ifndef INIT_PAUSE

#	ifdef _DEBUG
#		define INIT_PAUSE() atexit(sh_pause)
#	else
#		define INIT_PAUSE()	/* 别说了,都重新开始吧 */
#	endif

#endif // !INIT_PAUSE

//
// sh_strerr - linux 上面替代 strerror, winds 替代 FormatMessage 
// error	: linux 是 errno, winds 可以是 WSAGetLastError() ... 
// return	: system os 拔下来的提示字符串常量
//
extern const char * sh_strerr(int error);

//
// sh_isbig - 判断是大端序还是小端序,大端序返回true
// sh_hton - 将本地四字节数据转成'小端'网络字节
// sh_ntoh - 将'小端'网络四字节数值转成本地数值
//
extern bool sh_isbig(void);
extern uint32_t sh_hton(uint32_t x);
extern uint32_t sh_ntoh(uint32_t x);

#endif// ! _H_SIMPLEC_SCHEAD