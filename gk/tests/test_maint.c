#include "gk_test.h"

#include "gk/gk_maint.h"

#include <math.h>
#include <string.h>

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

static void test_periods(void)
{
    int i;

    for (i = 0; i <= GK_MAINT_INTERVAL; ++i) {
        GK_CHECK(strlen(gk_maint_period_name((gk_maint_period)i)) > 0);
        GK_CHECK(gk_maint_period_hours((gk_maint_period)i) > 0.0);
    }
    GK_CHECK_STR_EQ(gk_maint_period_name(GK_MAINT_DAILY), "daily");
    GK_CHECK(gk_maint_period_hours(GK_MAINT_YEARLY) >
             gk_maint_period_hours(GK_MAINT_MONTHLY));
}

static void test_schedule(void)
{
    gk_maint_schedule s;
    int d, w, m, y;

    gk_maint_schedule_init(&s);
    d = gk_maint_task_add(&s, "Daily check", GK_MAINT_DAILY);
    w = gk_maint_task_add(&s, "Weekly lube", GK_MAINT_WEEKLY);
    m = gk_maint_task_add(&s, "Monthly align", GK_MAINT_MONTHLY);
    y = gk_maint_task_add(&s, "Yearly overhaul", GK_MAINT_YEARLY);
    GK_CHECK(d > 0 && w > 0 && m > 0 && y > 0);
    GK_CHECK_EQ_INT(s.count, 4);
    GK_CHECK_EQ_INT(gk_maint_count_by_period(&s, GK_MAINT_DAILY), 1);
    GK_CHECK_EQ_INT(gk_maint_count_by_period(&s, GK_MAINT_MONTHLY), 1);

    /* daily interval 8h, last at 0 -> due in 8h */
    GK_CHECK(near(gk_maint_task_due_in(&s, d, 0.0), 8.0, 1e-9));
    GK_CHECK_EQ_INT(gk_maint_task_is_due(&s, d, 0.0), 0);
    GK_CHECK_EQ_INT(gk_maint_task_is_due(&s, d, 8.0), 1);
    GK_CHECK_EQ_INT(gk_maint_task_is_due(&s, d, 20.0), 1);
    GK_CHECK_EQ_INT(gk_maint_due_count(&s, 0.0), 0);
    GK_CHECK_EQ_INT(gk_maint_due_count(&s, 8.0), 1);

    GK_CHECK_EQ_INT(gk_maint_task_done(&s, d, 8.0), GK_OK);
    GK_CHECK_EQ_INT(s.tasks[0].done_count, 1);
    GK_CHECK(near(gk_maint_task_due_in(&s, d, 8.0), 8.0, 1e-9));
    GK_CHECK_EQ_INT(gk_maint_due_count(&s, 8.0), 0);
    GK_CHECK_EQ_INT(gk_maint_task_done(&s, 999, 0.0), GK_ERR_NOT_FOUND);
    GK_CHECK(near(gk_maint_task_due_in(&s, 999, 0.0), 0.0, 1e-9));
    GK_CHECK(near(gk_maint_next_due(&s.tasks[0], 0.0), 16.0, 1e-9));
    GK_CHECK(near(gk_maint_next_due(NULL, 0.0), 0.0, 1e-9));
}

