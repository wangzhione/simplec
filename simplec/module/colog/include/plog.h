#ifndef _H_SIMPLEC_PLOG
#define _H_SIMPLEC_PLOG

#include <schead.h>

//
// error info debug printf log  
//
#define PL_ERROR(fmt,	...)	PL_PRINTF("[ERROR]",	fmt, ##__VA_ARGS__)
#define PL_INFO( fmt,	...)	PL_PRINTF( "[INFO]",	fmt, ##__VA_ARGS__)
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
