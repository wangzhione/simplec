#include <sciconv.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//
// si_isutf8 - 判断当前字符串是否是utf-8编码
// in		: 待检测的字符串
// return	: true表示确实utf8编码, false不是
//
bool 
si_isutf8(const char * in) {
	uint8_t bytes = 0, c;		// bytes 表示编码字节数, utf-8 [1, 6] 字节编码
	bool isascii = true;

	while ((c = *in++)) {
		if ((c & 0x80)) // ascii 码最高位为0, 0xxx xxxx
			isascii = false;

		// 下面计算字节数, 计算字节首部
		if (bytes == 0) { 
			if (c >= 0x80) {
				if (c >= 0xFC && c <= 0xFD)
					bytes = 6;
				else if (c >= 0xF8)
					bytes = 5;
				else if (c >= 0xF0)
					bytes = 4;
				else if (c >= 0xE0)
					bytes = 3;
				else if (c >= 0xC0)
					bytes = 2;
				else	// 异常编码直接返回
					return false;
				--bytes;
			}
		}
		else { // 多字节的非首字节, 应为 10xx xxxx
			if ((c & 0xC0) != 0x80)
				return false;
			--bytes;
		}
	}
	// bytes > 0 违反utf-8规则, isacii == true 表示是ascii编码
	return bytes == 0 && !isascii;
}

//
// si_iconv - 将字符串 in 转码, from 码 -> to 码
// in		: 待转码的字符串 
// len		: 字符数组长度
// from		: 初始编码字符串
// to		: 转成的编码字符串 
// rlen		: 返回转换后字符串长度, 传入NULL表示不需要
// return	: 返回转码后的串, 需要自己销毁 
//
char * 
si_iconv(const char * in, const char * from, const char * to, size_t * rlen) {
	char * buff, * sin, * sout;
	size_t lenin, lenout, len;
	iconv_t ct;

	// 打开iconv 转换对象
	if ((ct = iconv_open(to, from)) == (iconv_t)-1)
		return NULL;
	
	// 构建参数, buff保存最终数据
	len = strlen(in) + 1;
	lenout = len << 1;
	if ((buff = malloc(lenout)) == NULL) {
		iconv_close(ct);
		return NULL;
	}
	sout = buff;
	sin = (char *)in;
	lenin = len + 1;

	// 开始转换
	if (iconv(ct, &sin, &lenin, &sout, &lenout) == -1) {
		free(buff);
		iconv_close(ct);
		return NULL;
	}
	iconv_close(ct);

	// 开始重新构建内存返回数据
	lenout = strlen(buff);
	// 返回数据最终结果数据
	if (rlen)
		*rlen = lenout;

	// 内存缩小一定成功
	return realloc(buff, lenout + 1);;
}

//
// si_iconv - 将字符串数组in 转码, 最后还是放在in数组中. 
// in		: 字符数组
// from		: 初始编码字符串
// to		: 转成的编码字符串 
// return	: void
// 
void 
si_aconv(char in[], const char * from, const char * to) {
	size_t len;
	char * out;
	if (!in || !from || !to)
		return;

	out = si_iconv(in, from, to, &len);
	if (NULL == out)
		return;

	// 开始处理数据构建
	memcpy(in, out, len);
	free(out);
}

//
// si_gbktoutf8 - 将字符串数组in, 转成utf8编码
// in		: 字符数组
// return	: void
//
inline void si_gbktoutf8(char in[]) {
	si_aconv(in, "gbk", "utf-8");
}

//
// si_utf8togbk - 将字符串数组in, 转成gbk编码
// in		: 字符数组
// return	: void
//
inline void si_utf8togbk(char in[]) {
	si_aconv(in, "utf-8", "gbk//IGNORE");
}