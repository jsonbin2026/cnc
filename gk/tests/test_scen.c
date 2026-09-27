#include "gk_test.h"
#include "gk/gk_scen.h"

#include <math.h>

static void test_names(void)
{
    GK_CHECK_STR_EQ(gk_scen_name(GK_SCEN_AL_HS), "aluminium-high-speed");
    GK_CHECK_STR_EQ(gk_scen_name(GK_SCEN_LASER), "laser-assisted");
    GK_CHECK_STR_EQ(gk_scen_name((gk_scen_kind)999), "unknown");
}

static void test_recommend(void)
{
    gk_scen_params p;
    GK_CHECK(gk_scen_recommend(GK_SCEN_AL_HS, &p) == GK_OK);
    GK_CHECK(p.surface_speed > 500.0);
    GK_CHECK_EQ_INT(p.teeth, 3);
    GK_CHECK(gk_scen_recommend(GK_SCEN_TITANIUM, &p) == GK_OK);
    GK_CHECK(p.surface_speed < 100.0);
    GK_CHECK(p.coolant_flow > 20.0);
    GK_CHECK(gk_scen_recommend((gk_scen_kind)999, &p) == GK_ERR_OUT_OF_RANGE);
}

static void test_calc(void)
{
    gk_scen_params p;
    double rpm, feed;
    gk_scen_recommend(GK_SCEN_AL_HS, &p);
    rpm = gk_scen_rpm(&p, 10.0);
    GK_CHECK(rpm > 20000.0);
    feed = gk_scen_feed_rate(&p, 10.0);
    GK_CHECK(fabs(feed - rpm * p.feed_per_tooth * p.teeth) < 1e-6);
    GK_CHECK(gk_scen_mrr(&p, 10.0, 5.0, 5.0) > 0.0);
    GK_CHECK(fabs(gk_scen_rpm(&p, 0.0)) < 1e-12);
}

static void test_coolant(void)
{
    gk_scen_params p;
    gk_scen_recommend(GK_SCEN_STEEL_HEAVY, &p);
    GK_CHECK(gk_scen_set_coolant(&p, GK_SCEN_DRY, 30.0) == GK_OK);
    GK_CHECK_EQ_INT(p.is_wet, 0);
    GK_CHECK(fabs(p.coolant_flow) < 1e-12);
    GK_CHECK(gk_scen_set_coolant(&p, GK_SCEN_MQL, 0.0) == GK_OK);
    GK_CHECK(p.coolant_flow > 0.0);
    GK_CHECK(gk_scen_set_coolant(&p, GK_SCEN_WET, 40.0) == GK_OK);
    GK_CHECK(fabs(p.coolant_flow - 40.0) < 1e-9);
    GK_CHECK_EQ_INT(gk_scen_needs_coolant(GK_SCEN_DRY), 0);
    GK_CHECK_EQ_INT(gk_scen_needs_coolant(GK_SCEN_TITANIUM), 1);
}

static void test_ratings(void)
{
    GK_CHECK(gk_scen_difficulty(GK_SCEN_SUPERALLOY) >
             gk_scen_difficulty(GK_SCEN_AL_HS));
    GK_CHECK(gk_scen_expected_ra(GK_SCEN_MIRROR) <
             gk_scen_expected_ra(GK_SCEN_STEEL_HEAVY));
    GK_CHECK(gk_scen_difficulty(GK_SCEN_MIRROR) == 5);
}

int main(void)
{
    test_names();
    test_recommend();
    test_calc();
    test_coolant();
    test_ratings();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
