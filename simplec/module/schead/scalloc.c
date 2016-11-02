#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 标识枚举
typedef enum {
	HF_Alloc,
	HF_Free
} header_e;

// 每次申请内存的[16-24]字节额外消耗, 用于记录内存申请情况
struct header {
	header_e flag;		// 当前内存使用的标识
	int line;			// 申请的文件行
	const char * file;	// 申请的文件名
	const char * func;	// 申请的函数名
};

// 内部使用的malloc, 返回内存会用'\0'初始化
void * 
sm_malloc_(size_t sz, const char * file, int line, const char * func) {
	struct header * ptr = malloc(sz + sizeof(struct header));
	// 检查内存分配的结果
	if(NULL == ptr) {
		fprintf(stderr, "_header_get >%s:%d:%s< alloc error not enough memory start fail!\n", file, line, func);
		exit(EXIT_FAILURE);
	}

	ptr->flag = HF_Alloc;
	ptr->line = line;
	ptr->file = file;
	ptr->func = func;
	memset(++ptr, 0, sz);
	return ptr;
}

// 得到申请内存的开头部分, 并检查
static struct header * _header_get(void * ptr, const char * file, int line, const char * func) {
	struct header * node = (struct header *)ptr - 1;
	// 正常情况直接返回
	if(HF_Alloc != node->flag) {	
		// 异常情况, 内存多次释放, 和内存无效释放
		fprintf(stderr, "_header_get free invalid memony flag %d by >%s:%d:%s<\n", node->flag, file, line, func);
		exit(EXIT_FAILURE);
	}
	return node;
}

// 内部使用的realloc
void * 
sm_realloc_(void * ptr, size_t sz, const char * file, int line, const char * func) {
	struct header * node , * buf;
	if(NULL == ptr)
		return sm_malloc_(sz, file, line, func);
	
	// 合理内存分割
	node = _header_get(ptr, file, line, func);
	node->flag = HF_Free;
	// 构造返回内存信息
	buf = realloc(node, sz + sizeof(struct header));
	buf->flag = HF_Alloc;
	buf->line = line;
	buf->file = file;
	buf->func = func;

	return buf + 1;
}

// 内部使用的free, 每次释放都会打印日志信息
void 
sm_free_(void * ptr, const char * file, int line, const char * func) {
	if(NULL !=  ptr) {
		// 得到内存地址, 并且标识一下, 开始释放
		struct header * node = _header_get(ptr, file, line, func);
		node->flag = HF_Free;
		free(node);
	}
}
