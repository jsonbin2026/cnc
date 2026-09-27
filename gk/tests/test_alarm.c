#include "gk_test.h"

#include "gk/gk_alarm.h"

#include <math.h>
#include <string.h>

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

static void test_detection(void)
{
    gk_alarm a;

    GK_CHECK(gk_alarm_check_overtravel(0, 105.0, 100.0, -100.0, &a) == 1);
    GK_CHECK_EQ_INT(a.category, GK_ALARM_OVERTRAVEL);
    GK_CHECK_EQ_INT(a.severity, GK_SEVERITY_FATAL);
    GK_CHECK_EQ_INT(a.axis, 0);
    GK_CHECK_STR_EQ(a.code, "E041");
    GK_CHECK(gk_alarm_check_overtravel(0, -101.0, 100.0, -100.0, &a) == 1);
    GK_CHECK(gk_alarm_check_overtravel(0, 50.0, 100.0, -100.0, &a) == 0);

    GK_CHECK(gk_alarm_check_overload(GK_ALARM_SERVO_OVERLOAD, 120.0, 100.0,
                                     1.1, &a) == 1);
    GK_CHECK_EQ_INT(a.severity, GK_SEVERITY_ERROR);
    GK_CHECK(gk_alarm_check_overload(GK_ALARM_SPINDLE_OVERLOAD, 90.0, 100.0,
                                     1.1, &a) == 0);

    GK_CHECK(gk_alarm_check_tool_breakage(100.0, 20.0, 0.5, &a) == 1);
    GK_CHECK_EQ_INT(a.category, GK_ALARM_TOOL_BREAKAGE);
    GK_CHECK(gk_alarm_check_tool_breakage(100.0, 250.0, 0.5, &a) == 1);
    GK_CHECK(gk_alarm_check_tool_breakage(100.0, 90.0, 0.5, &a) == 0);

    GK_CHECK(gk_alarm_check_tool_life(0.05, 0.1, 7, &a) == 1);
    GK_CHECK(strstr(a.text, "T07") != NULL);
    GK_CHECK(gk_alarm_check_tool_life(0.5, 0.1, 7, &a) == 0);

    GK_CHECK(gk_alarm_check_level(GK_ALARM_COOLANT_LOW, 0.1, 0.2, &a) == 1);
    GK_CHECK_EQ_INT(a.severity, GK_SEVERITY_WARNING);
    GK_CHECK(gk_alarm_check_level(GK_ALARM_LUBE_LOW, 0.9, 0.2, &a) == 0);

    GK_CHECK(gk_alarm_check_coord(110.0, -100.0, 100.0, &a) == 1);
    GK_CHECK_EQ_INT(a.category, GK_ALARM_COORD_OVERTRAVEL);
    GK_CHECK(gk_alarm_check_coord(50.0, -100.0, 100.0, &a) == 0);

    GK_CHECK(gk_alarm_estop(12.5, &a) == 1);
    GK_CHECK_EQ_INT(a.category, GK_ALARM_ESTOP);
    GK_CHECK(near(a.time, 12.5, 1e-9));
    GK_CHECK(gk_alarm_power_loss(&a) == 1);
    GK_CHECK_EQ_INT(a.category, GK_ALARM_POWER_LOSS);
}

static void test_manual(void)
{
    const gk_alarm_manual_entry *e = gk_alarm_manual_lookup(GK_ALARM_OVERTRAVEL);
    GK_CHECK(e != NULL);
    GK_CHECK_STR_EQ(e->code, "E041");
    GK_CHECK_STR_EQ(e->title, "Overtravel (OT)");
    GK_CHECK(gk_alarm_manual_count() >= 16);
    GK_CHECK(gk_alarm_manual_lookup(GK_ALARM_NONE) == NULL);
    GK_CHECK_STR_EQ(gk_alarm_category_name(GK_ALARM_TOOL_LIFE), "tool-life");
    GK_CHECK_STR_EQ(gk_severity_name(GK_SEVERITY_FATAL), "fatal");
}

