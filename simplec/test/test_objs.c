#include <objs.h>

static void _return_void(void) {
	static int _cnt = 198;
	RETURN(NIL, "美丽的泡沫, 全都是泡沫 = %d.", _cnt);
}

//
// 测试一下对象池和普通宏的用法
//
void test_objs(void) {
	
	objs_t pool = objs_create(sizeof(int), 4);

	_return_void();

	// 得到5个对象
	for (size_t i = 0; i < 6; ++i) {
		int * ptr = objs_malloc(pool);
		*ptr = i;
		printf("ptr = %p -> %d.\n", ptr, *ptr);
		objs_free(pool, ptr);
	}

	objs_delete(pool);
}