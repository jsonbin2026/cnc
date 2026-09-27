#include "gk_test.h"
#include "gk/gk_rec.h"

#include <math.h>
#include <string.h>

static void test_logbook(void)
{
    gk_rec_logbook lb;
    const gk_rec_entry *e;
    GK_CHECK_STR_EQ(gk_rec_log_name(GK_REC_LOG_ALARM), "alarm-log");
    GK_CHECK(gk_rec_logbook_init(&lb, GK_REC_LOG_MACHINING) == GK_OK);
    GK_CHECK_EQ_INT(gk_rec_log_count(&lb), 0);
    GK_CHECK(gk_rec_log_append(&lb, "12:00:01", "cycle start", 10.0) == GK_OK);
    GK_CHECK(gk_rec_log_append(&lb, "12:00:05", "cycle end", 20.0) == GK_OK);
    GK_CHECK_EQ_INT(gk_rec_log_count(&lb), 2);
    e = gk_rec_log_get(&lb, 0);
    GK_CHECK(e != NULL);
    GK_CHECK_STR_EQ(e->message, "cycle start");
    GK_CHECK_EQ_INT(e->kind, GK_REC_LOG_MACHINING);
    GK_CHECK(gk_rec_log_get(&lb, 5) == NULL);
    GK_CHECK(fabs(gk_rec_log_sum(&lb) - 30.0) < 1e-9);
    GK_CHECK(fabs(gk_rec_log_avg(&lb) - 15.0) < 1e-9);
    GK_CHECK(gk_rec_log_append(&lb, NULL, "x", 0.0) == GK_ERR_INVALID_ARG);
}

static void test_oee(void)
{
    gk_rec_production p;
    gk_rec_production_init(&p);
    p.planned_time_min = 480.0;
    p.run_time_min = 432.0;
    p.ideal_cycle_min = 0.5;
    p.total_count = 800.0;
    p.good_count = 780.0;
    GK_CHECK(fabs(gk_rec_availability(&p) - 0.9) < 1e-9);
    GK_CHECK(fabs(gk_rec_performance(&p) - (0.5 * 800.0 / 432.0)) < 1e-9);
    GK_CHECK(fabs(gk_rec_quality(&p) - 780.0 / 800.0) < 1e-9);
    GK_CHECK(gk_rec_oee(&p) > 0.0 && gk_rec_oee(&p) < 1.0);
    GK_CHECK(fabs(gk_rec_scrap_rate(&p) - 20.0 / 800.0) < 1e-9);
}

static void test_reliability(void)
{
    gk_rec_reliability r;
    gk_rec_reliability_init(&r);
    GK_CHECK(fabs(gk_rec_mtbf(&r)) < 1e-12);
    r.total_run_hours = 1000.0;
    r.total_repair_hours = 20.0;
    r.failures = 4;
    GK_CHECK(fabs(gk_rec_mtbf(&r) - 250.0) < 1e-9);
    GK_CHECK(fabs(gk_rec_mttr(&r) - 5.0) < 1e-9);
}

static void test_reports(void)
{
    gk_rec_report rpt;
    char out[128];
    GK_CHECK_STR_EQ(gk_rec_report_name(GK_REC_REPORT_MONTHLY), "monthly");
    gk_rec_report_init(&rpt, GK_REC_REPORT_DAILY);
    GK_CHECK(gk_rec_report_fill(&rpt, 95.0, 100.0, 400.0, 480.0) == GK_OK);
    GK_CHECK(fabs(gk_rec_report_yield(&rpt) - 0.95) < 1e-9);
    GK_CHECK(gk_rec_report_render(&rpt, out, sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "daily") != NULL);
    GK_CHECK(strstr(out, "0.950") != NULL);
    GK_CHECK(gk_rec_report_fill(&rpt, -1.0, 0.0, 0.0, 0.0) ==
             GK_ERR_INVALID_ARG);
}

int main(void)
{
    test_logbook();
    test_oee();
    test_reliability();
    test_reports();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
