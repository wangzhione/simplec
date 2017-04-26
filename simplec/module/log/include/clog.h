#ifndef _H_SIMPLEC_CLOG
#define _H_SIMPLEC_CLOG

//
// error info debug printf log  
//
#define CL_ERROR(fmt,	...)	CLOG_PRINTF("[ERROR]",	fmt, ##__VA_ARGS__)
#define CL_INFO( fmt,	...)	CLOG_PRINTF( "[INFO]",	fmt, ##__VA_ARGS__)
#if defined(_DEBUG)
#define CL_DEBUG(fmt,	...)	CLOG_PRINTF("[DEBUG]",	fmt, ##__VA_ARGS__)
#else
#define CL_DEBUG(fmt,	...)	/*  (^_−)☆ */
#endif

//
// CLOG_PRINTF - 拼接构建输出的格式串,最后输出数据
// fstr		: 上面 _STR_CLOG_* 相关的宏
// fmt		: 自己要打印的串,必须是双引号包裹. 
// return	: 返回待输出的串详细内容
//
#define CL_PRINTF(fstr, fmt, ...) \
	cl_printf(fstr "[%s:%s:%d]" fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__);

//
// cl_printf - 具体输出日志内容
// fmt		: 必须双引号包裹起来的串
// ...		: 对映fmt参数
// return	: void
//
void cl_printf(fmt, ...);

//
// cl_start - 开启单机日志库
// return	: void
//
#define _STR_CLOG_NAME	"simplec.log"
extern void cl_start(void);

#endif // !_H_SIMPLEC_CLOG
