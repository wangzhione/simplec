#include <scjson.h>

//
// cjson_delete - 删除json串内容  
// c		: 待释放json_t串内容
// return	: void
//
void 
cjson_delete(cjson_t c) {
	while (c) {
		cjson_t next = c->next;
		// 放弃引用和常量的优化选项
		free(c->key);
		if (c->type & CJSON_STRING)
			free(c->vs);
		// 递归删除子节点
		if (c->child)
			cjson_delete(c->child);
		free(c);
		c = next;
	}
}

//构造一个空 cjson 对象
static inline cjson_t _cjson_new(void) {
	cjson_t node = malloc(sizeof(struct cjson));
	if (NULL == node)
		EXIT("malloc struct cjson is null!");
	return memset(node, 0, sizeof(struct cjson));
}

// parse 4 digit hexadecimal number
static unsigned _parse_hex4(const char str[]) {
	unsigned c, h = 0, i = 0;
	// 开始转换16进制
	for(;;) {
		c = *str;
		if (c >= '0' && c <= '9')
			h += c - '0';
		else if (c >= 'A' && c <= 'F')
			h += 10 + c - 'A';
		else if (c >= 'a' && c <= 'z')
			h += 10 + c - 'a';
		else
			return 0;
		// shift left to make place for the next nibble
		if (4 == ++i)
			break;
		h <<= 4;
		++str;
	}

	return h;
}

// 分析字符串的子函数,
static const char * _parse_string(cjson_t item, const char * str) {
	static unsigned char _marks[] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
	const char * ptr;
	char c, * nptr, * out;
	unsigned len = 1, uc, nuc;

	// 检查是否是字符串内容, 并记录字符串大小
	if (*str != '\"')
		RETURN(NULL, "need \\\" str => %s error!", str);
	for (ptr = str + 1; (c = *ptr++) != '\"' && c; ++len)
		if (c == '\\') {
			//跳过转义字符
			if (*ptr == '\0')
				RETURN(NULL, "ptr is end len = %d.", len);
			++ptr;
		}
	if (c != '\"')
		RETURN(NULL, "need string \\\" end there c = %d, %c!", c, c);

	// 这里开始复制拷贝内容
	if (!(nptr = out = malloc(len)))
		EXIT("calloc size = %d is error!", len);
	for (ptr = str + 1; (c = *ptr) != '\"' && c; ++ptr) {
		if (c != '\\') {
			*nptr++ = c;
			continue;
		}
		// 处理转义字符
		switch ((c = *++ptr)) {
		case 'b': *nptr++ = '\b'; break;
		case 'f': *nptr++ = '\f'; break;
		case 'n': *nptr++ = '\n'; break;
		case 'r': *nptr++ = '\r'; break;
		case 't': *nptr++ = '\t'; break;
		case 'u': // 将utf16 => utf8, 专门的utf处理代码
			uc = _parse_hex4(ptr + 1);
			ptr += 4; //跳过后面四个字符, unicode
			if (0 == uc || (uc >= 0xDC00 && uc <= 0xDFFF))
				break;	/* check for invalid. */

			if (uc >= 0xD800 && uc <= 0xDBFF) { /* UTF16 surrogate pairs. */
				if (ptr[1] != '\\' || ptr[2] != 'u')	
					break;	/* missing second-half of surrogate. */
				nuc = _parse_hex4(ptr + 3);
				ptr += 6;
				if (nuc < 0xDC00 || nuc>0xDFFF)		
					break;	/* invalid second-half of surrogate.	*/
				uc = 0x10000 + (((uc & 0x3FF) << 10) | (nuc & 0x3FF));
			}

			if (uc < 0x80)
				len = 1;
			else if (uc < 0x800)
				len = 2;
			else if (uc < 0x10000)
				len = 3;
			else
				len = 4;
			nptr += len;

			switch (len) {
			case 4: *--nptr = ((uc | 0x80) & 0xBF); uc >>= 6;
			case 3: *--nptr = ((uc | 0x80) & 0xBF); uc >>= 6;
			case 2: *--nptr = ((uc | 0x80) & 0xBF); uc >>= 6;
			case 1: *--nptr = (uc | _marks[len]);
			}
			nptr += len;
			break;
		default: *nptr++ = c;
		}
	}
	*nptr = '\0';
	item->vs = out;
	item->type = CJSON_STRING;
	return ++ptr;
}

