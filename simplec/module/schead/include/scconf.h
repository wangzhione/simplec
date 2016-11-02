#ifndef _H_SIMPLEC_SCCONF
#define _H_SIMPLEC_SCCONF

#include <schead.h>

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
 * 后面可以通过, 配置文件读取出来.sc_get("heoo") = > "你好!"
 */

// 定义的不完全类型, 配置处理对象
typedef void * sconf_t;

/*
 * 通过制定配置路径创建解析后对象, 失败返回NULL 
 * path	: 配置所在路径
 *		: 返回解析后的配置对象 
 */
extern sconf_t sconf_new(const char * path);

/*
 * 销毁sconf_new返回的对象
 */
extern void sconf_delete(sconf_t conf);

/*
 * 得到配置中具体数据
 * conf	: sconf_new 返回的配置对象
 * key	: 具体键值
 *		: 成功得到具体配置的串, 失败或不存在返回NULL
 */
extern const char * sconf_get(sconf_t conf, const char * key);

// 主配置系统 --- 具体配置路径
#define _STR_MCCONF_PATH "./config/config.ini"

/*
 * 启动主配置系统, 只能在系统启动的时候执行一次
 *		: 返回创建好的主配置对象
 */
extern sconf_t mconf_start(void);

/*
 * 得到主配置对象中配置的配置, 必须在程序启动时候先执行 mconf_new
 * key	: 主配置的key值
 *		: 得到主配置中配置的数据
 */
extern const char * mconf_get(const char * key);

#endif // !_H_SIMPLEC_SCCONF