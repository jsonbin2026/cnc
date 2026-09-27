#include "gk_test.h"

#include "gk/gk_cost.h"

#include <math.h>
#include <string.h>

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

static void test_power_names(void)
{
    int i;

    for (i = 0; i < GK_POWER_COUNT; ++i) {
        GK_CHECK(strlen(gk_power_name((gk_power_kind)i)) > 0);
    }
    GK_CHECK_STR_EQ(gk_power_name(GK_POWER_SPINDLE), "spindle");
    GK_CHECK_STR_EQ(gk_power_name(GK_POWER_LIGHTING), "lighting");
    GK_CHECK_STR_EQ(gk_power_name((gk_power_kind)99), "unknown");
}

static void test_power_meter(void)
{
    gk_power_meter m;

    gk_power_meter_init(&m, 0.8);
    GK_CHECK(near(m.tariff, 0.8, 1e-9));
    GK_CHECK_EQ_INT(gk_power_add(&m, GK_POWER_SPINDLE, "Spindle"), GK_OK);
    GK_CHECK_EQ_INT(gk_power_add(&m, GK_POWER_SERVO, NULL), GK_OK);
    GK_CHECK_EQ_INT(gk_power_add(&m, GK_POWER_COOLANT, "Coolant"), GK_OK);
    GK_CHECK_EQ_INT(m.count, 3);
    GK_CHECK_STR_EQ(m.loads[0].name, "Spindle");
    GK_CHECK_STR_EQ(m.loads[1].name, "servo");
    GK_CHECK_EQ_INT(gk_power_set(&m, GK_POWER_SPINDLE, 10.0), GK_OK);
    GK_CHECK_EQ_INT(gk_power_set(&m, GK_POWER_SERVO, 2.0), GK_OK);
    GK_CHECK_EQ_INT(gk_power_set(&m, GK_POWER_COOLANT, 1.0), GK_OK);
    GK_CHECK(near(gk_power_total_kw(&m), 13.0, 1e-9));
    GK_CHECK(near(gk_power_kind_kw(&m, GK_POWER_SERVO), 2.0, 1e-9));
    GK_CHECK(near(gk_power_kind_kw(&m, GK_POWER_LIGHTING), 0.0, 1e-9));
    GK_CHECK_EQ_INT(gk_power_set(&m, GK_POWER_LIGHTING, 1.0),
                    GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_power_set(&m, GK_POWER_SPINDLE, -1.0),
                    GK_ERR_INVALID_ARG);
    /* one hour at 13 kW = 13 kWh */
    GK_CHECK_EQ_INT(gk_power_accumulate(&m, 3600.0), GK_OK);
    GK_CHECK(near(m.total_kwh, 13.0, 1e-9));
    GK_CHECK(near(m.loads[0].hours, 1.0, 1e-9));
    GK_CHECK(near(gk_electricity_cost(&m), 13.0 * 0.8, 1e-9));
    GK_CHECK_EQ_INT(gk_power_accumulate(&m, -1.0), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_power_add(NULL, GK_POWER_SPINDLE, "x"),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_power_add(&m, (gk_power_kind)99, "x"),
                    GK_ERR_INVALID_ARG);
}

static void test_tool_material_labor(void)
{
    gk_tool_cost t;
    gk_material_cost mc;
    gk_labor_cost lc;

    gk_tool_cost_init(&t, 100.0, 60.0);
    t.tool_usage_minutes = 15.0;
    GK_CHECK(near(gk_tool_cost_amount(&t), 25.0, 1e-9));
    GK_CHECK(gk_tool_cost_amount(NULL) == 0.0);
    gk_tool_cost_init(&t, 100.0, 0.0);
    GK_CHECK(gk_tool_cost_amount(&t) == 0.0);

    gk_material_cost_init(&mc, 5.0, 2.0, 0.1);
    GK_CHECK(near(gk_material_cost_amount(&mc), 5.0 * 2.0 * 1.1, 1e-9));
    GK_CHECK(gk_material_cost_amount(NULL) == 0.0);

    gk_labor_cost_init(&lc, 30.0, 2.0, 10.0);
    GK_CHECK(near(gk_labor_cost_amount(&lc), 80.0, 1e-9));
    GK_CHECK(gk_labor_cost_amount(NULL) == 0.0);
}

static void test_breakdown(void)
{
    gk_cost_breakdown c;

    gk_cost_breakdown_init(&c);
    gk_tool_cost_init(&c.tool, 120.0, 60.0);
    c.tool.tool_usage_minutes = 30.0;         /* 60 */
    gk_material_cost_init(&c.material, 4.0, 3.0, 0.0); /* 12 */
    gk_labor_cost_init(&c.labor, 25.0, 1.0, 5.0);      /* 30 */
    c.machine_rate = 40.0;
    c.machine_hours = 1.0;                    /* 40 */
    c.electricity = 10.0;                     /* 10 */
    GK_CHECK(near(gk_cost_machine_amount(&c), 40.0, 1e-9));
    GK_CHECK(near(gk_cost_total(&c), 60.0 + 12.0 + 30.0 + 40.0 + 10.0, 1e-9));
    GK_CHECK(gk_cost_total(NULL) == 0.0);
    GK_CHECK(gk_cost_machine_amount(NULL) == 0.0);
}

