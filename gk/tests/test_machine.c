#include "gk_test.h"

#include "gk/gk_machine.h"

#include <string.h>
#include <stdio.h>

static void test_machine_names(void)
{
    GK_CHECK_EQ_INT(gk_machine_type_count(), 30);
    GK_CHECK_STR_EQ(gk_machine_type_name(GK_MACHINE_VMC), "VMC");
    GK_CHECK_STR_EQ(gk_machine_type_name(GK_MACHINE_TURN), "lathe");
    GK_CHECK_STR_EQ(gk_machine_type_name(GK_MACHINE_TOOL_GRINDER),
                    "tool-grinder");
    GK_CHECK_STR_EQ(gk_machine_type_name((gk_machine_type)999), "unknown");
    GK_CHECK_STR_EQ(gk_process_kind_name(GK_PROCESS_MILL), "milling");
    GK_CHECK_STR_EQ(gk_process_kind_name(GK_PROCESS_ADDITIVE), "additive");
    GK_CHECK_STR_EQ(gk_process_kind_name((gk_process_kind)99), "unknown");
}

static void test_machine_def(void)
{
    const gk_machine_def *d = gk_machine_type_def(GK_MACHINE_VMC);
    GK_CHECK(d != NULL);
    GK_CHECK_EQ_INT(d->process, GK_PROCESS_MILL);
    GK_CHECK_EQ_INT(d->linear_axes, 3);
    GK_CHECK_EQ_INT(d->rotary_axes, 0);
    GK_CHECK(d->has_atc);
    GK_CHECK(d->max_spindle_rpm > 0.0);

    d = gk_machine_type_def(GK_MACHINE_5AX_HEAD);
    GK_CHECK(d != NULL);
    GK_CHECK_EQ_INT(d->simultaneous_axes, 5);
    GK_CHECK_EQ_INT(d->rotary_axes, 2);

    d = gk_machine_type_def(GK_MACHINE_TURN);
    GK_CHECK(d->has_tailstock);
    GK_CHECK(d->rotary_axes >= 1);

    d = gk_machine_type_def(GK_MACHINE_SURFACE_GRINDER);
    GK_CHECK(d->has_dresser);

    GK_CHECK(gk_machine_type_def((gk_machine_type)-1) == NULL);
    GK_CHECK(gk_machine_type_def(GK_MACHINE_TYPE_COUNT) == NULL);
}

static void test_machine_item_map(void)
{
    gk_machine_type t;
    GK_CHECK_EQ_INT(gk_machine_item_id(GK_MACHINE_VMC), 179);
    GK_CHECK_EQ_INT(gk_machine_item_id(GK_MACHINE_TOOL_GRINDER), 208);
    GK_CHECK_EQ_INT(gk_machine_item_id((gk_machine_type)99), -1);

    GK_CHECK(gk_machine_type_from_item(179, &t));
    GK_CHECK_EQ_INT(t, GK_MACHINE_VMC);
    GK_CHECK(gk_machine_type_from_item(185, &t));
    GK_CHECK_EQ_INT(t, GK_MACHINE_TURN);
    GK_CHECK(!gk_machine_type_from_item(178, &t));
    GK_CHECK(!gk_machine_type_from_item(209, &t));
}

static void test_machine_predicates(void)
{
    GK_CHECK(gk_machine_is_turning(GK_MACHINE_TURN));
    GK_CHECK(gk_machine_is_turning(GK_MACHINE_TURN_MILL));
    GK_CHECK(!gk_machine_is_turning(GK_MACHINE_VMC));

    GK_CHECK(gk_machine_is_grinding(GK_MACHINE_CYL_GRINDER));
    GK_CHECK(!gk_machine_is_grinding(GK_MACHINE_VMC));

    GK_CHECK(gk_machine_is_five_axis(GK_MACHINE_5AX_HEAD));
    GK_CHECK(gk_machine_is_five_axis(GK_MACHINE_MILL_TURN));
    GK_CHECK(gk_machine_is_five_axis(GK_MACHINE_TOOL_GRINDER));
    GK_CHECK(!gk_machine_is_five_axis(GK_MACHINE_VMC));

    GK_CHECK(gk_machine_supports_axis(GK_MACHINE_VMC, GK_AXIS_X));
    GK_CHECK(gk_machine_supports_axis(GK_MACHINE_VMC, GK_AXIS_Z));
    GK_CHECK(!gk_machine_supports_axis(GK_MACHINE_VMC, GK_AXIS_A));
    GK_CHECK(gk_machine_supports_axis(GK_MACHINE_TURN, GK_AXIS_A));
    GK_CHECK(!gk_machine_supports_axis(GK_MACHINE_TURN, GK_AXIS_C));
    GK_CHECK(!gk_machine_supports_axis(GK_MACHINE_VMC, -1));
}

static void test_controller_names(void)
{
    GK_CHECK_EQ_INT(gk_controller_count(), 19);
    GK_CHECK_STR_EQ(gk_controller_name(GK_CTRL_FANUC_0I), "FANUC 0i");
    GK_CHECK_STR_EQ(gk_controller_name(GK_CTRL_HEIDENHAIN_TNC),
                    "HEIDENHAIN TNC");
    GK_CHECK_STR_EQ(gk_controller_name(GK_CTRL_HNC8), "HNC-8");
    GK_CHECK_STR_EQ(gk_controller_name((gk_controller)99), "unknown");
    GK_CHECK_STR_EQ(gk_controller_vendor(GK_CTRL_SIEMENS_840D), "SIEMENS");
    GK_CHECK_STR_EQ(gk_controller_vendor(GK_CTRL_HAAS), "HAAS");
}

