#include "gk_test.h"

#include "gk/gk_canned.h"
#include "gk/gk_tool.h"
#include "gk/gk_executor.h"
#include "gk/gk_parser.h"

#include <math.h>
#include <string.h>

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

static void test_canned_names(void)
{
    GK_CHECK_STR_EQ(gk_canned_name(GK_CANNED_G81), "G81");
    GK_CHECK_STR_EQ(gk_canned_name(GK_CANNED_G83), "G83");
    GK_CHECK(gk_canned_is_cycle(73));
    GK_CHECK(gk_canned_is_cycle(89));
    GK_CHECK(!gk_canned_is_cycle(80));
    GK_CHECK(!gk_canned_is_cycle(1));
    GK_CHECK_STR_EQ(gk_canned_name((gk_canned_cycle)99), "unknown");
}

static void test_canned_g81(void)
{
    gk_canned_params p;
    gk_canned_plan plan;
    memset(&p, 0, sizeof(p));
    p.cycle = GK_CANNED_G81;
    p.retract = GK_RETRACT_INITIAL;
    p.hole = gk_vec3_make(10, 20, 0);
    p.r_plane = 2.0;
    p.z_depth = -15.0;
    p.f_feed = 100.0;
    p.q_peck = 0.0;

    GK_CHECK_EQ_INT(gk_canned_expand(&p, gk_vec3_make(10, 20, 0), 5.0,
                                     &plan), GK_OK);
    /* rapid to XY, rapid to R, feed plunge, rapid out */
    GK_CHECK_EQ_INT(plan.action_count, 4);
    GK_CHECK_EQ_INT(plan.actions[0].kind, GK_CANNED_MOVE_RAPID_TO_XY);
    GK_CHECK_EQ_INT(plan.actions[1].kind, GK_CANNED_MOVE_RAPID_TO_R);
    GK_CHECK_EQ_INT(plan.actions[2].kind, GK_CANNED_MOVE_FEED_PLUNGE);
    GK_CHECK(near(plan.actions[2].target.z, -15.0, 1e-9));
    GK_CHECK_EQ_INT(plan.actions[3].kind, GK_CANNED_MOVE_RAPID_OUT);
    GK_CHECK(near(plan.actions[3].target.z, 5.0, 1e-9)); /* G98 initial */
    gk_canned_plan_free(&plan);
}

static void test_canned_g99_retract(void)
{
    gk_canned_params p;
    gk_canned_plan plan;
    memset(&p, 0, sizeof(p));
    p.cycle = GK_CANNED_G81;
    p.retract = GK_RETRACT_R_PLANE;
    p.r_plane = 2.0;
    p.z_depth = -10.0;
    p.f_feed = 100.0;
    GK_CHECK_EQ_INT(gk_canned_expand(&p, gk_vec3_make(0, 0, 0), 5.0, &plan),
                    GK_OK);
    GK_CHECK(near(plan.actions[plan.action_count - 1].target.z, 2.0, 1e-9));
    gk_canned_plan_free(&plan);
}

static void test_canned_g83_peck(void)
{
    gk_canned_params p;
    gk_canned_plan plan;
    size_t plunges = 0;
    size_t i;
    memset(&p, 0, sizeof(p));
    p.cycle = GK_CANNED_G83;
    p.retract = GK_RETRACT_INITIAL;
    p.r_plane = 0.0;
    p.z_depth = -10.0;
    p.q_peck = 3.0;
    p.f_feed = 50.0;
    GK_CHECK_EQ_INT(gk_canned_expand(&p, gk_vec3_make(0, 0, 0), 5.0, &plan),
                    GK_OK);
    for (i = 0; i < plan.action_count; ++i) {
        if (plan.actions[i].kind == GK_CANNED_MOVE_FEED_PLUNGE) {
            plunges += 1;
        }
    }
    /* R=0 to Z=-10 with q=3 -> plunges at -3,-6,-9,-10 = 4 */
    GK_CHECK_EQ_INT(plunges, 4);
    GK_CHECK(plan.action_count > 4);
    gk_canned_plan_free(&plan);
}

