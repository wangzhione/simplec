#ifndef _H_SIMPLEC_SCICONV
#define _H_SIMPLEC_SCICONV

#include <iconv.h>
#include <stdbool.h>

//
// iconv for window helper
//		by simplec wz
//

//
// si_isutf8 - 判断当前字符串是否是utf-8编码
// in		: 待检测的字符串
// return	: true表示确实utf8编码, false不是
//
extern bool si_isutf8(const char * in);

//
// si_iconv - 将字符串 in 转码, from 码 -> to 码
// in		: 待转码的字符串 
// len		: 字符数组长度
// from		: 初始编码字符串
// to		: 转成的编码字符串 
// rlen		: 返回转换后字符串长度, 传入NULL表示不需要
// return	: 返回转码后的串, 需要自己销毁 
//
extern char * si_iconv(const char * in, const char * from, const char * to, size_t * rlen);

//
// si_iconv - 将字符串数组in 转码, 最后还是放在in数组中. 
// in		: 字符数组
// from		: 初始编码字符串
// to		: 转成的编码字符串 
// return	: void
// 
extern void si_aconv(char in[], const char * from, const char * to);

//
// si_gbktoutf8 - 将字符串数组in, 转成utf8编码
// in		: 字符数组
// len		: 字符数组长度
// return	: void
//
extern void si_gbktoutf8(char in[]);

//
// si_utf8togbk - 将字符串数组in, 转成gbk编码
// in		: 字符数组
// len		: 字符数组长度
// return	: void
//
extern void si_utf8togbk(char in[]);

#endif