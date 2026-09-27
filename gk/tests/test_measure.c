#include "gk_test.h"

#include "gk/gk_measure.h"

#include <math.h>
#include <string.h>

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

static void test_gauge_init(void)
{
    gk_gauge g;
    GK_CHECK_EQ_INT(gk_gauge_init(&g, GK_GAUGE_CALIPER), GK_OK);
    GK_CHECK(near(g.resolution, 0.01, 1e-12));
    GK_CHECK_STR_EQ(gk_gauge_name(g.kind), "caliper");
    GK_CHECK(gk_gauge_is_in_range(&g, 100.0));
    GK_CHECK(!gk_gauge_is_in_range(&g, 1000.0));

    GK_CHECK_EQ_INT(gk_gauge_init(&g, GK_GAUGE_MICROMETER), GK_OK);
    GK_CHECK(near(g.resolution, 0.001, 1e-12));
    GK_CHECK(g.range_max <= 25.0);

    GK_CHECK_EQ_INT(gk_gauge_init(&g, GK_GAUGE_CMM), GK_OK);
    GK_CHECK(g.points > 1);
    GK_CHECK_STR_EQ(gk_gauge_name(GK_GAUGE_LASER_SCAN), "laser-scanner");
    GK_CHECK_STR_EQ(gk_gauge_name((gk_gauge_kind)99), "unknown");
    GK_CHECK_EQ_INT(gk_gauge_init(&g, (gk_gauge_kind)99), GK_ERR_INVALID_ARG);
}

static void test_gauge_read(void)
{
    gk_gauge g;
    gk_gauge_init(&g, GK_GAUGE_CALIPER);
    GK_CHECK(near(gk_gauge_read(&g, 12.344), 12.34, 1e-9));
    GK_CHECK(near(gk_gauge_read(&g, 12.346), 12.35, 1e-9));
    gk_gauge_init(&g, GK_GAUGE_MICROMETER);
    GK_CHECK(near(gk_gauge_read(&g, 5.0004), 5.0, 1e-9));
}

static void test_measure_eval(void)
{
    gk_gauge g;
    gk_measure_result r;
    gk_gauge_init(&g, GK_GAUGE_MICROMETER);
    r = gk_measure_eval(&g, 10.0, 10.0, 0.01, "OD");
    GK_CHECK(near(r.measured, 10.0, 1e-9));
    GK_CHECK(near(r.error, 0.0, 1e-9));
    GK_CHECK(!r.out_of_tolerance);
    GK_CHECK(gk_measure_is_conforming(&r));
    GK_CHECK_STR_EQ(r.label, "OD");

    r = gk_measure_eval(&g, 10.0, 10.05, 0.01, "OD");
    GK_CHECK(r.out_of_tolerance);
    GK_CHECK(gk_measure_out_of_tolerance(&r));
    GK_CHECK(!gk_measure_is_conforming(&r));
}

static void test_cycle(void)
{
    gk_measure_cycle c;
    double reading = 0.0;
    int guard = 0;
    gk_measure_cycle_init(&c, 5.0, 3.0, 100.0, 2);
    GK_CHECK_EQ_INT(c.state, GK_CYCLE_IDLE);
    while (c.state != GK_CYCLE_DONE && guard < 50) {
        gk_measure_cycle_step(&c, 0.1, &reading);
        guard += 1;
    }
    GK_CHECK_EQ_INT(c.state, GK_CYCLE_DONE);
    GK_CHECK_EQ_INT(c.index, 2);
    GK_CHECK(near(reading, 3.0, 1e-9));
}

static void test_gdt(void)
{
    gk_gdt_tolerance t;
    gk_gdt_result r;
    t.nominal = 10.0;
    t.upper_tol = 0.1;
    t.lower_tol = 0.1;
    t.is_positional = 0;

    r = gk_gdt_check(&t, 10.05);
    GK_CHECK(r.within);
    r = gk_gdt_check(&t, 10.15);
    GK_CHECK(!r.within);
    GK_CHECK(near(r.deviation, 0.15, 1e-9));

    t.is_positional = 1;
    r = gk_gdt_check(&t, 9.95);
    GK_CHECK(r.within);

    {
        double plus[3] = {0.1, 0.2, 0.05};
        double minus[3] = {0.1, 0.2, 0.05};
        double tols[3] = {0.1, 0.2, 0.05};
        GK_CHECK(near(gk_gdt_worst_case(plus, minus, 3), 0.7, 1e-9));
        GK_CHECK(near(gk_gdt_rss(tols, 3), sqrt(0.01 + 0.04 + 0.0025), 1e-9));
    }
}

