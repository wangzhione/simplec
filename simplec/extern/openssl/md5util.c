#include <md5util.h>
#include <stdio.h>

// 将16位md5转成32位md5码
static uint8_t * _tomd5s(const uint8_t md5[MD5_DIGEST_LENGTH], md5s_t md5s) {
	int i = 0;
	uint8_t * mds = md5s;
	while (i < MD5_DIGEST_LENGTH) {
		uint8_t x = md5[i++];
		*mds++ = _HEXA(x);
		*mds++ = _HEXB(x);
	}
	*mds = '\0';
	return md5s;
}

//
// md5_strs - 得到内存块的md5值, 并返回
// d		: 内存块首地址
// n		: 内存块长度
// md5s		: 返回的数据
// return	: 返回md5s 首地址
//
inline uint8_t * 
md5_strs(const uint8_t * d, size_t n, md5s_t md5s) {
	uint8_t md5[MD5_DIGEST_LENGTH];
	MD5(d, n, md5);
	return _tomd5s(md5, md5s);
}

//
// md5_strs - 得到内存块的md5值, 并返回
// file		: 文件路径
// md5s		: 返回的数据
// return	: 返回md5s 首地址, 失败返回NULL
//
uint8_t * 
md5_file(const char * file, md5s_t md5s) {
	MD5_CTX ctx;
	uint8_t ts[BUFSIZ];
	uint8_t md5[MD5_DIGEST_LENGTH];
	FILE * txt = fopen(file, "rb");
	if (NULL == txt)
		return NULL;

	MD5_Init(&ctx);
	fread(ts, sizeof(uint8_t), BUFSIZ, txt);
	MD5_Update(&ctx, ts, BUFSIZ);
	MD5_Final(md5, &ctx);

	fclose(txt);
	return _tomd5s(md5, md5s);
}