// 分析数值的子函数,写的可以
static const char * _parse_number(cjson_t item, const char * str) {
	double n = .0, ns = 1.0, nd = .0; // ns表示开始正负, 负为-1, nd表示小数后面位数
	int e = 0, es = 1; // e表示后面指数, es表示 指数的正负, 负为-1
	char c;

	if ((c = *str) == '-' || c == '+') {
		ns = c == '-' ? -1.0 : 1.0; // 正负号检测, 1表示负数
		++str;
	}
	// 处理整数部分
	for (c = *str; c >= '0' && c <= '9'; c = *++str)
		n = n * 10 + c - '0';
	if (c == '.')
		for (; (c = *++str) >= '0' && c <= '9'; --nd)
			n = n * 10 + c - '0';

	// 处理科学计数法
	if (c == 'e' || c == 'E') {
		if ((c = *++str) == '+') //处理指数部分
			++str;
		else if (c == '-')
			es = -1, ++str;
		for (; (c = *str) >= '0' && c <= '9'; ++str)
			e = e * 10 + c - '0';
	}

	//返回最终结果 number = +/- number.fraction * 10^+/- exponent
	item->vd = ns * n * pow(10.0, nd + es * e);
	item->type = CJSON_NUMBER;
	return str;
}

// 递归下降分析 需要声明这些函数
static const char * _parse_value(cjson_t item, const char * str);
static const char * _parse_array(cjson_t item, const char * str);
static const char * _parse_object(cjson_t item, const char * str);

// 分析数组的子函数, 采用递归下降分析
static const char * 
_parse_array(cjson_t item, const char * str) {
	cjson_t child;

	if (*str != '[') {
		RETURN(NULL, "array str error start: %s.", str);
	}

	item->type = CJSON_ARRAY;
	if (*++str == ']') // 低估提前结束, 跳过']'
		return str + 1;

	item->child = child = _cjson_new();
	str = _parse_value(child, str);
	if (NULL == str) {
		RETURN(NULL, "array str error e n d one: %s.", str);
	}

	while (*str == ',') {
		// 支持行尾处理多余 ','
		if (str[1] == ']')
			return str + 2;

		// 写代码是一件很爽的事
		child->next = _cjson_new();
		child = child->next;
		str = _parse_value(child, str + 1);
		if (NULL == str) {
			RETURN(NULL, "array str error e n d two: %s.", str);
		}
	}

	if (*str != ']') {
		RETURN(NULL, "array str error e n d: %s.", str);
	}
	return str + 1;
}

// 分析对象的子函数
static const char * 
_parse_object(cjson_t item, const char * str) {
	cjson_t child;

	if (*str != '{') {
		RETURN(NULL, "object str error start: %s.", str);
	}

	item->type = CJSON_OBJECT;
	if (*++str == '}')
		return str + 1;

	//处理结点, 开始读取一个 key
	item->child = child = _cjson_new();
	str = _parse_string(child, str);
	if (!str || *str != ':') {
		RETURN(NULL, "_parse_string is error : %s!", str);
	}
	child->key = child->vs;

	child->vs = NULL;
	str = _parse_value(child, str + 1);
	if (!str) {
		RETURN(NULL, "_parse_value is error 2!");
	}

	// 递归解析
	while (*str == ',') {
		// 支持行尾处理多余 ','
		if (str[1] == '}')
			return str + 2;

		child->next = _cjson_new();
		child = child->next;
		str = _parse_string(child, str + 1);
		if (!str || *str != ':'){
			RETURN(NULL, "_parse_string need name or no equal ':' %s.", str);
		}
		child->key = child->vs;

		child->vs = NULL;
		str = _parse_value(child, str+1);
		if (!str) {
			RETURN(NULL, "_parse_value need item two ':' %s.", str);
		}
	}

	if (*str != '}') {
		RETURN(NULL, "object str error e n d: %s.", str);
	}
	return str + 1;
}

