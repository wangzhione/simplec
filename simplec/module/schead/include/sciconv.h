#ifndef _H_SIMPLEC_SCICONV
#define _H_SIMPLEC_SCICONV

#include <stddef.h>
#include <stdbool.h>

//
// * gbk 是 ascii 扩展码
// * LEN(gbk) * 2 + 1 > LEN(utf8) >= LEN(gbk)
//

//
// si_isutf8 - 判断当前字符串是否是utf-8编码
// in		: 待检测的字符串
// return	: true表示确实utf8编码, false不是
//
extern bool si_isutf8(const char * in);

//
// si_gbktoutf8 - 将待转换字符串数组, 转成utf8编码
// strgbk	: gbk字符数组
// strutf8	: utf8保存的字符数组
// cap		: strgbk字符数组最多能够保存多大的串
// return	: void
//
extern void si_gbktoutf8(char strgbk[], char strutf8[]);
extern void si_gbktoutf8s(char strgbk[], size_t cap);

//
// si_utf8togbk - 将待转换字符串数组, 转成gbk编码
// strutf8	: utf8字符数组
// strgbk	: gbk输出数组
// return	: void
//
extern void si_utf8togbk(char strutf8[], char strgbk[]);
extern void si_utf8togbks(char strutf8[]);

#endif // !_H_SIMPLEC_SCICONV