static void test_canned_invalid(void)
{
    gk_canned_params p;
    gk_canned_plan plan;
    memset(&p, 0, sizeof(p));
    p.cycle = GK_CANNED_G81;
    p.r_plane = -5.0;
    p.z_depth = 0.0; /* Z above R -> invalid */
    GK_CHECK_EQ_INT(gk_canned_expand(&p, gk_vec3_make(0, 0, 0), 0.0, &plan),
                    GK_ERR_OUT_OF_RANGE);
    p.cycle = GK_CANNED_NONE;
    GK_CHECK_EQ_INT(gk_canned_expand(&p, gk_vec3_make(0, 0, 0), 0.0, &plan),
                    GK_ERR_INVALID_ARG);
}

static void test_tool_table(void)
{
    gk_tool_table t;
    gk_tool tool;
    gk_tool_table_init(&t);
    memset(&tool, 0, sizeof(tool));
    tool.number = 1;
    tool.diameter = 10.0;
    tool.length = 50.0;
    tool.life_limit = 60.0;
    tool.life_warn = 0.8;
    strcpy(tool.name, "endmill");
    GK_CHECK_EQ_INT(gk_tool_table_add(&t, &tool), GK_OK);
    GK_CHECK_EQ_INT(gk_tool_table_add(&t, &tool), GK_ERR_ALREADY_EXISTS);
    GK_CHECK_EQ_INT(gk_tool_table_count(&t), 1);
    GK_CHECK(gk_tool_table_get(&t, 1) != NULL);
    GK_CHECK(gk_tool_table_get(&t, 2) == NULL);

    GK_CHECK_EQ_INT(gk_tool_consume_time(&t, 1, 10.0), GK_OK);
    GK_CHECK(!gk_tool_needs_warning(gk_tool_table_get(&t, 1)));
    GK_CHECK_EQ_INT(gk_tool_consume_time(&t, 1, 40.0), GK_OK);
    GK_CHECK(gk_tool_needs_warning(gk_tool_table_get(&t, 1)));
    GK_CHECK(!gk_tool_life_expired(gk_tool_table_get(&t, 1)));
    GK_CHECK_EQ_INT(gk_tool_consume_time(&t, 1, 15.0), GK_OK);
    GK_CHECK(gk_tool_life_expired(gk_tool_table_get(&t, 1)));
    GK_CHECK_EQ_INT(gk_tool_consume_time(&t, 9, 1.0), GK_ERR_NOT_FOUND);

    GK_CHECK_EQ_INT(gk_tool_table_remove(&t, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_tool_table_remove(&t, 1), GK_ERR_NOT_FOUND);
    gk_tool_table_free(&t);
}

static void test_cutter_comp(void)
{
    gk_point3 p;
    /* Moving +X, left comp radius 5 -> shift +Y by 5 */
    p = gk_comp_apply(GK_COMP_LEFT, 5.0, gk_vec3_make(0, 0, 0),
                      gk_vec3_make(0, 0, 0), gk_vec3_make(10, 0, 0));
    GK_CHECK(near(p.y, 5.0, 1e-9));
    /* Right comp -> shift -Y */
    p = gk_comp_apply(GK_COMP_RIGHT, 5.0, gk_vec3_make(0, 0, 0),
                      gk_vec3_make(0, 0, 0), gk_vec3_make(10, 0, 0));
    GK_CHECK(near(p.y, -5.0, 1e-9));
    /* Cancel -> no shift */
    p = gk_comp_apply(GK_COMP_CANCEL, 5.0, gk_vec3_make(0, 0, 0),
                      gk_vec3_make(0, 0, 0), gk_vec3_make(10, 0, 0));
    GK_CHECK(near(p.y, 0.0, 1e-9));
}

static void test_length_comp(void)
{
    GK_CHECK(near(gk_len_comp_apply(GK_LEN_COMP_POS, 50.0, 0.0), -50.0, 1e-9));
    GK_CHECK(near(gk_len_comp_apply(GK_LEN_COMP_NEG, 50.0, 0.0), 50.0, 1e-9));
    GK_CHECK(near(gk_len_comp_apply(GK_LEN_COMP_CANCEL, 50.0, 7.0),
                  7.0, 1e-9));
}

static void test_exec_drill_cycle(void)
{
    gk_program p;
    gk_executor ex;
    const char *src =
        "G21 G90 G17\n"
        "G00 X10 Y10 Z5\n"
        "G81 X10 Y10 Z-15 R2 F100\n"
        "X20\n"
        "X30\n"
        "G80\n"
        "M30\n";
    gk_program_init(&p, NULL);
    GK_CHECK_EQ_INT(gk_program_parse(&p, src, strlen(src)), GK_OK);
    gk_executor_init(&ex, NULL);
    gk_executor_load(&ex, &p);
    gk_executor_start(&ex);
    GK_CHECK_EQ_INT(gk_executor_run(&ex, 100), GK_OK);
    GK_CHECK_EQ_INT(ex.run_state, GK_EXEC_FINISHED);
    /* Last hole X30, Z at depth */
    GK_CHECK(near(ex.state.coord[GK_AXIS_X], 30.0, 1e-9));
    GK_CHECK(near(ex.state.coord[GK_AXIS_Z], -15.0, 1e-9));
    gk_executor_destroy(&ex);
    gk_program_free(&p);
}

static void test_exec_tool_change(void)
{
    gk_program p;
    gk_executor ex;
    gk_tool tool;
    const char *src =
        "G21 G90\n"
        "T01 M06\n"
        "G00 X10\n"
        "M30\n";
    gk_program_init(&p, NULL);
    GK_CHECK_EQ_INT(gk_program_parse(&p, src, strlen(src)), GK_OK);
    gk_executor_init(&ex, NULL);
    memset(&tool, 0, sizeof(tool));
    tool.number = 1;
    tool.diameter = 8.0;
    tool.life_limit = 1.0;
    tool.life_warn = 0.5;
    gk_tool_table_add(&ex.tools, &tool);
    gk_executor_load(&ex, &p);
    gk_executor_start(&ex);
    GK_CHECK_EQ_INT(gk_executor_run(&ex, 100), GK_OK);
    GK_CHECK_EQ_INT(ex.run_state, GK_EXEC_FINISHED);
    GK_CHECK_EQ_INT(ex.last_tool, 1);
    /* Some life consumed by the G00 move. */
    GK_CHECK(gk_tool_table_get(&ex.tools, 1)->life_used > 0.0);
    gk_executor_destroy(&ex);
    gk_program_free(&p);
}

static void test_exec_tool_missing(void)
{
    gk_program p;
    gk_executor ex;
    gk_tool tool;
    const char *src =
        "G21 G90\n"
        "T05 M06\n"
        "M30\n";
    gk_program_init(&p, NULL);
    gk_program_parse(&p, src, strlen(src));
    gk_executor_init(&ex, NULL);
    memset(&tool, 0, sizeof(tool));
    tool.number = 1;
    gk_tool_table_add(&ex.tools, &tool);
    gk_executor_load(&ex, &p);
    gk_executor_start(&ex);
    gk_executor_run(&ex, 100);
    GK_CHECK_EQ_INT(ex.run_state, GK_EXEC_ERROR);
    gk_executor_destroy(&ex);
    gk_program_free(&p);
}

int main(void)
{
    test_canned_names();
    test_canned_g81();
    test_canned_g99_retract();
    test_canned_g83_peck();
    test_canned_invalid();
    test_tool_table();
    test_cutter_comp();
    test_length_comp();
    test_exec_drill_cycle();
    test_exec_tool_change();
    test_exec_tool_missing();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
