#include <stdio.h>

#include "check.h"
#include "../src/hashmap.h"

static int vals[16];

static void empty_map(void)
{
    HashMap m;
    hm_init(&m);
    void *out = &vals[0];
    CHECK_EQ_LONG(hm_len(&m), 0);
    CHECK_EQ_LONG(hm_capacity(&m), 0); /* nothing allocated yet */
    CHECK_EQ_LONG(hm_get(&m, "x", &out), 0);
    CHECK(out == &vals[0]);
    CHECK_EQ_LONG(hm_remove(&m, "x", NULL), 0);
    size_t pos = 0;
    CHECK_EQ_LONG(hm_next(&m, &pos, NULL, NULL), 0);
    hm_free(&m);
}

static void put_get_replace(void)
{
    HashMap m;
    hm_init(&m);
    CHECK_EQ_LONG(hm_put(&m, "apple", &vals[1]), 1);
    CHECK_EQ_LONG(hm_put(&m, "pear", &vals[2]), 1);
    CHECK_EQ_LONG(hm_put(&m, "", &vals[3]), 1); /* empty string is a key too */
    CHECK_EQ_LONG(hm_put(&m, "nothing", NULL), 1); /* NULL values allowed */

    void *out = NULL;
    CHECK_EQ_LONG(hm_get(&m, "apple", &out), 1);
    CHECK(out == &vals[1]);
    CHECK_EQ_LONG(hm_get(&m, "", &out), 1);
    CHECK(out == &vals[3]);
    CHECK_EQ_LONG(hm_get(&m, "nothing", &out), 1);
    CHECK(out == NULL);
    CHECK_EQ_LONG(hm_get(&m, "appl", NULL), 0);
    CHECK_EQ_LONG(hm_get(&m, "apples", NULL), 0);

    /* duplicate key replaces the value and does not add an entry */
    CHECK_EQ_LONG(hm_put(&m, "apple", &vals[9]), 0);
    CHECK_EQ_LONG(hm_len(&m), 4);
    hm_get(&m, "apple", &out);
    CHECK(out == &vals[9]);

    /* the map copied the key, so changing the caller's buffer is harmless */
    char buf[] = "temp";
    hm_put(&m, buf, &vals[4]);
    buf[0] = 'X';
    CHECK_EQ_LONG(hm_get(&m, "temp", NULL), 1);
    CHECK_EQ_LONG(hm_get(&m, "Xemp", NULL), 0);
    hm_free(&m);
}

static void resize_at_load_factor(void)
{
    HashMap m;
    hm_init(&m);
    char key[16];
    /* capacity 8 holds 5 entries (5/8 = 0.625); the 6th would be 0.75 */
    for (int i = 0; i < 5; i++) {
        snprintf(key, sizeof key, "k%d", i);
        hm_put(&m, key, &vals[i]);
    }
    CHECK_EQ_LONG(hm_capacity(&m), 8);
    hm_put(&m, "k5", &vals[5]);
    CHECK_EQ_LONG(hm_capacity(&m), 16);

    /* everything is still reachable after the rebuild */
    int ok = 1;
    for (int i = 0; i < 6; i++) {
        void *out;
        snprintf(key, sizeof key, "k%d", i);
        if (!hm_get(&m, key, &out) || out != &vals[i])
            ok = 0;
    }
    CHECK(ok);
    /* 16 * 0.7 = 11.2, so 11 fit and the 12th doubles again */
    for (int i = 6; i < 11; i++) {
        snprintf(key, sizeof key, "k%d", i);
        hm_put(&m, key, NULL);
    }
    CHECK_EQ_LONG(hm_capacity(&m), 16);
    hm_put(&m, "k11", NULL);
    CHECK_EQ_LONG(hm_capacity(&m), 32);
    hm_free(&m);
}

/* Finds `n` keys that all start probing at the same slot in a table of
 * capacity `cap`, so the tests can build collisions on purpose. */
static void colliding_keys(char keys[][16], int n, size_t cap)
{
    size_t want = (size_t)-1;
    int found = 0;
    for (int i = 0; found < n; i++) {
        char k[16];
        snprintf(k, sizeof k, "c%d", i);
        size_t slot = (size_t)hm_hash(k) & (cap - 1);
        if (want == (size_t)-1)
            want = slot;
        if (slot == want)
            snprintf(keys[found++], 16, "%s", k);
    }
}

