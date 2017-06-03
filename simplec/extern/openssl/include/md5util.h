#ifndef _H_SIMPLEC_MD5UTIL
#define _H_SIMPLEC_MD5UTIL

#include <openssl/md5.h>

#ifndef _STR_HEX16
//
// 16进制使用的串操作
// 1byte = 8bit = 0xa0 | 0xb
// x : 必须是char or unsigned char
//
#define _STR_HEX16	"0123456789ABCDEF"
#define _HEXA(x)	(unsigned char)_STR_HEX16[(unsigned char)(x) >> 4]
#define _HEXB(x)	(unsigned char)_STR_HEX16[(unsigned char)(x) & 15]

#endif // !_STR_HEX16

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