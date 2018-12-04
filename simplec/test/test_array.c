#include <schead.h>
#include <array.h>

//简单结构
struct dict {
	char * key;
	char * value;
};

// 单独遍历函数
static int _dict_echo(struct dict * node, void * arg)
{
	printf("[%s]	=> [%s]\n", node->key, node->value);
	return SufBase;
}

// 比较函数
static int _dict_cmp(struct dict * ln, struct dict * rn) {
	return strcmp(ln->key, rn->key);
}

/*
 * 主要测试 array 动态数组模块代码
 */
void test_array(void) {
	struct dict * elem;

	struct dict dicts[] = {
		{ "zero"	, "零" },
		{ "one"		, "一" },
		{ "two"		, "二" },
		{ "three"	, "三" },
		{ "four"	, "四" },
		{ "five"	, "五" },
		{ "six"		, "六" },
		{ "seven"	, "七" },
		{ "eight"	, "八" },
		{ "night"	, "九" },
		{ "night"	, "十" },
	};

	/* Step 1 : 测试堆上array对象 */
	int i = -1, len = LEN(dicts);
	array_t a = array_new(len, sizeof(struct dict));

	// 插入数据
	while (++i < len) {
		elem = array_push(a);
		*elem = dicts[i];
	}

	// 打印数据测试
	puts("----------- start data look at the following:");
	array_each(a, (each_f)_dict_echo, NULL);

	// 排序一下
	array_sort(a, _dict_cmp);

	// 打印数据测试
	puts("----------- sort data look at the following:");
	array_each(a, (each_f)_dict_echo, NULL);

	array_die(a);
}

void test_array_stack(void) {
	struct dict * elem;

	struct dict dicts[] = {
		{ "zero"	, "零" },
		{ "one"		, "一" },
		{ "two"		, "二" },
		{ "three"	, "三" },
		{ "four"	, "四" },
		{ "five"	, "五" },
		{ "six"		, "六" },
		{ "seven"	, "七" },
		{ "eight"	, "八" },
		{ "night"	, "九" },
		{ "night"	, "十" },
	};

	/* Step 1 : 测试堆上array对象 */
	int i = -1, len = LEN(dicts);
	ARRAY_NEW(a, len, sizeof(struct dict));

	// 插入数据
	while (++i < len) {
		elem = array_push(a);
		*elem = dicts[i];
	}

	// 打印数据测试
	puts("----------- start data look at the following:");
	array_each(a, (each_f)_dict_echo, NULL);

	// 排序一下
	array_sort(a, (cmp_f)_dict_cmp);

	// 打印数据测试
	puts("----------- sort data look at the following:");
	for(i=0; i<len; ++i) {
		elem = array_get(a, i);
		_dict_echo(elem, NULL);
	}

	ARRAY_DIE(a);
}