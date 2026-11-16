#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>

/*
 * Growable array of fixed-size elements (like std::vector or a Python list).
 *
 * Elements are copied in and out with memcpy, so the vector owns the bytes
 * of each element. If an element is itself a pointer, the vector does NOT own
 * what it points to; free those yourself before calling vec_free.
 *
 * Pointers returned by vec_get are only valid until the next call that can
 * grow the buffer (vec_push, vec_insert, vec_reserve).
 *
 * Functions that can allocate return 0 on success and -1 if malloc/realloc
 * failed; the vector is left unchanged in that case.
 */

typedef struct {
    unsigned char *data;
    size_t len;       /* number of elements in use */
    size_t cap;       /* number of elements allocated */
    size_t elem_size; /* size of one element in bytes */
} Vec;

void vec_init(Vec *v, size_t elem_size);
void vec_free(Vec *v);

int vec_reserve(Vec *v, size_t min_cap);
int vec_push(Vec *v, const void *elem);
int vec_pop(Vec *v, void *out); /* returns -1 if empty; out may be NULL */
int vec_insert(Vec *v, size_t index, const void *elem); /* index <= len */
int vec_remove(Vec *v, size_t index, void *out);        /* index < len */

void *vec_get(const Vec *v, size_t index); /* NULL if out of range */
int vec_set(Vec *v, size_t index, const void *elem);

size_t vec_len(const Vec *v);
void vec_clear(Vec *v); /* len = 0, keeps the buffer */

#endif
