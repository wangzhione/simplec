#ifndef _H_SIMPLEC_PLOG
#define _H_SIMPLEC_PLOG

#include <struct.h>
#include <sctime.h>

#ifndef _STR_PLOG_NAME
#define _STR_PLOG_NAME		_STR_LOGDIR "/simplec-%04d%02d%02d-%02d%02d%02d-%03d.log"
#endif // !_STR_PLOG_NAME

//
// plog 日志库, 相当于传统端游日志. 用到了很多套路, 但......
//	a) 日志分级 b) 消息队列 c) 并发安全 d) 规则构建 e) 线程轮询 f) 对象缓存
// 
// Simple is best, but this is complicated.
//
#define PL_ERROR(fmt,	...)	PL_PRINTF("[ERROR]",	fmt, ##__VA_ARGS__)
#define PL_INFOS(fmt,	...)	PL_PRINTF("[INFOS]",	fmt, ##__VA_ARGS__)
#if defined(_DEBUG)
#define PL_DEBUG(fmt,	...)	PL_PRINTF("[DEBUG]",	fmt, ##__VA_ARGS__)
#else
#define PL_DEBUG(fmt,	...)	/* ( ´﹀` )礼貌的微笑 */
#endif

//
// SL_PRINTF - 拼接构建输出的格式串,最后输出数据
// fstr		: 上面 _STR_CLOG_* 相关的宏
// fmt		: 自己要打印的串,必须是双引号包裹. 
// return	: 返回待输出的串详细内容
//
#define PL_PRINTF(fstr, fmt, ...) \
	pl_printf(fstr "[%s:%s:%d]" fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__);

//
// sl_printf - 具体输出日志内容
// fmt		: 必须双引号包裹起来的串
// ...		: 对映fmt参数
// return	: void
//
void pl_printf(const char * fmt, ...);

//
// sl_start - 开启单机日志库
// return	: void
//
extern void pl_start(void);

#endif // !_H_SIMPLEC_PLOG
