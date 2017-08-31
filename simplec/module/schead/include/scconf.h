#ifndef _H_SIMPLEC_SCCONF
#define _H_SIMPLEC_SCCONF

#include <dict.h>

/*
 * 这里是配置文件读取接口,
 * 写配置,读取配置,需要指定配置的路径
 * 
 * 配置规则 , 参照 php 变量定义.
 * 举例, 文件中可以配置如下:

	 $heoo = "Hello World\n";

	 $yexu = "\"你好吗\",
	 我很好.谢谢!";

	 $end = "coding future 123 runing, ";
 
 * 
 * 后面可以通过, 配置文件读取出来. conf_get("heoo") => "Hello World\n"
 */

//
// mcnf_xxx 启动系统主配置, 得到配置中值
// key		: 待查询的key
// return	: 返回要的对象, 创建的或查询的
//
#define _STR_MCNFPATH	"./config/config.ini"
extern void mcnf_start(void);
extern const char * mcnf_get(const char * key);

//
// conf_xxxx 得到配置写对象, 失败都返回NULL 
// path		: 配置所在路径
// conf		: conf_create 创建的对象
// key		: 查找的key
// return	: 返回要得到的对象, 失败为NULL 
//
extern dict_t conf_create(const char * path);
extern void conf_delete(dict_t conf);
extern const char * conf_get(dict_t conf, const char * key);

#endif // !_H_SIMPLEC_SCCONF