static void test_dialects(void)
{
    GK_CHECK_EQ_INT(gk_controller_dialect(GK_CTRL_FANUC_0I),
                    GK_DIALECT_FANUC);
    GK_CHECK_EQ_INT(gk_controller_dialect(GK_CTRL_SIEMENS_840D),
                    GK_DIALECT_SIEMENS);
    GK_CHECK_EQ_INT(gk_controller_dialect(GK_CTRL_HEIDENHAIN_TNC),
                    GK_DIALECT_HEIDENHAIN);
    GK_CHECK_EQ_INT(gk_controller_dialect(GK_CTRL_MITSUBISHI_M70),
                    GK_DIALECT_MITSUBISHI);
    GK_CHECK_EQ_INT(gk_controller_dialect(GK_CTRL_MAZAK_SMOOTH),
                    GK_DIALECT_MAZAK);
    GK_CHECK_EQ_INT(gk_controller_dialect(GK_CTRL_HNC8), GK_DIALECT_HNC);
    GK_CHECK_EQ_INT(gk_controller_dialect(GK_CTRL_HAAS), GK_DIALECT_FANUC);
    GK_CHECK_STR_EQ(gk_dialect_name(GK_DIALECT_SIEMENS), "SIEMENS/DIN");
    GK_CHECK_STR_EQ(gk_dialect_name((gk_dialect)99), "unknown");

    GK_CHECK_EQ_INT(gk_controller_item_id(GK_CTRL_FANUC_0I), 209);
    GK_CHECK_EQ_INT(gk_controller_item_id(GK_CTRL_DIMA), 227);
    {
        gk_controller c;
        GK_CHECK(gk_controller_from_item(214, &c));
        GK_CHECK_EQ_INT(c, GK_CTRL_SIEMENS_840D);
        GK_CHECK(!gk_controller_from_item(228, &c));
    }
}

static void test_alarms(void)
{
    size_t n = 0;
    const gk_alarm_entry *e = gk_controller_alarms(GK_CTRL_FANUC_0I, &n);
    GK_CHECK(e != NULL);
    GK_CHECK(n > 0);
    GK_CHECK_STR_EQ(gk_controller_alarm_message(GK_CTRL_FANUC_0I, 401),
                    "SERVO ALARM");
    GK_CHECK(gk_controller_alarm_message(GK_CTRL_FANUC_0I, 99999) == NULL);

    /* SIEMENS has different scheme */
    GK_CHECK_STR_EQ(gk_controller_alarm_message(GK_CTRL_SIEMENS_840D, 700000),
                    "Emergency stop");
    GK_CHECK(gk_controller_alarm_message(GK_CTRL_SIEMENS_840D, 401) == NULL);

    GK_CHECK_STR_EQ(gk_controller_alarm_message(GK_CTRL_HNC8, 1),
                    "伺服报警");
}

static void test_param_pages(void)
{
    size_t n = gk_controller_param_page_count(GK_CTRL_FANUC_0I);
    GK_CHECK(n >= 6);
    GK_CHECK_STR_EQ(gk_controller_param_page_name(GK_CTRL_FANUC_0I, 0),
                    "SETTING");
    GK_CHECK(gk_controller_param_page_name(GK_CTRL_FANUC_0I, n) == NULL);
    GK_CHECK(gk_controller_param_page_count(GK_CTRL_HEIDENHAIN_TNC) == 9);
    GK_CHECK_STR_EQ(gk_controller_param_page_name(GK_CTRL_HNC8, 0), "参数");
}

static void test_dialect_convert(void)
{
    char buf[64];
    size_t n;
    n = gk_dialect_convert("G84 X10", GK_CTRL_FANUC_0I, GK_CTRL_FANUC_31I,
                           buf, sizeof(buf));
    GK_CHECK(n > 0);
    GK_CHECK_STR_EQ(buf, "G84 X10");

    gk_dialect_convert("G84 X10", GK_CTRL_FANUC_0I, GK_CTRL_SIEMENS_840D,
                       buf, sizeof(buf));
    GK_CHECK_STR_EQ(buf, "G841 X10");

    gk_dialect_convert("G28 Z0", GK_CTRL_FANUC_0I, GK_CTRL_SIEMENS_840D,
                       buf, sizeof(buf));
    GK_CHECK_STR_EQ(buf, "G74 Z0");

    gk_dialect_convert("G841 X10", GK_CTRL_SIEMENS_840D, GK_CTRL_FANUC_0I,
                       buf, sizeof(buf));
    GK_CHECK_STR_EQ(buf, "G84 X10");

    GK_CHECK_EQ_INT(gk_dialect_convert(NULL, 0, 0, buf, sizeof(buf)), 0);
}

static void test_compare(void)
{
    gk_ctrl_diff d;
    GK_CHECK_EQ_INT(gk_controller_compare(GK_CTRL_FANUC_0I, GK_CTRL_HAAS, &d),
                    GK_OK);
    GK_CHECK(d.same_dialect);
    GK_CHECK(d.same_alarm_scheme);

    GK_CHECK_EQ_INT(
        gk_controller_compare(GK_CTRL_FANUC_0I, GK_CTRL_SIEMENS_840D, &d),
        GK_OK);
    GK_CHECK(!d.same_dialect);
    GK_CHECK(!d.same_alarm_scheme);
    GK_CHECK(d.diff_count > 0);

    GK_CHECK_EQ_INT(gk_controller_compare((gk_controller)99, GK_CTRL_HAAS, &d),
                    GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_controller_compare(GK_CTRL_HAAS, GK_CTRL_HAAS, NULL),
                    GK_ERR_INVALID_ARG);
}

int main(void)
{
    test_machine_names();
    test_machine_def();
    test_machine_item_map();
    test_machine_predicates();
    test_controller_names();
    test_dialects();
    test_alarms();
    test_param_pages();
    test_dialect_convert();
    test_compare();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