// 将 value 转换塞入 item json 值中一部分
static const char * 
_parse_value(cjson_t item, const char * str) {
	char c = '\0'; 
	if ((str) && (c = *str)) {
		switch (c) {
		// n = null, f = false, t = true ... 
		case 'n' : return item->type = CJSON_NULL, str + 4;
		case 'f' : return item->type = CJSON_FALSE, str + 5;
		case 't' : return item->type = CJSON_TRUE, item->vd = 1.0, str + 4;
		case '\"': return _parse_string(item, str);
		case '0' : case '1' : case '2' : case '3' : case '4' : case '5' :
		case '6' : case '7' : case '8' : case '9' :
		case '+' : case '-' : case '.' : return _parse_number(item, str);
		case '[' : return _parse_array(item, str);
		case '{' : return _parse_object(item, str);
		}
	}
	// 循环到这里是意外 数据
	RETURN(NULL, "params value = %c, %s!", c, str);
}

//  将 jstr中 不需要解析的字符串都去掉, 返回压缩后串的长度. 并且纪念mini 比男的还平
static size_t _cjson_mini(char * jstr) {
	char c, * in = jstr, * to = jstr;

	while ((c = *to)) {
		// step 1 : 处理字符串
		if (c == '"') {
			*in++ = c;
			while ((c = *++to) && (c != '"' || to[-1] == '\\'))
				*in++ = c;
			if (c) {
				*in++ = c;
				++to;
			}
			continue;
		}
		// step 2 : 处理不可见特殊字符
		if (c < '!') {
			++to;
			continue;
		}
		if (c == '/') {
			// step 3 : 处理 // 解析到行末尾
			if (to[1] == '/') {
				while ((c = *++to) && c != '\n')
					;
				continue;
			}

			// step 4 : 处理 /*
			if (to[1] == '*') {
				while ((c = *++to) && (c != '*' || to[1] != '/'))
					;
				if (c)
					to += 2;
				continue;
			}
		}
		// step 5 : 合法数据直接保存
		*in++ = *to++;
	}

	*in = '\0';
	return in - jstr;
}

// jstr 必须是 _cjson_mini 解析好的串
static cjson_t _cjson_parse(const char * jstr) {
	const char * end;
	cjson_t json = _cjson_new();

	if (!(end = _parse_value(json, jstr))) {
		cjson_delete(json);
		RETURN(NULL, "_parse_value params end = %s!", end);
	}

	return json;
}

//
// cjson_newxxx - 通过特定源, 得到内存中json对象
// str		: 普通格式的串
// tstr		: tstr_t 字符串, 成功后会压缩 tstr_t
// path		: json 文件路径
// return	: 解析好的 json_t对象, 失败为NULL
//
inline cjson_t 
cjson_newstr(const char * str) {
	cjson_t json;
	TSTR_CREATE(tstr);
	tstr_appends(tstr, str);

	_cjson_mini(tstr->str);
	json = _cjson_parse(tstr->str);

	TSTR_DELETE(tstr);
	return json;
}

inline cjson_t 
cjson_newtstr(tstr_t tstr) {
	tstr->len = _cjson_mini(tstr->str);
	return _cjson_parse(tstr->str);
}

