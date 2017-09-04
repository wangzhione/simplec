#ifndef _H_SIMPLEC_SCJSON
#define _H_SIMPLEC_SCJSON

#include <tstr.h>

// json中几种数据结构和方式定义, 对于程序开发而言最难的还是理解思路(思想 or 业务)
#define CJSON_NULL          (0u << 0)
#define CJSON_FALSE         (1u << 0)
#define CJSON_TRUE          (1u << 1)
#define CJSON_NUMBER        (1u << 2)
#define CJSON_STRING        (1u << 3)
#define CJSON_ARRAY         (1u << 4)
#define CJSON_OBJECT        (1u << 5)

struct cjson {
    struct cjson * next;    // 采用链表结构处理, 放弃二叉树结构, 优化内存
    struct cjson * child;   // type == ( CJSON_ARRAY or CJSON_OBJECT ) 那么 child 就不为空

    unsigned char type;     // 数据类型 CJSON_XXXX, 一个美好的意愿
    char * key;             // json内容那块的 key名称 	
    union {
        char * vs;          // type == CJSON_STRING, 是一个字符串 	
        double vd;          // type == CJSON_NUMBER, 是一个num值, ((int)c->vd) 转成int 或 bool
    };
};

//定义cjson_t json类型
typedef struct cjson * cjson_t;

//
// cjson_getint - 这个宏, 协助我们得到 int 值
// item		: 待处理的目标cjson_t结点
// return	: int
//
#define cjson_getvi(item) ((int)((item)->vd))

//
// cjson_delete - 删除json串内容  
// c		: 待释放json_t串内容
// return	: void
//
extern void cjson_delete(cjson_t c);

//
// cjson_newxxx - 通过特定源, 得到内存中json对象
// str		: 普通格式的串
// tstr		: tstr_t 字符串, 成功后会压缩 tstr_t
// path		: json 文件路径
// return	: 解析好的 json_t对象, 失败为 NULL
//
extern cjson_t cjson_newstr(const char * str);
extern cjson_t cjson_newtstr(tstr_t tstr);
extern cjson_t cjson_newfile(const char * path);

//
// cjson_getlen - 得到当前数组个数
// array	: 待处理的cjson_t数组对象
// return	: 返回这个数组中长度
//
extern size_t cjson_getlen(cjson_t array);

//
// cjson_getxxx - 得到指定的json结点对象
// array	: json数组对象
// idx		: 数组查询索引
// object	: json关联对象
// key		: 具体的key信息
// return	: 返回查询到的json结点
//
extern cjson_t cjson_getarray(cjson_t array, size_t idx);
extern cjson_t cjson_getobject(cjson_t object, const char * key);

// --------------------------------- 下面是 cjson 输出部分的接口 -----------------------------------------

//
// cjson_gett?str - 通过json对象得到输出串
// json		: 模板json内容
// return	: 指定的类型保存json串内容, 需要自己free
//
extern char * cjson_getstr(cjson_t json);
extern tstr_t cjson_gettstr(cjson_t json);

// --------------------------------- 下面是 cjson 构建部分的接口 -----------------------------------------

//
// cjson_newxxx - 创建对应对象
// b		: bool 值
// vd		: double 值
// vs		: string 值
// return	: 返回创建好的对映对象
//
extern cjson_t cjson_newnull(void);
extern cjson_t cjson_newarray(void);
extern cjson_t cjson_newobject(void);
extern cjson_t cjson_newbool(bool b);
extern cjson_t cjson_newnumber(double vd);
extern cjson_t cjson_newstring(const char * vs);

//
// cjson_newtypearray - 按照基础类型, 创建对映类型的数组 cjson对象
// type		: 类型宏
// array	: 源数组对象
// len		: 源数组长度
// return	: 返回创建好的json数组对象
//
extern cjson_t cjson_newtypearray(unsigned char type, const void * array, size_t len);

//
// cjson_detacharray - 在array中分离第idx个索引项内容.
// array	: 待处理的json_t 数组内容
// idx		: 索引内容
// return	: 返回分离的json_t内容
//
extern cjson_t cjson_detacharray(cjson_t array, size_t idx);

//
// cjson_detachobject - 在 object json 中分离 key 的项出去
// object	: 待分离的对象主体内容
// key		: 关联的键
// return	: 返回分离的 object中 key的项json_t
//
extern cjson_t cjson_detachobject(cjson_t object, const char * key);

#endif // !_H_SIMPLEC_SCJSON