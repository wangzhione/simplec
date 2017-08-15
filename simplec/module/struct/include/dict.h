#ifndef _H_SIMPLEC_DICT
#define _H_SIMPLEC_DICT

#include <struct.h>

typedef struct dict * dict_t;

//
// dict_create - 创建一个以C字符串为key的字典
// die		: val 销毁函数
// return	: void
//
extern dict_t dict_create(node_f die);
extern void dict_delete(dict_t d);

//
// dict_set - 设置一个<k, v> 结构
// d		: dict_create 创建的字典对象
// k		: 插入的key, 重复插入会销毁已经插入的
// v		: 插入数据的值
// return	: void
//
extern void dict_set(dict_t d, const char * k, void * v);
extern void dict_die(dict_t d, const char * k);

extern void * dict_get(dict_t d, const char * k);

#endif//_H_SIMPLEC_DICT