#include <scmd5.h>
#include <stdio.h>
#include <stdint.h>

struct md5ctx {
	uint32_t nl, nh;      /* Number of _bits_ handled mod 2^64 */
	uint32_t a, b, c, d;  /* Scratch buffer a, b, c, d */
	uint8_t in[64];       /* Input data */
};

static uint8_t _md5_padding[64] = {
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* HASH_MD5_F, HASH_MD5_G and HASH_MD5_H are basic MD5 functions: selection, majority, parity */
#define HASH_MD5_F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define HASH_MD5_G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define HASH_MD5_H(x, y, z) ((x) ^ (y) ^ (z))
#define HASH_MD5_I(x, y, z) ((y) ^ ((x) | (~z)))

/* HASH_ROTATE rotates x left n bits */
#define HASH_ROTATE(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* HASH_MD5_FF, HASH_MD5_GG, HASH_MD5_HH, and HASH_MD5_II transformations for */
/* rounds 1, 2, 3, and 4. Rotation is separate from addition to prevent recomputation */
#define HASH_MD5_FF(a, b, c, d, x, s, ac) { \
		(a) += HASH_MD5_F ((b), (c), (d)) + (x) + (uint32_t)(ac); \
		(a) = HASH_ROTATE ((a), (s)); (a) += (b); }

#define HASH_MD5_GG(a, b, c, d, x, s, ac) { \
		(a) += HASH_MD5_G ((b), (c), (d)) + (x) + (uint32_t)(ac); \
		(a) = HASH_ROTATE ((a), (s)); (a) += (b); }

#define HASH_MD5_HH(a, b, c, d, x, s, ac) { \
		(a) += HASH_MD5_H ((b), (c), (d)) + (x) + (uint32_t)(ac); \
		(a) = HASH_ROTATE ((a), (s)); (a) += (b); }

#define HASH_MD5_II(a, b, c, d, x, s, ac) { \
		(a) += HASH_MD5_I ((b), (c), (d)) + (x) + (uint32_t)(ac); \
		(a) = HASH_ROTATE ((a), (s)); (a) += (b); }

/* Constants for transformation */
#define HASH_MD5_S11  7  /* Round 1 */
#define HASH_MD5_S12 12
#define HASH_MD5_S13 17
#define HASH_MD5_S14 22
#define HASH_MD5_S21  5  /* Round 2 */
#define HASH_MD5_S22  9
#define HASH_MD5_S23 14
#define HASH_MD5_S24 20
#define HASH_MD5_S31  4  /* Round 3 */
#define HASH_MD5_S32 11
#define HASH_MD5_S33 16
#define HASH_MD5_S34 23
#define HASH_MD5_S41  6  /* Round 4 */
#define HASH_MD5_S42 10
#define HASH_MD5_S43 15
#define HASH_MD5_S44 21

