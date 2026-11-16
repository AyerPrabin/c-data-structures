/*
 * Rough lookup benchmark: linear search over an array vs the BST vs the hash
 * map, all with the same string keys. It is a single run with wall-clock
 * timing, so treat the numbers as "which order of magnitude", not as precise
 * measurements.
 *
 * Every lookup result is added to a checksum that gets printed, so the
 * compiler cannot throw the loops away.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../src/bst.h"
#include "../src/hashmap.h"

#define LOOKUPS 20000

static double now_sec(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static unsigned long rng_state = 88172645UL;

static unsigned long rng(void)
{
    /* xorshift, good enough for shuffling test data */
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state & 0xffffffffUL;
}

static void shuffle(char **a, size_t n)
{
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)(rng() % (i + 1));
        char *t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
}

static int cmp_str(const void *a, const void *b)
{
    return strcmp(a, b);
}

static long linear_find(char **keys, size_t n, const char *k)
{
    for (size_t i = 0; i < n; i++)
        if (strcmp(keys[i], k) == 0)
            return (long)i;
    return -1;
}

/* Per-lookup time in nanoseconds. Half of the lookups are for keys that are
 * present and half for keys that are not. */
static void run(size_t n, int sorted_bst)
{
    char **keys = malloc(n * sizeof *keys);
    char **queries = malloc(LOOKUPS * sizeof *queries);
    if (keys == NULL || queries == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    for (size_t i = 0; i < n; i++) {
        keys[i] = malloc(32);
        snprintf(keys[i], 32, "key%08zu", i);
    }
    for (size_t i = 0; i < LOOKUPS; i++) {
        queries[i] = malloc(32);
        if (i % 2 == 0)
            snprintf(queries[i], 32, "key%08zu", (size_t)(rng() % n));
        else
            snprintf(queries[i], 32, "nope%07zu", i);
    }
    /* The keys were made in sorted order. Shuffle them unless we want to
     * show the BST's worst case. */
    if (!sorted_bst)
        shuffle(keys, n);

    HashMap m;
    Bst t;
    hm_init(&m);
    bst_init(&t, cmp_str);
    for (size_t i = 0; i < n; i++) {
        hm_put(&m, keys[i], keys[i]);
        bst_insert(&t, keys[i], keys[i]);
    }

    long check = 0;
    double t0 = now_sec();
    for (size_t i = 0; i < LOOKUPS; i++)
        check += linear_find(keys, n, queries[i]);
    double t1 = now_sec();
    for (size_t i = 0; i < LOOKUPS; i++)
        check += bst_find(&t, queries[i], NULL);
    double t2 = now_sec();
    for (size_t i = 0; i < LOOKUPS; i++)
        check += hm_get(&m, queries[i], NULL);
    double t3 = now_sec();

    double per = 1e9 / LOOKUPS;
    printf("%8zu  %-8s %12.0f %10.0f %8zu %10.0f   (checksum %ld)\n", n,
           sorted_bst ? "sorted" : "shuffled", (t1 - t0) * per,
           (t2 - t1) * per, bst_height(&t), (t3 - t2) * per, check);

    hm_free(&m);
    bst_free(&t);
    for (size_t i = 0; i < n; i++)
        free(keys[i]);
    for (size_t i = 0; i < LOOKUPS; i++)
        free(queries[i]);
    free(keys);
    free(queries);
}

int main(void)
{
    printf("%d lookups per row, half hits and half misses; times are ns per lookup\n\n",
           LOOKUPS);
    printf("%8s  %-8s %12s %10s %8s %10s\n", "n", "insert", "linear", "bst",
           "height", "hashmap");
    size_t sizes[] = {100, 1000, 10000, 100000};
    for (size_t i = 0; i < sizeof sizes / sizeof sizes[0]; i++)
        run(sizes[i], 0);
    /* Sorted insertion: the BST becomes a linked list. Kept small because
     * every lookup walks the whole chain. */
    run(1000, 1);
    run(10000, 1);
    return 0;
}
