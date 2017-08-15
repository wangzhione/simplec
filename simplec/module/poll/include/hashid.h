#ifndef _H_SIMPLEC_HASHID
#define _H_SIMPLEC_HASHID

#include <assert.h>
#include <stdlib.h>

struct nodeid {
	int id;
	struct nodeid * next;
};

struct hashid {
	int mod;
	int cap;
	int cnt;
	struct nodeid * ids;
	struct nodeid ** hash;
};

static void hashid_init(struct hashid * hi, int max) {
	int hcap = 16;
	assert(hi && max > 1);
	while (hcap < max)
		hcap *= 2;

	hi->mod = hcap - 1;
	hi->cap = max;
	hi->ids = malloc(max * sizeof(struct nodeid));
	hi->cnt = 0;
	for (int i = 0; i < max; ++i) 
		hi->ids[i] = (struct nodeid) { -1, NULL };
	
	hi->hash = calloc(hcap, sizeof(struct hashid *));
}

static void inline hashid_clear(struct hashid * hi) {
	free(hi->ids); 	hi->ids = NULL;
	free(hi->hash); hi->hash = NULL;
	hi->mod = 1;
	hi->cap = 0;
	hi->cnt = 0;
}

static int hashid_lookup(struct hashid * hi, int id) {
	int h = id & hi->mod;
	struct nodeid * c = hi->hash[h];
	while (c) {
		if (c->id == id)
			return c - hi->ids;
		c = c->next;
	}
	return -1;
}

static int hashid_remove(struct hashid * hi, int id) {
	int h = id & hi->mod;
	struct nodeid * c = hi->hash[h];
	if (c == NULL)
		return -1;

	if (c->id == id) {
		hi->hash[h] = c->next;
		goto _clear;
	}
	while (c->next) {
		if (c->next->id == id) {
			struct nodeid * temp = c->next;
			c->next = temp->next;
			c = temp;
			goto _clear;
		}
		c = c->next;
	}

_clear:
	c->id = -1;
	c->next = NULL;
	--hi->cnt;
	return c - hi->ids;
}

static int hashid_insert(struct hashid * hi, int id) {
	struct nodeid * c = NULL;
	for (int i = 0; i < hi->cap; ++i) {
		int idx = (i + id) % hi->cap;
		if (hi->ids[idx].id == -1) {
			c = hi->ids + idx;
			break;
		}
	}
	assert(c && c->next == NULL);

	++hi->cnt;
	c->id = id;
	int h = id & hi->mod;
	if (hi->hash[h])
		c->next = hi->hash[h];
	hi->hash[h] = c;
	return c - hi->ids;
}

static inline int hashid_full(struct hashid * hi) {
	return hi->cnt >= hi->cap;
}

#endif//_H_SIMPLEC_HASHID