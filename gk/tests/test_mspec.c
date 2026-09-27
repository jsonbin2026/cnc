#include "gk_test.h"
#include "gk/gk_mspec.h"

#include <math.h>
#include <string.h>

static void test_defaults(void)
{
    gk_mspec m;
    gk_mspec_init(&m);
    GK_CHECK_STR_EQ(m.model, "GK-VMC850");
    GK_CHECK_STR_EQ(m.spindle_taper, "BT40");
    GK_CHECK(fabs(m.weight_kg - 5500.0) < 1e-9);
    GK_CHECK(fabs(m.travel_x_mm - 800.0) < 1e-9);
    GK_CHECK_EQ_INT(m.atc_capacity, 24);
    GK_CHECK_EQ_INT(m.has_warmup_program, 1);
}

static void test_str_fields(void)
{
    gk_mspec m;
    gk_mspec_init(&m);
    GK_CHECK_STR_EQ(gk_mspec_field_name(1476), "nameplate");
    GK_CHECK_STR_EQ(gk_mspec_field_name(1525), "signal-tower-hookup");
    GK_CHECK_STR_EQ(gk_mspec_field_name(9999), "unknown");
    GK_CHECK(gk_mspec_set_str(&m, 1477, "SN-12345") == GK_OK);
    GK_CHECK_STR_EQ(gk_mspec_get_str(&m, 1477), "SN-12345");
    GK_CHECK(gk_mspec_set_str(&m, 1476, NULL) == GK_ERR_INVALID_ARG);
    GK_CHECK(gk_mspec_set_str(&m, 9999, "x") == GK_ERR_NOT_FOUND);
    GK_CHECK(gk_mspec_get_str(&m, 9999) == NULL);
}

static void test_num_fields(void)
{
    gk_mspec m;
    gk_mspec_init(&m);
    GK_CHECK(gk_mspec_set_num(&m, 1479, 6000.0) == GK_OK);
    GK_CHECK(fabs(gk_mspec_get_num(&m, 1479) - 6000.0) < 1e-9);
    GK_CHECK(gk_mspec_set_num(&m, 1485, 15000.0) == GK_OK);
    GK_CHECK(fabs(gk_mspec_get_num(&m, 1485) - 15000.0) < 1e-9);
    GK_CHECK(gk_mspec_set_num(&m, 1500, 400.0) == GK_OK);
    GK_CHECK(fabs(gk_mspec_get_num(&m, 1500) - 400.0) < 1e-9);
    GK_CHECK(gk_mspec_set_num(&m, 9999, 1.0) == GK_ERR_NOT_FOUND);
    GK_CHECK(fabs(gk_mspec_get_num(&m, 9999)) < 1e-12);
}

static void test_flag_fields(void)
{
    gk_mspec m;
    gk_mspec_init(&m);
    GK_CHECK_EQ_INT(gk_mspec_get_flag(&m, 1516), 0);
    GK_CHECK(gk_mspec_set_flag(&m, 1516, 1) == GK_OK);
    GK_CHECK_EQ_INT(gk_mspec_get_flag(&m, 1516), 1);
    GK_CHECK(gk_mspec_set_flag(&m, 1525, 1) == GK_OK);
    GK_CHECK_EQ_INT(gk_mspec_get_flag(&m, 1525), 1);
    GK_CHECK(gk_mspec_set_flag(&m, 9999, 1) == GK_ERR_NOT_FOUND);
}

static void test_derived(void)
{
    gk_mspec m;
    gk_mspec_init(&m);
    GK_CHECK_EQ_INT(gk_mspec_spindle_range_ok(&m), 1);
    GK_CHECK_EQ_INT(gk_mspec_tool_fits(&m, 50.0, 200.0, 5.0), 1);
    GK_CHECK_EQ_INT(gk_mspec_tool_fits(&m, 100.0, 200.0, 5.0), 0);
    GK_CHECK_EQ_INT(gk_mspec_fully_hooked_up(&m), 0);
    gk_mspec_set_flag(&m, 1516, 1);
    gk_mspec_set_flag(&m, 1517, 1);
    gk_mspec_set_flag(&m, 1518, 1);
    gk_mspec_set_flag(&m, 1519, 1);
    gk_mspec_set_flag(&m, 1520, 1);
    gk_mspec_set_flag(&m, 1521, 1);
    gk_mspec_set_flag(&m, 1523, 1);
    GK_CHECK_EQ_INT(gk_mspec_fully_hooked_up(&m), 1);
    GK_CHECK_EQ_INT(gk_mspec_ready_for_use(&m), 0);
    gk_mspec_set_flag(&m, 1512, 1);
    gk_mspec_set_flag(&m, 1513, 1);
    gk_mspec_set_flag(&m, 1514, 1);
    gk_mspec_set_flag(&m, 1510, 1);
    GK_CHECK_EQ_INT(gk_mspec_ready_for_use(&m), 1);
}

int main(void)
{
    test_defaults();
    test_str_fields();
    test_num_fields();
    test_flag_fields();
    test_derived();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
