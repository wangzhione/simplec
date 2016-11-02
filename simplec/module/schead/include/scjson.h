#ifndef _H_SIMPLEC_SCJSON
#define _H_SIMPLEC_SCJSON

#include <tstr.h>

// json 中几种数据类型定义 , 对于C而言 最难的是看不见源码,而不是api复杂, 更不是业务复杂
#define _CJSON_FALSE	(0)
#define _CJSON_TRUE		(1)
#define _CJSON_NULL		(2)
#define _CJSON_NUMBER	(3)
#define _CJSON_STRING	(4)
#define _CJSON_ARRAY	(5)
#define _CJSON_OBJECT	(6)

#define _CJSON_ISREF	(256)		//set 时候用如果是引用就不释放了
#define _CJSON_ISCONST	(512)		//set时候用, 如果是const char* 就不释放了

struct cjson {
	struct cjson * next, * prev;
	struct cjson * child; // type == _CJSON_ARRAY or type == _CJSON_OBJECT 那么 child 就不为空

	int type;
	char * key;	// json内容那块的 key名称 	
	char * vs;	// type == _CJSON_STRING, 是一个字符串 	
	double vd;  // type == _CJSON_NUMBER, 是一个num值, ((int)c->vd) 转成int 或 bool
};

//定义cjson_t json类型
typedef struct cjson * cjson_t;

/*
 * 这个宏,协助我们得到 int 值 或 bool 值 
 * 
 * item : 待处理的目标cjson_t结点
 */
#define cjson_getint(item) \
	((int)((item)->vd))

/*
 *  删除json串内容  
 *  c		: 待释放json_t串内容
 */
extern void cjson_delete(cjson_t c);

/*
 * 对json字符串解析返回解析后的结果
 * jstr		: 待解析的字符串
 */
extern cjson_t cjson_newtstr(tstr_t str);

/*
 *	将json文件解析成json内容返回. 需要自己调用 cjson_delete
 * path		: json串路径
 *			: 返回处理好的cjson_t 内容,失败返回NULL
 */
extern cjson_t cjson_newfile(const char * path);

/*
 * 根据 item当前结点的 next 一直寻找到 NULL, 返回个数. 推荐在数组的时候使用
 * array	: 待处理的cjson_t数组对象
 *			: 返回这个数组中长度
 */
extern int cjson_getlen(cjson_t array);

/*
 * 根据索引得到这个数组中对象
 * array	: 数组对象
 * idx		: 查找的索引 必须 [0,cjson_getlen(array)) 范围内
 *			: 返回查找到的当前对象
 */
extern cjson_t cjson_getarray(cjson_t array, int idx);

/*
 * 根据key得到这个对象 相应位置的值
 * object	: 待处理对象中值
 * key		: 寻找的key
 *			: 返回 查找 cjson_t 对象
 */
extern cjson_t cjson_getobject(cjson_t object, const char * key);


// --------------------------------- 下面是 cjson 输出部分的处理代码 -----------------------------------------

/*
 *  这里是将 cjson_t item 转换成字符串内容,需要自己free 
 * item		: cjson的具体结点
 *			: 返回生成的item的json串内容
 */
extern char* cjson_print(cjson_t item);

// --------------------------------- 下面是 cjson 输出部分的辅助代码 -----------------------------------------

/*
 * 创建一个bool的对象 b==0表示false,否则都是true, 需要自己释放 cjson_delete
 * b		: bool 值 最好是 _Bool
 *			: 返回 创建好的json 内容
 */
extern cjson_t cjson_newnull();
extern cjson_t cjson_newbool(int b);
extern cjson_t cjson_newnumber(double vd);
extern cjson_t cjson_newstring(const char * vs);
extern cjson_t cjson_newarray(void);
extern cjson_t cjson_newobject(void);

/*
 * 按照类型,创建 对映类型的数组 cjson对象
 *目前支持 _CJSON_NULL _CJSON_BOOL/FALSE or TRUE , _CJSON_NUMBER, _CJSON_STRING
 * NULL => array 传入NULL, FALSE 使用char[],也可以传入NULL, NUMBER 只接受double, string 只接受char**
 * type		: 类型目前支持 上面几种类型
 * array	: 数组原始数据
 * len		: 数组中元素长度
 *			: 返回创建的数组对象
 */
extern cjson_t cjson_newtypearray(int type, const void * array, int len);

/*
 * 在array中分离第idx个索引项内容.
 * array	: 待处理的json_t 数组内容
 * idx		: 索引内容
 *			: 返回分离的json_t内容
 */
extern cjson_t cjson_detacharray(cjson_t array, int idx);

/*
 * 在object json 中分离 key 的项出去
 * object	: 待分离的对象主体内容
 * key		: 关联的键
 *			: 返回分离的 object中 key的项json_t
 */
extern cjson_t cjson_detachobject(cjson_t object, const char * key);

#endif // !_H_SIMPLEC_SCJSON
