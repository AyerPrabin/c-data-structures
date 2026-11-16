#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "../src/bst.h"

static int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

static int cmp_str(const void *a, const void *b)
{
    return strcmp(a, b);
}

static int keys[100];

static void fill_keys(void)
{
    for (int i = 0; i < 100; i++)
        keys[i] = i;
}

/* In-order walk into `out`; returns how many keys it saw. */
static size_t walk(const Bst *t, int *out, size_t max)
{
    BstIter it;
    bst_iter_init(&it, t);
    const void *k;
    size_t n = 0;
    while (bst_iter_next(&it, &k, NULL) == 1 && n < max)
        out[n++] = *(const int *)k;
    bst_iter_free(&it);
    return n;
}

static void empty_tree(void)
{
    Bst t;
    bst_init(&t, cmp_int);
    CHECK_EQ_LONG(bst_len(&t), 0);
    CHECK_EQ_LONG(bst_height(&t), 0);
    CHECK_EQ_LONG(bst_find(&t, &keys[1], NULL), 0);
    CHECK_EQ_LONG(bst_remove(&t, &keys[1], NULL), 0);
    CHECK_EQ_LONG(bst_min(&t, NULL, NULL), -1);
    CHECK_EQ_LONG(bst_max(&t, NULL, NULL), -1);
    int out[1];
    CHECK_EQ_LONG(walk(&t, out, 1), 0);
    bst_free(&t);
}

static void insert_find_iterate(void)
{
    Bst t;
    bst_init(&t, cmp_int);
    int order[] = {50, 30, 70, 20, 40, 60, 80, 35, 45, 65};
    int ok = 1;
    for (int i = 0; i < 10; i++)
        if (bst_insert(&t, &keys[order[i]], &keys[order[i]]) != 1)
            ok = 0;
    CHECK(ok);
    CHECK_EQ_LONG(bst_len(&t), 10);
    CHECK_EQ_LONG(bst_height(&t), 4);

    void *v = NULL;
    CHECK_EQ_LONG(bst_find(&t, &keys[45], &v), 1);
    CHECK(v == &keys[45]);
    CHECK_EQ_LONG(bst_find(&t, &keys[46], NULL), 0);

    /* a duplicate key replaces the value, the size stays the same */
    int replacement = -1;
    CHECK_EQ_LONG(bst_insert(&t, &keys[45], &replacement), 0);
    CHECK_EQ_LONG(bst_len(&t), 10);
    bst_find(&t, &keys[45], &v);
    CHECK(v == &replacement);

    int out[10];
    int want[] = {20, 30, 35, 40, 45, 50, 60, 65, 70, 80};
    CHECK_EQ_LONG(walk(&t, out, 10), 10);
    CHECK(memcmp(out, want, sizeof want) == 0);

    const void *k;
    bst_min(&t, &k, NULL);
    CHECK_EQ_LONG(*(const int *)k, 20);
    bst_max(&t, &k, NULL);
    CHECK_EQ_LONG(*(const int *)k, 80);
    bst_free(&t);
}

