#include "check.h"

int checks_run = 0;
int checks_failed = 0;

int main(void)
{
    struct {
        const char *name;
        void (*fn)(void);
    } suites[] = {
        {"vector", test_vector},
        {"list", test_list},
        {"hashmap", test_hashmap},
        {"heap", test_heap},
        {"bst", test_bst},
    };

    for (size_t i = 0; i < sizeof suites / sizeof suites[0]; i++) {
        int before_run = checks_run, before_failed = checks_failed;
        suites[i].fn();
        printf("%-8s %4d checks, %d failed\n", suites[i].name,
               checks_run - before_run, checks_failed - before_failed);
    }
    printf("total    %4d checks, %d failed\n", checks_run, checks_failed);
    return checks_failed == 0 ? 0 : 1;
}
