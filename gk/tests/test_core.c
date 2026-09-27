#include "gk_test.h"

#include "gk/gk_error.h"
#include "gk/gk_log.h"
#include "gk/gk_mem.h"
#include "gk/gk_string.h"
#include "gk/gk_vec.h"
#include "gk/gk_registry.h"
#include "gk/gk_catalog.h"

#include <string.h>

static void test_error(void)
{
    GK_CHECK_STR_EQ(gk_status_name(GK_OK), "GK_OK");
    GK_CHECK_STR_EQ(gk_status_message(GK_OK), "success");
    GK_CHECK(!gk_status_is_error(GK_OK));
    GK_CHECK(gk_status_is_error(GK_ERR_PARSE));
    GK_CHECK_STR_EQ(gk_status_name((gk_status)999), "GK_ERR_UNKNOWN");
    GK_CHECK_STR_EQ(gk_status_message((gk_status)-1), "unknown error");
}

static void test_log(void)
{
    gk_log_level lv;
    gk_log_set_level(GK_LOG_WARN);
    GK_CHECK_EQ_INT(gk_log_get_level(), GK_LOG_WARN);
    GK_CHECK_STR_EQ(gk_log_level_name(GK_LOG_ERROR), "ERROR");
    GK_CHECK(gk_log_level_from_name("DEBUG", &lv) == 1);
    GK_CHECK_EQ_INT(lv, GK_LOG_DEBUG);
    GK_CHECK(gk_log_level_from_name("NOPE", &lv) == 0);
    GK_CHECK(gk_log_level_from_name(NULL, &lv) == 0);
    gk_log_set_level(GK_LOG_INFO);
}

static void test_string(void)
{
    char buf[8];
    size_t n;
    char trim[] = "  hi  ";

    n = gk_strlcpy(buf, "abcdefghij", sizeof(buf));
    GK_CHECK_EQ_INT(n, 10);
    GK_CHECK_STR_EQ(buf, "abcdefg");

    gk_strlcpy(buf, "ab", sizeof(buf));
    gk_strlcat(buf, "cd", sizeof(buf));
    GK_CHECK_STR_EQ(buf, "abcd");

    GK_CHECK_EQ_INT(gk_strcasecmp("AbC", "aBc"), 0);
    GK_CHECK(gk_strcasecmp("abc", "abd") < 0);
    GK_CHECK(gk_str_eq("x", "x"));
    GK_CHECK(!gk_str_eq("x", "y"));

    GK_CHECK_STR_EQ(gk_str_trim_inplace(trim), "hi");
    GK_CHECK(gk_str_has_prefix("G00X10", "G00"));
    GK_CHECK(!gk_str_has_prefix("G01", "G00"));
    GK_CHECK(gk_str_has_suffix("prog.nc", ".nc"));
    GK_CHECK(!gk_str_has_suffix("prog.nc", ".txt"));
}

static void test_vec(void)
{
    gk_vec v;
    int i;
    int sum = 0;
    GK_CHECK_EQ_INT(gk_vec_init(&v, sizeof(int), 0, NULL), GK_OK);
    for (i = 1; i <= 100; ++i) {
        GK_CHECK_EQ_INT(gk_vec_push(&v, &i), GK_OK);
    }
    GK_CHECK_EQ_INT(gk_vec_size(&v), 100);
    for (i = 0; i < 100; ++i) {
        sum += *(int *)gk_vec_at(&v, (size_t)i);
    }
    GK_CHECK_EQ_INT(sum, 5050);
    GK_CHECK(gk_vec_at(&v, 1000) == NULL);
    gk_vec_clear(&v);
    GK_CHECK_EQ_INT(gk_vec_size(&v), 0);
    gk_vec_destroy(&v);
    GK_CHECK_EQ_INT(gk_vec_init(NULL, 4, 0, NULL), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_vec_init(&v, 0, 0, NULL), GK_ERR_INVALID_ARG);
}