cjson_t 
cjson_newfile(const char * path) {
	cjson_t json;
	tstr_t tstr = tstr_freadend(path);
	if (!tstr) {
		RETURN(NULL, "tstr_freadend path:%s is error!", path);
	}

	json = cjson_newtstr(tstr);
	tstr_delete(tstr);
	return json;
}

//
// cjson_getlen - 得到当前数组个数
// array	: 待处理的cjson_t数组对象
// return	: 返回这个数组中长度
//
size_t 
cjson_getlen(cjson_t array) {
	size_t len = 0;
	if (array) {
		for (array = array->child; array; array = array->next)
			++len;
	}
	return len;
}

//
// cjson_getxxx - 得到指定的json结点对象
// array	: json数组对象
// idx		: 数组查询索引
// object	: json关联对象
// key		: 具体的key信息
// return	: 返回查询到的json结点
//
cjson_t 
cjson_getarray(cjson_t array, size_t idx) {
	cjson_t c;

	for (c = array->child; c && idx > 0; c = c->next)
		--idx;

	return c;
}

cjson_t 
cjson_getobject(cjson_t object, const char * key) {
	cjson_t c;
	DEBUG_CODE({
		if (!object || !key || !*key) {
			RETURN(NULL, "object:%p, key=%s params is error!", object, key);
		}
	});

	for (c = object->child; c && tstr_icmp(key, c->key); c = c->next)
		;

	return c;
}

// --------------------------------- 下面是 cjson 输出部分的辅助代码 -----------------------------------------

// 将item 中值转换成字符串 保存到p中
static char * _print_number(cjson_t item, tstr_t p) {
	char * str = NULL;
	double d = item->vd;
	int i = (int)d;
	
	if (d == 0) {  // 普通0
		str = tstr_expand(p, 2);
		str[0] = '0', str[1] = '\0';
	}
	else if ((fabs(d - i)) <= DBL_EPSILON && d <= INT_MAX && d >= INT_MIN) {
		str = tstr_expand(p, 21); // int 值 
		sprintf(str, "%d", i);
	}
	else {
		// 得到正值开始比较
		double nd = fabs(d);
		str = tstr_expand(p, 64);
		if (fabs(floor(d) - d) <= DBL_EPSILON && nd < 1.0e60)
			sprintf(str, "%.0f", d);
		else if (nd < 1.0e-6 || nd > 1.0e9) // 科学计数法
			sprintf(str, "%e", d);
		else
			sprintf(str, "%f", d);
	}

	return str;
}

// 输出字符串内容
static char * _print_string(char * str, tstr_t p) {
	size_t len = 0;
    unsigned char c;
	const char * ptr;
	char * nptr, *out;

	if (!str || !*str) { //最特殊情况,什么都没有 返回NULL
		out = tstr_expand(p, 3);
		out[0] = '\"', out[1] = '\"', out[2] = '\0';
		return out;
	}

	//处理 存在 "和转义字符内容
	for (ptr = str; (c = *ptr) && ++len; ++ptr) {
		if (strchr("\"\\\b\f\n\r\t", c))
			++len;
		else if (c < 32) //隐藏字符的处理, 这里可以改
			len += 5;
	}

	//扩充内存, 然后添加 \"
	nptr = out = tstr_expand(p, len + 3);
	*nptr++ = '\"';

	//没有特殊字符直接处理结果
	if (len == ptr - str) {
		strcpy(nptr, str);
		nptr[len] = '\"';
		nptr[len + 1] = '\0';
		return out;
	}

	for (ptr = str; (c = *ptr); ++ptr) {
		if (c > 31 && c != '\"' && c != '\\') {
			*nptr++ = c;
			continue;
		}
		*nptr++ = '\\';
		switch (c){
		case '\b': *nptr++ = 'b'; break;
		case '\f': *nptr++ = 'f'; break;
		case '\n': *nptr++ = 'n'; break;
		case '\r': *nptr++ = 'r'; break;
		case '\t': *nptr++ = 't'; break;
		case '\\': case '\"': *nptr++ = c; break;
		// 不可见字符 采用4字节字符编码
		default : sprintf(nptr, "u%04x", c); nptr += 5;
		}
	}

	*nptr++ = '\"';
	*nptr = '\0';
	return out;
}