static void tombstones(void)
{
    char k[3][16];
    colliding_keys(k, 3, 8);

    HashMap m;
    hm_init(&m);
    hm_put(&m, k[0], &vals[0]);
    hm_put(&m, k[1], &vals[1]);
    hm_put(&m, k[2], &vals[2]); /* sits two slots after its home slot */
    CHECK_EQ_LONG(hm_capacity(&m), 8);

    /* Deleting the middle one must not cut the probe run for k[2]. */
    void *out = NULL;
    CHECK_EQ_LONG(hm_remove(&m, k[1], &out), 1);
    CHECK(out == &vals[1]);
    CHECK_EQ_LONG(hm_tombstones(&m), 1);
    CHECK_EQ_LONG(hm_len(&m), 2);
    CHECK_EQ_LONG(hm_get(&m, k[1], NULL), 0);
    CHECK_EQ_LONG(hm_get(&m, k[2], &out), 1);
    CHECK(out == &vals[2]);
    CHECK_EQ_LONG(hm_remove(&m, k[1], NULL), 0); /* already gone */

    /* Putting k[2] again must find the existing entry past the tombstone,
     * not drop a duplicate into the tombstone slot. */
    CHECK_EQ_LONG(hm_put(&m, k[2], &vals[7]), 0);
    CHECK_EQ_LONG(hm_len(&m), 2);
    CHECK_EQ_LONG(hm_tombstones(&m), 1);
    size_t pos = 0, seen = 0;
    const char *key;
    while (hm_next(&m, &pos, &key, NULL))
        seen++;
    CHECK_EQ_LONG(seen, 2);
    hm_remove(&m, k[2], NULL);
    CHECK_EQ_LONG(hm_get(&m, k[2], NULL), 0); /* really gone, no copy left */

    /* A new key reuses the first tombstone on its run. */
    CHECK_EQ_LONG(hm_put(&m, k[1], &vals[1]), 1);
    CHECK_EQ_LONG(hm_tombstones(&m), 1);
    CHECK_EQ_LONG(hm_len(&m), 2);
    hm_free(&m);
}

static void tombstone_churn(void)
{
    /* Add and delete thousands of different keys while only one is ever
     * live. Tombstones pile up, but the rebuild clears them, so the table
     * must stay small and lookups for missing keys must still finish. */
    HashMap m;
    hm_init(&m);
    char key[16];
    size_t max_cap = 0;
    int ok = 1;
    for (int i = 0; i < 5000; i++) {
        snprintf(key, sizeof key, "t%d", i);
        if (hm_put(&m, key, NULL) != 1)
            ok = 0;
        if (hm_remove(&m, key, NULL) != 1)
            ok = 0;
        if (hm_capacity(&m) > max_cap)
            max_cap = hm_capacity(&m);
    }
    CHECK(ok);
    CHECK_EQ_LONG(hm_len(&m), 0);
    CHECK(max_cap <= 8);
    CHECK(hm_tombstones(&m) < hm_capacity(&m));
    CHECK_EQ_LONG(hm_get(&m, "never-added", NULL), 0);
    hm_free(&m);
}

static void many_keys(void)
{
    enum { N = 20000 };
    static int data[N];
    HashMap m;
    hm_init(&m);
    char key[16];
    int ok = 1;
    for (int i = 0; i < N; i++) {
        data[i] = i;
        snprintf(key, sizeof key, "key-%d", i);
        if (hm_put(&m, key, &data[i]) != 1)
            ok = 0;
    }
    CHECK(ok);
    CHECK_EQ_LONG(hm_len(&m), N);
    /* load factor stays at or under 0.7 */
    CHECK(hm_len(&m) * 10 <= hm_capacity(&m) * 7);

    for (int i = 0; i < N; i += 2) {
        snprintf(key, sizeof key, "key-%d", i);
        if (hm_remove(&m, key, NULL) != 1)
            ok = 0;
    }
    CHECK(ok);
    CHECK_EQ_LONG(hm_len(&m), N / 2);

    int odd_ok = 1, even_gone = 1;
    for (int i = 0; i < N; i++) {
        void *out = NULL;
        snprintf(key, sizeof key, "key-%d", i);
        int found = hm_get(&m, key, &out);
        if (i % 2 == 0 && found)
            even_gone = 0;
        if (i % 2 == 1 && (!found || out != &data[i]))
            odd_ok = 0;
    }
    CHECK(even_gone);
    CHECK(odd_ok);

    /* iteration sees each live entry exactly once */
    size_t pos = 0, count = 0;
    long sum = 0;
    void *v;
    while (hm_next(&m, &pos, NULL, &v)) {
        count++;
        sum += *(int *)v;
    }
    CHECK_EQ_LONG(count, N / 2);
    CHECK_EQ_LONG(sum, (long)(N / 2) * (N / 2)); /* 1 + 3 + ... + (N-1) */
    hm_free(&m);
}

void test_hashmap(void)
{
    empty_map();
    put_get_replace();
    resize_at_load_factor();
    tombstones();
    tombstone_churn();
    many_keys();
}
