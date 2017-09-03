#include <sctime.h>
#include <stdio.h>
#include <stdint.h>

// 为 Visual Studio 导入一些 GCC 上优质思路
#ifdef _MSC_VER

//
// usleep - 微秒级别等待函数
// usec		: 等待的微秒
// return	: The usleep() function returns 0 on success.  On error, -1 is returned.
//
int
usleep(unsigned usec) {
	int rt = -1;
	// Convert to 100 nanosecond interval, negative value indicates relative time
	LARGE_INTEGER ft = { .QuadPart = -10ll * usec };

	HANDLE timer = CreateWaitableTimer(NULL, TRUE, NULL);
	if (timer) {
		// 负数以100ns为单位等待, 正数以标准FILETIME格式时间
		SetWaitableTimer(timer, &ft, 0, NULL, NULL, FALSE);
		WaitForSingleObject(timer, INFINITE);
		if (GetLastError() == ERROR_SUCCESS)
			rt = 0;
		CloseHandle(timer);
	}

	return rt;
}

#endif

// 从时间串中提取出来年月日时分秒
static bool _stu_gettm(stime_t tstr, struct tm * otm) {
	char c;
	int sum, * py, * es;

	if ((!tstr) || !(c = *tstr) || c < '0' || c > '9')
		return false;

	py = &otm->tm_year;
	es = &otm->tm_sec;
	sum = 0;
	while ((c = *tstr) && py >= es) {
		if (c >= '0' && c <= '9') {
			sum = 10 * sum + c - '0';
			++tstr;
			continue;
		}

		*py-- = sum;
		sum = 0;

		// 去掉特殊字符, 一直找到下一个数字
		while ((c = *++tstr) && (c < '0' || c > '9'))
			;
	}
	// 非法, 最后解析出错
	if (py != es)
		return false;

	*es = sum; // 保存最后秒数据
	return true;
}

/*
 * 将 [2016-7-10 21:22:34] 格式字符串转成时间戳
 * tstr	: 时间串分隔符只能是单字节的.
 * pt	: 返回得到的时间戳
 * otm	: 返回得到的时间结构体
 *		: 返回false表示构造失败
 */
bool
stu_gettime(stime_t tstr, time_t * pt, struct tm * otm) {
	time_t t;
	struct tm st;

	// 先高效解析出年月日时分秒
	if (!_stu_gettm(tstr, &st))
		return false;

	st.tm_year -= 1900;
	st.tm_mon -= 1;
	// 得到时间戳, 失败返回false
	if ((t = mktime(&st)) == -1)
		return false;

	// 返回最终结果
	if (pt)
		*pt = t;
	if (otm)
		*otm = st;

	return true;
}

/*
 * 判断当前时间戳是否是同一天的.
 * lt : 判断时间一
 * rt : 判断时间二
 *    : 返回true表示是同一天, 返回false表示不是
 */
inline bool
stu_tisday(time_t lt, time_t rt) {
	// GMT [World] + 8 * 3600 = CST [China], 得到各自当前天数
	lt = (lt + 8UL * 3600) / (24 * 3600);
	rt = (rt + 8UL * 3600) / (24 * 3600);
	return lt == rt;
}

/*
 * 判断当前时间戳是否是同一周的.
 * lt : 判断时间一
 * rt : 判断时间二
 *    : 返回true表示是同一周, 返回false表示不是
 */
bool
stu_tisweek(time_t lt, time_t rt) {
	time_t mt;
	struct tm st;

	if (lt < rt) { //得到最大时间, 保存在lt中
		mt = lt;
		lt = rt;
		rt = mt;
	}

	// 得到lt 表示的当前时间
	localtime_r(&lt, &st);

	// 得到当前时间到周一起点的时间差
	st.tm_wday = st.tm_wday ? st.tm_wday - 1 : 6;
	mt = st.tm_wday * 24 * 3600 + st.tm_hour * 3600 
        + st.tm_min * 60 + st.tm_sec;

	// [min, lt], lt = max(lt, rt) 就表示在同一周内
	return rt >= lt - mt;
}

//
// stu_sisday - 判断当前时间串是否是同一天的.
// ls : 判断时间一
// rs : 判断时间二
//    : 返回true表示是同一天, 返回false表示不是
//
bool
stu_sisday(stime_t ls, stime_t rs) {
	time_t lt, rt;
	// 解析失败直接返回结果
	if (!stu_gettime(ls, &lt, NULL) || !stu_gettime(rs, &rt, NULL))
		return false;

	return stu_tisday(lt, rt);
}

//
// 判断当前时间串是否是同一周的.
// ls : 判断时间一
// rs : 判断时间二
//    : 返回true表示是同一周, 返回false表示不是
//
bool
stu_sisweek(stime_t ls, stime_t rs) {
	time_t lt, rt;
	// 解析失败直接返回结果
	if (!stu_gettime(ls, &lt, NULL) || !stu_gettime(rs, &rt, NULL))
		return false;

	return stu_tisweek(lt, rt);
}

/*
 * 将时间戳转成时间串 [2016-7-10 22:38:34]
 * nt	: 当前待转的时间戳
 * tstr	: 保存的转后时间戳位置
 *		: 返回传入tstr的首地址
 */
#define _STR_TIME			"%04d-%02d-%02d %02d:%02d:%02d"
inline char * 
stu_gettstr(time_t nt, stime_t tstr) {
	struct tm st;
	localtime_r(&nt, &st);
	snprintf(tstr, sizeof(stime_t), _STR_TIME,
			st.tm_year + 1900, st.tm_mon + 1, st.tm_mday,
			st.tm_hour, st.tm_min, st.tm_sec);
	return tstr;
}

/*
 * 得到当前时间戳 [2016-7-10 22:38:34]
 * tstr	: 保存的转后时间戳位置
 *		: 返回传入tstr的首地址
 */
inline char * 
stu_getntstr(stime_t tstr) {
	return stu_gettstr(time(NULL), tstr);
}

//
// stu_getmstr - 得到加毫秒的串 [2016-7-10 22:38:34 500]
// tstr		: 保存最终结果的串
// return	: 返回当前串长度
//
size_t 
stu_getmstr(stime_t tstr) {
    time_t t;
    struct tm st;
    struct timespec tv;

    timespec_get(&tv, TIME_UTC);
    t = tv.tv_sec;
    localtime_r(&t, &st);
    return snprintf(tstr, sizeof(stime_t), _STR_MTIME,
                    st.tm_year + 1900, st.tm_mon + 1, st.tm_mday,
                    st.tm_hour, st.tm_min, st.tm_sec,
                    tv.tv_nsec / 1000000);
}

//
// stu_getmstrn - 得到特定包含时间串, fmt 依赖 _STR_MTIME
// buf		: 保存最终结果的串
// len		: 当前buf串长度
// fmt		: 输出格式串例如 -> "simplec-%04d%02d%02d-%02d%02d%02d-%03ld.log"
// return	: 返回当前串长度
//
size_t 
stu_getmstrn(char buf[], size_t len, const char * const fmt) {
    time_t t;
    struct tm st;
    struct timespec tv;

    timespec_get(&tv, TIME_UTC);
    t = tv.tv_sec;
    localtime_r(&t, &st);
    return snprintf(buf, len, fmt,
                    st.tm_year + 1900, st.tm_mon + 1, st.tm_mday,
                    st.tm_hour, st.tm_min, st.tm_sec,
                    tv.tv_nsec / 1000000);
}