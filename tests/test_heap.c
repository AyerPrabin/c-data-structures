#include <stdlib.h>

#include "check.h"
#include "../src/heap.h"

static void empty_heap(void)
{
    Heap h;
    heap_init(&h);
    long p = 123;
    void *d = NULL;
    CHECK_EQ_LONG(heap_len(&h), 0);
    CHECK_EQ_LONG(heap_pop(&h, &p, &d), -1);
    CHECK_EQ_LONG(heap_peek(&h, &p, &d), -1);
    CHECK_EQ_LONG(p, 123);
    CHECK(heap_is_valid(&h));
    heap_free(&h);
}

static void push_pop_order(void)
{
    Heap h;
    heap_init(&h);
    long in[] = {5, 3, 8, 1, 9, 2, 7};
    static const char *names[] = {"e", "c", "h", "a", "i", "b", "g"};
    for (int i = 0; i < 7; i++)
        heap_push(&h, in[i], (void *)names[i]);
    CHECK(heap_is_valid(&h));
    CHECK_EQ_LONG(heap_len(&h), 7);

    long p;
    void *d;
    CHECK_EQ_LONG(heap_peek(&h, &p, &d), 0);
    CHECK_EQ_LONG(p, 1);
    CHECK_STR((const char *)d, "a");
    CHECK_EQ_LONG(heap_len(&h), 7); /* peek does not remove */

    long want[] = {1, 2, 3, 5, 7, 8, 9};
    int ok = 1;
    for (int i = 0; i < 7; i++)
        if (heap_pop(&h, &p, NULL) != 0 || p != want[i])
            ok = 0;
    CHECK(ok);
    CHECK_EQ_LONG(heap_len(&h), 0);
    heap_free(&h);
}

static void duplicates_and_negatives(void)
{
    Heap h;
    heap_init(&h);
    long in[] = {4, -1, 4, 0, -1, 4, -100};
    for (int i = 0; i < 7; i++)
        heap_push(&h, in[i], NULL);
    long want[] = {-100, -1, -1, 0, 4, 4, 4};
    int ok = 1;
    long p;
    for (int i = 0; i < 7; i++) {
        heap_pop(&h, &p, NULL);
        if (p != want[i])
            ok = 0;
    }
    CHECK(ok);
    heap_free(&h);
}

static void random_against_sort(void)
{
    /* Push a few thousand pseudo-random priorities, interleaving some pops,
     * and check every pop returns the smallest so far. */
    Heap h;
    heap_init(&h);
    unsigned seed = 12345;
    int ok = 1, valid = 1;
    long last = 0;
    for (int round = 0; round < 3000; round++) {
        seed = seed * 1103515245u + 12345u;
        heap_push(&h, (long)(seed >> 16) % 1000, NULL);
        if (round % 3 == 2) {
            long p, top;
            heap_peek(&h, &top, NULL);
            heap_pop(&h, &p, NULL);
            if (p != top)
                ok = 0;
        }
        if (round % 500 == 0 && !heap_is_valid(&h))
            valid = 0;
    }
    CHECK(ok);
    CHECK(valid);
    CHECK_EQ_LONG(heap_len(&h), 2000);
    /* draining it gives a non-decreasing sequence */
    int sorted = 1;
    for (int i = 0; heap_len(&h) > 0; i++) {
        long p;
        heap_pop(&h, &p, NULL);
        if (i > 0 && p < last)
            sorted = 0;
        last = p;
    }
    CHECK(sorted);
    heap_free(&h);
}

static void heapify(void)
{
    enum { N = 1000 };
    HeapItem *items = malloc(N * sizeof *items);
    for (int i = 0; i < N; i++) {
        items[i].prio = (long)((i * 7919) % N); /* a permutation of 0..N-1 */
        items[i].data = NULL;
    }
    Heap h;
    heap_from_array(&h, items, N);
    CHECK(heap_is_valid(&h));
    CHECK_EQ_LONG(heap_len(&h), N);
    int ok = 1;
    for (long want = 0; want < N; want++) {
        long p;
        if (heap_pop(&h, &p, NULL) != 0 || p != want)
            ok = 0;
    }
    CHECK(ok);
    /* still usable as a normal heap after being emptied */
    CHECK_EQ_LONG(heap_push(&h, 5, NULL), 0);
    CHECK_EQ_LONG(heap_len(&h), 1);
    heap_free(&h);

    Heap empty;
    heap_from_array(&empty, NULL, 0);
    CHECK(heap_is_valid(&empty));
    CHECK_EQ_LONG(heap_push(&empty, 1, NULL), 0);
    heap_free(&empty);
}

void test_heap(void)
{
    empty_heap();
    push_pop_order();
    duplicates_and_negatives();
    random_against_sort();
    heapify();
}
