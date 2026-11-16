#include "list.h"

#include <stdlib.h>

static int same(const void *key, const void *data, list_eq_fn eq)
{
    return eq != NULL ? eq(key, data) : key == data;
}

/* ---------- singly linked ---------- */

void slist_init(SList *l)
{
    l->head = NULL;
    l->tail = NULL;
    l->len = 0;
}

void slist_free_with(SList *l, void (*free_data)(void *))
{
    SNode *n = l->head;
    while (n != NULL) {
        SNode *next = n->next;
        if (free_data != NULL)
            free_data(n->data);
        free(n);
        n = next;
    }
    slist_init(l);
}

void slist_free(SList *l)
{
    slist_free_with(l, NULL);
}

static SNode *snode_new(void *data, SNode *next)
{
    SNode *n = malloc(sizeof *n);
    if (n != NULL) {
        n->data = data;
        n->next = next;
    }
    return n;
}

int slist_push_front(SList *l, void *data)
{
    SNode *n = snode_new(data, l->head);
    if (n == NULL)
        return -1;
    l->head = n;
    if (l->tail == NULL)
        l->tail = n;
    l->len++;
    return 0;
}

int slist_push_back(SList *l, void *data)
{
    SNode *n = snode_new(data, NULL);
    if (n == NULL)
        return -1;
    if (l->tail != NULL)
        l->tail->next = n;
    else
        l->head = n;
    l->tail = n;
    l->len++;
    return 0;
}

int slist_pop_front(SList *l, void **out)
{
    SNode *n = l->head;
    if (n == NULL)
        return -1;
    l->head = n->next;
    if (l->head == NULL)
        l->tail = NULL;
    if (out != NULL)
        *out = n->data;
    free(n);
    l->len--;
    return 0;
}

SNode *slist_find(const SList *l, const void *key, list_eq_fn eq)
{
    for (SNode *n = l->head; n != NULL; n = n->next)
        if (same(key, n->data, eq))
            return n;
    return NULL;
}

int slist_remove_first(SList *l, const void *key, list_eq_fn eq)
{
    /* Walk with a pointer to the link we would rewrite, so removing the
     * head is not a special case. */
    SNode **link = &l->head;
    SNode *prev = NULL;
    while (*link != NULL) {
        SNode *n = *link;
        if (same(key, n->data, eq)) {
            *link = n->next;
            if (l->tail == n)
                l->tail = prev;
            free(n);
            l->len--;
            return 1;
        }
        prev = n;
        link = &n->next;
    }
    return 0;
}

void slist_reverse(SList *l)
{
    SNode *prev = NULL;
    SNode *cur = l->head;
    l->tail = l->head;
    while (cur != NULL) {
        SNode *next = cur->next;
        cur->next = prev;
        prev = cur;
        cur = next;
    }
    l->head = prev;
}

/* ---------- doubly linked ---------- */

void dlist_init(DList *l)
{
    l->head = NULL;
    l->tail = NULL;
    l->len = 0;
}

void dlist_free_with(DList *l, void (*free_data)(void *))
{
    DNode *n = l->head;
    while (n != NULL) {
        DNode *next = n->next;
        if (free_data != NULL)
            free_data(n->data);
        free(n);
        n = next;
    }
    dlist_init(l);
}

void dlist_free(DList *l)
{
    dlist_free_with(l, NULL);
}

int dlist_insert_after(DList *l, DNode *node, void *data)
{
    DNode *n = malloc(sizeof *n);
    if (n == NULL)
        return -1;
    n->data = data;
    n->prev = node;
    n->next = node != NULL ? node->next : l->head;
    if (n->next != NULL)
        n->next->prev = n;
    else
        l->tail = n;
    if (node != NULL)
        node->next = n;
    else
        l->head = n;
    l->len++;
    return 0;
}

int dlist_push_front(DList *l, void *data)
{
    return dlist_insert_after(l, NULL, data);
}

int dlist_push_back(DList *l, void *data)
{
    return dlist_insert_after(l, l->tail, data);
}

void *dlist_remove(DList *l, DNode *node)
{
    if (node->prev != NULL)
        node->prev->next = node->next;
    else
        l->head = node->next;
    if (node->next != NULL)
        node->next->prev = node->prev;
    else
        l->tail = node->prev;
    void *data = node->data;
    free(node);
    l->len--;
    return data;
}

int dlist_pop_front(DList *l, void **out)
{
    if (l->head == NULL)
        return -1;
    void *data = dlist_remove(l, l->head);
    if (out != NULL)
        *out = data;
    return 0;
}

int dlist_pop_back(DList *l, void **out)
{
    if (l->tail == NULL)
        return -1;
    void *data = dlist_remove(l, l->tail);
    if (out != NULL)
        *out = data;
    return 0;
}

DNode *dlist_find(const DList *l, const void *key, list_eq_fn eq)
{
    for (DNode *n = l->head; n != NULL; n = n->next)
        if (same(key, n->data, eq))
            return n;
    return NULL;
}