/* Basic MD5 step. HASH_MD5_Transform buf based on in */
static void _md5_transform(struct md5ctx * ctx, const uint32_t * in) {
	uint32_t a = ctx->a, b = ctx->b, c = ctx->c, d = ctx->d;

	/* Round 1 */
	HASH_MD5_FF(a, b, c, d, in[ 0], HASH_MD5_S11, 3614090360u); /*  1 */
	HASH_MD5_FF(d, a, b, c, in[ 1], HASH_MD5_S12, 3905402710u); /*  2 */
	HASH_MD5_FF(c, d, a, b, in[ 2], HASH_MD5_S13,  606105819u); /*  3 */
	HASH_MD5_FF(b, c, d, a, in[ 3], HASH_MD5_S14, 3250441966u); /*  4 */
	HASH_MD5_FF(a, b, c, d, in[ 4], HASH_MD5_S11, 4118548399u); /*  5 */
	HASH_MD5_FF(d, a, b, c, in[ 5], HASH_MD5_S12, 1200080426u); /*  6 */
	HASH_MD5_FF(c, d, a, b, in[ 6], HASH_MD5_S13, 2821735955u); /*  7 */
	HASH_MD5_FF(b, c, d, a, in[ 7], HASH_MD5_S14, 4249261313u); /*  8 */
	HASH_MD5_FF(a, b, c, d, in[ 8], HASH_MD5_S11, 1770035416u); /*  9 */
	HASH_MD5_FF(d, a, b, c, in[ 9], HASH_MD5_S12, 2336552879u); /* 10 */
	HASH_MD5_FF(c, d, a, b, in[10], HASH_MD5_S13, 4294925233u); /* 11 */
	HASH_MD5_FF(b, c, d, a, in[11], HASH_MD5_S14, 2304563134u); /* 12 */
	HASH_MD5_FF(a, b, c, d, in[12], HASH_MD5_S11, 1804603682u); /* 13 */
	HASH_MD5_FF(d, a, b, c, in[13], HASH_MD5_S12, 4254626195u); /* 14 */
	HASH_MD5_FF(c, d, a, b, in[14], HASH_MD5_S13, 2792965006u); /* 15 */
	HASH_MD5_FF(b, c, d, a, in[15], HASH_MD5_S14, 1236535329u); /* 16 */

	/* Round 2 */
	HASH_MD5_GG(a, b, c, d, in[ 1], HASH_MD5_S21, 4129170786u); /* 17 */
	HASH_MD5_GG(d, a, b, c, in[ 6], HASH_MD5_S22, 3225465664u); /* 18 */
	HASH_MD5_GG(c, d, a, b, in[11], HASH_MD5_S23,  643717713u); /* 19 */
	HASH_MD5_GG(b, c, d, a, in[ 0], HASH_MD5_S24, 3921069994u); /* 20 */
	HASH_MD5_GG(a, b, c, d, in[ 5], HASH_MD5_S21, 3593408605u); /* 21 */
	HASH_MD5_GG(d, a, b, c, in[10], HASH_MD5_S22,   38016083u); /* 22 */
	HASH_MD5_GG(c, d, a, b, in[15], HASH_MD5_S23, 3634488961u); /* 23 */
	HASH_MD5_GG(b, c, d, a, in[ 4], HASH_MD5_S24, 3889429448u); /* 24 */
	HASH_MD5_GG(a, b, c, d, in[ 9], HASH_MD5_S21,  568446438u); /* 25 */
	HASH_MD5_GG(d, a, b, c, in[14], HASH_MD5_S22, 3275163606u); /* 26 */
	HASH_MD5_GG(c, d, a, b, in[ 3], HASH_MD5_S23, 4107603335u); /* 27 */
	HASH_MD5_GG(b, c, d, a, in[ 8], HASH_MD5_S24, 1163531501u); /* 28 */
	HASH_MD5_GG(a, b, c, d, in[13], HASH_MD5_S21, 2850285829u); /* 29 */
	HASH_MD5_GG(d, a, b, c, in[ 2], HASH_MD5_S22, 4243563512u); /* 30 */
	HASH_MD5_GG(c, d, a, b, in[ 7], HASH_MD5_S23, 1735328473u); /* 31 */
	HASH_MD5_GG(b, c, d, a, in[12], HASH_MD5_S24, 2368359562u); /* 32 */

	/* Round 3 */
	HASH_MD5_HH(a, b, c, d, in[ 5], HASH_MD5_S31, 4294588738u); /* 33 */
	HASH_MD5_HH(d, a, b, c, in[ 8], HASH_MD5_S32, 2272392833u); /* 34 */
	HASH_MD5_HH(c, d, a, b, in[11], HASH_MD5_S33, 1839030562u); /* 35 */
	HASH_MD5_HH(b, c, d, a, in[14], HASH_MD5_S34, 4259657740u); /* 36 */
	HASH_MD5_HH(a, b, c, d, in[ 1], HASH_MD5_S31, 2763975236u); /* 37 */
	HASH_MD5_HH(d, a, b, c, in[ 4], HASH_MD5_S32, 1272893353u); /* 38 */
	HASH_MD5_HH(c, d, a, b, in[ 7], HASH_MD5_S33, 4139469664u); /* 39 */
	HASH_MD5_HH(b, c, d, a, in[10], HASH_MD5_S34, 3200236656u); /* 40 */
	HASH_MD5_HH(a, b, c, d, in[13], HASH_MD5_S31,  681279174u); /* 41 */
	HASH_MD5_HH(d, a, b, c, in[ 0], HASH_MD5_S32, 3936430074u); /* 42 */
	HASH_MD5_HH(c, d, a, b, in[ 3], HASH_MD5_S33, 3572445317u); /* 43 */
	HASH_MD5_HH(b, c, d, a, in[ 6], HASH_MD5_S34,   76029189u); /* 44 */
	HASH_MD5_HH(a, b, c, d, in[ 9], HASH_MD5_S31, 3654602809u); /* 45 */
	HASH_MD5_HH(d, a, b, c, in[12], HASH_MD5_S32, 3873151461u); /* 46 */
	HASH_MD5_HH(c, d, a, b, in[15], HASH_MD5_S33,  530742520u); /* 47 */
	HASH_MD5_HH(b, c, d, a, in[ 2], HASH_MD5_S34, 3299628645u); /* 48 */

	/* Round 4 */
	HASH_MD5_II(a, b, c, d, in[ 0], HASH_MD5_S41, 4096336452u); /* 49 */
	HASH_MD5_II(d, a, b, c, in[ 7], HASH_MD5_S42, 1126891415u); /* 50 */
	HASH_MD5_II(c, d, a, b, in[14], HASH_MD5_S43, 2878612391u); /* 51 */
	HASH_MD5_II(b, c, d, a, in[ 5], HASH_MD5_S44, 4237533241u); /* 52 */
	HASH_MD5_II(a, b, c, d, in[12], HASH_MD5_S41, 1700485571u); /* 53 */
	HASH_MD5_II(d, a, b, c, in[ 3], HASH_MD5_S42, 2399980690u); /* 54 */
	HASH_MD5_II(c, d, a, b, in[10], HASH_MD5_S43, 4293915773u); /* 55 */
	HASH_MD5_II(b, c, d, a, in[ 1], HASH_MD5_S44, 2240044497u); /* 56 */
	HASH_MD5_II(a, b, c, d, in[ 8], HASH_MD5_S41, 1873313359u); /* 57 */
	HASH_MD5_II(d, a, b, c, in[15], HASH_MD5_S42, 4264355552u); /* 58 */
	HASH_MD5_II(c, d, a, b, in[ 6], HASH_MD5_S43, 2734768916u); /* 59 */
	HASH_MD5_II(b, c, d, a, in[13], HASH_MD5_S44, 1309151649u); /* 60 */
	HASH_MD5_II(a, b, c, d, in[ 4], HASH_MD5_S41, 4149444226u); /* 61 */
	HASH_MD5_II(d, a, b, c, in[11], HASH_MD5_S42, 3174756917u); /* 62 */
	HASH_MD5_II(c, d, a, b, in[ 2], HASH_MD5_S43,  718787259u); /* 63 */
	HASH_MD5_II(b, c, d, a, in[ 9], HASH_MD5_S44, 3951481745u); /* 64 */

	ctx->a += a;
	ctx->b += b;
	ctx->c += c;
	ctx->d += d;
}

