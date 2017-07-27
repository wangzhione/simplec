#include <scurl.h>
#include <struct.h>

//
// url_encode - url 编码, 需要自己free
// s		: url串
// len		: url串长度
// nlen		: 返回编码后串长度
// return	: 返回编码后串的首地址
// 
char *
url_encode(const char * s, int len, int * nlen) {
	register uint8_t c;
	uint8_t * to, * st;
	const uint8_t * from, * end;

	DEBUG_CODE({
		if (!s || !*s || len < 0)
			return NULL;
	});

	from = (uint8_t *)s;
	end = (uint8_t *)s + len;
	st = to = (uint8_t *)calloc(3 * len + 1, sizeof(uint8_t));

	while (from < end) {
		c = *from++;
		if (c == ' ') {
			*to++ = '+';
			continue;
		}

		// [a-z] [A-Z] [0-9] [&-./:=?_] 以外字符采用二进制替代
		if ((c < '0' && c != '&' && c != '-' && c != '.' && c != '/') ||
			(c < 'A' && c > '9' && c != ':' && c != '=' && c != '?') ||
			(c > 'Z' && c < 'a' && c != '_') ||
			(c > 'z')) {
			to[0] = '%';
			to[1] = (uint8_t)"0123456789ABCDEF"[c >> 4];
			to[2] = (uint8_t)"0123456789ABCDEF"[c & 15];
			to += 3;
			continue;
		}

		*to++ = c;
	}
	*to = '\0';

	// 返回结果
	if (nlen)
		*nlen = to - st;
	return (char *)st;
}

// 2字节变成16进制数表示
inline static char _htoi(uint8_t * s) {
	int value, c;

	c = s[0];
	// 小写变大写是兼容性写法
	if (islower(c)) c = toupper(c);
	value = (c >= '0' && c <= '9' ? c - '0' : c - 'A' + 10) * 16;

	c = s[1];
	if (islower(c)) c = toupper(c);
	value += (c >= '0' && c <= '9' ? c - '0' : c - 'A' + 10);

	return (char)value;
}


//
// url_decode - url 解码,解码后也是放在s[]中
// s		: 待解码的串
// len		: 解码串长度
// return	: 返回解码串的长度, < 0 表示失败
//
int
url_decode(char s[], int len) {
	char * dest, * data, c;

	DEBUG_CODE({
		if (!s || !*s || len <= 0)
			return ErrParam;
	});

	dest = data = s;
	while (len--) {
		c = *data++;
		// 反向解码
		if (c == '+')
			*dest = ' ';
		else if (c == '%' && len >= 2 && isxdigit(data[0]) && isxdigit(data[1])) {
			*dest = _htoi((uint8_t *)data);
			data += 2;
			len -= 2;
		}
		else
			*dest = c;
		++dest;
	}
	*dest = 0;

	return dest - s;
}