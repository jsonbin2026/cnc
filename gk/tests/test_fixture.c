#include "gk_test.h"
#include "gk/gk_fixture.h"

#include <math.h>

static void test_kinds(void)
{
    GK_CHECK_STR_EQ(gk_fixture_name(GK_FIX_VISE), "bench-vise");
    GK_CHECK_STR_EQ(gk_fixture_name(GK_FIX_QUICK_CHANGE), "quick-change");
    GK_CHECK_STR_EQ(gk_fixture_name((gk_fixture_kind)999), "unknown");
    GK_CHECK(gk_fixture_max_clamp_force(GK_FIX_HYDRAULIC) >
             gk_fixture_max_clamp_force(GK_FIX_VACUUM));
}

static void test_clamp(void)
{
    gk_fixture f;
    GK_CHECK(gk_fixture_init(&f, GK_FIX_VISE) == GK_OK);
    GK_CHECK_EQ_INT(f.clamped, 0);
    GK_CHECK_EQ_INT(gk_fixture_secure(&f, 100.0), 0);
    GK_CHECK(gk_fixture_clamp(&f, 10000.0) == GK_OK);
    GK_CHECK_EQ_INT(f.clamped, 1);
    GK_CHECK_EQ_INT(gk_fixture_secure(&f, 1000.0), 1);
    GK_CHECK_EQ_INT(gk_fixture_secure(&f, 9000.0), 0);
    GK_CHECK(gk_fixture_clamp(&f, 999999.0) == GK_ERR_OUT_OF_RANGE);
    GK_CHECK(gk_fixture_unclamp(&f) == GK_OK);
    GK_CHECK_EQ_INT(f.clamped, 0);
    GK_CHECK(gk_fixture_clamp(&f, -1.0) == GK_ERR_INVALID_ARG);
}

static void test_lifecycle(void)
{
    gk_fixture_lifecycle l;
    gk_fixture_lifecycle_init(&l, "plate-fixture", 0.05);
    GK_CHECK(gk_fixture_manufacture(&l) == GK_ERR_STATE);
    GK_CHECK(gk_fixture_design(&l) == GK_OK);
    GK_CHECK(gk_fixture_manufacture(&l) == GK_OK);
    GK_CHECK(gk_fixture_commission(&l, 0.10) == GK_ERR_OUT_OF_RANGE);
    GK_CHECK(gk_fixture_commission(&l, 0.02) == GK_OK);
    GK_CHECK_EQ_INT(gk_fixture_ready(&l), 1);
    GK_CHECK(gk_fixture_maintain(&l) == GK_OK);
    GK_CHECK_EQ_INT(l.maintained, 1);
}

static void test_stock(void)
{
    gk_fixture_stock s;
    gk_fixture_stock_init(&s);
    GK_CHECK(gk_fixture_stock_add(&s, "F01", GK_FIX_VISE, 500.0, 2) == GK_OK);
    GK_CHECK(gk_fixture_stock_add(&s, "F02", GK_FIX_COLLET, 200.0, 3) == GK_OK);
    GK_CHECK(gk_fixture_stock_add(&s, "F01", GK_FIX_VISE, 500.0, 1) == GK_OK);
    GK_CHECK_EQ_INT(s.count, 2);
    GK_CHECK_EQ_INT(gk_fixture_stock_total_qty(&s), 6);
    GK_CHECK(fabs(gk_fixture_stock_total_cost(&s) - 2100.0) < 1e-9);
    GK_CHECK(gk_fixture_stock_consume(&s, "F02", 2) == GK_OK);
    GK_CHECK_EQ_INT(gk_fixture_stock_total_qty(&s), 4);
    GK_CHECK(gk_fixture_stock_consume(&s, "F02", 99) == GK_ERR_OUT_OF_RANGE);
    GK_CHECK(gk_fixture_stock_consume(&s, "F99", 1) == GK_ERR_NOT_FOUND);
}

int main(void)
{
    test_kinds();
    test_clamp();
    test_lifecycle();
    test_stock();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
