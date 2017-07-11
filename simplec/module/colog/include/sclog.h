#ifndef _H_SIMPLE_SCLOG
#define _H_SIMPLE_SCLOG

#include <struct.h>
#include <sctime.h>

//-------------------------------------------------------------------------------------------|
// 第一部分 共用的参数宏
//-------------------------------------------------------------------------------------------|

//
// 关于日志切分,需要用第三方插件例如crontab , 或者下次我自己写一个监测程序.
#define _INT_LITTLE			(64)						//保存时间或IP长度

#define _STR_SCLOG_LOG		_STR_LOGDIR "/sc.log"		//普通log日志 DEBUG,INFO,NOTICE,WARNING,FATAL都会输出
#define _STR_SCLOG_WFLOG	_STR_LOGDIR "/sc_wf.log"	//级别比较高的日志输出 FATAL和WARNING

//
// SL_PRINTF - 为 sl_printf 输出包装的一个模板宏
// fstr		: 只能是""开头文件标识串
// fmt		: 必须是""括起来的宏.单独输出的格式宏
// ...		: 对映fmt参数集
// return	: void
//
#define SL_PRINTF(fstr, fmt, ...) \
	sl_printf("[" fstr "][%s:%d:%s][logid:%u][reqip:%s][mod:%s]" fmt "\n", \
		__FILE__, __LINE__, __func__, sl_getlogid(), sl_getreqip(), sl_getmod(), ##__VA_ARGS__)


//
// SL_FATAL - 错误,后端使用
// SL_WARNG - 警告,前端使用错误,用这个
// SL_NOTIE - 系统使用,一般标记一条请求完成,使用这个日志
// SL_INFOS - 普通的日志打印
// SL_TRACE - 测试用的日志标记当前日志的开始和结束
// SL_DEBUG - 测试用的日志打印,在发布版这些日志会被清除掉
//
// fmt		: 输出的格式串,需要""包裹起来
// ...		: 后面的参数,服务于fmt
// return	: void
//
#define SL_FATAL(fmt,	...)	SL_PRINTF("FATAL",		fmt, ##__VA_ARGS__)
#define SL_WARNG(fmt,	...)	SL_PRINTF("WARNG",		fmt, ##__VA_ARGS__)
#define SL_NOTIE(fmt,	...)	SL_PRINTF("NOTIE",		fmt, ##__VA_ARGS__)
#define SL_INFOS(fmt,	...)	SL_PRINTF("INFOS",		fmt, ##__VA_ARGS__)

// 发布状态下,关闭SL_DEBUG 宏,需要重新编译,没有改成运行时的判断,这个框架主要围绕单机部分多服务器
#if defined(_DEBUG)
#	define SL_TRACE(fmt, ...)   SL_PRINTF("TRACE",		fmt, ##__VA_ARGS__)
#	define SL_DEBUG(fmt, ...)   SL_PRINTF("DEBUG",		fmt, ##__VA_ARGS__)
#else
#	define SL_TRACE(fmt, ...)    /* 人生难道就是123*/
#	define SL_DEBUG(fmt, ...)	 /* 爱过哎 */
#endif

//-------------------------------------------------------------------------------------------|
// 第二部分 对日志信息体操作的get和set,这里隐藏了信息体的实现
//-------------------------------------------------------------------------------------------|

/**
*	线程的私有数据初始化
**
** mod		: 当前线程名称
** reqip	: 请求的ip
** return	: Success_Base 表示正常, Error_Alloc内存分配错误
**/
extern int sl_init(const char mod[_INT_LITTLE], const char reqip[_INT_LITTLE]);

/**
*	获取日志信息体的唯一的logid
**/
extern unsigned sl_getlogid(void);

/**
*	获取日志信息体的请求ip串,返回NULL表示没有初始化
**/
extern const char * sl_getreqip(void);

/**
*	获取日志信息体的名称,返回NULL表示没有初始化
**/
extern const char * sl_getmod(void);


//-------------------------------------------------------------------------------------------|
// 第三部分 对日志系统具体的输出输入接口部分
//-------------------------------------------------------------------------------------------|

/**
*	日志系统首次使用初始化,找对对映日志文件路径,创建指定路径
**/
extern void sl_start(void);

/**
*	这个函数不希望你使用,是一个内部限定死的日志输出内容.推荐使用相应的宏
** 打印相应级别的日志到对映的文件中.
**
**	format		: 必须是""号括起来的宏,开头必须是 [FALTAL:%s]后端错误
**				[WARNING:%s]前端错误, [NOTICE:%s]系统使用, [INFO:%s]普通信息,
**				[DEBUG:%s] 开发测试用
**
** return		: 返回操作结果
**/
void sl_printf(const char * format, ...);

#endif // !_H_SIMPLE_SCLOG