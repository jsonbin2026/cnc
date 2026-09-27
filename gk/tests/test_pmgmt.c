#include "gk_test.h"
#include "gk/gk_pmgmt.h"

#include <math.h>

static void test_orders(void)
{
    gk_pmgmt_order o;
    GK_CHECK_STR_EQ(gk_pmgmt_order_state_name(GK_PMGMT_ORDER_NEW), "new");
    gk_pmgmt_order_init(&o, "SO-1", "shaft", 100, 18.0);
    GK_CHECK_STR_EQ(o.id, "SO-1");
    GK_CHECK_EQ_INT(o.state, GK_PMGMT_ORDER_NEW);
    GK_CHECK(gk_pmgmt_order_start(&o) == GK_ERR_STATE);
    GK_CHECK(gk_pmgmt_order_schedule(&o) == GK_OK);
    GK_CHECK(gk_pmgmt_order_start(&o) == GK_OK);
    GK_CHECK_EQ_INT(o.state, GK_PMGMT_ORDER_RUNNING);
    GK_CHECK(gk_pmgmt_order_finish(&o) == GK_OK);
    GK_CHECK_EQ_INT(gk_pmgmt_order_late(&o, 20.0), 0);
    gk_pmgmt_order_init(&o, "SO-2", "plate", 10, 10.0);
    GK_CHECK_EQ_INT(gk_pmgmt_order_late(&o, 12.0), 1);
}

static void test_schedule(void)
{
    gk_pmgmt_schedule s;
    gk_pmgmt_schedule_init(&s);
    GK_CHECK(gk_pmgmt_schedule_add(&s, "A", 0.0, 10.0) == GK_OK);
    GK_CHECK(gk_pmgmt_schedule_add(&s, "B", 10.0, 20.0) == GK_OK);
    GK_CHECK(gk_pmgmt_schedule_add(&s, "C", 5.0, 15.0) ==
             GK_ERR_ALREADY_EXISTS);
    GK_CHECK_EQ_INT(gk_pmgmt_schedule_overlaps(&s), 0);
    GK_CHECK(fabs(gk_pmgmt_schedule_makespan(&s) - 20.0) < 1e-9);
    GK_CHECK(gk_pmgmt_schedule_add(&s, "X", 15.0, 5.0) == GK_ERR_INVALID_ARG);
}

static void test_progress_delivery(void)
{
    gk_pmgmt_progress p;
    gk_pmgmt_delivery d;
    gk_pmgmt_progress_init(&p, 100);
    GK_CHECK(fabs(gk_pmgmt_progress_ratio(&p)) < 1e-12);
    GK_CHECK(gk_pmgmt_progress_set(&p, 40) == GK_OK);
    GK_CHECK(fabs(gk_pmgmt_progress_ratio(&p) - 0.4) < 1e-9);
    GK_CHECK(gk_pmgmt_progress_set(&p, 101) == GK_ERR_OUT_OF_RANGE);
    gk_pmgmt_delivery_init(&d, 20.0, 19.0);
    GK_CHECK_EQ_INT(gk_pmgmt_delivery_ontime(&d), 1);
    d.actual_day = 21.0;
    GK_CHECK_EQ_INT(gk_pmgmt_delivery_ontime(&d), 0);
}

static void test_modules(void)
{
    gk_pmgmt_registry r;
    GK_CHECK_STR_EQ(gk_pmgmt_module_name(GK_PMGMT_MODULE_KANBAN), "kanban");
    GK_CHECK_STR_EQ(gk_pmgmt_module_name(GK_PMGMT_MODULE_PERFORMANCE),
                    "performance");
    gk_pmgmt_registry_init(&r, GK_PMGMT_MODULE_COST);
    GK_CHECK_EQ_INT(gk_pmgmt_registry_count(&r), 0);
    GK_CHECK(gk_pmgmt_registry_add(&r, 5) == GK_OK);
    GK_CHECK_EQ_INT(gk_pmgmt_registry_count(&r), 5);
    GK_CHECK(gk_pmgmt_registry_add(&r, -1) == GK_ERR_INVALID_ARG);
}

static void test_exception_meeting(void)
{
    gk_pmgmt_exception e;
    gk_pmgmt_meeting m;
    gk_pmgmt_exception_init(&e, "EX-01", 0.8);
    GK_CHECK_STR_EQ(e.code, "EX-01");
    GK_CHECK_EQ_INT(e.resolved, 0);
    GK_CHECK(gk_pmgmt_exception_resolve(&e) == GK_OK);
    GK_CHECK_EQ_INT(e.resolved, 1);
    gk_pmgmt_meeting_init(&m, "daily", 6);
    GK_CHECK_EQ_INT(m.attendees, 6);
    GK_CHECK(gk_pmgmt_meeting_add_decision(&m, 15.0) == GK_OK);
    GK_CHECK(gk_pmgmt_meeting_add_decision(&m, 10.0) == GK_OK);
    GK_CHECK_EQ_INT(m.decisions, 2);
    GK_CHECK(fabs(m.minutes - 25.0) < 1e-9);
}

int main(void)
{
    test_orders();
    test_schedule();
    test_progress_delivery();
    test_modules();
    test_exception_meeting();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
