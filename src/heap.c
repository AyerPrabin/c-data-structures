#include "heap.h"

#include <stdint.h>
#include <stdlib.h>

void heap_init(Heap *h)
{
    h->items = NULL;
    h->len = 0;
    h->cap = 0;
}

void heap_free(Heap *h)
{
    free(h->items);
    heap_init(h);
}

static void swap(HeapItem *a, HeapItem *b)
{
    HeapItem t = *a;
    *a = *b;
    *b = t;
}

static void sift_up(HeapItem *a, size_t i)
{
    while (i > 0) {
        size_t parent = (i - 1) / 2;
        if (a[parent].prio <= a[i].prio)
            break;
        swap(&a[parent], &a[i]);
        i = parent;
    }
}

static void sift_down(HeapItem *a, size_t len, size_t i)
{
    for (;;) {
        size_t left = 2 * i + 1;
        size_t right = left + 1;
        size_t smallest = i;
        if (left < len && a[left].prio < a[smallest].prio)
            smallest = left;
        if (right < len && a[right].prio < a[smallest].prio)
            smallest = right;
        if (smallest == i)
            return;
        swap(&a[i], &a[smallest]);
        i = smallest;
    }
}

int heap_push(Heap *h, long prio, void *data)
{
    if (h->len == h->cap) {
        size_t new_cap = h->cap == 0 ? 8 : h->cap * 2;
        if (new_cap > SIZE_MAX / sizeof *h->items)
            return -1;
        HeapItem *p = realloc(h->items, new_cap * sizeof *p);
        if (p == NULL)
            return -1;
        h->items = p;
        h->cap = new_cap;
    }
    h->items[h->len].prio = prio;
    h->items[h->len].data = data;
    sift_up(h->items, h->len);
    h->len++;
    return 0;
}

int heap_peek(const Heap *h, long *prio, void **data)
{
    if (h->len == 0)
        return -1;
    if (prio != NULL)
        *prio = h->items[0].prio;
    if (data != NULL)
        *data = h->items[0].data;
    return 0;
}

int heap_pop(Heap *h, long *prio, void **data)
{
    if (heap_peek(h, prio, data) != 0)
        return -1;
    h->len--;
    if (h->len > 0) {
        h->items[0] = h->items[h->len];
        sift_down(h->items, h->len, 0);
    }
    return 0;
}

size_t heap_len(const Heap *h)
{
    return h->len;
}

void heap_from_array(Heap *h, HeapItem *items, size_t len)
{
    h->items = items;
    h->len = len;
    h->cap = len;
    /* Leaves are already valid heaps; fix each parent from the bottom up. */
    for (size_t i = len / 2; i-- > 0;)
        sift_down(items, len, i);
}

int heap_is_valid(const Heap *h)
{
    for (size_t i = 1; i < h->len; i++)
        if (h->items[(i - 1) / 2].prio > h->items[i].prio)
            return 0;
    return 1;
}
