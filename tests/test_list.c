#include <stdlib.h>

#include "check.h"
#include "../src/list.h"

static int int_eq(const void *a, const void *b)
{
    return *(const int *)a == *(const int *)b;
}

/* Walks the list and compares against the expected ints. Also checks that
 * tail really is the last node. */
static int slist_is(const SList *l, const int *want, size_t n)
{
    if (l->len != n)
        return 0;
    const SNode *node = l->head, *last = NULL;
    for (size_t i = 0; i < n; i++) {
        if (node == NULL || *(int *)node->data != want[i])
            return 0;
        last = node;
        node = node->next;
    }
    return node == NULL && l->tail == last;
}

/* Same for the doubly linked list, walking forwards and then backwards. */
static int dlist_is(const DList *l, const int *want, size_t n)
{
    if (l->len != n)
        return 0;
    const DNode *node = l->head, *last = NULL;
    for (size_t i = 0; i < n; i++) {
        if (node == NULL || *(int *)node->data != want[i])
            return 0;
        if (node->prev != last)
            return 0;
        last = node;
        node = node->next;
    }
    if (node != NULL || l->tail != last)
        return 0;
    for (size_t i = n; i-- > 0;) {
        if (*(int *)last->data != want[i])
            return 0;
        last = last->prev;
    }
    return last == NULL;
}

static int nums[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

static void slist_basics(void)
{
    SList l;
    slist_init(&l);
    void *out = NULL;
    CHECK_EQ_LONG(slist_pop_front(&l, &out), -1);
    CHECK(slist_find(&l, &nums[1], int_eq) == NULL);
    CHECK_EQ_LONG(slist_remove_first(&l, &nums[1], int_eq), 0);
    slist_reverse(&l); /* reversing an empty list does nothing */
    CHECK(l.head == NULL && l.tail == NULL);

    slist_push_back(&l, &nums[2]);
    slist_push_front(&l, &nums[1]);
    slist_push_back(&l, &nums[3]);
    slist_push_front(&l, &nums[0]);
    CHECK(slist_is(&l, (int[]){0, 1, 2, 3}, 4));

    CHECK_EQ_LONG(slist_pop_front(&l, &out), 0);
    CHECK(out == &nums[0]);
    CHECK(slist_is(&l, (int[]){1, 2, 3}, 3));

    slist_reverse(&l);
    CHECK(slist_is(&l, (int[]){3, 2, 1}, 3));
    slist_free(&l);
    CHECK(l.head == NULL && l.len == 0);
}

static void slist_remove_cases(void)
{
    SList l;
    slist_init(&l);
    int two = 2;
    int a[] = {1, 2, 3, 2, 4};
    for (int i = 0; i < 5; i++)
        slist_push_back(&l, &a[i]);

    /* by value: only the first 2 goes */
    CHECK_EQ_LONG(slist_remove_first(&l, &two, int_eq), 1);
    CHECK(slist_is(&l, (int[]){1, 3, 2, 4}, 4));
    /* by pointer (eq == NULL): `two` is a different object, so no match */
    CHECK_EQ_LONG(slist_remove_first(&l, &two, NULL), 0);
    CHECK_EQ_LONG(slist_remove_first(&l, &a[3], NULL), 1);
    CHECK(slist_is(&l, (int[]){1, 3, 4}, 3));

    /* removing the tail must move tail back, or push_back breaks */
    CHECK_EQ_LONG(slist_remove_first(&l, &a[4], NULL), 1);
    slist_push_back(&l, &nums[9]);
    CHECK(slist_is(&l, (int[]){1, 3, 9}, 3));

    /* removing the head */
    CHECK_EQ_LONG(slist_remove_first(&l, &a[0], NULL), 1);
    CHECK(slist_is(&l, (int[]){3, 9}, 2));

    /* down to empty and back */
    slist_remove_first(&l, &a[2], NULL);
    slist_remove_first(&l, &nums[9], NULL);
    CHECK(l.head == NULL && l.tail == NULL && l.len == 0);
    slist_push_back(&l, &nums[5]);
    CHECK(slist_is(&l, (int[]){5}, 1));
    slist_free(&l);
}

static void slist_owned_data(void)
{
    /* data that the caller malloc'd is released through free_with */
    SList l;
    slist_init(&l);
    for (int i = 0; i < 3; i++) {
        int *p = malloc(sizeof *p);
        *p = i;
        slist_push_back(&l, p);
    }
    CHECK_EQ_LONG(l.len, 3);
    slist_free_with(&l, free); /* Valgrind in CI checks nothing leaks */
    CHECK_EQ_LONG(l.len, 0);
}

static void dlist_basics(void)
{
    DList l;
    dlist_init(&l);
    void *out = NULL;
    CHECK_EQ_LONG(dlist_pop_front(&l, &out), -1);
    CHECK_EQ_LONG(dlist_pop_back(&l, &out), -1);

    dlist_push_back(&l, &nums[2]);
    dlist_push_front(&l, &nums[1]);
    dlist_push_back(&l, &nums[3]);
    CHECK(dlist_is(&l, (int[]){1, 2, 3}, 3));

    CHECK_EQ_LONG(dlist_pop_back(&l, &out), 0);
    CHECK(out == &nums[3]);
    CHECK_EQ_LONG(dlist_pop_front(&l, &out), 0);
    CHECK(out == &nums[1]);
    CHECK(dlist_is(&l, (int[]){2}, 1));
    CHECK_EQ_LONG(dlist_pop_back(&l, NULL), 0);
    CHECK(l.head == NULL && l.tail == NULL);
    dlist_free(&l);
}

static void dlist_insert_remove(void)
{
    DList l;
    dlist_init(&l);
    dlist_insert_after(&l, NULL, &nums[1]); /* into empty list */
    dlist_insert_after(&l, NULL, &nums[0]); /* NULL = at the front */
    dlist_insert_after(&l, l.tail, &nums[3]);
    DNode *one = dlist_find(&l, &nums[1], int_eq);
    CHECK(one != NULL);
    dlist_insert_after(&l, one, &nums[2]); /* in the middle */
    CHECK(dlist_is(&l, (int[]){0, 1, 2, 3}, 4));

    /* O(1) removal given the node: middle, head, tail */
    CHECK(dlist_remove(&l, one) == &nums[1]);
    CHECK(dlist_is(&l, (int[]){0, 2, 3}, 3));
    CHECK(dlist_remove(&l, l.head) == &nums[0]);
    CHECK(dlist_remove(&l, l.tail) == &nums[3]);
    CHECK(dlist_is(&l, (int[]){2}, 1));
    CHECK(dlist_find(&l, &nums[7], int_eq) == NULL);
    dlist_free(&l);
}

void test_list(void)
{
    slist_basics();
    slist_remove_cases();
    slist_owned_data();
    dlist_basics();
    dlist_insert_remove();
}
