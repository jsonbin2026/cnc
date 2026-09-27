#include "gk_test.h"
#include "gk/gk_fscen.h"

#include <math.h>

static void test_names(void)
{
    GK_CHECK_STR_EQ(gk_fscen_name(GK_FSCEN_POWER_LOSS), "power-loss");
    GK_CHECK_STR_EQ(gk_fscen_name(GK_FSCEN_INJURY), "personnel-injury");
    GK_CHECK_STR_EQ(gk_fscen_name((gk_fscen_kind)999), "unknown");
}

static void test_severity(void)
{
    GK_CHECK(gk_fscen_severity(GK_FSCEN_INJURY) == 5);
    GK_CHECK(gk_fscen_severity(GK_FSCEN_ESTOP_MISTOUCH) == 1);
    GK_CHECK(gk_fscen_severity(GK_FSCEN_FIRE) > gk_fscen_severity(GK_FSCEN_LUBE_LOW));
    GK_CHECK(gk_fscen_is_safety(GK_FSCEN_FIRE) == 1);
    GK_CHECK(gk_fscen_is_safety(GK_FSCEN_POWER_LOSS) == 0);
}

static void test_lifecycle(void)
{
    gk_fscen_event e;
    GK_CHECK(gk_fscen_trigger(&e, GK_FSCEN_TOOL_BREAK) == GK_OK);
    GK_CHECK_STR_EQ(e.description, "tool-break");
    GK_CHECK_EQ_INT(gk_fscen_handled(&e), 0);
    GK_CHECK(gk_fscen_respond(&e, 1) == GK_ERR_STATE);
    GK_CHECK(gk_fscen_detect(&e, 1) == GK_OK);
    GK_CHECK(gk_fscen_respond(&e, 1) == GK_OK);
    GK_CHECK_EQ_INT(gk_fscen_handled(&e), 1);
    GK_CHECK(gk_fscen_trigger(&e, GK_FSCEN_FIRE) == GK_OK);
    GK_CHECK(e.probability > 0.0);
}

static void test_recovery(void)
{
    GK_CHECK_STR_EQ(gk_fscen_recovery(GK_FSCEN_POWER_LOSS),
                    "check power, restore, re-home");
    GK_CHECK_STR_EQ(gk_fscen_recovery(GK_FSCEN_FIRE), "extinguish, evacuate, inspect");
}

static void test_stats(void)
{
    gk_fscen_stats s;
    gk_fscen_stats_init(&s);
    GK_CHECK(fabs(gk_fscen_mtbf(&s)) < 1e-12);
    GK_CHECK(gk_fscen_stats_add(&s, 100.0, 4.0) == GK_OK);
    GK_CHECK(gk_fscen_stats_add(&s, 200.0, 6.0) == GK_OK);
    GK_CHECK(fabs(gk_fscen_mtbf(&s) - 150.0) < 1e-9);
    GK_CHECK(fabs(gk_fscen_mttr(&s) - 5.0) < 1e-9);
    GK_CHECK(fabs(gk_fscen_availability(&s) - 300.0 / 310.0) < 1e-9);
    GK_CHECK(gk_fscen_stats_add(&s, -1.0, 0.0) == GK_ERR_INVALID_ARG);
}

int main(void)
{
    test_names();
    test_severity();
    test_lifecycle();
    test_recovery();
    test_stats();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