static void test_lube(void)
{
    gk_lube_system l;

    gk_lube_system_init(&l, 10.0, 0.2);
    l.consumption_lph = 0.5;
    GK_CHECK(near(gk_lube_level_fraction(&l), 1.0, 1e-9));
    GK_CHECK_EQ_INT(gk_lube_level_low(&l), 0);
    GK_CHECK_EQ_INT(gk_lube_consume(&l, 10.0), GK_OK);
    GK_CHECK(near(l.level_l, 5.0, 1e-9));
    GK_CHECK(near(gk_lube_level_fraction(&l), 0.5, 1e-9));
    GK_CHECK_EQ_INT(gk_lube_consume(&l, 8.0), GK_OK);   /* -4 -> 1.0 */
    GK_CHECK_EQ_INT(gk_lube_level_low(&l), 1);
    GK_CHECK_EQ_INT(gk_lube_refill(&l, 100.0), GK_OK);
    GK_CHECK(near(l.level_l, 10.0, 1e-9));
    /* 2 hours at 0.5 l/h = 1.0 l; pulse 0.25 -> 4 */
    GK_CHECK_EQ_INT(gk_lube_auto_pulse(&l, 2.0, 0.25), 4);
    GK_CHECK_EQ_INT(gk_lube_auto_pulse(&l, 0.0, 0.25), 0);
    GK_CHECK_EQ_INT(gk_lube_auto_pulse(NULL, 2.0, 0.25), 0);
    GK_CHECK_EQ_INT(gk_lube_consume(&l, -1.0), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_lube_refill(&l, -1.0), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_lube_level_low(NULL), 1);
}

static void test_guide_filter_belt(void)
{
    gk_guide_clean g;
    gk_filter f;
    gk_belt b;

    gk_guide_clean_init(&g, 100.0);
    GK_CHECK_EQ_INT(gk_guide_clean_due(&g, 50.0), 1);  /* never cleaned */
    GK_CHECK_EQ_INT(gk_guide_clean_run(&g, 50.0), GK_OK);
    GK_CHECK_EQ_INT(gk_guide_clean_due(&g, 100.0), 0);
    GK_CHECK_EQ_INT(gk_guide_clean_due(&g, 150.0), 1);

    gk_filter_init(&f, 500.0, 2.0);
    GK_CHECK_EQ_INT(gk_filter_needs_change(&f, 0.0), 1);  /* never replaced */
    GK_CHECK_EQ_INT(gk_filter_replace(&f, 0.0), GK_OK);
    GK_CHECK_EQ_INT(gk_filter_needs_change(&f, 100.0), 0);
    GK_CHECK_EQ_INT(gk_filter_needs_change(&f, 500.0), 1);
    f.pressure_drop = 2.5;
    f.installed_hours = 100.0;
    GK_CHECK_EQ_INT(gk_filter_needs_change(&f, 120.0), 1);

    gk_belt_init(&b, 50.0, 100.0);
    GK_CHECK_EQ_INT(gk_belt_ok(&b), 0);
    GK_CHECK_EQ_INT(gk_belt_check(&b, 75.0, 0.1), GK_OK);
    GK_CHECK_EQ_INT(gk_belt_ok(&b), 1);
    GK_CHECK_EQ_INT(gk_belt_check(&b, 40.0, 0.1), GK_OK);
    GK_CHECK_EQ_INT(gk_belt_ok(&b), 0);
    GK_CHECK_EQ_INT(gk_belt_check(&b, 75.0, 0.9), GK_OK);
    GK_CHECK_EQ_INT(gk_belt_ok(&b), 0);
    GK_CHECK_EQ_INT(gk_belt_check(&b, -1.0, 0.1), GK_ERR_INVALID_ARG);
}

static void test_precision(void)
{
    gk_precision_check c;

    gk_precision_check_init(&c);
    GK_CHECK_EQ_INT(gk_precision_add(&c, 100.0, 100.01, 0.02), GK_OK);
    GK_CHECK_EQ_INT(gk_precision_add(&c, 200.0, 199.99, 0.02), GK_OK);
    GK_CHECK_EQ_INT(c.count, 2);
    GK_CHECK(near(c.axes[0].error, 0.01, 1e-9));
    GK_CHECK_EQ_INT(gk_precision_all_pass(&c), 1);
    GK_CHECK(near(gk_precision_max_error(&c), 0.01, 1e-9));
    gk_precision_add(&c, 300.0, 300.20, 0.05);
    GK_CHECK_EQ_INT(gk_precision_all_pass(&c), 0);
    GK_CHECK(near(gk_precision_max_error(&c), 0.20, 1e-9));
    GK_CHECK_EQ_INT(gk_precision_add(&c, 0.0, 0.0, -1.0),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_precision_all_pass(NULL), 0);
}

