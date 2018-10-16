#ifndef _H_SCCONF_SIMPLEC
#define _H_SCCONF_SIMPLEC

#include "tstr.h"
#include "dict.h"

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
// conf_create - 得到配置写对象
// path     : 配置所在路径
// return   : 返回配置对象, NULL is error
//
extern dict_t conf_create(const char * path);

inline const char * conf_get(dict_t conf, const char * key) {
    return dict_get(conf, key);
}

//
// cnf_instance 启动系统主配置, 得到配置中值
// key      : 待查询的 key
// return   : 返回要的对象, 创建的或查询的
//
extern dict_t cnf_instance(void);

inline const char * cnf_get(const char * key) {
    return conf_get(cnf_instance(), key);
}

#endif//_H_SCCONF_SIMPLEC