static void test_spc(void)
{
    gk_sample_set s;
    size_t i;
    double ucl, lcl, center;
    gk_sample_set_init(&s, 10.0);
    for (i = 0; i < 10; ++i) {
        GK_CHECK_EQ_INT(gk_sample_add(&s, 10.0 + (double)(i % 3) * 0.01), GK_OK);
    }
    GK_CHECK_EQ_INT(s.count, 10);
    GK_CHECK(gk_sample_mean(&s) > 9.9 && gk_sample_mean(&s) < 10.1);
    GK_CHECK(gk_sample_stddev(&s, 1) > 0.0);
    GK_CHECK(near(gk_sample_range(&s), 0.02, 1e-9));
    GK_CHECK(gk_sample_min(&s) <= gk_sample_max(&s));

    gk_spc_limits(&s, &ucl, &lcl, &center);
    GK_CHECK(ucl > center && center > lcl);
    GK_CHECK(near(center, gk_sample_mean(&s), 1e-9));

    /* inject an out-of-control point */
    gk_sample_add(&s, 100.0);
    GK_CHECK(gk_spc_out_of_control(&s, 10));
    GK_CHECK(gk_spc_out_of_control_count(&s) >= 1);
    GK_CHECK(!gk_spc_out_of_control(&s, 999));
}

static void test_capability(void)
{
    gk_sample_set s;
    size_t i;
    double cpk = 0.0, cp = 0.0, ppk = 0.0;
    gk_sample_set_init(&s, 10.0);
    for (i = 0; i < 30; ++i) {
        double v = 10.0 + ((double)(i % 5) - 2.0) * 0.02;
        gk_sample_add(&s, v);
    }
    GK_CHECK_EQ_INT(gk_cpk(&s, 11.0, 9.0, &cpk), GK_OK);
    GK_CHECK(cpk > 0.0);
    GK_CHECK_EQ_INT(gk_cp(&s, 11.0, 9.0, &cp), GK_OK);
    GK_CHECK(cp > 0.0);
    GK_CHECK_EQ_INT(gk_ppk(&s, 11.0, 9.0, &ppk), GK_OK);
    GK_CHECK(ppk > 0.0);
    GK_CHECK(cp >= cpk);
    GK_CHECK_EQ_INT(gk_cpk(&s, 9.0, 11.0, &cpk), GK_ERR_INVALID_ARG);
}

static void test_report(void)
{
    gk_report rep;
    gk_gauge g;
    gk_measure_result r;
    char csv[256];
    gk_sample_set s;
    size_t n;

    gk_report_init(&rep);
    gk_gauge_init(&g, GK_GAUGE_MICROMETER);
    r = gk_measure_eval(&g, 10.0, 10.0, 0.01, "OD1");
    GK_CHECK_EQ_INT(gk_report_add_line(&rep, &r, "mm"), GK_OK);
    r = gk_measure_eval(&g, 10.0, 10.5, 0.01, "OD2");
    GK_CHECK_EQ_INT(gk_report_add_line(&rep, &r, "mm"), GK_OK);
    GK_CHECK_EQ_INT(rep.passed, 1);
    GK_CHECK_EQ_INT(rep.failed, 1);
    GK_CHECK(strstr(gk_report_text(&rep), "OD1") != NULL);
    GK_CHECK(strstr(gk_report_text(&rep), "FAIL") != NULL);

    gk_sample_set_init(&s, 0.0);
    gk_sample_add(&s, 1.5);
    gk_sample_add(&s, 2.5);
    n = gk_samples_to_csv(&s, csv, sizeof(csv));
    GK_CHECK(n > 0);
    GK_CHECK(strstr(csv, "index,value") != NULL);
    GK_CHECK(strstr(csv, "1.500000") != NULL);
}

static void test_alarm(void)
{
    gk_tolerance_alarm a;
    gk_gauge g;
    gk_measure_result r;
    gk_tolerance_alarm_init(&a);
    gk_gauge_init(&g, GK_GAUGE_MICROMETER);
    r = gk_measure_eval(&g, 10.0, 10.05, 0.01, "OD");
    GK_CHECK_EQ_INT(gk_tolerance_alarm_check(&a, &r), GK_OK);
    GK_CHECK(a.active);
    GK_CHECK(a.overage > 0.0);
    GK_CHECK_STR_EQ(a.label, "OD");

    r = gk_measure_eval(&g, 10.0, 10.001, 0.01, "OD");
    GK_CHECK_EQ_INT(gk_tolerance_alarm_check(&a, &r), GK_OK);
    GK_CHECK(!a.active);
    GK_CHECK(near(a.overage, 0.0, 1e-9));
}

int main(void)
{
    test_gauge_init();
    test_gauge_read();
    test_measure_eval();
    test_cycle();
    test_gdt();
    test_spc();
    test_capability();
    test_report();
    test_alarm();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
