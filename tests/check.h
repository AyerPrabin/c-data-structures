#ifndef CHECK_H
#define CHECK_H

#include <stdio.h>
#include <string.h>

/* Tiny test helpers. No framework: each CHECK counts one check, and a failed
 * one prints where it was and carries on so you see every failure at once. */

extern int checks_run;
extern int checks_failed;

#define CHECK(cond)                                                       \
    do {                                                                  \
        checks_run++;                                                     \
        if (!(cond)) {                                                    \
            checks_failed++;                                              \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__,       \
                    __LINE__, #cond);                                     \
        }                                                                 \
    } while (0)

#define CHECK_EQ_LONG(got, want)                                          \
    do {                                                                  \
        long got_ = (long)(got), want_ = (long)(want);                    \
        checks_run++;                                                     \
        if (got_ != want_) {                                              \
            checks_failed++;                                              \
            fprintf(stderr, "%s:%d: %s == %ld, expected %ld\n", __FILE__, \
                    __LINE__, #got, got_, want_);                         \
        }                                                                 \
    } while (0)

#define CHECK_STR(got, want)                                              \
    do {                                                                  \
        const char *got_ = (got), *want_ = (want);                        \
        checks_run++;                                                     \
        if (got_ == NULL || strcmp(got_, want_) != 0) {                   \
            checks_failed++;                                              \
            fprintf(stderr, "%s:%d: %s == \"%s\", expected \"%s\"\n",    \
                    __FILE__, __LINE__, #got, got_ ? got_ : "(null)",     \
                    want_);                                               \
        }                                                                 \
    } while (0)

void test_vector(void);
void test_list(void);
void test_hashmap(void);
void test_heap(void);
void test_bst(void);

#endif
