#include <scconf.h>
#include <tree.h>
#include <tstr.h>
#include <sclog.h>

//简单二叉树结构
struct sconf {
	_TREE_HEAD;
	char * key;
	char * value;
};

// 函数创建函数, kv 是 [ abc\012345 ]这样的结构
static void* _sconf_new(tstr_t tstr) {
	char* ptr; //临时用的变量
	struct sconf * node = sm_malloc(sizeof(struct sconf) + sizeof(char)*(tstr->len+1));
	// 多读书, 剩下的就是伤感, 1% ,不是我,
	node->key = ptr = (char*)node + sizeof(struct sconf);
	memcpy(ptr, tstr->str, tstr->len + 1);
	// 找到第一个分隔点
	while (*ptr++)
		;
	node->value = ptr;

	return node;
}

// 开始添加
static inline int _sconf_acmp(tstr_t tstr, struct sconf * rnode) {
	return strcmp(tstr->str, rnode->key);
}

//查找和删除
static inline int _sconf_gdcmp(const char * lstr, struct sconf * rnode) {
	return strcmp(lstr, rnode->key);
}

static inline void _sconf_del(void * arg) {
	sm_free(arg);
}

/*
 * 销毁sconf_new返回的对象
 */
inline void 
sconf_delete(sconf_t conf) {
	tree_destroy((tree_t *)&conf);
}

// 开始解析串
static void _analysis_start(FILE * txt, tree_t * proot) {
	char c, n;
	TSTR_NEW(tstr);

	//这里处理读取问题
	while ((c = fgetc(txt)) != EOF) {
		//1.0 先跳过空白字符
		while ((c != EOF) && sh_isspace(c))
			c = fgetc(txt);

		//2.0 如果遇到第一个字符不是 '$'
		if (c != '$') { //将这一行读取完毕
			while (c != EOF && c != '\n')
				c = fgetc(txt);
			continue;
		}
		//2.1 第一个字符是 $ 合法字符, 开头不能是空格,否则也读取完毕
		if ((c = fgetc(txt)) != EOF && isspace(c)) {
			while (c != EOF && c != '\n')
				c = fgetc(txt);
			continue;
		}
		//开始记录了
		tstr->len = 0;

		//3.0 找到第一个等号 
		while (c != EOF && c != '=') {
			if(!sh_isspace(c))
				tstr_append(tstr, c);
			c = fgetc(txt);
		}
		if (c != '=') //无效的解析直接结束
			break;

		c = '\0';
		//4.0 找到 第一个 "
		while (c != EOF && c != '\"') {
			if (!sh_isspace(c))
				tstr_append(tstr, c);
			c = fgetc(txt);
		}
		if (c != '\"') //无效的解析直接结束
			break;

		//4.1 寻找第二个等号
		for (n = c; (c = fgetc(txt)) != EOF; n = c) {
			if (c == '\"' ) {
				if (n != '\\')
					break;
				// 回退一个 '\\' 字符
				--tstr->len;
			}
			tstr_append(tstr, c);
		}
		if (c != '\"') //无效的解析直接结束
			break;

		//这里就是合法字符了,开始检测 了, 
		tree_add(proot, tstr);

		//最后读取到行末尾
		while (c != EOF && c != '\n')
			c = fgetc(txt);
		if (c != '\n')
			break;
	}

	TSTR_DELETE(tstr);
}

/*
* 通过制定配置路径创建解析后对象, 失败返回NULL
* path	: 配置所在路径
*		: 返回解析后的配置对象
*/
extern 
sconf_t sconf_new(const char * path) {
	tree_t conf = NULL;
	
	FILE * txt = fopen(path, "rb");
	if (NULL == txt) {
		SL_WARNING("fopen  r is error! path = %s.", path);
		return NULL;
	}

	// 创建具体配置二叉树对象
	conf = tree_create(_sconf_new, _sconf_acmp, _sconf_gdcmp, _sconf_del);
	// 解析添加具体内容
	_analysis_start(txt, &conf);

	fclose(txt);
	return conf;
}

/*
 * 得到配置中具体数据
 * conf	: sconf_new 返回的配置对象
 * key	: 具体键值
 *		: 成功得到具体配置的串, 失败或不存在返回NULL
 */
extern const char * 
sconf_get(sconf_t conf, const char * key) {
	struct sconf * kv = NULL;
	if ((!conf) || (!key) || (!*key) || !(kv = tree_get(conf, key, NULL))) {
		SL_WARNING("conf, key, kv => %p, %p, %p.", conf, key, kv);
		return NULL;
	}
	return kv->value;
}

// 主配置单例对象
static tree_t _mconf;
/*
 * 启动主配置系统, 只能在系统启动的时候执行一次
 *		: 返回创建好的主配置对象
 */
inline sconf_t 
mconf_start(void) {
	return _mconf ? (_mconf) : (_mconf = sconf_new(_STR_MCCONF_PATH));
}

/*
 * 得到主配置对象中配置的配置, 必须在程序启动时候先执行 mconf_new
 * key	: 主配置的key值
 *		: 得到主配置中配置的数据
 */
inline const char * 
mconf_get(const char * key) {
	return sconf_get(_mconf, key);
}