static void test_interferometer(void)
{
    gk_interferometer i;

    gk_interferometer_init(&i);
    GK_CHECK(near(i.wavelength, 632.8, 1e-9));
    GK_CHECK(near(gk_interferometer_linear_error(&i), 0.0, 1e-9));
    gk_interferometer_sample(&i, 0.0, 0.0);
    gk_interferometer_sample(&i, 100.0, 0.010);
    gk_interferometer_sample(&i, 200.0, 0.020);
    /* slope = 0.01/100 = 1e-4 */
    GK_CHECK(near(gk_interferometer_linear_error(&i), 1e-4, 1e-9));
    GK_CHECK_EQ_INT(i.count, 3);
}

static void test_ballbar(void)
{
    gk_ballbar b;

    gk_ballbar_init(&b, 150.0);
    GK_CHECK_EQ_INT(gk_ballbar_run(&b, 0.008, 0.004), GK_OK);
    GK_CHECK(near(b.circularity, 0.008 * 2 + 0.004, 1e-9));
    GK_CHECK_EQ_INT(gk_ballbar_pass(&b, 0.02), 1);
    GK_CHECK_EQ_INT(gk_ballbar_pass(&b, 0.01), 0);
    GK_CHECK_EQ_INT(gk_ballbar_run(&b, -1.0, 0.0), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_ballbar_pass(NULL, 1.0), 0);
}

static void test_fault_tree(void)
{
    gk_maint_fault_tree t;

    gk_maint_fault_tree_init(&t);
    /* root 1: "spindle noise"; children: bearing 0.1, lube 0.2 */
    GK_CHECK_EQ_INT(gk_maint_fault_node_add(&t, 1, 0, "spindle noise", 0.0, 0),
                    GK_OK);
    GK_CHECK_EQ_INT(gk_maint_fault_node_add(&t, 2, 1, "bearing wear", 0.1, 1),
                    GK_OK);
    GK_CHECK_EQ_INT(gk_maint_fault_node_add(&t, 3, 1, "low lube", 0.2, 1),
                    GK_OK);
    /* OR: 1 - 0.9*0.8 = 0.28 */
    GK_CHECK(near(gk_maint_fault_probability(&t, 1), 0.28, 1e-9));
    GK_CHECK(near(gk_maint_fault_probability(&t, 2), 0.10, 1e-9));
    GK_CHECK(near(gk_maint_fault_probability(&t, 3), 0.20, 1e-9));
    GK_CHECK(near(gk_maint_fault_probability(&t, 99), 0.0, 1e-9));
}

static void test_spares(void)
{
    gk_spare_store s;
    int a, b;

    gk_spare_store_init(&s);
    a = gk_spare_add(&s, "Bearing 6205", 10, 4, 12.0);
    b = gk_spare_add(&s, "Filter", 2, 5, 30.0);
    GK_CHECK(a > 0 && b > 0);
    GK_CHECK_EQ_INT(s.count, 2);
    GK_CHECK(near(gk_spare_value(&s), 10 * 12.0 + 2 * 30.0, 1e-9));
    GK_CHECK_EQ_INT(gk_spare_below_min(&s), 1);   /* filter below min */
    GK_CHECK_EQ_INT(gk_spare_consume(&s, a, 7), GK_OK);
    GK_CHECK_EQ_INT(s.parts[0].stock, 3);
    GK_CHECK_EQ_INT(gk_spare_below_min(&s), 2);
    GK_CHECK_EQ_INT(gk_spare_consume(&s, a, 100), GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_spare_restock(&s, a, 10), GK_OK);
    GK_CHECK_EQ_INT(s.parts[0].stock, 13);
    GK_CHECK_EQ_INT(gk_spare_consume(&s, 999, 1), GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_spare_restock(&s, 999, 1), GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_spare_restock(&s, a, -1), GK_ERR_INVALID_ARG);
}

