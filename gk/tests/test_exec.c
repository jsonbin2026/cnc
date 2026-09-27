#include "gk_test.h"

#include "gk/gk_executor.h"
#include "gk/gk_parser.h"
#include "gk/gk_transform.h"

#include <math.h>
#include <string.h>

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

static void load_program(gk_program *p, gk_executor *ex, const char *src)
{
    gk_program_init(p, NULL);
    GK_CHECK_EQ_INT(gk_program_parse(p, src, strlen(src)), GK_OK);
    GK_CHECK_EQ_INT(gk_executor_init(ex, NULL), GK_OK);
    GK_CHECK_EQ_INT(gk_executor_load(ex, p), GK_OK);
}

static void test_exec_linear(void)
{
    gk_program p;
    gk_executor ex;
    const char *src =
        "G21 G90 G17\n"
        "G00 X0 Y0 Z5\n"
        "G01 Z-2 F100\n"
        "G01 X50 F200\n"
        "M30\n";
    load_program(&p, &ex, src);
    gk_executor_start(&ex);
    GK_CHECK_EQ_INT(gk_executor_run(&ex, 100), GK_OK);
    GK_CHECK_EQ_INT(ex.run_state, GK_EXEC_FINISHED);
    GK_CHECK(near(ex.state.coord[GK_AXIS_X], 50.0, 1e-9));
    GK_CHECK(near(ex.state.coord[GK_AXIS_Z], -2.0, 1e-9));
    GK_CHECK(near(ex.state.feed, 200.0, 1e-9));
    GK_CHECK(ex.move_count >= 3);
    GK_CHECK(ex.elapsed_time > 0.0);
    gk_executor_destroy(&ex);
    gk_program_free(&p);
}

static void test_exec_modal_and_units(void)
{
    gk_program p;
    gk_executor ex;
    const char *src =
        "G20 G91\n"
        "G01 X1.0 F10\n"
        "G01 X1.0\n"
        "M30\n";
    load_program(&p, &ex, src);
    gk_executor_start(&ex);
    gk_executor_run(&ex, 100);
    /* Incremental: after first X=1.0 stays 1.0 (start 0) */
    GK_CHECK(near(ex.state.coord[GK_AXIS_X], 2.0, 1e-9));
    GK_CHECK_EQ_INT(ex.state.unit, GK_UNIT_INCH);
    GK_CHECK_EQ_INT(ex.state.distance, GK_DIST_INCREMENTAL);
    {
        gk_point3 pt = gk_vec3_make(1.0, 2.0, 3.0);
        gk_point3 m = gk_transform_to_machine(&ex.transform, &ex.state, pt);
        /* inch unit does not scale coordinates in this engine */
        GK_CHECK(near(m.x, 1.0, 1e-9));
    }
    gk_executor_destroy(&ex);
    gk_program_free(&p);
}

static void test_exec_arc(void)
{
    gk_program p;
    gk_executor ex;
    const char *src =
        "G21 G90 G17\n"
        "G00 X10 Y0\n"
        "G03 X0 Y10 I-10 J0 F100\n"
        "M30\n";
    load_program(&p, &ex, src);
    gk_executor_start(&ex);
    GK_CHECK_EQ_INT(gk_executor_run(&ex, 100), GK_OK);
    GK_CHECK_EQ_INT(ex.run_state, GK_EXEC_FINISHED);
    GK_CHECK(ex.has_last_move);
    GK_CHECK_EQ_INT(ex.last_move.mode, GK_MOTION_CCW);
    GK_CHECK(near(ex.last_move.radius, 10.0, 1e-6));
    GK_CHECK(near(gk_move_length(&ex.last_move), GK_PI * 10.0 / 2.0, 1e-6));
    gk_executor_destroy(&ex);
    gk_program_free(&p);
}

static void test_exec_incremental_arc(void)
{
    gk_program p;
    gk_executor ex;
    const char *src =
        "G21 G91 G17\n"
        "G00 X10 Y0\n"
        "G02 X-10 Y10 I-10 J0 F100\n"
        "M30\n";
    load_program(&p, &ex, src);
    gk_executor_start(&ex);
    GK_CHECK_EQ_INT(gk_executor_run(&ex, 100), GK_OK);
    GK_CHECK_EQ_INT(ex.run_state, GK_EXEC_FINISHED);
    GK_CHECK_EQ_INT(ex.last_move.mode, GK_MOTION_CW);
    gk_executor_destroy(&ex);
    gk_program_free(&p);
}

static void test_exec_single_step(void)
{
    gk_program p;
    gk_executor ex;
    const char *src =
        "G21 G90\n"
        "G00 X10\n"
        "G01 X20 F100\n"
        "G01 X30\n"
        "M30\n";
    load_program(&p, &ex, src);
    gk_executor_set_single_block(&ex, 1);
    gk_executor_start(&ex);
    GK_CHECK_EQ_INT(gk_executor_step(&ex), GK_OK);
    GK_CHECK_EQ_INT(ex.run_state, GK_EXEC_PAUSED);
    GK_CHECK_EQ_INT(ex.blocks_executed, 1);
    gk_executor_resume(&ex);
    GK_CHECK_EQ_INT(gk_executor_step(&ex), GK_OK);
    GK_CHECK_EQ_INT(ex.blocks_executed, 2);
    GK_CHECK(near(ex.state.coord[GK_AXIS_X], 10.0, 1e-9));
    gk_executor_destroy(&ex);
    gk_program_free(&p);
}

