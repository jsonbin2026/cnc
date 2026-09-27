#include "gk_test.h"
#include "gk/gk_learn.h"

#include <math.h>
#include <string.h>

static void test_screenreader(void)
{
    char out[128];
    GK_CHECK_STR_EQ(gk_learn_role_name(GK_LEARN_ROLE_BUTTON), "button");
    GK_CHECK(gk_learn_screenreader_desc(GK_LEARN_ROLE_BUTTON, "Start",
                                        NULL, out, sizeof(out)) == GK_OK);
    GK_CHECK_STR_EQ(out, "button, Start");
    GK_CHECK(gk_learn_screenreader_desc(GK_LEARN_ROLE_SLIDER, "Feed", "120",
                                        out, sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "120") != NULL);
}

static void test_kbdnav(void)
{
    gk_learn_kbdnav n;
    gk_learn_kbdnav_init(&n);
    GK_CHECK_EQ_INT(gk_learn_kbdnav_add(&n, "a"), 1);
    GK_CHECK_EQ_INT(gk_learn_kbdnav_add(&n, "b"), 2);
    GK_CHECK_EQ_INT(gk_learn_kbdnav_add(&n, "c"), 3);
    GK_CHECK_EQ_INT(gk_learn_kbdnav_next(&n), 1);
    GK_CHECK_EQ_INT(gk_learn_kbdnav_next(&n), 2);
    GK_CHECK_EQ_INT(gk_learn_kbdnav_next(&n), 0);
    GK_CHECK_EQ_INT(gk_learn_kbdnav_prev(&n), 2);
    GK_CHECK_EQ_INT(gk_learn_kbdnav_focus(&n, "a"), 0);
    GK_CHECK_EQ_INT(gk_learn_kbdnav_focus(&n, "z"), -1);
}

static void test_touch(void)
{
    gk_learn_touch t;
    gk_learn_touch_init(&t);
    GK_CHECK(gk_learn_touch_target(&t, 20.0, 20.0) == GK_OK);
    GK_CHECK(gk_learn_touch_size(&t, 20.0) == 20.0);
    GK_CHECK(gk_learn_touch_target(&t, 4.5, 6.0) == GK_OK);
    GK_CHECK(gk_learn_touch_size(&t, 4.5) >= 9.0);
}

static void test_student(void)
{
    gk_learn_student s;
    int weakest;
    char course[64];
    gk_learn_student_init(&s, "alice");
    GK_CHECK_STR_EQ(s.name, "alice");
    GK_CHECK(gk_learn_student_set(&s, GK_LEARN_DIM_SAFETY, 90.0) == GK_OK);
    GK_CHECK(gk_learn_student_set(&s, GK_LEARN_DIM_SETUP, 70.0) == GK_OK);
    GK_CHECK(gk_learn_student_set(&s, GK_LEARN_DIM_PROGRAM, 40.0) == GK_OK);
    GK_CHECK(gk_learn_student_set(&s, GK_LEARN_DIM_OPERATION, 80.0) == GK_OK);
    GK_CHECK(gk_learn_student_set(&s, GK_LEARN_DIM_INSPECTION, 60.0) == GK_OK);
    GK_CHECK(gk_learn_student_set(&s, GK_LEARN_DIM_TROUBLESHOOT, 50.0) ==
             GK_OK);
    GK_CHECK(gk_learn_student_set(&s, GK_LEARN_DIMS, 50.0) ==
             GK_ERR_INVALID_ARG);
    GK_CHECK(fabs(gk_learn_student_avg(&s) - 65.0) < 1e-9);
    weakest = gk_learn_student_weakest(&s);
    GK_CHECK_EQ_INT(weakest, GK_LEARN_DIM_PROGRAM);
    GK_CHECK(fabs(gk_learn_radar_point(&s, GK_LEARN_DIM_SAFETY) - 0.9) < 1e-9);
    GK_CHECK(gk_learn_recommend_course(&s, course, sizeof(course)) == GK_OK);
    GK_CHECK(strstr(course, "programming") != NULL);
    GK_CHECK_STR_EQ(gk_learn_dim_name(GK_LEARN_DIM_TROUBLESHOOT),
                    "troubleshooting");
}

static void test_errors(void)
{
    gk_learn_errors e;
    int top;
    gk_learn_errors_init(&e);
    GK_CHECK(gk_learn_errors_add(&e, "wrong-offset") == GK_OK);
    GK_CHECK(gk_learn_errors_add(&e, "wrong-offset") == GK_OK);
    GK_CHECK(gk_learn_errors_add(&e, "missed-toolchange") == GK_OK);
    GK_CHECK(gk_learn_errors_add(&e, "wrong-offset") == GK_OK);
    top = gk_learn_errors_top(&e);
    GK_CHECK_EQ_INT(top, 0);
    GK_CHECK_STR_EQ(e.errors[top].pattern, "wrong-offset");
    GK_CHECK_EQ_INT(e.errors[top].count, 3);
}