static void test_registry(void)
{
    gk_registry r;
    int a = 1;
    int b = 2;
    GK_CHECK_EQ_INT(gk_registry_init(&r, NULL), GK_OK);
    GK_CHECK_EQ_INT(gk_registry_set(&r, "a", &a), GK_OK);
    GK_CHECK_EQ_INT(gk_registry_set(&r, "b", &b), GK_OK);
    GK_CHECK_EQ_INT(gk_registry_size(&r), 2);
    GK_CHECK(gk_registry_get(&r, "a") == &a);
    GK_CHECK_EQ_INT(gk_registry_set(&r, "a", &b), GK_OK);
    GK_CHECK(gk_registry_get(&r, "a") == &b);
    GK_CHECK_EQ_INT(gk_registry_size(&r), 2);
    GK_CHECK(gk_registry_get(&r, "zzz") == NULL);
    GK_CHECK_EQ_INT(gk_registry_remove(&r, "a"), GK_OK);
    GK_CHECK_EQ_INT(gk_registry_remove(&r, "a"), GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_registry_size(&r), 1);
    gk_registry_destroy(&r);
    GK_CHECK_EQ_INT(gk_registry_set(NULL, "x", NULL), GK_ERR_INVALID_ARG);
}

static void test_catalog(void)
{
    int errors = -1;
    const gk_feature *f;
    size_t total;

    total = gk_catalog_count();
    GK_CHECK_EQ_INT(total, 1525);
    GK_CHECK(gk_catalog_all() != NULL);

    f = gk_catalog_get(1);
    GK_CHECK(f != NULL);
    if (f != NULL) {
        GK_CHECK_EQ_INT(f->id, 1);
        GK_CHECK_EQ_INT(f->domain, GK_DOMAIN_GCODE);
    }
    f = gk_catalog_get(1525);
    GK_CHECK(f != NULL);
    if (f != NULL) {
        GK_CHECK_EQ_INT(f->id, 1525);
        GK_CHECK_EQ_INT(f->domain, GK_DOMAIN_REALISM_FINAL);
    }
    GK_CHECK(gk_catalog_get(0) == NULL);
    GK_CHECK(gk_catalog_get(1526) == NULL);

    f = gk_catalog_find_by_name("G01 直线插补");
    GK_CHECK(f != NULL);
    if (f != NULL) {
        GK_CHECK_EQ_INT(f->id, 2);
    }
    GK_CHECK(gk_catalog_find_by_name("no such feature") == NULL);

    GK_CHECK_EQ_INT(gk_catalog_count_by_domain(GK_DOMAIN_COUNT), 0);
    GK_CHECK(gk_catalog_count_by_domain(GK_DOMAIN_GCODE) == 82);
    GK_CHECK(gk_catalog_count_by_domain(GK_DOMAIN_REALISM) == 505);
    GK_CHECK_EQ_INT(gk_catalog_count_by_status(GK_STATUS_NOT_IMPLEMENTED), 0);
    GK_CHECK_EQ_INT(gk_catalog_count_by_status(GK_STATUS_IMPLEMENTED), 52);
    GK_CHECK_EQ_INT(gk_catalog_count_by_status(GK_STATUS_VERIFIED), 1473);
    {
        const gk_feature *ver = gk_catalog_get(20);
        GK_CHECK(ver != NULL);
        if (ver != NULL) {
            GK_CHECK_EQ_INT(ver->status, GK_STATUS_IMPLEMENTED);
            GK_CHECK(ver->impl_symbol != NULL);
        }
        {
            const gk_feature *v2 = gk_catalog_get(3);
            GK_CHECK(v2 != NULL);
            if (v2 != NULL) {
                GK_CHECK_EQ_INT(v2->status, GK_STATUS_VERIFIED);
                GK_CHECK(v2->impl_symbol != NULL);
            }
        }
        {
            const gk_feature *nf = gk_catalog_get(9999);
            GK_CHECK(nf == NULL);
        }
    }

    GK_CHECK_EQ_INT(gk_catalog_validate(&errors), GK_OK);
    GK_CHECK_EQ_INT(errors, 0);

    GK_CHECK_STR_EQ(gk_domain_code(GK_DOMAIN_GCODE), "GCODE");
    GK_CHECK_STR_EQ(gk_feature_status_name(GK_STATUS_NOT_IMPLEMENTED),
                    "NOT_IMPLEMENTED");
    GK_CHECK_STR_EQ(gk_domain_name((gk_domain)999), "未知");
}

static void test_catalog_domain_sum(void)
{
    int d;
    size_t sum = 0;
    for (d = 0; d < (int)GK_DOMAIN_COUNT; ++d) {
        sum += gk_catalog_count_by_domain((gk_domain)d);
    }
    GK_CHECK_EQ_INT(sum, gk_catalog_count());
}

int main(void)
{
    test_error();
    test_log();
    test_string();
    test_vec();
    test_registry();
    test_catalog();
    test_catalog_domain_sum();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