static void test_exec_breakpoint(void)
{
    gk_program p;
    gk_executor ex;
    const char *src =
        "G21 G90\n"
        "G00 X10\n"
        "G01 X20 F100\n"
        "G01 X30\n"
        "G01 X40\n"
        "M30\n";
    load_program(&p, &ex, src);
    GK_CHECK_EQ_INT(gk_executor_add_breakpoint(&ex, 3), GK_OK);
    GK_CHECK(gk_executor_has_breakpoint(&ex, 3));
    GK_CHECK_EQ_INT(gk_executor_add_breakpoint(&ex, 3),
                    GK_ERR_ALREADY_EXISTS);
    gk_executor_start(&ex);
    gk_executor_run(&ex, 100);
    GK_CHECK_EQ_INT(ex.run_state, GK_EXEC_PAUSED);
    GK_CHECK_EQ_INT(ex.cursor, 3);
    GK_CHECK_EQ_INT(gk_executor_remove_breakpoint(&ex, 3), GK_OK);
    GK_CHECK_EQ_INT(gk_executor_remove_breakpoint(&ex, 3), GK_ERR_NOT_FOUND);
    gk_executor_resume(&ex);
    gk_executor_run(&ex, 100);
    GK_CHECK_EQ_INT(ex.run_state, GK_EXEC_FINISHED);
    gk_executor_destroy(&ex);
    gk_program_free(&p);
}

static void test_exec_m00_pause(void)
{
    gk_program p;
    gk_executor ex;
    const char *src =
        "G21 G90\n"
        "G00 X10\n"
        "M00\n"
        "G01 X20 F100\n"
        "M30\n";
    load_program(&p, &ex, src);
    gk_executor_start(&ex);
    gk_executor_run(&ex, 100);
    GK_CHECK_EQ_INT(ex.run_state, GK_EXEC_PAUSED);
    gk_executor_resume(&ex);
    gk_executor_run(&ex, 100);
    GK_CHECK_EQ_INT(ex.run_state, GK_EXEC_FINISHED);
    gk_executor_destroy(&ex);
    gk_program_free(&p);
}

static void test_exec_estop(void)
{
    gk_program p;
    gk_executor ex;
    const char *src =
        "G21 G90\n"
        "M03 S1000\n"
        "G00 X10\n"
        "M30\n";
    load_program(&p, &ex, src);
    gk_executor_start(&ex);
    gk_executor_step(&ex); /* G21 G90 (modal) */
    gk_executor_step(&ex); /* M03 S1000 */
    GK_CHECK(ex.state.spindle_on == 1);
    gk_executor_step(&ex); /* G00 X10 */
    gk_executor_estop(&ex);
    GK_CHECK_EQ_INT(ex.run_state, GK_EXEC_ESTOP);
    GK_CHECK(ex.state.spindle_on == 0);
    GK_CHECK_EQ_INT(gk_executor_step(&ex), GK_ERR_STATE);
    gk_executor_resume(&ex);
    GK_CHECK_EQ_INT(ex.run_state, GK_EXEC_RUNNING);
    gk_executor_destroy(&ex);
    gk_program_free(&p);
}

static void test_exec_work_offset(void)
{
    gk_program p;
    gk_executor ex;
    const char *src =
        "G21 G90 G54\n"
        "G00 X0 Y0\n"
        "M30\n";
    load_program(&p, &ex, src);
    gk_transform_set_work_offset(&ex.transform, 0, gk_vec3_make(100, 200, 0));
    gk_executor_start(&ex);
    gk_executor_run(&ex, 100);
    GK_CHECK(ex.has_last_move);
    GK_CHECK(near(ex.last_move.end.x, 100.0, 1e-9));
    GK_CHECK(near(ex.last_move.end.y, 200.0, 1e-9));
    GK_CHECK_EQ_INT(ex.state.work_offset, 0);
    gk_executor_destroy(&ex);
    gk_program_free(&p);
}

static void test_exec_scale_rotate(void)
{
    gk_transform t;
    gk_machine_state s;
    gk_point3 out;
    gk_transform_init(&t);
    gk_state_init(&s);
    /* scale x2 */
    gk_transform_set_scale(&t, 2.0);
    out = gk_transform_to_machine(&t, &s, gk_vec3_make(3, 4, 0));
    GK_CHECK(near(out.x, 6.0, 1e-9));
    GK_CHECK(near(out.y, 8.0, 1e-9));
    gk_transform_cancel_scale(&t);
    /* rotate 90 deg about origin */
    gk_transform_set_rotation(&t, 90.0, gk_vec3_make(0, 0, 0));
    out = gk_transform_to_machine(&t, &s, gk_vec3_make(10, 0, 0));
    GK_CHECK(near(out.x, 0.0, 1e-9));
    GK_CHECK(near(out.y, 10.0, 1e-9));
    gk_transform_cancel_rotation(&t);
    /* local offset */
    gk_transform_set_local(&t, gk_vec3_make(5, 5, 5));
    out = gk_transform_to_machine(&t, &s, gk_vec3_make(1, 1, 1));
    GK_CHECK(near(out.x, 6.0, 1e-9));
}

static void test_exec_error_arc_missing_center(void)
{
    gk_program p;
    gk_executor ex;
    const char *src =
        "G21 G90 G17\n"
        "G00 X10 Y0\n"
        "G02 X0 Y10 F100\n"
        "M30\n";
    load_program(&p, &ex, src);
    gk_executor_start(&ex);
    gk_executor_run(&ex, 100);
    GK_CHECK_EQ_INT(ex.run_state, GK_EXEC_ERROR);
    gk_executor_destroy(&ex);
    gk_program_free(&p);
}

int main(void)
{
    test_exec_linear();
    test_exec_modal_and_units();
    test_exec_arc();
    test_exec_incremental_arc();
    test_exec_single_step();
    test_exec_breakpoint();
    test_exec_m00_pause();
    test_exec_estop();
    test_exec_work_offset();
    test_exec_scale_rotate();
    test_exec_error_arc_missing_center();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