static void test_carbon(void)
{
    gk_carbon_factors f;
    gk_cost_breakdown c;
    gk_power_meter m;

    gk_carbon_factors_init(&f, 0.5, 2.0, 1.5);
    gk_cost_breakdown_init(&c);
    gk_material_cost_init(&c.material, 1.0, 2.0, 0.0);
    gk_tool_cost_init(&c.tool, 1.0, 60.0);
    c.tool.tool_usage_minutes = 60.0;   /* 1 tool */
    gk_power_meter_init(&m, 0.8);
    gk_power_add(&m, GK_POWER_SPINDLE, NULL);
    gk_power_set(&m, GK_POWER_SPINDLE, 10.0);
    gk_power_accumulate(&m, 3600.0);   /* 10 kWh */
    /* 0.5*10 + 2.0*2 + 1.5*1 = 5 + 4 + 1.5 = 10.5 */
    GK_CHECK(near(gk_carbon_footprint(&f, &c, &m), 10.5, 1e-9));
    GK_CHECK(gk_carbon_footprint(NULL, &c, &m) == 0.0);
    /* null breakdown still counts grid only */
    GK_CHECK(near(gk_carbon_footprint(&f, NULL, &m), 5.0, 1e-9));
}

static void test_advisor(void)
{
    gk_energy_advisor a;
    gk_power_meter m;
    gk_cost_breakdown c;
    int n;

    gk_cost_breakdown_init(&c);
    c.machine_hours = 10.0;
    gk_power_meter_init(&m, 1.0);
    gk_power_add(&m, GK_POWER_SPINDLE, NULL);
    gk_power_add(&m, GK_POWER_COOLANT, NULL);
    gk_power_add(&m, GK_POWER_LIGHTING, NULL);
    gk_power_add(&m, GK_POWER_AUXILIARY, NULL);
    gk_power_set(&m, GK_POWER_SPINDLE, 1.0);
    gk_power_set(&m, GK_POWER_COOLANT, 1.0);
    gk_power_set(&m, GK_POWER_LIGHTING, 1.0);
    gk_power_set(&m, GK_POWER_AUXILIARY, 1.0);
    gk_power_accumulate(&m, 3600.0);
    n = gk_energy_advise(&a, &c, &m);
    GK_CHECK(n >= 4);
    GK_CHECK(gk_energy_total_saving(&a) > 0.0);

    gk_energy_advisor_init(&a);
    gk_power_meter_init(&m, 1.0);
    gk_power_add(&m, GK_POWER_SPINDLE, NULL);
    gk_power_set(&m, GK_POWER_SPINDLE, 5.0);
    n = gk_energy_advise(&a, NULL, &m);
    GK_CHECK_EQ_INT(n, 1);
    GK_CHECK_STR_EQ(a.tips[0].text, "Energy usage already efficient");
    GK_CHECK(near(gk_energy_total_saving(&a), 0.0, 1e-9));
    GK_CHECK_EQ_INT(gk_energy_advise(NULL, &c, &m), 0);
    GK_CHECK(gk_energy_total_saving(NULL) == 0.0);
}

static void test_cost_book(void)
{
    gk_cost_book b;
    gk_cost_breakdown c1, c2;
    char buf[GK_COST_REPORT];

    gk_cost_breakdown_init(&c1);
    gk_labor_cost_init(&c1.labor, 20.0, 1.0, 0.0);  /* 20 */
    gk_cost_breakdown_init(&c2);
    gk_labor_cost_init(&c2.labor, 20.0, 2.0, 0.0);  /* 40 */

    gk_cost_book_init(&b);
    GK_CHECK_EQ_INT(gk_cost_book_add(&b, "Bracket", &c1), 1);
    GK_CHECK_EQ_INT(gk_cost_book_add(&b, "Flange", &c2), 2);
    GK_CHECK_EQ_INT(b.count, 2);
    GK_CHECK_STR_EQ(b.records[0].part, "Bracket");
    GK_CHECK_EQ_INT(gk_cost_book_add(NULL, "x", &c1), -1);
    GK_CHECK_EQ_INT(gk_cost_book_add(&b, "x", NULL), -1);

    GK_CHECK(near(gk_cost_compare(&b, "Bracket", "Flange"), -20.0, 1e-9));
    GK_CHECK(near(gk_cost_compare(&b, "Flange", "Bracket"), 20.0, 1e-9));
    GK_CHECK(near(gk_cost_compare(&b, "none", "Flange"), 0.0, 1e-9));

    GK_CHECK(gk_cost_report(&b, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "COST REPORT|items=2") != NULL);
    GK_CHECK(strstr(buf, "Bracket") != NULL);

    GK_CHECK(near(gk_cost_predict(&b.records[0], 5), 100.0, 1e-9));
    GK_CHECK(near(gk_cost_predict(&b.records[0], 0), 0.0, 1e-9));
    GK_CHECK(near(gk_cost_predict(&b.records[0], -1), 0.0, 1e-9));
    GK_CHECK(gk_cost_predict(NULL, 5) == 0.0);
}

int main(void)
{
    test_power_names();
    test_power_meter();
    test_tool_material_labor();
    test_breakdown();
    test_carbon();
    test_advisor();
    test_cost_book();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
