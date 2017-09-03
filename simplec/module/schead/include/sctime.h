#ifndef _H_SIMPLEC_SCTIMEUTIL
#define _H_SIMPLEC_SCTIMEUTIL

#include <time.h>
#include <stdbool.h>

//
// 1s = 1000ms = 1000000us = 1000000000ns
// 1秒  1000毫秒  1000000微秒  1000000000纳秒
// ~ 力求最小时间业务单元 ~ 
//

#ifdef __GNUC__

#include <unistd.h>
#include <sys/time.h>

//
// sh_msleep - 睡眠函数, 时间颗粒度是毫秒.
// m		: 待睡眠的毫秒数
// return	: void
//
#define sh_msleep(m) \
		usleep(m * 1000)

#endif

// 为Visual Studio导入一些和linux上优质思路
#ifdef _MSC_VER

#include <windows.h>

#define sh_msleep(m) \
		Sleep(m)

//
// usleep - 微秒级别等待函数
// usec		: 等待的微秒
// return	: The usleep() function returns 0 on success.  On error, -1 is returned.
//
extern int usleep(unsigned usec);

/*
 * 返回当前得到的时间结构体, 高仿linux上调用
 * pt	: const time_t * , 输入的时间戳指针
 * ptm	: struct tm * , 输出的时间结构体
 *		: 返回 ptm 值
 */
#define localtime_r(pt, ptm) localtime_s(ptm, pt), ptm

#endif

// 定义时间串类型
#define _INT_STULEN (64)
typedef char stime_t[_INT_STULEN];

/*
 * 将 [2016-7-10 21:22:34] 格式字符串转成时间戳
 * tstr	: 时间串分隔符只能是单字节的.
 * pt	: 返回得到的时间戳
 * otm	: 返回得到的时间结构体
 *		: 返回false表示构造失败
 */
extern bool stu_gettime(stime_t tstr, time_t * pt, struct tm * otm);

/*
 * 判断当前时间戳是否是同一天的.
 * lt : 判断时间一
 * rt : 判断时间二
 *    : 返回true表示是同一天, 返回false表示不是
 */
extern bool stu_tisday(time_t lt, time_t rt);

/*
 * 判断当前时间戳是否是同一周的.
 * lt : 判断时间一
 * rt : 判断时间二
 *    : 返回true表示是同一周, 返回false表示不是
 */
extern bool stu_tisweek(time_t lt, time_t rt);

//
// stu_sisday - 判断当前时间串是否是同一天的.
// ls : 判断时间一
// rs : 判断时间二
//    : 返回true表示是同一天, 返回false表示不是
//
extern bool stu_sisday(stime_t ls, stime_t rs);

//
// 判断当前时间串是否是同一周的.
// ls : 判断时间一
// rs : 判断时间二
//    : 返回true表示是同一周, 返回false表示不是
//
extern bool stu_sisweek(stime_t ls, stime_t rs);

/*
 * 将时间戳转成时间串 [2016-07-10 22:38:34]
 * nt	: 当前待转的时间戳
 * tstr	: 保存的转后时间戳位置
 *		: 返回传入tstr的首地址
 */
extern char * stu_gettstr(time_t nt, stime_t tstr);

/*
 * 得到当前时间戳 [2016-7-10 22:38:34]
 * tstr	: 保存的转后时间戳位置
 *		: 返回传入tstr的首地址
 */
extern char * stu_getntstr(stime_t tstr);

//
// stu_getmstr - 得到加毫秒的串 [2016-07-10 22:38:34 500]
// tstr		: 保存最终结果的串
// return	: 返回当前串长度
//
#define _STR_MTIME			"%04d-%02d-%02d %02d:%02d:%02d %03ld"
extern size_t stu_getmstr(stime_t tstr);

//
// stu_getmstrn - 得到特定包含时间串, fmt 依赖 _STR_MTIME
// buf		: 保存最终结果的串
// len		: 当前buf串长度
// fmt		: 输出格式串例如 -> "simplec-%04d%02d%02d-%02d%02d%02d-%03ld.log"
// return	: 返回当前串长度
//
extern size_t stu_getmstrn(char buf[], size_t len, const char * const fmt);

#endif // !_H_SIMPLEC_SCTIMEUTIL