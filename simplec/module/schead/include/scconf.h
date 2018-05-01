#ifndef _H_SIMPLEC_SCCONF
#define _H_SIMPLEC_SCCONF

#include <dict.h>

/*
 * 配置文件读取接口
 * 解析配置, 读取配置, 需要指定配置的路径
 * 
 * 配置规则, 参照 php 变量定义. 举例:

    $heoo = "Hello World\n";

    $yexu = "\"你好吗\",
    我很好.谢谢!";

    $end = "coding future 123 runing, ";

 * 
 * 后面可以通过读取接口获得 : 
    conf_get(xxx, "heoo") => "Hello World\n"
 */

//
// conf_xxx 得到配置写对象, 失败都返回 NULL
// conf     : conf_create 创建的对象
// path     : 配置所在路径
// key      : 查找的key
// return   : 返回要得到的对象, 失败为 NULL
//
extern dict_t conf_create(const char * path);

inline void conf_delete(dict_t conf) {
    dict_delete(conf);
}

inline const char * conf_get(dict_t conf, const char * key) {
    return dict_get(conf, key);
}

#define STR_MCNFPATH    "config/config.ini"

//
// mcnf_xxx 启动系统主配置, 得到配置中值
// key      : 待查询的key
// return   : 返回要的对象, 创建的或查询的
//
extern dict_t mcnf_instance(void);

inline const char * mcnf_get(const char * key) {
    return conf_get(mcnf_instance(), key);
}

#endif // !_H_SIMPLEC_SCCONF