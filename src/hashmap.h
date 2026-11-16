#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>
#include <stdint.h>

/*
 * Hash map from C strings to void pointers.
 *
 * Open addressing with linear probing: every entry lives directly in one
 * array of slots. A deleted entry leaves a tombstone so that lookups for keys
 * further along the same probe run still find them (see README).
 *
 * The table is rebuilt when (live entries + tombstones) would go above 70%
 * of the capacity. Capacity is always a power of two (or 0 before the first
 * insert).
 *
 * Ownership: the map makes its own copy of every key and frees it on remove
 * and in hm_free. Values are NOT owned; the map only stores the pointer.
 *
 * Key and value pointers seen through hm_next or returned by hm_get stay
 * valid only until the next hm_put or hm_remove (a put can rebuild the
 * table).
 */

typedef struct {
    char *key;     /* NULL in empty and tombstone slots */
    void *value;
    uint64_t hash;
    unsigned char state;
} HmSlot;

typedef struct {
    HmSlot *slots;
    size_t cap;
    size_t len;   /* live entries */
    size_t tombs; /* tombstone slots */
} HashMap;

void hm_init(HashMap *m);
void hm_free(HashMap *m);

/* Returns 1 if the key was new, 0 if an existing value was replaced,
 * -1 on allocation failure (map unchanged). */
int hm_put(HashMap *m, const char *key, void *value);

/* Returns 1 and stores the value in *out (if out != NULL) when found,
 * otherwise returns 0. */
int hm_get(const HashMap *m, const char *key, void **out);

/* Returns 1 if the key was removed (its value goes in *out if out != NULL),
 * 0 if it was not there. */
int hm_remove(HashMap *m, const char *key, void **out);

size_t hm_len(const HashMap *m);
size_t hm_capacity(const HashMap *m);
size_t hm_tombstones(const HashMap *m);

/* Iterate in slot order (not insertion order):
 *     size_t pos = 0; const char *k; void *v;
 *     while (hm_next(&m, &pos, &k, &v)) { ... }
 */
int hm_next(const HashMap *m, size_t *pos, const char **key, void **value);

uint64_t hm_hash(const char *key); /* FNV-1a, exposed for tests */

#endif
