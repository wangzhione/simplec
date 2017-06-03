#ifndef _H_SIMPLEC_MD5UTIL
#define _H_SIMPLEC_MD5UTIL

#include <scurl.h>
#include <openssl/md5.h>

//
// 采用C字符串格式16位md5串, 最后一位 '\0'
//
typedef uint8_t md5s_t[2 * MD5_DIGEST_LENGTH + 1];

//
// md5_strs - 得到内存块的md5值, 并返回
// d		: 内存块首地址
// n		: 内存块长度
// md5s		: 返回的数据
// return	: 返回md5s 首地址
//
extern uint8_t * md5_strs(const uint8_t * d, size_t n, md5s_t md5s);

//
// md5_strs - 得到内存块的md5值, 并返回
// file		: 文件路径
// md5s		: 返回的数据
// return	: 返回md5s 首地址, 失败返回NULL
//
extern uint8_t * md5_file(const char * file, md5s_t md5s);

#endif // !_H_SIMPLEC_MD5UTIL