static void remove_cases(void)
{
    Bst t;
    bst_init(&t, cmp_int);
    int order[] = {50, 30, 70, 20, 40, 60, 80, 35, 45, 65};
    for (int i = 0; i < 10; i++)
        bst_insert(&t, &keys[order[i]], NULL);

    int out[10];
    /* two children, and the successor is further down (30 -> 35) */
    CHECK_EQ_LONG(bst_remove(&t, &keys[30], NULL), 1);
    /* leaf */
    CHECK_EQ_LONG(bst_remove(&t, &keys[20], NULL), 1);
    /* one child (60 has only 65) */
    CHECK_EQ_LONG(bst_remove(&t, &keys[60], NULL), 1);
    CHECK_EQ_LONG(walk(&t, out, 10), 7);
    CHECK(memcmp(out, (int[]){35, 40, 45, 50, 65, 70, 80}, 7 * sizeof(int)) == 0);
    /* the root, with two children */
    CHECK_EQ_LONG(bst_remove(&t, &keys[50], NULL), 1);
    CHECK_EQ_LONG(bst_find(&t, &keys[50], NULL), 0);
    CHECK_EQ_LONG(walk(&t, out, 10), 6);
    CHECK(memcmp(out, (int[]){35, 40, 45, 65, 70, 80}, 6 * sizeof(int)) == 0);
    /* removing something that is not there */
    CHECK_EQ_LONG(bst_remove(&t, &keys[99], NULL), 0);
    CHECK_EQ_LONG(bst_len(&t), 6);

    /* remove everything, in an awkward order */
    int rest[] = {70, 35, 80, 45, 40, 65};
    for (int i = 0; i < 6; i++)
        bst_remove(&t, &keys[rest[i]], NULL);
    CHECK_EQ_LONG(bst_len(&t), 0);
    CHECK(t.root == NULL);
    bst_free(&t);
}

static void string_keys_and_early_break(void)
{
    Bst t;
    bst_init(&t, cmp_str);
    const char *words[] = {"pear", "apple", "fig", "kiwi", "banana"};
    for (int i = 0; i < 5; i++)
        bst_insert(&t, words[i], NULL);
    BstIter it;
    bst_iter_init(&it, &t);
    const void *k;
    bst_iter_next(&it, &k, NULL);
    CHECK_STR(k, "apple");
    bst_iter_next(&it, &k, NULL);
    CHECK_STR(k, "banana");
    /* stop half way: bst_iter_free must still release the stack */
    bst_iter_free(&it);
    bst_free(&t);
}

static void degenerate_tree(void)
{
    /* Sorted input turns the tree into a list: height == n. This is the
     * case that would overflow the stack if the code were recursive. */
    enum { N = 20000 };
    int *k = malloc(N * sizeof *k);
    Bst t;
    bst_init(&t, cmp_int);
    for (int i = 0; i < N; i++) {
        k[i] = i;
        bst_insert(&t, &k[i], NULL);
    }
    CHECK_EQ_LONG(bst_height(&t), N);
    CHECK_EQ_LONG(bst_find(&t, &k[N - 1], NULL), 1);

    BstIter it;
    bst_iter_init(&it, &t);
    const void *key;
    int ok = 1, count = 0;
    while (bst_iter_next(&it, &key, NULL) == 1)
        if (*(const int *)key != count++)
            ok = 0;
    bst_iter_free(&it);
    CHECK(ok);
    CHECK_EQ_LONG(count, N);
    bst_free(&t);
    free(k);
}

static void shuffled_is_shallow(void)
{
    /* The same keys in a shuffled order give a much shallower tree. For
     * random insertion order the expected height is about 4.3 ln(n), which
     * is around 30 here, against 4096 for sorted input. Fixed seed, so the
     * result is the same on every run. */
    enum { N = 4096 };
    int *k = malloc(N * sizeof *k);
    for (int i = 0; i < N; i++)
        k[i] = i;
    unsigned seed = 7;
    for (int i = N - 1; i > 0; i--) {
        seed = seed * 1103515245u + 12345u;
        int j = (int)((seed >> 8) % (unsigned)(i + 1));
        int tmp = k[i];
        k[i] = k[j];
        k[j] = tmp;
    }
    Bst t;
    bst_init(&t, cmp_int);
    for (int i = 0; i < N; i++)
        bst_insert(&t, &k[i], NULL);
    CHECK_EQ_LONG(bst_len(&t), N);
    CHECK(bst_height(&t) < 50);
    bst_free(&t);
    free(k);
}

void test_bst(void)
{
    fill_keys();
    empty_tree();
    insert_find_iterate();
    remove_cases();
    string_keys_and_early_break();
    degenerate_tree();
    shuffled_is_shallow();
}