//这里是 递归下降 的函数声明处, 分别是处理值, 数组, object
static char * _print_value(cjson_t item, tstr_t p);
static char * _print_array(cjson_t item, tstr_t p);
static char * _print_object(cjson_t item, tstr_t p);

// 定义实现部分, 内部私有函数 认为 item 和 p都是存在的
static char * _print_value(cjson_t item, tstr_t p) {
	char * out = NULL;
	switch (item->type) {
	case CJSON_FALSE: out = tstr_expand(p, 6); strcpy(out, "false"); break;
	case CJSON_TRUE: out = tstr_expand(p, 5); strcpy(out, "true"); break;
	case CJSON_NULL: out = tstr_expand(p, 5); strcpy(out, "null"); break;
	case CJSON_NUMBER	: out = _print_number(item, p); break;
	case CJSON_STRING	: out = _print_string(item->vs, p); break;
	case CJSON_ARRAY	: out = _print_array(item, p); break;
	case CJSON_OBJECT	: out = _print_object(item, p); break;
	}
	p->len += strlen(p->str + p->len);

	return out;
}

// 同样 假定 item 和 p都是存在且不为NULL
static char * _print_array(cjson_t item, tstr_t p) {
	size_t i;
	char * ptr;
	cjson_t child;

	i = p->len;
	ptr = tstr_expand(p, 1);
	*ptr = '[';
	++p->len;

	for (child = item->child; (child); child = child->next) {
		_print_value(child, p);
		if (child->next) {
			ptr = tstr_expand(p, 2);
			*ptr++ = ',';
			*ptr = '\0';
			p->len += 1;
		}
	}

	ptr = tstr_expand(p, 2);
	*ptr++ = ']';
	*ptr = '\0';
	return p->str + i;

}

// 同样 假定 item 和 p都是存在且不为NULL, 相信这些代码是安全的
static char * _print_object(cjson_t item, tstr_t p) {
	size_t i;
	char * ptr;
	cjson_t child;

	i = p->len;
	ptr = tstr_expand(p, 1);
	*ptr++ = '{';
	++p->len;

	// 根据子结点 处理
	for (child = item->child; (child); child = child->next) {
		_print_string(child->key, p);
		p->len += strlen(p->str + p->len);

		//加入一个冒号
		ptr = tstr_expand(p, 1);
		*ptr++ = ':';
		p->len += 1;

		//继续打印一个值
		_print_value(child, p);

		//结算最后内容
		if (child->next) {
			ptr = tstr_expand(p, 2);
			*ptr++ = ',';
			*ptr = '\0';
			p->len += 1;
		}
	}

	ptr = tstr_expand(p, 2);
	*ptr++ = '}';
	*ptr = '\0';
	return p->str + i;
}

//
// cjson_gett?str - 通过json对象得到输出串
// json		: 模板json内容
// return	: 指定的类型保存json串内容
//
char *
cjson_getstr(cjson_t json) {
	TSTR_CREATE(tstr);
	if (NULL == _print_value(json, tstr)) {
		TSTR_DELETE(tstr);
		RETURN(NULL, "_print_value json is error :%p !", json);
	}
	return realloc(tstr->str, tstr->len + 1);
}

tstr_t
cjson_gettstr(cjson_t json) {
	tstr_t tstr = tstr_creates(NULL);
	if (NULL == _print_value(json, tstr)) {
		tstr_delete(tstr);
		RETURN(NULL, "_print_value json is error :%p !", json);
	}
	return tstr;
}

// --------------------------------- 下面是 cjson 构建部分的辅助代码 -----------------------------------------

