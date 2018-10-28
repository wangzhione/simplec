#ifndef _H_HAID
#define _H_HAID

#include <assert.h>
#include <stdlib.h>

struct noid {
    struct noid * next;
    int id;
};

struct haid {
    struct noid ** hash;
    struct noid * ids;
    int mod;
    int cap;
    int cnt;
};

static void haid_init(struct haid * h, int max) {
    int cap = sizeof(struct haid);
    assert(h && max >= 0);
    while (cap < max)
        cap <<= 1;

    for (int i = 0; i < max; ++i)
        h->ids[i] = (struct noid) { NULL, -1 };
    h->hash = calloc(cap, sizeof(struct haid *));
    assert(h->hash && cap);

    h->ids = malloc(max * sizeof(struct noid));
    assert(h->ids && max);

    h->mod = cap - 1;
    h->cap = max;
    h->cnt = 0;
}

static inline void haid_clear(struct haid * h) {
    free(h->hash); h->hash = NULL;
    free(h->ids); h->ids = NULL;
    h->mod = 1;
    h->cnt = h->cap = 0;
}

static int haid_lookup(struct haid * h, int id) {
    struct noid * c = h->hash[id & h->mod];
    while (c) {
        if (c->id == id)
            return c - h->ids;
        c = c->next;
    }
    return -1;
}

static int haid_remove(struct haid * h, int id) {
    int i = id & h->mod;
    struct noid * c = h->hash[i];
    if (NULL == c)
        return -1;

    if (c->id == id) {
        h->hash[i] = c->next;
        goto out_clr;
    }

    while (c->next) {
        if (c->next->id == id) {
            struct noid * tmp = c->next;
            c->next = tmp->next;
            c = tmp;
            goto out_clr;
        }
        c = c->next;
    }

out_clr:
    c->id = -1;
    c->next = NULL;
    --h->cnt;
    return c - h->ids;
}

static int haid_insert(struct haid * h, int id) {
    int i;
    struct noid * c = NULL;
    for (i = 0; i < h->cap; ++i) {
        int j = (i + id) % h->cap;
        if (h->ids[j].id == -1) {
            c = h->ids + j;
            break;
        }
    }
    assert(c && c->next == NULL);

    ++h->cnt;
    c->id = id;
    i = id & h->mod;
    if (h->hash[i])
        c->next = h->hash[i];
    h->hash[i] = c;
    return c - h->ids;
}

static inline int haid_full(struct haid * h) {
    return h->cnt >= h->cap;
}

#endif//_H_HAID