static void test_work_orders(void)
{
    gk_work_order_book b;
    int w1, w2;

    gk_work_order_book_init(&b);
    w1 = gk_work_order_create(&b, "Replace bearing", 1);
    w2 = gk_work_order_create(&b, "Align Z axis", 2);
    GK_CHECK(w1 > 0 && w2 > 0);
    GK_CHECK_EQ_INT(b.count, 2);
    GK_CHECK_EQ_INT(gk_work_order_open_count(&b), 2);
    GK_CHECK_EQ_INT(gk_work_order_assign(&b, w1, 5), GK_OK);
    GK_CHECK_EQ_INT(b.orders[0].status, GK_WO_IN_PROGRESS);
    GK_CHECK_EQ_INT(b.orders[0].assignee, 5);
    GK_CHECK_EQ_INT(gk_work_order_log(&b, w1, 1.5), GK_OK);
    GK_CHECK_EQ_INT(gk_work_order_log(&b, w1, 2.5), GK_OK);
    GK_CHECK(near(b.orders[0].hours_spent, 4.0, 1e-9));
    GK_CHECK_EQ_INT(gk_work_order_complete(&b, w1), GK_OK);
    GK_CHECK_EQ_INT(b.orders[0].status, GK_WO_DONE);
    GK_CHECK_EQ_INT(gk_work_order_open_count(&b), 1);
    GK_CHECK_EQ_INT(gk_work_order_assign(&b, w1, 9), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_work_order_assign(&b, 999, 1), GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_work_order_log(&b, 999, 1.0), GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_work_order_log(&b, w2, -1.0), GK_ERR_INVALID_ARG);
    GK_CHECK_STR_EQ(gk_work_order_status_name(GK_WO_OPEN), "open");
    GK_CHECK_STR_EQ(gk_work_order_status_name(GK_WO_IN_PROGRESS),
                    "in-progress");
}

static void test_repair_history(void)
{
    gk_repair_history h;

    gk_repair_history_init(&h);
    GK_CHECK_EQ_INT(gk_repair_record_add(&h, 1, "replaced bearing", 2.0, 50.0),
                    GK_OK);
    GK_CHECK_EQ_INT(gk_repair_record_add(&h, 2, "realigned", 1.0, 20.0), GK_OK);
    GK_CHECK_EQ_INT(h.count, 2);
    GK_CHECK(near(h.total_cost, 70.0, 1e-9));
    GK_CHECK(near(gk_repair_total_hours(&h), 3.0, 1e-9));
    GK_CHECK_EQ_INT(gk_repair_record_add(&h, 3, "x", -1.0, 0.0),
                    GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_repair_record_add(NULL, 1, "x", 1.0, 1.0),
                    GK_ERR_OUT_OF_RANGE);
}

static void test_component_life(void)
{
    gk_component_life l;

    gk_component_life_init(&l, 1000.0);
    GK_CHECK(near(gk_component_rul(&l), 1000.0, 1e-9));
    GK_CHECK(near(gk_component_health(&l), 1.0, 1e-9));
    l.used_hours = 400.0;
    GK_CHECK(near(gk_component_rul(&l), 600.0, 1e-9));
    GK_CHECK(near(gk_component_health(&l), 0.6, 1e-9));
    l.degradation = 2.0;
    GK_CHECK(near(gk_component_rul(&l), (1000.0 - 800.0) / 2.0, 1e-9));
    l.used_hours = 600.0;
    GK_CHECK(near(gk_component_rul(&l), 0.0, 1e-9));
    GK_CHECK(near(gk_component_health(&l), 0.0, 1e-9));
    GK_CHECK(gk_component_rul(NULL) == 0.0);
    GK_CHECK(gk_component_health(NULL) == 0.0);
}

int main(void)
{
    test_periods();
    test_schedule();
    test_lube();
    test_guide_filter_belt();
    test_precision();
    test_interferometer();
    test_ballbar();
    test_fault_tree();
    test_spares();
    test_work_orders();
    test_repair_history();
    test_component_life();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
