#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

/*
 * Binary min-heap used as a priority queue. The smallest priority comes out
 * first. It is stored in one array: the children of index i are at 2i+1 and
 * 2i+2, and the parent of i is at (i-1)/2.
 *
 * Ownership: the heap owns its array but NOT the data pointers.
 *
 * It is not stable: items with equal priority can come out in any order.
 */

typedef struct {
    long prio;
    void *data;
} HeapItem;

typedef struct {
    HeapItem *items;
    size_t len;
    size_t cap;
} Heap;

void heap_init(Heap *h);
void heap_free(Heap *h);

int heap_push(Heap *h, long prio, void *data); /* 0, or -1 on alloc failure */
int heap_pop(Heap *h, long *prio, void **data); /* -1 if empty; outs may be NULL */
int heap_peek(const Heap *h, long *prio, void **data);
size_t heap_len(const Heap *h);

/* Turn an unordered array into a heap in O(n) (Floyd's method). The heap
 * takes over `items`, which must come from malloc; it is freed by heap_free. */
void heap_from_array(Heap *h, HeapItem *items, size_t len);

/* Checks the heap property on every parent/child pair. For tests. */
int heap_is_valid(const Heap *h);

#endif