// Set pseudoRandomNumber to zero for RFC MD5 implementation
static inline void _md5_init(struct md5ctx * ctx) {
	ctx->nl = ctx->nh = 0;
	/* Load magic initialization constants */
	ctx->a = 0x67452301;
	ctx->b = 0xefcdab89;
	ctx->c = 0x98badcfe;
	ctx->d = 0x10325476;
}

static void _md5_update(struct md5ctx * ctx, const uint8_t * input, size_t len) {
	uint32_t in[16];
	uint32_t i, ii, mdi;

	/* Compute number of bytes mod 64 */
	mdi = (ctx->nl >> 3) & 0x3F;

	/* Update number of bits */
	if ((ctx->nl + (uint32_t)(len << 3)) < ctx->nl)
		ctx->nh++;
	ctx->nl += (uint32_t)(len << 3);
	ctx->nh += (uint32_t)(len >> 29);

	while (len--) {
		/* Add new character to buffer, increment mdi */
		ctx->in[mdi++] = *input++;

		/* Transform if necessary */
		if (mdi == 0x40) {
			for (i = 0, ii = 0; i < 16; ++i, ii += 4)
				in[i] = (((uint32_t)ctx->in[ii + 3]) << 8 * 3) |
						(((uint32_t)ctx->in[ii + 2]) << 8 * 2) |
						(((uint32_t)ctx->in[ii + 1]) << 8 * 1) |
						(((uint32_t)ctx->in[ii + 0]) << 8 * 0) ;

			_md5_transform(ctx, in);
			mdi = 0;
		}
	}
}

