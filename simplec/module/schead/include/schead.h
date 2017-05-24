#ifndef _H_SIMPLEC_SCHEAD
#define _H_SIMPLEC_SCHEAD

#include <clog.h>
#include <struct.h>
#include <sctime.h>

//
//  跨平台的丑陋从这里开始, 封装一些共用实现
//  __GNUC__	= > linux 平台特殊操作
//  __MSC_VER	= > winds 平台特殊操作
//
#ifdef __GNUC__  // 下面是依赖GCC编译器实现

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <termio.h>

/*
 * 睡眠函数, 时间颗粒度是毫秒.
 *  m		: 待睡眠的毫秒数
 *  return	: void
 */
#define sh_sleep(m) \
		usleep(m * 1000)

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

#elif _MSC_VER // 下面是依赖Visual Studio编译器实现

#include <Windows.h>
#include <direct.h> 
#include <conio.h>

#define sh_cls() \
		system("cls")

#define sh_sleep(m) \
		Sleep(m)	

#define sh_mkdir(path) \
	mkdir(path)

#else
	#error "error : Currently only supports the Visual Studio and GCC!"
#endif

#if !defined(_H_LOG_HELP)

//
// 所有日志相对路径目录, 如果不需要需要配置成""
// _STR_LOGDIR	- 存放所有日志的目录
// _INT_LOG		- 每条日志的大小
// _INT_PATH	- 日志路径大小
//
#define _STR_LOGDIR		"logs"
#define _UINT_LOG		(2048u)
#define _UINT_PATH		(256u)
#define _STR_LOGTIME	"[" _STR_MTIME "]"

 // 添加双引号的宏 
#define CSTR(a)	_STR(a)
#define _STR(a) #a

#define _H_LOG_HELP
#endif

/* 栈上辅助操作宏 */
#if !defined(_H_ARRAY_HELP)

// 获取数组长度,只能是数组类型或""字符串常量,后者包含'\0'
#define LEN(arr) (sizeof(arr) / sizeof(*(arr)))

// 置空操作, v必须是个变量
#define BZERO(v) \
	memset(&(v), 0, sizeof(v))

/*
 * 比较两个结构体栈上内容是否相等,相等返回true,不等返回false
 * a	: 第一个结构体值
 * b	: 第二个结构体值
 *		: 相等返回true, 否则false
 */
#define STRUCTCMP(a, b) \
	(!memcmp(&a, &b, sizeof(a)))

#define _H_ARRAY_HELP
#endif

/* 比较大小辅助宏 */
#if !defined(_H_EQUAL)

// 浮点数据判断宏帮助, __开头表示不希望你使用的宏
#define __DIFF(x, y)				((x)-(y))					//两个表达式做差宏
#define __IF_X(x, z)				((x)<z && (x)>-z)			//判断宏,z必须是宏常量
#define EQ(x, y, c)					EQ_ZERO(__DIFF(x,y), c)		//判断x和y是否在误差范围内相等

// float判断定义的宏
#define _FLOAT_ZERO				(0.000001f)						//float 0的误差判断值
#define EQ_FLOAT_ZERO(x)		__IF_X(x, _FLOAT_ZERO)			//float 判断x是否为零是返回true
#define EQ_FLOAT(x, y)			EQ(x, y, _FLOAT_ZERO)			//判断表达式x与y是否相等

// double判断定义的宏
#define _DOUBLE_ZERO			(0.000000000001)				//double 0误差判断值
#define EQ_DOUBLE_ZERO(x)		__IF_X(x, _DOUBLE_ZERO)			//double 判断x是否为零是返回true
#define EQ_DOUBLE(x,y)			EQ(x, y, _DOUBLE_ZERO)			//判断表达式x与y是否相等

#define _H_EQUAL
#endif 

// scanf 健壮的多次输入宏
#ifndef SAFE_SCANF
#define _STR_SAFE_SCANF "Input error, please according to the prompt!"
#define SAFE_SCANF(scanf_code, ...) \
		while(printf(##__VA_ARGS__), scanf_code){\
			rewind(stdin);\
			puts(_STR_SAFE_SCANF);\
		}\
		rewind(stdin);\
	} while(0)
#endif // !SAFE_SCANF

// 简单的time时间记录宏
#ifndef TIME_PRINT
#define _STR_TIME_PRINT "The current code block running time:%lf seconds\n"
#define TIME_PRINT(code) \
	do {\
		clock_t $st, $et;\
		$st = clock();\
		code\
		$et = clock();\
		printf(_STR_TIME_PRINT, (0.0 + $et - $st) / CLOCKS_PER_SEC);\
	} while(0)
#endif // !TIME_PRINT

// 等待的宏 是个单线程没有加锁 | "请按任意键继续. . ."
extern void sh_pause(void);
#ifndef INIT_PAUSE

#	ifdef _DEBUG
#		define INIT_PAUSE() atexit(sh_pause)
#	else
#		define INIT_PAUSE()	/* 别说了,都重新开始吧 */
#	endif

#endif // !INIT_PAUSE

/*
 * c 如果是空白字符返回 true, 否则返回false
 * c : 必须是 int 值,最好是 char 范围
 */
#define sh_isspace(c)	((c==' ')||(c>='\t'&&c<='\r'))

//12.0 判断是大端序还是小端序,大端序返回true
extern bool sh_isbig(void);

//
// sh_free - 简单粗暴的野指针销毁函数,并置空
// pobj		: 指向待释放内存的指针(void*)
// return	: void
//
extern void sh_free(void ** pobj);

#endif// ! _H_SIMPLEC_SCHEAD