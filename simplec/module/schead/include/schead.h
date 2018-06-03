#ifndef _H_SIMPLEC_SCHEAD
#define _H_SIMPLEC_SCHEAD

#include <clog.h>
#include <scrand.h>
#include <struct.h>
#include <pthread.h>
#include <semaphore.h>

//
//  宏就是C的金字塔最底层, 所有丑陋的起源~
//  [ __clang__ -> clang | __GNUC__ -> gcc | __MSC_VER -> cl ]
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

// 添加双引号的宏 
#define _STR(v) #v
#define CSTR(v)	_STR(v)

// 获取数组长度,只能是数组类型或""字符串常量,后者包含'\0'
#define LEN(a) (sizeof(a) / sizeof(*(a)))

// 置空操作, v必须是个变量
#define BZERO(v) \
        memset(&(v), 0, sizeof(v))

/*
 * 比较两个结构体栈上内容是否相等,相等返回true,不等返回false
 * a	: 第一个结构体值
 * b	: 第二个结构体值
 *		: 相等返回true, 否则false
 */
#define STRUCTCMP(a, b) (!memcmp(&a, &b, sizeof(a)))

//
// EXTERN_RUN - 简单的声明, 并立即使用的宏
// test		: 需要执行的函数名称
//
#define EXTERN_RUN(test, ...) \
    do { \
	    extern void test(); \
	    test (__VA_ARGS__); \
    } while(0)

// 简单的time时间记录宏
#define TIME_PRINT(code) \
    do { \
	    clock_t $s, $e; \
	    $s = clock(); \
	    code \
	    $e = clock(); \
	    printf("Now code run time:%lfs.\n", ((double)$e - $s) / CLOCKS_PER_SEC); \
    } while (0)

//
// sh_pause - 等待的宏 是个单线程没有加锁 | "请按任意键继续. . ."
// return	: void
//
extern void sh_pause(void);

#ifndef SH_PAUSE

#   ifdef _DEBUG
#       define SH_PAUSE() atexit(sh_pause)
#   else
#       define SH_PAUSE() /* 别说了, 都重新开始吧 */
#   endif

#endif // !INIT_PAUSE

//
// sh_isbe - 判断是大端序还是小端序,大端序返回true
// sh_hton - 将本地四字节数据转成'大端'网络字节
// sh_ntoh - 将'大端'网络四字节数值转成本地数值
//
extern bool sh_isbe(void);
extern uint32_t sh_hton(uint32_t x);
extern uint32_t sh_ntoh(uint32_t x);

//
// async_run - 开启一个自销毁的线程 运行 run
// run		: 运行的主体
// arg		: run的参数
// return	: >= SufBase 表示成功
//
extern int async_run_(node_f run, void * arg);
#define async_run(run, arg) \
        async_run_((node_f)(run), (void *)(intptr_t)arg)

#endif//_H_SIMPLEC_SCHEAD