static inline cjson_t _cjson_newt(unsigned char type) {
	cjson_t item = _cjson_new();
	item->type = type;
	return item;
}

//
// cjson_newxxx - 创建对应对象
// b		: bool 值
// vd		: double 值
// vs		: string 值
// return	: 返回创建好的对映对象
//
inline cjson_t
cjson_newnull(void) {
	return _cjson_new();
}

inline cjson_t
cjson_newarray(void) {
	return _cjson_newt(CJSON_ARRAY);
}

inline cjson_t
cjson_newobject(void) {
	return _cjson_newt(CJSON_OBJECT);
}

inline cjson_t
cjson_newbool(bool b) {
	cjson_t item = _cjson_newt(1u << (1 + b));
	item->vd = b;
	return item;
}

inline cjson_t
cjson_newnumber(double vd) {
	cjson_t item = _cjson_newt(CJSON_NUMBER);
	item->vd = vd;
	return item;
}

inline cjson_t
cjson_newstring(const char * vs) {
	cjson_t item = _cjson_newt(CJSON_STRING);
	item->vs = tstr_dup(vs);
	return item;
}

//
// cjson_newtypearray - 按照基础类型, 创建对映类型的数组 cjson对象
// type		: 类型宏
// array	: 源数组对象
// len		: 源数组长度
// return	: 返回创建好的json数组对象
//
extern cjson_t cjson_newtypearray(unsigned char type, const void * array, size_t len) {
	size_t i;
	cjson_t n = NULL, p = NULL, a = NULL;

	for(i = 0; i < len; ++i){
		switch(type){
		case CJSON_NULL   : n = cjson_newnull(); break;
		case CJSON_TRUE   : n = cjson_newbool(array ? ((bool *)array)[i] : true); break;
		case CJSON_FALSE  : n = cjson_newbool(array? ((bool *)array)[i] : false); break;
		case CJSON_NUMBER : n = cjson_newnumber(((double *)array)[i]); break;
		case CJSON_STRING : n = cjson_newstring(((char **)array)[i]); break;
		default: RETURN(NULL, "type is error = %d.", type);
		}
		if(i) // 有你更好
			p->next = n;
		else {
			a = cjson_newarray();
			a->child = n;
		}
		p = n;
	}
	return a;
}

//
// cjson_detacharray - 在array中分离第idx个索引项内容.
// array	: 待处理的json_t 数组内容
// idx		: 索引内容
// return	: 返回分离的json_t内容
//
cjson_t 
cjson_detacharray(cjson_t array, size_t idx) {
    cjson_t c, p;
    if ((!array) || !(c = array->child))
        RETURN(NULL, "check param is array:%p, idx:%zu is error!", array, idx);

    // 查找我们要数据
    for (p = NULL; idx > 0 && c; c = c->next) {
        --idx;
        p = c;
    }
    if (NULL == c) {
        RETURN(NULL, "check param is too dig idx:sub %zu.", idx);
    }

    if (NULL == p) 
        array->child = c->next;
    else
        p->next = c->next;
    c->next = NULL;
	return c;
}

//
// cjson_detachobject - 在 object json 中分离 key 的项出去
// object	: 待分离的对象主体内容
// key		: 关联的键
// return	: 返回分离的 object中 key的项json_t
//
cjson_t 
cjson_detachobject(cjson_t object, const char * key) {
    cjson_t c, p;
    if ((!object) || !(c = object->child) || !key || !*key) {
        RETURN(NULL, "check param is object:%p, key:%s.", object, key);
    }
	
    // 开始找到数据
    for (p = NULL; c && tstr_icmp(c->key, key); c = c->next) {
        p = c;
    }
    if(NULL == c) {
        RETURN(NULL, "check param key:%s to empty!", key);
    }

    // 返回最终结果
    if (NULL == p)
        object->child = c->next;
    else
        p->next = c->next;
    c->next = NULL;
    return c;
}