static void _md5_final(struct md5ctx * ctx, uint8_t digest[16]) {
	uint32_t in[16];
	uint32_t mdi, i, ii, padlen;

	/* Save number of bits */
	in[14] = ctx->nl;
	in[15] = ctx->nh;

	/* Compute number of bytes mod 64 */
	mdi = (ctx->nl >> 3) & 0x3F;

	/* Pad out to 56 mod 64 */
	padlen = (mdi < 56) ? (56 - mdi) : (120 - mdi);
	_md5_update(ctx, _md5_padding, padlen);

	/* Append length in bits and transform */
	for (i = 0, ii = 0; i < 14; ++i, ii += 4)
		in[i] = (((uint32_t)ctx->in[ii + 3]) << 8 * 3) |
				(((uint32_t)ctx->in[ii + 2]) << 8 * 2) |
				(((uint32_t)ctx->in[ii + 1]) << 8 * 1) |
				(((uint32_t)ctx->in[ii + 0]) << 8 * 0) ;
	_md5_transform(ctx, in);

	/* Store buffer in digest */
	digest[0 * 4 + 0] = (uint8_t)((ctx->a >> 8 * 0) & 0xFF);
	digest[0 * 4 + 1] = (uint8_t)((ctx->a >> 8 * 1) & 0xFF);
	digest[0 * 4 + 2] = (uint8_t)((ctx->a >> 8 * 2) & 0xFF);
	digest[0 * 4 + 3] = (uint8_t)((ctx->a >> 8 * 3) & 0xFF);

	digest[1 * 4 + 0] = (uint8_t)((ctx->b >> 8 * 0) & 0xFF);
	digest[1 * 4 + 1] = (uint8_t)((ctx->b >> 8 * 1) & 0xFF);
	digest[1 * 4 + 2] = (uint8_t)((ctx->b >> 8 * 2) & 0xFF);
	digest[1 * 4 + 3] = (uint8_t)((ctx->b >> 8 * 3) & 0xFF);

	digest[2 * 4 + 0] = (uint8_t)((ctx->c >> 8 * 0) & 0xFF);
	digest[2 * 4 + 1] = (uint8_t)((ctx->c >> 8 * 1) & 0xFF);
	digest[2 * 4 + 2] = (uint8_t)((ctx->c >> 8 * 2) & 0xFF);
	digest[2 * 4 + 3] = (uint8_t)((ctx->c >> 8 * 3) & 0xFF);

	digest[3 * 4 + 0] = (uint8_t)((ctx->d >> 8 * 0) & 0xFF);
	digest[3 * 4 + 1] = (uint8_t)((ctx->d >> 8 * 1) & 0xFF);
	digest[3 * 4 + 2] = (uint8_t)((ctx->d >> 8 * 2) & 0xFF);
	digest[3 * 4 + 3] = (uint8_t)((ctx->d >> 8 * 3) & 0xFF);
}

// 将16位md5转成32位md5码
static uint8_t * _convmd5s(const uint8_t md5[16], md5s_t md5s) {
	int i = 0;
	uint8_t * mds = md5s;
	while (i < 16) {
		uint8_t x = md5[i++];
		*mds++ = (uint8_t)"0123456789ABCDEF"[x >> 4];
		*mds++ = (uint8_t)"0123456789ABCDEF"[x & 15];
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
md5_strs(const void * d, size_t n, md5s_t md5s) {
	uint8_t md5[16];
	struct md5ctx ctx;

	_md5_init(&ctx);
	_md5_update(&ctx, d, n);
	_md5_final(&ctx, md5);

	return _convmd5s(md5, md5s);
}

//
// md5_strs - 得到内存块的md5值, 并返回
// file		: 文件路径
// md5s		: 返回的数据
// return	: 返回md5s 首地址, 失败返回NULL
//
uint8_t * 
md5_file(const char * file, md5s_t md5s) {
	size_t len;
	struct md5ctx ctx;
	uint8_t md5[16], ts[BUFSIZ];
	FILE * txt = fopen(file, "rb");
	if (NULL == txt)
		return NULL;

	_md5_init(&ctx);
	do {
		len = fread(ts, sizeof(uint8_t), BUFSIZ, txt);
		_md5_update(&ctx, ts, len);
	} while (len == BUFSIZ);
	_md5_final(&ctx, md5);

	fclose(txt);
	return _convmd5s(md5, md5s);
}