#include "vector.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define VEC_MIN_CAP 4

void vec_init(Vec *v, size_t elem_size)
{
    v->data = NULL;
    v->len = 0;
    v->cap = 0;
    v->elem_size = elem_size;
}

void vec_free(Vec *v)
{
    free(v->data);
    v->data = NULL;
    v->len = 0;
    v->cap = 0;
}

int vec_reserve(Vec *v, size_t min_cap)
{
    if (min_cap <= v->cap)
        return 0;
    if (v->elem_size != 0 && min_cap > SIZE_MAX / v->elem_size)
        return -1;
    unsigned char *p = realloc(v->data, min_cap * v->elem_size);
    if (p == NULL)
        return -1;
    v->data = p;
    v->cap = min_cap;
    return 0;
}

/* Doubling keeps push amortised O(1): n pushes copy at most ~2n elements. */
static int grow_for_one_more(Vec *v)
{
    if (v->len < v->cap)
        return 0;
    size_t new_cap = v->cap < VEC_MIN_CAP ? VEC_MIN_CAP : v->cap * 2;
    if (new_cap < v->cap) /* overflow */
        return -1;
    return vec_reserve(v, new_cap);
}

int vec_push(Vec *v, const void *elem)
{
    if (grow_for_one_more(v) != 0)
        return -1;
    memcpy(v->data + v->len * v->elem_size, elem, v->elem_size);
    v->len++;
    return 0;
}

int vec_pop(Vec *v, void *out)
{
    if (v->len == 0)
        return -1;
    v->len--;
    if (out != NULL)
        memcpy(out, v->data + v->len * v->elem_size, v->elem_size);
    return 0;
}

int vec_insert(Vec *v, size_t index, const void *elem)
{
    if (index > v->len)
        return -1;
    if (grow_for_one_more(v) != 0)
        return -1;
    unsigned char *at = v->data + index * v->elem_size;
    memmove(at + v->elem_size, at, (v->len - index) * v->elem_size);
    memcpy(at, elem, v->elem_size);
    v->len++;
    return 0;
}

int vec_remove(Vec *v, size_t index, void *out)
{
    if (index >= v->len)
        return -1;
    unsigned char *at = v->data + index * v->elem_size;
    if (out != NULL)
        memcpy(out, at, v->elem_size);
    memmove(at, at + v->elem_size, (v->len - index - 1) * v->elem_size);
    v->len--;
    return 0;
}

void *vec_get(const Vec *v, size_t index)
{
    if (index >= v->len)
        return NULL;
    return v->data + index * v->elem_size;
}

int vec_set(Vec *v, size_t index, const void *elem)
{
    if (index >= v->len)
        return -1;
    memcpy(v->data + index * v->elem_size, elem, v->elem_size);
    return 0;
}

size_t vec_len(const Vec *v)
{
    return v->len;
}

void vec_clear(Vec *v)
{
    v->len = 0;
}
