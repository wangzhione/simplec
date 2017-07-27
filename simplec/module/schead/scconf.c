#include <scconf.h>
#include <tstr.h>

// 主配置单例对象
static dict_t _mcnf;

//
// mcnf_xxx 启动系统主配置, 得到配置中值
// key		: 待查询的key
// return	: 返回要的对象, 创建的或查询的
//
inline void
mcnf_start(void) {
	if (!_mcnf) {
		_mcnf = conf_create(_STR_MCNFPATH);
		if (!_mcnf) {
			CERR_EXIT("conf_create is open error = " _STR_MCNFPATH);
		}
		// 最终销毁主程序对象数据, 交给操作系统
	}
}

inline const char *
mcnf_get(const char * key) {
	return conf_get(_mcnf, key);
}

// 函数创建函数, kv 是 [ abc\012345 ]这样的结构
static void _sconf_create(dict_t conf, char * key) {
	char * value = key;
	while (*value++)
		;
	dict_set(conf, key, strdup(value));
}

// 将这一行读取完毕
#define READBR(txt, c) \
	while (c != EOF && c != '\n') \
		c = fgetc(txt)

// 开始解析串
static void _sconf_parse(dict_t root, FILE * txt) {
	char c, n;
	TSTR_CREATE(tstr);

	//这里处理读取问题
	while ((c = fgetc(txt)) != EOF) {
		//1.0 先跳过空白字符
		while (c != EOF && isspace(c))
			c = fgetc(txt);

		//2.0 如果遇到第一个字符不是 '$'
		if (c != '$') { 
			READBR(txt, c);
			continue;
		}
		//2.1 第一个字符是 $ 合法字符, 开头不能是空格,否则也读取完毕
		if ((c = fgetc(txt)) != EOF && isspace(c)) {
			READBR(txt, c);
			continue;
		}

		//开始记录了
		tstr->len = 0;

		//3.0 找到第一个等号 
		while (c != EOF && c != '=') {
			if(!isspace(c))
				tstr_appendc(tstr, c);
			c = fgetc(txt);
		}
		if (c != '=') // 无效的解析直接结束
			break;

		c = '\0';
		//4.0 找到 第一个 "
		while (c != EOF && c != '\"') {
			if (!isspace(c))
				tstr_appendc(tstr, c);
			c = fgetc(txt);
		}
		if (c != '\"') // 无效的解析直接结束
			break;

		//4.1 寻找第二个等号
		for (n = c; (c = fgetc(txt)) != EOF; n = c) {
			if (c == '\"' ) {
				if (n != '\\')
					break;
				// 回退一个 '\\' 字符
				--tstr->len;
			}
			tstr_appendc(tstr, c);
		}
		if (c != '\"') //无效的解析直接结束
			break;

		// 这里就是合法字符了,开始检测 了, 
		_sconf_create(root, tstr_cstr(tstr));

		// 最后读取到行末尾
		READBR(txt, c);
		if (c != '\n')
			break;
	}

	TSTR_DELETE(tstr);
}

//
// conf_xxxx 得到配置写对象, 失败都返回NULL 
// path		: 配置所在路径
// conf		: conf_create 创建的对象
// key		: 查找的key
// return	: 返回要得到的对象, 失败为NULL 
//

dict_t 
conf_create(const char * path) {
	dict_t conf;
	FILE * txt = fopen(path, "rb");
	if (NULL == txt) {
		RETURN(NULL, "fopen  r is error! path = %s.", path);
	}

	// 创建具体配置二叉树对象
	conf = dict_create(free);
	if (conf) {
		// 解析添加具体内容
		_sconf_parse(conf, txt);
	}

	fclose(txt);
	return conf;
}

inline void 
conf_delete(dict_t conf) {
	dict_delete(conf);
}

inline const char * 
conf_get(dict_t conf, const char * key) {
	return dict_get(conf, key);
}