#include "gk_test.h"
#include "gk/gk_toolsys.h"

#include <math.h>

static void test_id(void)
{
    gk_toolsys_id t;
    gk_toolsys_id_init(&t, "T01", "flat-endmill");
    GK_CHECK_STR_EQ(t.code, "T01");
    GK_CHECK_STR_EQ(t.name, "flat-endmill");
    GK_CHECK_EQ_INT(gk_toolsys_match_rfid(&t, "RF1"), 0);
    GK_CHECK(gk_toolsys_set_rfid(&t, "RF1") == GK_OK);
    GK_CHECK_EQ_INT(gk_toolsys_match_rfid(&t, "RF1"), 1);
    GK_CHECK_EQ_INT(gk_toolsys_match_rfid(&t, "RF2"), 0);
    GK_CHECK(gk_toolsys_set_rfid(&t, "") == GK_ERR_INVALID_ARG);
}

static void test_preset(void)
{
    gk_toolsys_preset p;
    gk_toolsys_preset_init(&p);
    GK_CHECK(gk_toolsys_preset_set(&p, 100.0, 10.0) == GK_OK);
    p.measured_length_mm = 100.02;
    p.measured_diameter_mm = 10.03;
    GK_CHECK(fabs(gk_toolsys_preset_len_error(&p) - 0.02) < 1e-9);
    GK_CHECK(fabs(gk_toolsys_preset_dia_error(&p) - 0.03) < 1e-9);
    GK_CHECK_EQ_INT(gk_toolsys_preset_ok(&p, 0.05), 1);
    GK_CHECK_EQ_INT(gk_toolsys_preset_ok(&p, 0.01), 0);
    GK_CHECK(gk_toolsys_preset_set(&p, -1.0, 10.0) == GK_ERR_OUT_OF_RANGE);
}

static void test_assembly_condition(void)
{
    gk_toolsys_assembly a;
    gk_toolsys_condition c;
    gk_toolsys_assembly_init(&a);
    GK_CHECK(gk_toolsys_assembly_add(&a, "holder") == GK_OK);
    GK_CHECK(gk_toolsys_assembly_add(&a, "collet") == GK_OK);
    GK_CHECK_EQ_INT(a.components, 2);
    GK_CHECK_EQ_INT(gk_toolsys_balance_ok(&a), 1);
    a.balance_grade = 6.3;
    GK_CHECK_EQ_INT(gk_toolsys_balance_ok(&a), 0);
    gk_toolsys_condition_init(&c);
    GK_CHECK(gk_toolsys_set_runout(&c, 5.0) == GK_OK);
    GK_CHECK_EQ_INT(gk_toolsys_runout_ok(&c, 10.0), 1);
    GK_CHECK_EQ_INT(gk_toolsys_runout_ok(&c, 3.0), 0);
    c.wear_mm = 0.3;
    GK_CHECK_EQ_INT(gk_toolsys_worn(&c, 0.25), 1);
    GK_CHECK(gk_toolsys_mark_broken(&c) == GK_OK);
    GK_CHECK_EQ_INT(c.broken, 1);
}

static void test_life(void)
{
    gk_toolsys_life l;
    gk_toolsys_life_init(&l, 60.0);
    GK_CHECK(fabs(gk_toolsys_life_remaining(&l) - 60.0) < 1e-9);
    GK_CHECK(gk_toolsys_life_use(&l, 20.0) == GK_OK);
    GK_CHECK(fabs(gk_toolsys_life_remaining(&l) - 40.0) < 1e-9);
    GK_CHECK_EQ_INT(gk_toolsys_life_expired(&l), 0);
    GK_CHECK(gk_toolsys_life_use(&l, 50.0) == GK_OK);
    GK_CHECK_EQ_INT(gk_toolsys_life_expired(&l), 1);
    GK_CHECK(fabs(gk_toolsys_life_remaining(&l)) < 1e-9);
}

static void test_crib(void)
{
    gk_toolsys_crib c;
    GK_CHECK_STR_EQ(gk_toolsys_status_name(GK_TOOLSYS_STATUS_REGRIND), "regrind");
    gk_toolsys_crib_init(&c);
    GK_CHECK(gk_toolsys_crib_add(&c, "T01", "supplierA", 120.0) == GK_OK);
    GK_CHECK(gk_toolsys_crib_add(&c, "T02", "supplierB", 80.0) == GK_OK);
    GK_CHECK(gk_toolsys_crib_add(&c, "T01", "x", 1.0) == GK_ERR_ALREADY_EXISTS);
    GK_CHECK_EQ_INT(gk_toolsys_crib_count(&c), 2);
    GK_CHECK(gk_toolsys_crib_find(&c, "T02") != NULL);
    GK_CHECK(gk_toolsys_crib_find(&c, "T99") == NULL);
    GK_CHECK(gk_toolsys_crib_set_status(&c, "T01", GK_TOOLSYS_STATUS_IN_USE) ==
             GK_OK);
    GK_CHECK_EQ_INT(gk_toolsys_crib_count_status(&c, GK_TOOLSYS_STATUS_IN_USE),
                    1);
    GK_CHECK(fabs(gk_toolsys_crib_total_value(&c) - 200.0) < 1e-9);
    GK_CHECK(gk_toolsys_crib_set_status(&c, "T99", GK_TOOLSYS_STATUS_IN_USE) ==
             GK_ERR_NOT_FOUND);
}

static void test_order_trial(void)
{
    gk_toolsys_order o;
    gk_toolsys_trial t;
    gk_toolsys_order_init(&o, "T01", 10, 25.0, 14.0, "supplierA");
    GK_CHECK_STR_EQ(o.supplier, "supplierA");
    GK_CHECK(fabs(gk_toolsys_order_total(&o) - 250.0) < 1e-9);
    gk_toolsys_trial_init(&t);
    t.tool_life_min = 100.0;
    t.surface_finish_ra = 0.8;
    t.cost_per_part = 1.0;
    GK_CHECK(gk_toolsys_trial_score(&t) > 0.0);
}

int main(void)
{
    test_id();
    test_preset();
    test_assembly_condition();
    test_life();
    test_crib();
    test_order_trial();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
