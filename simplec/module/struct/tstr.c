#include <tstr.h>

/*
 * 主要采用jshash 返回计算后的hash值
 * 不冲突率在 80% 左右还可以, 不要传入NULL
 */
unsigned 
tstr_hash(const char * str) {
	unsigned i, h = (unsigned)strlen(str), sp = (h >> 5) + 1;
	unsigned char * ptr = (unsigned char *)str;

	for (i = h; i >= sp; i -= sp)
		h ^= ((h<<5) + (h>>2) + ptr[i-1]);

	return h ? h : 1;
}

/*
 * 这是个不区分大小写的比较函数
 * ls		: 左边比较字符串
 * rs		: 右边比较字符串
 *			: 返回 ls>rs => >0 ; ls = rs => 0 ; ls<rs => <0
 */
int 
tstr_icmp(const char * ls, const char * rs) {
	int l, r;
	if(!ls || !rs)
		return (int)(ls - rs);
	
	do {
		if((l=*ls++)>='a' && l<='z')
			l -= 'a' - 'A';
		if((r=*rs++)>='a' && r<='z')
			r -= 'a' - 'A';
	} while(l && l==r);
	
	return l - r;
}

/*
 * 这个代码是 对 strdup 的再实现, 调用之后需要free
 * str		: 待复制的源码内容
 *			: 返回 复制后的串内容
 */
char * 
tstr_dup(const char * str)
{
	size_t len; 
	char * nstr;
	if (NULL == str)
		return NULL;

	len = sizeof(char) * (strlen(str) + 1);
	nstr = sm_malloc(len);
	// 返回最后结果
	return memcpy(nstr, str, len);
}

//------------------------------------------------简单文本字符串辅助操作----------------------------------

/*
 * tstr_t 的创建函数, 会根据str创建一个 tstr_t 结构的字符串
 * str	: 待创建的字符串
 *		: 返回创建好的字符串,如果创建失败返回NULL
 */
tstr_t 
tstr_new(const char * str) {
	tstr_t tstr = sm_malloc(sizeof(struct tstr));
	tstr_appends(tstr, str);
	return tstr;
}

/*
 * tstr_t 析构函数
 * tstr : tstr_t字符串指针量
 */
inline void 
tstr_delete(tstr_t tstr) {
	if (tstr) {
		sm_free(tstr->str);
		sm_free(tstr);
	}
}

//文本字符串创建的度量值
#define _INT_TSTRING (32)

//简单分配函数,智力一定会分配内存的, len > size的时候调用这个函数
static void _tstr_realloc(tstr_t tstr, int len)
{
	int size = tstr->size;
	for (size = size < _INT_TSTRING ? _INT_TSTRING : size; size < len; size <<= 1)
		;
	//分配内存
	tstr->str = sm_realloc(tstr->str, size);
	tstr->size = size;
}

/*
 *  向简单文本字符串tstr中添加 一个字符c
 * tstr : 简单字符串对象
 * c	: 待添加的字符
 */
void 
tstr_append(tstr_t tstr, int c) {
	//不做安全检查
	int len = tstr->len + 1 + 1; // c + '\0' 而len只指向 字符串strlen长度

	//需要的❀, 需要进行内存分配, 唯一损失
	if (len > tstr->size)
		_tstr_realloc(tstr, len);

	tstr->len = --len;
	tstr->str[len - 1] = c;
	tstr->str[len] = '\0';
}

/*
 *  向简单文本串中添加只读字符串
 * tstr	: 文本串
 * str	: 待添加的素材串
 *		: 返回状态码主要是 _RT_EP _RT_EM
 */
void 
tstr_appends(tstr_t tstr, const char * str) {
	int len;
	if (!tstr || !str || !*str)
		return;

	// 检查内存是否需要重新构建
	len = tstr->len + (int)strlen(str) + 1;
	if (len > tstr->size)
		_tstr_realloc(tstr, len);

	strcpy(tstr->str + tstr->len, str);
	tstr->len = len - 1;
}

/*
 * 复制tstr中内容,得到char *, 需要自己 free释放
 * 假如你要清空tstr_t字符串只需要 设置 len = 0.就可以了
 * tstr	: 待分配的字符串
 *		: 返回分配好的字符串首地址
 */
char * 
tstr_dupstr(tstr_t tstr) {
	char * str;
	if (!tstr || tstr->len <= 0)
		return NULL;
	
	//下面就可以复制了,采用最快的一种方式
	str = sm_malloc(tstr->len + 1);
	return memcpy(str, tstr->str, tstr->len + 1);
}

//------------------------------------------------简单文件辅助操作----------------------------------

/*
 * 简单的文件帮助类,会读取完毕这个文件内容返回,失败返回NULL.
 * 需要事后使用 tstr_delete(ret); 销毁这个字符串对象
 * path	: 文件路径
 *		: 返回创建好的字符串内容,返回NULL表示读取失败
 */
tstr_t 
tstr_file_readend(const char * path) {
	int c;
	tstr_t tstr;
	FILE * txt = fopen(path, "r");
	if (NULL == txt) {
		CERR("fopen r %s is error!", path);
		return NULL;
	}

	//这里创建文本串对象
	tstr = tstr_new(NULL);

	//这里读取文本内容
	while ((c = fgetc(txt)) != EOF)
		tstr_append(tstr, c);

	fclose(txt);//很重要创建了就要释放,否则会出现隐藏的句柄bug
	return tstr;
}

int _tstr_file_writes(const char * path, const char * str, const char * mode) {
	FILE* txt;
	// 检查参数是否有问题
	if (!path || !*path || !str) {
		CERR("check is '!path || !*path || !str'");
		return RT_ErrorParam;
	}

	if ((txt = fopen(path, mode)) == NULL) {
		CERR("fopen mode = '%s', path = '%s' error!", mode, path);
		return RT_ErrorFopen;
	}

	//这里写入信息
	fputs(str, txt);

	fclose(txt);
	return RT_SuccessBase;
}

/*
 * 文件写入,没有好说的, 会返回 RT_SuccessBase | RT_ErrorParam | RT_ErrorFopen
 * path	: 文件路径
 * str	: 待写入的字符串
 *		: 返回操作的结果 见上面枚举
 */
inline int
tstr_file_writes(const char * path, const char * str) {
	return _tstr_file_writes(path, str, "wb");
}

/*
 * 文件追加内容 会返回 RT_SuccessBase | RT_ErrorParam | RT_ErrorFopen
 * path	: 文件路径
 * str	: 待写入的字符串
 *		: 返回操作的结果 见上面枚举
 */
inline int 
tstr_file_appends(const char * path, const char * str) {
	return _tstr_file_writes(path, str, "ab");
}