static void test_codes(void)
{
    char buf[GK_ALARM_CODE_LEN];
    gk_alarm_make_code(GK_ALARM_COLLISION, buf, sizeof(buf));
    GK_CHECK(strcmp(buf, "E171") == 0);
    gk_alarm_make_code(GK_ALARM_NONE, buf, sizeof(buf));
    GK_CHECK_STR_EQ(buf, "E001");
}

static void test_manager(void)
{
    gk_alarm_manager m;
    gk_alarm a;
    const gk_alarm *p;

    gk_alarm_manager_init(&m);
    GK_CHECK_EQ_INT(gk_alarm_active_count(&m), 0);
    GK_CHECK(!gk_alarm_buzzer_on(&m));

    gk_alarm_check_level(GK_ALARM_COOLANT_LOW, 0.1, 0.2, &a);
    a.time = 1.0;
    GK_CHECK_EQ_INT(gk_alarm_raise(&m, &a), GK_OK);
    GK_CHECK_EQ_INT(gk_alarm_active_count(&m), 1);
    GK_CHECK(gk_alarm_buzzer_on(&m));

    gk_alarm_check_overtravel(2, 200.0, 100.0, -100.0, &a);
    a.time = 3.0;
    GK_CHECK_EQ_INT(gk_alarm_raise(&m, &a), GK_OK);
    GK_CHECK_EQ_INT(gk_alarm_active_count(&m), 2);
    GK_CHECK_EQ_INT(gk_alarm_max_severity(&m), GK_SEVERITY_FATAL);

    p = gk_alarm_first_active(&m);
    GK_CHECK(p != NULL);
    GK_CHECK_EQ_INT(p->id, 1);

    GK_CHECK_EQ_INT(gk_alarm_clear(&m, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_alarm_active_count(&m), 1);
    GK_CHECK_EQ_INT(gk_alarm_clear(&m, 999), GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_alarm_clear(&m, 2), GK_OK);
    GK_CHECK(!gk_alarm_buzzer_on(&m));

    GK_CHECK_EQ_INT(m.history_count, 2);
    GK_CHECK(gk_alarm_history_at(&m, 0) != NULL);
    GK_CHECK(gk_alarm_history_at(&m, 5) == NULL);

    p = gk_alarm_find_category(&m, GK_ALARM_OVERTRAVEL);
    GK_CHECK(p != NULL);
    GK_CHECK_EQ_INT(p->id, 2);
    GK_CHECK(gk_alarm_find_category(&m, GK_ALARM_ESTOP) == NULL);

    GK_CHECK(near(gk_alarm_timeline_span(&m), 2.0, 1e-9));
    GK_CHECK(gk_alarm_timeline_at(&m, 1) != NULL);

    /* blink */
    gk_alarm_check_tool_life(0.05, 0.1, 1, &a);
    a.time = 4.0;
    GK_CHECK_EQ_INT(gk_alarm_raise(&m, &a), GK_OK);
    m.lamp_phase = 0.0;
    gk_alarm_update_indicators(&m, 0.2);
    GK_CHECK(gk_alarm_lamp_on(&m) == 1);
    gk_alarm_update_indicators(&m, 0.4);
    GK_CHECK(gk_alarm_lamp_on(&m) == 0);
    gk_alarm_clear_all(&m);
    gk_alarm_update_indicators(&m, 0.1);
    GK_CHECK(!gk_alarm_lamp_on(&m));
}

static void test_program_checks(void)
{
    gk_alarm a;

    GK_CHECK_EQ_INT(gk_alarm_check_syntax("G00 X10 (rapid)", &a), GK_OK);
    GK_CHECK_EQ_INT(gk_alarm_check_syntax("G00 X10 (bad", &a), GK_ERR_PARSE);
    GK_CHECK_EQ_INT(a.category, GK_ALARM_SYNTAX);
    GK_CHECK_EQ_INT(gk_alarm_check_syntax("G00 X)(10", &a), GK_ERR_PARSE);

    GK_CHECK_EQ_INT(gk_alarm_check_gcode(0, &a), GK_OK);
    GK_CHECK_EQ_INT(gk_alarm_check_gcode(83, &a), GK_OK);
    GK_CHECK_EQ_INT(gk_alarm_check_gcode(500, &a), GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(a.category, GK_ALARM_UNDEFINED_G);
    GK_CHECK(strstr(a.text, "G500") != NULL);

    GK_CHECK_EQ_INT(gk_alarm_check_mcode(6, &a), GK_OK);
    GK_CHECK_EQ_INT(gk_alarm_check_mcode(99, &a), GK_OK);
    GK_CHECK_EQ_INT(gk_alarm_check_mcode(200, &a), GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(a.category, GK_ALARM_UNDEFINED_M);
}

static void test_fault_tree(void)
{
    gk_fault_tree t;
    memset(&t, 0, sizeof(t));
    GK_CHECK_EQ_INT(gk_fault_tree_add(&t, 1, "machine down", -1, 0.0), GK_OK);
    GK_CHECK_EQ_INT(gk_fault_tree_add(&t, 2, "power", 1, 0.1), GK_OK);
    GK_CHECK_EQ_INT(gk_fault_tree_add(&t, 3, "controller", 1, 0.2), GK_OK);
    GK_CHECK_EQ_INT(gk_fault_tree_add(&t, 4, "sensor", 3, 0.5), GK_OK);

    GK_CHECK(near(gk_fault_tree_probability(&t, 1), 0.28, 1e-9));
    GK_CHECK(near(gk_fault_tree_probability(&t, 3), 0.5, 1e-9));
    GK_CHECK_EQ_INT(gk_fault_tree_subtree_count(&t, 1), 4);
    GK_CHECK_EQ_INT(gk_fault_tree_subtree_count(&t, 3), 2);
    GK_CHECK_EQ_INT(gk_fault_tree_subtree_count(&t, 2), 1);
}

static void test_fault_sim(void)
{
    gk_fault_sim s;
    gk_alarm out[4];

    GK_CHECK_STR_EQ(gk_fault_injection_name(GK_FAULT_INJ_POWER), "power");
    gk_fault_sim_init(&s);
    GK_CHECK_EQ_INT(gk_fault_sim_add(&s, GK_FAULT_INJ_SENSOR, 1.0, 2.0), GK_OK);
    GK_CHECK_EQ_INT(gk_fault_sim_add(&s, GK_FAULT_INJ_POWER, 5.0, 0.0), GK_OK);

    GK_CHECK_EQ_INT(gk_fault_sim_step(&s, 0.5, out, 4), 0);
    GK_CHECK_EQ_INT(gk_fault_sim_step(&s, 1.5, out, 4), 1);
    GK_CHECK_EQ_INT(out[0].category, GK_ALARM_SERVO_OVERLOAD);
    GK_CHECK_EQ_INT(gk_fault_sim_step(&s, 2.0, out, 4), 0);
    /* window closed then reopened */
    GK_CHECK_EQ_INT(gk_fault_sim_step(&s, 3.5, out, 4), 0);
    GK_CHECK_EQ_INT(gk_fault_sim_step(&s, 5.0, out, 4), 1);
    GK_CHECK_EQ_INT(out[0].category, GK_ALARM_POWER_LOSS);
    /* permanent injection stays active -> not re-emitted */
    GK_CHECK_EQ_INT(gk_fault_sim_step(&s, 6.0, out, 4), 0);

    s.enabled = 0;
    GK_CHECK_EQ_INT(gk_fault_sim_step(&s, 10.0, out, 4), 0);
}

static void test_diagnosis(void)
{
    gk_diagnosis d;
    GK_CHECK_EQ_INT(gk_alarm_diagnose(GK_ALARM_OVERTRAVEL, &d), GK_OK);
    GK_CHECK(strstr(d.action, "limit") != NULL || d.action[0] != '\0');
    GK_CHECK_EQ_INT(gk_alarm_diagnose(GK_ALARM_SERVO_OVERLOAD, &d), GK_OK);
    GK_CHECK(d.summary[0] != '\0');
    GK_CHECK_EQ_INT(gk_alarm_diagnose(GK_ALARM_NONE, &d), GK_ERR_NOT_FOUND);
}

int main(void)
{
    test_detection();
    test_manual();
    test_codes();
    test_manager();
    test_program_checks();
    test_fault_tree();
    test_fault_sim();
    test_diagnosis();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
