#ifndef LIST_H
#define LIST_H

#include <stddef.h>

/*
 * Singly and doubly linked lists of void pointers.
 *
 * Ownership: the lists own their nodes but NOT the data pointers stored in
 * them. slist_free / dlist_free release the nodes only. If the data was
 * malloc'd, either free it while walking the list first or pass a free
 * function to *_free_with.
 *
 * Functions that allocate return 0 on success and -1 on allocation failure.
 */

typedef int (*list_eq_fn)(const void *a, const void *b); /* nonzero if equal */

/* ---------- singly linked ---------- */

typedef struct SNode {
    void *data;
    struct SNode *next;
} SNode;

typedef struct {
    SNode *head;
    SNode *tail; /* kept so push_back is O(1) */
    size_t len;
} SList;

void slist_init(SList *l);
void slist_free(SList *l);
void slist_free_with(SList *l, void (*free_data)(void *));

int slist_push_front(SList *l, void *data);
int slist_push_back(SList *l, void *data);
int slist_pop_front(SList *l, void **out); /* -1 if empty */

/* Removes the first node whose data equals `key` (by eq, or by pointer if
 * eq is NULL). Returns 1 if a node was removed, 0 if not found. */
int slist_remove_first(SList *l, const void *key, list_eq_fn eq);
SNode *slist_find(const SList *l, const void *key, list_eq_fn eq);
void slist_reverse(SList *l);

/* ---------- doubly linked ---------- */

typedef struct DNode {
    void *data;
    struct DNode *prev;
    struct DNode *next;
} DNode;

typedef struct {
    DNode *head;
    DNode *tail;
    size_t len;
} DList;

void dlist_init(DList *l);
void dlist_free(DList *l);
void dlist_free_with(DList *l, void (*free_data)(void *));

int dlist_push_front(DList *l, void *data);
int dlist_push_back(DList *l, void *data);
int dlist_pop_front(DList *l, void **out);
int dlist_pop_back(DList *l, void **out);

/* Insert after `node`; node == NULL means insert at the front. */
int dlist_insert_after(DList *l, DNode *node, void *data);
/* Unlinks and frees `node` (which must belong to l) in O(1); returns its data. */
void *dlist_remove(DList *l, DNode *node);
DNode *dlist_find(const DList *l, const void *key, list_eq_fn eq);

#endif
