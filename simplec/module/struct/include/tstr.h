#ifndef _H_SIMPLEC_TSTR
#define _H_SIMPLEC_TSTR

#include <schead.h>

//------------------------------------------------简单字符串辅助操作----------------------------------

/*
 * 主要采用jshash 返回计算后的hash值
 * 不冲突率在 80% 左右还可以, 不要传入NULL
 */
extern unsigned tstr_hash(const char * str);

/*
 * 这是个不区分大小写的比较函数
 * ls		: 左边比较字符串
 * rs		: 右边比较字符串
 *			: 返回 ls>rs => >0 ; ls = rs => 0 ; ls<rs => <0
 */
extern int tstr_icmp(const char * ls, const char * rs);

/*
 * 这个代码是 对 strdup 的再实现, 调用之后需要free 
 * str		: 待复制的源码内容
 *			: 返回 复制后的串内容
 */
extern char * tstr_dup(const char * str);

//------------------------------------------------简单文本字符串辅助操作----------------------------------

#ifndef _STRUCT_TSTR
#define _STRUCT_TSTR
//简单字符串结构,并定义文本字符串类型tstring
struct tstr {
	char * str;		//字符串实际保存的内容
	int len;		//当前字符串大小
	int size;		//字符池大小
};
typedef struct tstr * tstr_t;
#endif // !_STRUCT_TSTR

//文本串栈上创建内容,不想用那些技巧了,就这样吧
#define TSTR_NEW(var) \
	struct tstr $__##var = { NULL, 0, 0 }, * var = &$__##var;
#define TSTR_DELETE(var) \
	sm_free(var->str)

/*
 * tstr_t 的创建函数, 会根据str创建一个 tstr_t 结构的字符串
 * str	: 待创建的字符串
 *		: 返回创建好的字符串,如果创建失败返回NULL
 */
extern tstr_t tstr_new(const char * str);

/*
 * tstr_t 析构函数
 * tstr : tstr_t字符串指针量
 */
extern void tstr_delete(tstr_t tstr);

/*
 *  向简单文本字符串tstr中添加 一个字符c
 * tstr : 简单字符串对象
 * c	: 待添加的字符
 */
extern void tstr_append(tstr_t tstr, int c);

/*
 *  向简单文本串中添加只读字符串 
 * tstr	: 文本串
 * str	: 待添加的素材串
 */
extern void tstr_appends(tstr_t tstr, const char * str);

/*
 * 复制tstr中内容,得到char *, 需要自己 free释放
 * 假如你要清空tstr_t字符串只需要 设置 len = 0.就可以了
 * tstr	: 待分配的字符串
 *		: 返回分配好的字符串首地址
 */
extern char * tstr_dupstr(tstr_t tstr);

//------------------------------------------------简单文件辅助操作----------------------------------

/*
 * 简单的文件帮助类,会读取完毕这个文件内容返回,失败返回NULL.
 * 需要事后使用 tstr_delete(ret); 销毁这个字符串对象
 * path	: 文件路径
 *		: 返回创建好的字符串内容,返回NULL表示读取失败
 */
extern tstr_t tstr_file_readend(const char * path);

/*
 * 文件写入,没有好说的, 会返回 RT_SuccessBase | RT_ErrorParam | RT_ErrorFopen
 * path	: 文件路径
 * str	: 待写入的字符串
 *		: 返回操作的结果 见上面枚举
 */
extern int tstr_file_writes(const char * path, const char * str);

/*
 * 文件追加内容 会返回 RT_SuccessBase | RT_ErrorParam | RT_ErrorFopen
 * path	: 文件路径
 * str	: 待写入的字符串
 *		: 返回操作的结果 见上面枚举
 */
extern int tstr_file_append(const char * path, const char * str);

#endif // !_H_SIMPLEC_TSTR