#include "bst.h"

#include <stdlib.h>

void bst_init(Bst *t, bst_cmp_fn cmp)
{
    t->root = NULL;
    t->len = 0;
    t->cmp = cmp;
}

void bst_free(Bst *t)
{
    /* Free without recursion or a stack: rotate left children up until the
     * current node has none, then free it and move right. */
    BstNode *n = t->root;
    while (n != NULL) {
        if (n->left != NULL) {
            BstNode *l = n->left;
            n->left = l->right;
            l->right = n;
            n = l;
        } else {
            BstNode *right = n->right;
            free(n);
            n = right;
        }
    }
    t->root = NULL;
    t->len = 0;
}

int bst_insert(Bst *t, const void *key, void *value)
{
    BstNode **link = &t->root;
    while (*link != NULL) {
        int c = t->cmp(key, (*link)->key);
        if (c == 0) {
            (*link)->value = value;
            return 0;
        }
        link = c < 0 ? &(*link)->left : &(*link)->right;
    }
    BstNode *n = malloc(sizeof *n);
    if (n == NULL)
        return -1;
    n->key = key;
    n->value = value;
    n->left = NULL;
    n->right = NULL;
    *link = n;
    t->len++;
    return 1;
}

/* Returns the link (pointer to the parent's child pointer) that points at the
 * node with `key`, or NULL if the key is not in the tree. */
static BstNode **find_link(const Bst *t, const void *key)
{
    BstNode **link = (BstNode **)&t->root;
    while (*link != NULL) {
        int c = t->cmp(key, (*link)->key);
        if (c == 0)
            return link;
        link = c < 0 ? &(*link)->left : &(*link)->right;
    }
    return NULL;
}

int bst_find(const Bst *t, const void *key, void **out)
{
    BstNode **link = find_link(t, key);
    if (link == NULL)
        return 0;
    if (out != NULL)
        *out = (*link)->value;
    return 1;
}

int bst_remove(Bst *t, const void *key, void **out)
{
    BstNode **link = find_link(t, key);
    if (link == NULL)
        return 0;
    BstNode *n = *link;
    if (out != NULL)
        *out = n->value;

    if (n->left != NULL && n->right != NULL) {
        /* Two children: take the in-order successor (smallest key in the
         * right subtree), move its key/value into n, and unlink the
         * successor instead. The successor has no left child. */
        BstNode **succ_link = &n->right;
        while ((*succ_link)->left != NULL)
            succ_link = &(*succ_link)->left;
        BstNode *succ = *succ_link;
        n->key = succ->key;
        n->value = succ->value;
        *succ_link = succ->right;
        free(succ);
    } else {
        /* Zero or one child: the child (or NULL) takes n's place. */
        *link = n->left != NULL ? n->left : n->right;
        free(n);
    }
    t->len--;
    return 1;
}

size_t bst_len(const Bst *t)
{
    return t->len;
}

size_t bst_height(const Bst *t)
{
    /* Level by level, using two vectors as the current and next level. */
    Vec level, next;
    vec_init(&level, sizeof(BstNode *));
    vec_init(&next, sizeof(BstNode *));
    size_t height = 0;
    if (t->root != NULL && vec_push(&level, &t->root) != 0)
        goto out;
    while (vec_len(&level) > 0) {
        height++;
        vec_clear(&next);
        for (size_t i = 0; i < vec_len(&level); i++) {
            BstNode *n = *(BstNode **)vec_get(&level, i);
            if ((n->left != NULL && vec_push(&next, &n->left) != 0) ||
                (n->right != NULL && vec_push(&next, &n->right) != 0)) {
                height = 0; /* out of memory: report 0 rather than a wrong number */
                goto out;
            }
        }
        Vec tmp = level;
        level = next;
        next = tmp;
    }
out:
    vec_free(&level);
    vec_free(&next);
    return height;
}

int bst_min(const Bst *t, const void **key, void **value)
{
    BstNode *n = t->root;
    if (n == NULL)
        return -1;
    while (n->left != NULL)
        n = n->left;
    if (key != NULL)
        *key = n->key;
    if (value != NULL)
        *value = n->value;
    return 0;
}

int bst_max(const Bst *t, const void **key, void **value)
{
    BstNode *n = t->root;
    if (n == NULL)
        return -1;
    while (n->right != NULL)
        n = n->right;
    if (key != NULL)
        *key = n->key;
    if (value != NULL)
        *value = n->value;
    return 0;
}

void bst_iter_init(BstIter *it, const Bst *t)
{
    vec_init(&it->stack, sizeof(BstNode *));
    it->next = t->root;
}

int bst_iter_next(BstIter *it, const void **key, void **value)
{
    /* it->next is the root of a subtree we have not started yet. Go as far
     * left as possible, stacking the nodes on the way; the top of the stack
     * is then the next node in order. After visiting it, its right subtree
     * is the next one to start. */
    while (it->next != NULL) {
        if (vec_push(&it->stack, &it->next) != 0)
            return -1;
        it->next = it->next->left;
    }
    BstNode *n;
    if (vec_pop(&it->stack, &n) != 0)
        return 0;
    if (key != NULL)
        *key = n->key;
    if (value != NULL)
        *value = n->value;
    it->next = n->right;
    return 1;
}

void bst_iter_free(BstIter *it)
{
    vec_free(&it->stack);
}