static void test_prediction_trend(void)
{
    double h1[5] = {50.0, 55.0, 60.0, 65.0, 70.0};
    double h2[3] = {80.0, 75.0, 70.0};
    gk_learn_trend tr;
    double pred = gk_learn_predict_grade(h1, 5);
    GK_CHECK(pred > 70.0 && pred < 76.0);
    GK_CHECK(gk_learn_analyze_trend(h1, 5, &tr) == GK_OK);
    GK_CHECK_EQ_INT(tr.improving, 1);
    GK_CHECK(tr.slope > 0.0);
    GK_CHECK(gk_learn_analyze_trend(h2, 3, &tr) == GK_OK);
    GK_CHECK_EQ_INT(tr.improving, 0);
    GK_CHECK(gk_learn_predict_grade(h1, 1) == 50.0);
}

static void test_cohort_dashboard(void)
{
    double scores[5] = {40.0, 55.0, 70.0, 85.0, 95.0};
    gk_learn_dashboard d;
    gk_learn_student students[3];
    int i, dim;
    double levels[3] = {100.0, 50.0, 20.0};
    GK_CHECK_EQ_INT(gk_learn_cohort_percentile(scores, 5, 70.0), 40);
    for (i = 0; i < 3; i++) {
        gk_learn_student_init(&students[i], "s");
        for (dim = 0; dim < GK_LEARN_DIMS; dim++) {
            gk_learn_student_set(&students[i], (gk_learn_dim)dim, levels[i]);
        }
    }
    GK_CHECK(gk_learn_dashboard_build(students, 3, 40.0, &d) == GK_OK);
    GK_CHECK_EQ_INT(d.students, 3);
    GK_CHECK(fabs(d.max - 100.0) < 1e-9);
    GK_CHECK(fabs(d.min - 20.0) < 1e-9);
    GK_CHECK_EQ_INT(d.at_risk, 1);
}

static void test_abtest(void)
{
    gk_learn_abtest a;
    gk_learn_abtest_init(&a);
    a.control_n = 100;
    a.control_pass = 50;
    a.variant_n = 100;
    a.variant_pass = 70;
    GK_CHECK(fabs(gk_learn_abtest_uplift(&a) - 0.2) < 1e-9);
    GK_CHECK_EQ_INT(gk_learn_abtest_significant(&a), 1);
    a.variant_pass = 52;
    GK_CHECK_EQ_INT(gk_learn_abtest_significant(&a), 0);
}

static void test_eval(void)
{
    gk_learn_eval e;
    gk_learn_eval_init(&e);
    GK_CHECK(gk_learn_eval_add(&e, 0.6, 90.0) == GK_OK);
    GK_CHECK(gk_learn_eval_add(&e, 0.4, 50.0) == GK_OK);
    GK_CHECK(fabs(gk_learn_eval_total(&e) - 74.0) < 1e-9);
    GK_CHECK(gk_learn_eval_add(&e, -1.0, 0.0) == GK_ERR_INVALID_ARG);
}

static void test_viz_report_export(void)
{
    char out[2048];
    double values[3] = {2.0, 5.0, 1.0};
    gk_learn_student students[2];
    GK_CHECK(gk_learn_bar_chart("ABC", values, 3, out, sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "A|##") != NULL);
    GK_CHECK(strstr(out, "B|#####") != NULL);
    gk_learn_student_init(&students[0], "alice");
    gk_learn_student_set(&students[0], GK_LEARN_DIM_SAFETY, 90.0);
    gk_learn_student_init(&students[1], "bob");
    gk_learn_student_set(&students[1], GK_LEARN_DIM_SAFETY, 60.0);
    GK_CHECK(gk_learn_report(students, 2, "Class", out, sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "alice") != NULL);
    GK_CHECK(strstr(out, "Class") != NULL);
    GK_CHECK(gk_learn_export_csv(students, 2, out, sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "name,safety") != NULL);
    GK_CHECK(strstr(out, "bob,60.0") != NULL);
}

static void test_privacy(void)
{
    char out[256];
    unsigned long a, b, c;
    GK_CHECK(gk_learn_redact("user alice id 42", "alice", "[X]", out,
                             sizeof(out)) == GK_OK);
    GK_CHECK_STR_EQ(out, "user [X] id 42");
    GK_CHECK(gk_learn_redact("aaa", "a", "", out, sizeof(out)) == GK_OK);
    GK_CHECK_STR_EQ(out, "");
    a = gk_learn_anonymize("alice", 7);
    b = gk_learn_anonymize("alice", 7);
    c = gk_learn_anonymize("bob", 7);
    GK_CHECK(a == b);
    GK_CHECK(a != c);
}

int main(void)
{
    test_screenreader();
    test_kbdnav();
    test_touch();
    test_student();
    test_errors();
    test_prediction_trend();
    test_cohort_dashboard();
    test_abtest();
    test_eval();
    test_viz_report_export();
    test_privacy();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
