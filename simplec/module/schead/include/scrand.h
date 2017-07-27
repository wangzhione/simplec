#ifndef _H_SIMPLEC_SCRAND
#define _H_SIMPLEC_SCRAND

#include <stdint.h>

//
// sh_srand - 初始化随机数种子, (int32_t)time(NULL)
// seed		: 种子数
// return	: void
//
extern void sh_srand(int32_t seed);

//
// sh_rand  - 得到[0, INT32_MAX]随机数
// sh_rands - 得到[min, max] 范围内随机数
// sh_randk - 得到一个64位的key
//
extern int32_t sh_rand(void);
extern int32_t sh_rands(int32_t min, int32_t max);
extern int64_t sh_randk(void);

#endif//_H_SIMPLEC_SCRAND