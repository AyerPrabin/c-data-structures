#include "hashmap.h"

#include <stdlib.h>
#include <string.h>

enum { SLOT_EMPTY = 0, SLOT_USED, SLOT_TOMB };

#define HM_MIN_CAP 8

/* Max load is 7/10. Kept as integers to avoid floating point. */
#define LOAD_NUM 7
#define LOAD_DEN 10

uint64_t hm_hash(const char *key)
{
    uint64_t h = 14695981039346656037ULL;
    for (const unsigned char *p = (const unsigned char *)key; *p != '\0'; p++) {
        h ^= *p;
        h *= 1099511628211ULL;
    }
    return h;
}

void hm_init(HashMap *m)
{
    m->slots = NULL;
    m->cap = 0;
    m->len = 0;
    m->tombs = 0;
}

void hm_free(HashMap *m)
{
    for (size_t i = 0; i < m->cap; i++)
        free(m->slots[i].key);
    free(m->slots);
    hm_init(m);
}

static char *copy_string(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p != NULL)
        memcpy(p, s, n);
    return p;
}

/* Index of the slot holding `key`, or cap if it is not in the table.
 * Stops at the first EMPTY slot; tombstones are stepped over. This always
 * terminates because the load rule guarantees at least one EMPTY slot. */
static size_t find_slot(const HashMap *m, const char *key, uint64_t h)
{
    if (m->cap == 0)
        return 0;
    size_t mask = m->cap - 1;
    for (size_t i = (size_t)h & mask;; i = (i + 1) & mask) {
        const HmSlot *s = &m->slots[i];
        if (s->state == SLOT_EMPTY)
            return m->cap;
        if (s->state == SLOT_USED && s->hash == h && strcmp(s->key, key) == 0)
            return i;
    }
}

/* Where a key that is known to be absent should go: the first tombstone on
 * its probe run if there is one (reusing it), otherwise the EMPTY slot that
 * ended the run. */
static size_t insert_slot(const HashMap *m, uint64_t h)
{
    size_t mask = m->cap - 1;
    for (size_t i = (size_t)h & mask;; i = (i + 1) & mask)
        if (m->slots[i].state != SLOT_USED)
            return i;
}

/* Move every live entry into a fresh array of new_cap slots. This also throws
 * away all tombstones. The key strings are moved, not copied. */
static int rebuild(HashMap *m, size_t new_cap)
{
    HmSlot *slots = calloc(new_cap, sizeof *slots);
    if (slots == NULL)
        return -1;
    size_t mask = new_cap - 1;
    for (size_t i = 0; i < m->cap; i++) {
        HmSlot *old = &m->slots[i];
        if (old->state != SLOT_USED)
            continue;
        size_t j = (size_t)old->hash & mask;
        while (slots[j].state == SLOT_USED)
            j = (j + 1) & mask;
        slots[j] = *old;
    }
    free(m->slots);
    m->slots = slots;
    m->cap = new_cap;
    m->tombs = 0;
    return 0;
}

int hm_put(HashMap *m, const char *key, void *value)
{
    uint64_t h = hm_hash(key);
    size_t i = find_slot(m, key, h);
    if (i < m->cap) {
        m->slots[i].value = value;
        return 0;
    }

    /* New key. Tombstones count towards the load: they make probe runs
     * longer just like live entries do. */
    if ((m->len + m->tombs + 1) * LOAD_DEN > m->cap * LOAD_NUM) {
        /* Size the new table so it is at most half full afterwards. If most
         * of the old load was tombstones this keeps the same capacity (or
         * even shrinks it) and just cleans up. */
        size_t new_cap = HM_MIN_CAP;
        while ((m->len + 1) * 2 > new_cap)
            new_cap *= 2;
        if (rebuild(m, new_cap) != 0)
            return -1;
    }

    char *copy = copy_string(key);
    if (copy == NULL)
        return -1;
    i = insert_slot(m, h);
    if (m->slots[i].state == SLOT_TOMB)
        m->tombs--;
    m->slots[i].key = copy;
    m->slots[i].value = value;
    m->slots[i].hash = h;
    m->slots[i].state = SLOT_USED;
    m->len++;
    return 1;
}

int hm_get(const HashMap *m, const char *key, void **out)
{
    size_t i = find_slot(m, key, hm_hash(key));
    if (i >= m->cap)
        return 0;
    if (out != NULL)
        *out = m->slots[i].value;
    return 1;
}

int hm_remove(HashMap *m, const char *key, void **out)
{
    size_t i = find_slot(m, key, hm_hash(key));
    if (i >= m->cap)
        return 0;
    HmSlot *s = &m->slots[i];
    if (out != NULL)
        *out = s->value;
    free(s->key);
    s->key = NULL;
    s->value = NULL;
    s->state = SLOT_TOMB;
    m->len--;
    m->tombs++;
    return 1;
}

size_t hm_len(const HashMap *m)
{
    return m->len;
}

size_t hm_capacity(const HashMap *m)
{
    return m->cap;
}

size_t hm_tombstones(const HashMap *m)
{
    return m->tombs;
}

int hm_next(const HashMap *m, size_t *pos, const char **key, void **value)
{
    while (*pos < m->cap) {
        const HmSlot *s = &m->slots[(*pos)++];
        if (s->state == SLOT_USED) {
            if (key != NULL)
                *key = s->key;
            if (value != NULL)
                *value = s->value;
            return 1;
        }
    }
    return 0;
}
