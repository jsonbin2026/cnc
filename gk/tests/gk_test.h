#ifndef GK_TEST_H
#define GK_TEST_H

#include <stdio.h>

extern int g_tests_run;
extern int g_tests_failed;

#define GK_CHECK(cond)                                                     \
    do {                                                                   \
        g_tests_run += 1;                                                  \
        if (!(cond)) {                                                     \
            g_tests_failed += 1;                                           \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                  \
    } while (0)

#define GK_CHECK_EQ_INT(a, b)                                              \
    do {                                                                   \
        long long va = (long long)(a);                                     \
        long long vb = (long long)(b);                                     \
        g_tests_run += 1;                                                  \
        if (va != vb) {                                                    \
            g_tests_failed += 1;                                           \
            fprintf(stderr, "FAIL %s:%d: %s == %s (%lld != %lld)\n",       \
                    __FILE__, __LINE__, #a, #b, va, vb);                   \
        }                                                                  \
    } while (0)

#define GK_CHECK_STR_EQ(a, b)                                              \
    do {                                                                   \
        const char *sa = (a);                                              \
        const char *sb = (b);                                              \
        g_tests_run += 1;                                                  \
        if (sa == NULL || sb == NULL || __builtin_strcmp(sa, sb) != 0) {   \
            g_tests_failed += 1;                                           \
            fprintf(stderr, "FAIL %s:%d: %s == %s (\"%s\" != \"%s\")\n",   \
                    __FILE__, __LINE__, #a, #b,                            \
                    sa != NULL ? sa : "(null)",                            \
                    sb != NULL ? sb : "(null)");                           \
        }                                                                  \
    } while (0)

#endif
