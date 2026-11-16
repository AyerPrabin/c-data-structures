#include "check.h"
#include "../src/vector.h"

static void empty_vector(void)
{
    Vec v;
    vec_init(&v, sizeof(int));
    int x = 42;
    CHECK_EQ_LONG(vec_len(&v), 0);
    CHECK(vec_get(&v, 0) == NULL);
    CHECK_EQ_LONG(vec_pop(&v, &x), -1);
    CHECK_EQ_LONG(x, 42); /* out untouched on failure */
    CHECK_EQ_LONG(vec_remove(&v, 0, NULL), -1);
    CHECK_EQ_LONG(vec_set(&v, 0, &x), -1);
    vec_free(&v); /* freeing an empty vector is fine */
    vec_free(&v); /* and so is freeing twice */
    CHECK(v.data == NULL);
}

static void push_get_and_grow(void)
{
    Vec v;
    vec_init(&v, sizeof(int));
    int ok = 1;
    size_t last_cap = 0, grows = 0;
    for (int i = 0; i < 1000; i++) {
        if (vec_push(&v, &i) != 0)
            ok = 0;
        if (v.cap != last_cap) {
            grows++;
            last_cap = v.cap;
        }
    }
    CHECK(ok);
    CHECK_EQ_LONG(vec_len(&v), 1000);
    /* 4, 8, 16, ..., 1024: doubling means only 9 reallocations */
    CHECK_EQ_LONG(grows, 9);
    CHECK_EQ_LONG(v.cap, 1024);
    for (int i = 0; i < 1000; i++)
        if (*(int *)vec_get(&v, (size_t)i) != i)
            ok = 0;
    CHECK(ok);
    CHECK(vec_get(&v, 1000) == NULL);
    vec_free(&v);
}

static void pop_insert_remove(void)
{
    Vec v;
    vec_init(&v, sizeof(int));
    for (int i = 0; i < 5; i++)
        vec_push(&v, &i); /* 0 1 2 3 4 */

    int x = -1;
    CHECK_EQ_LONG(vec_pop(&v, &x), 0);
    CHECK_EQ_LONG(x, 4);
    CHECK_EQ_LONG(vec_len(&v), 4);

    int ten = 10;
    CHECK_EQ_LONG(vec_insert(&v, 0, &ten), 0); /* 10 0 1 2 3 */
    CHECK_EQ_LONG(*(int *)vec_get(&v, 0), 10);
    CHECK_EQ_LONG(*(int *)vec_get(&v, 1), 0);
    int eleven = 11;
    CHECK_EQ_LONG(vec_insert(&v, vec_len(&v), &eleven), 0); /* at the end */
    CHECK_EQ_LONG(*(int *)vec_get(&v, 5), 11);
    CHECK_EQ_LONG(vec_insert(&v, 99, &eleven), -1); /* past the end */

    CHECK_EQ_LONG(vec_remove(&v, 2, &x), 0); /* 10 0 2 3 11 */
    CHECK_EQ_LONG(x, 1);
    CHECK_EQ_LONG(*(int *)vec_get(&v, 2), 2);
    CHECK_EQ_LONG(vec_len(&v), 5);
    CHECK_EQ_LONG(vec_remove(&v, 5, NULL), -1);

    int seven = 7;
    CHECK_EQ_LONG(vec_set(&v, 4, &seven), 0);
    CHECK_EQ_LONG(*(int *)vec_get(&v, 4), 7);

    size_t cap = v.cap;
    vec_clear(&v);
    CHECK_EQ_LONG(vec_len(&v), 0);
    CHECK_EQ_LONG(v.cap, cap); /* clear keeps the buffer */
    vec_free(&v);
}

static void reserve_and_structs(void)
{
    struct point {
        double x, y;
        char tag[8];
    };
    Vec v;
    vec_init(&v, sizeof(struct point));
    CHECK_EQ_LONG(vec_reserve(&v, 100), 0);
    CHECK_EQ_LONG(v.cap, 100);
    CHECK_EQ_LONG(vec_reserve(&v, 10), 0); /* never shrinks */
    CHECK_EQ_LONG(v.cap, 100);

    struct point p = {1.5, -2.0, "abc"};
    vec_push(&v, &p);
    p.x = 9.0; /* the vector holds a copy */
    struct point *got = vec_get(&v, 0);
    CHECK(got->x == 1.5 && got->y == -2.0);
    CHECK_STR(got->tag, "abc");
    vec_free(&v);
}

void test_vector(void)
{
    empty_vector();
    push_get_and_grow();
    pop_insert_remove();
    reserve_and_structs();
}
