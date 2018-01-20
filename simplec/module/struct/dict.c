#include <dict.h>
#include <tstr.h>

#define _UINT_INITSZ	(1 << 8)

struct keypair {
	struct keypair * next;
	unsigned hash;
	char * key;
	void * val;
};

struct dict {
	node_f die;
	unsigned used; // 用户使用的
	unsigned size; // table 的 size, 等同于桶数目
	struct keypair ** table;
};

//
// dict_create - 创建一个以C字符串为key的字典
// die		: val 销毁函数
// return	: void
//
dict_t 
dict_create(node_f die) {
	struct dict * d = malloc(sizeof(struct dict));
	if (NULL == d) {
		RETURN(NULL, "malloc sizeof struct dict is error!");
	}

	d->table = calloc(_UINT_INITSZ, sizeof(struct keypair *));
	if (NULL == d->table) {
		free(d);
		RETURN(NULL, "calloc sizeof(struct keypair) is error!");
	}

	d->die = die;
	d->used = 0;
	d->size = _UINT_INITSZ;

	return d;
}

// 销毁数据
static inline void _keypair_delete(struct dict * d, struct keypair * pair) {
	free(pair->key);
	if (pair->val && d->die)
		d->die(pair->val);
	free(pair);
}

void 
dict_delete(dict_t d) {
	if (NULL == d)
		return;

	for (unsigned i = 0; i < d->size; ++i) {
		struct keypair * pair = d->table[i];
		while (pair) {
			struct keypair * next = pair->next;
			_keypair_delete(d, pair);
			pair = next;
		}
	}
	free(d->table);
	free(d);
}

// 重新调整hash表大小
static void _dict_resize(dict_t d) {
	unsigned size = d->size;
	unsigned used = d->used;
	struct keypair ** table;

	if (used < size)
		return;
	
	// 开始构建新内存
	do size <<= 1; while (size > used);
	table = calloc(size, sizeof(struct keypair *));
	if (NULL == table) {
		RETURN(NIL, "_dict_resize struct keypair * size = %u.", size);
	}

	// 开始转移数据
	for (unsigned i = 0; i < d->size; ++i) {
		struct keypair * pair = d->table[i];
		while (pair) {
			struct keypair * next = pair->next;
			unsigned idx = pair->hash & (size - 1);

            pair->next = table[idx];
            table[idx] = pair;
			pair = next;
		}
	}
	free(d->table);

	d->size = size;
	d->table = table;
}

static inline struct keypair * _keypair_create(const char * k, void * v, unsigned hash) {
	struct keypair * pair = malloc(sizeof(struct keypair));
	if (pair) {
		pair->hash = hash;
		pair->key = strdup(k);
		pair->val = v;
	}
	return pair;
}

//
// dict_set - 设置一个<k, v> 结构
// d		: dict_create 创建的字典对象
// k		: 插入的key, 重复插入会销毁已经插入的
// v		: 插入数据的值
// return	: void
//
void 
dict_set(dict_t d, const char * k, void * v) {
	unsigned hash, idx;
	struct keypair * pair;
	assert(d != NULL && k != NULL);
	
	// 检查一下内存, 看是否需要重构
	_dict_resize(d);

	// 开始插入数据
	hash = tstr_hash(k);
	idx = hash & (d->size - 1);
	pair = d->table[idx];

	// 数据 modify
	while (pair) {
		if (pair->hash == hash && !strcmp(pair->key, k)) {
            if (pair->val == v)
                return;
			if (d->die)
				d->die(pair->val);
			pair->val = v;
			return;
		}
		pair = pair->next;
	}

	// 没有找见直接创建数据
	pair = _keypair_create(k, v, hash);
	if (pair) {
		++d->used;
		pair->next = d->table[idx];
		d->table[idx] = pair;
	}
}

void 
dict_die(dict_t d, const char * k) {
	unsigned hash, idx;
	struct keypair * pair, * front;
	if (!d || !k)
		return;

	hash = tstr_hash(k);
	idx = hash & (d->size - 1);
	pair = d->table[idx];

	front = NULL;
	while (pair) {
		if (pair->hash == hash && !strcmp(pair->key, k)) {
			// 找见数据, 调整结点关系和开始删除
			if (front == NULL)
				d->table[idx] = pair->next;
			else
				front->next = pair->next;
			
			// 删除数据
			_keypair_delete(d, pair);
			--d->used;
			break;
		}
		front = pair;
		pair = pair->next;
	}
}

void * 
dict_get(dict_t d, const char * k) {
	unsigned hash, idx;
	struct keypair * pair;
	assert(d != NULL && k != NULL);

	hash = tstr_hash(k);
	idx = hash & (d->size - 1);
	pair = d->table[idx];

	while (pair) {
		// 开始查找, 找到之后直接返回
		if (!strcmp(pair->key, k))
			return pair->val;
		pair = pair->next;
	}

	return NULL;
}