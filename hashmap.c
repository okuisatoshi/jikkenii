#include <stdlib.h>
#include <string.h>
#include "tidwall/tidwall_hashmap.h"
#include "hashmap.h"

struct item {
    const char *key;
    int val;
};


static uint64_t item_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const struct item * it = item;
    return hashmap_sip(it->key, strlen(it->key), seed0, seed1);
}


static int item_compare(const void *a, const void *b, void *_unused) {
    const struct item *ia = a;
    const struct item *ib = b;
    return strcmp(ia->key, ib->key);
}


static void item_free(void *item) {
    struct { char *key; } *i = item;
    free(i->key);
}


struct hashmap *hnew(void) {
    return hashmap_new(sizeof(struct item), 0, 0, 0,
        item_hash, item_compare, item_free, NULL);
}


void hinsert(hashmap *hmap, const char *key, int value)
{
    struct item kv = { key, value }; // kvはhashmap_set()内でコピーされる
    hashmap_set(hmap, &kv);
}


int hget(hashmap *hmap, const char *key)
{
    struct item kv = { key };
    struct item * ret = hashmap_get(hmap, &kv);
    // 0は関数の仮引数の数で値として使用するので-1に変更
    if (!ret) return -1;
    return ret->val;
}


void hdestroy(hashmap *hmap)
{
    hashmap_free(hmap);
}

