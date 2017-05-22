#include <sctime.h>
#include <stdio.h>

// 为Visual Studio导入一些和linux上优质思路
#if defined(_MSC_VER)

/*
 * Linux sys/time.h 中获取时间函数在Windows上一种移植实现
 * tv	:	返回结果包含秒数和微秒数
 * tz	:	包含的时区,在window上这个变量没有用不返回
 * 		:   默认返回0
 */
inline int
gettimeofday(struct timeval * tv, void * tz) {
	struct tm st;
	SYSTEMTIME wtm;

	GetLocalTime(&wtm);
	st.tm_year = wtm.wYear - _INT_YEAROFFSET;
	st.tm_mon = wtm.wMonth - _INT_MONOFFSET; // window的计数更好些
	st.tm_mday = wtm.wDay;
	st.tm_hour = wtm.wHour;
	st.tm_min = wtm.wMinute;
	st.tm_sec = wtm.wSecond;
	st.tm_isdst = -1; // 不考虑夏令时

	tv->tv_sec = (long)mktime(&st); // 32位使用数据强转
	tv->tv_usec = wtm.wMilliseconds * 1000; // 毫秒转成微秒

	return 0;
}

#endif

// 从时间串中提取出来年月日时分秒
static bool _stu_gettm(stime_t tstr, struct tm * otm) {
	int * py, * es;
	char c;
	int sum;

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
		while ((c = *++tstr) && (c<'0' || c>'9'))
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
 *		: 返回这个字符串转成的时间戳, -1表示构造失败
 */
bool
stu_gettime(stime_t tstr, time_t * pt, struct tm * otm) {
	time_t t;
	struct tm st;

	// 先高效解析出年月日时分秒
	if (!_stu_gettm(tstr, &st))
		return false;

	st.tm_year -= _INT_YEAROFFSET;
	st.tm_mon -= _INT_MONOFFSET;
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
	// 得到是各自第几天的
	lt = (lt + _INT_DAYSTART - _INT_DAYNEWSTART) / _INT_DAYSECOND;
	rt = (rt + _INT_DAYSTART - _INT_DAYNEWSTART) / _INT_DAYSECOND;
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

	lt -= _INT_DAYNEWSTART;
	rt -= _INT_DAYNEWSTART;

	if (lt < rt) { //得到最大时间, 保存在lt中
		mt = lt;
		lt = rt;
		rt = mt;
	}

	// 得到lt 表示的当前时间
	localtime_r(&lt, &st);

	// 得到当前时间到周一起点的时间差
	st.tm_wday = 0 == st.tm_wday ? 7 : st.tm_wday;
	mt = (st.tm_wday - 1) * _INT_DAYSECOND + st.tm_hour * _INT_HOURSECOND
		+ st.tm_min * _INT_MINSECOND + st.tm_sec;

	// [min, lt], lt = max(lt, rt) 就表示在同一周内
	return rt >= lt - mt;
}

/*
 * 将时间戳转成时间串 [2016-7-10 22:38:34]
 * nt	: 当前待转的时间戳
 * tstr	: 保存的转后时间戳位置
 *		: 返回传入tstr的首地址
 */
inline char * 
stu_gettstr(time_t nt, stime_t tstr) {
	struct tm st;
	localtime_r(&nt, &st);
	strftime(tstr, sizeof(stime_t), "%F %X", &st);
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

/*
 * 判断当前时间戳是否是同一天的.
 * ls : 判断时间一
 * rs : 判断时间二
 *    : 返回true表示是同一天, 返回false表示不是
 */
bool
stu_sisday(stime_t ls, stime_t rs) {
	time_t lt, rt;
	// 解析失败直接返回结果
	if (!stu_gettime(ls, &lt, NULL) || !stu_gettime(rs, &rt, NULL))
		return false;

	return stu_tisday(lt, rt);
}

/*
 * 判断当前时间戳是否是同一周的.可以优化
 * ls : 判断时间一
 * rs : 判断时间二
 *    : 返回true表示是同一周, 返回false表示不是
 */
bool
stu_sisweek(stime_t ls, stime_t rs) {
	time_t lt, rt;
	// 解析失败直接返回结果
	if (!stu_gettime(ls, &lt, NULL) || !stu_gettime(rs, &rt, NULL))
		return false;

	return stu_tisweek(lt, rt);
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
	size_t len, tlen;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	t = tv.tv_sec;
	localtime_r(&t, &st);
	len = strftime(tstr, sizeof(stime_t), "%F %X", &st);
	tlen = snprintf(tstr + len, sizeof(stime_t) - len, " %ld", tv.tv_usec / 1000);
	return len + tlen;
}