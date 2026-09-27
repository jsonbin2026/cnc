#include "gk_test.h"
#include "gk/gk_csys.h"

#include <math.h>
#include <string.h>

static void test_selftest_load(void)
{
    gk_csys c;
    gk_csys_init(&c);
    GK_CHECK_STR_EQ(gk_csys_state_name(c.state), "powered-off");
    GK_CHECK(gk_csys_load_params(&c, "params.dat") == GK_ERR_STATE);
    GK_CHECK(gk_csys_selftest(&c, 0) == GK_ERR_STATE);
    GK_CHECK(c.state == GK_CSYS_FAULT);
    GK_CHECK_EQ_INT(c.alarmed, 1);
    GK_CHECK(gk_csys_reset_alarm(&c) == GK_OK);
    GK_CHECK_EQ_INT(c.alarmed, 0);
    gk_csys_init(&c);
    GK_CHECK(gk_csys_selftest(&c, 1) == GK_OK);
    GK_CHECK(c.state == GK_CSYS_LOADING);
    GK_CHECK_EQ_INT(c.selftest_passed, 1);
    GK_CHECK(gk_csys_load_params(&c, "params.dat") == GK_OK);
    GK_CHECK_EQ_INT(c.params_loaded, 1);
    GK_CHECK(gk_csys_load_params(&c, "") == GK_ERR_INVALID_ARG);
    GK_CHECK(gk_csys_load_tools(&c, 12) == GK_OK);
    GK_CHECK(gk_csys_load_wcs(&c, 6) == GK_OK);
    GK_CHECK(gk_csys_load_offsets(&c, 30) == GK_OK);
    GK_CHECK(gk_csys_load_macros(&c, 4) == GK_OK);
    GK_CHECK_EQ_INT(c.tools_loaded, 1);
    GK_CHECK_EQ_INT(c.macros_loaded, 1);
}

static void test_program(void)
{
    gk_csys c;
    gk_csys_init(&c);
    gk_csys_selftest(&c, 1);
    gk_csys_load_params(&c, "p");
    GK_CHECK(gk_csys_verify_program(&c, 1) == GK_ERR_STATE);
    GK_CHECK(gk_csys_load_program(&c, "O1001.nc") == GK_OK);
    GK_CHECK(gk_csys_verify_program(&c, 0) == GK_ERR_PARSE);
    GK_CHECK_EQ_INT(c.alarmed, 1);
    gk_csys_reset_alarm(&c);
    GK_CHECK(gk_csys_verify_program(&c, 1) == GK_OK);
}

static void test_startup_sequence(void)
{
    gk_csys c;
    int axis = 0;
    gk_csys_init(&c);
    gk_csys_selftest(&c, 1);
    gk_csys_load_params(&c, "p");
    gk_csys_load_program(&c, "p");
    GK_CHECK(gk_csys_start_machining(&c) == GK_ERR_STATE);
    GK_CHECK(gk_csys_home(&c, 3) == GK_ERR_STATE);
    GK_CHECK(gk_csys_plc_start(&c) == GK_OK);
    GK_CHECK_EQ_INT(c.plc_running, 1);
    GK_CHECK(gk_csys_enable_servos(&c) == GK_OK);
    GK_CHECK_EQ_INT(c.servos_enabled, 1);
    GK_CHECK(gk_csys_home(&c, 3) == GK_OK);
    GK_CHECK_EQ_INT(c.homed, 1);
    GK_CHECK(gk_csys_start_machining(&c) == GK_OK);
    GK_CHECK(c.state == GK_CSYS_RUNNING);
    GK_CHECK(gk_csys_pause(&c) == GK_OK);
    GK_CHECK(c.state == GK_CSYS_PAUSED);
    GK_CHECK(gk_csys_resume(&c) == GK_OK);
    GK_CHECK(c.state == GK_CSYS_RUNNING);
    GK_CHECK(gk_csys_end_program(&c) == GK_OK);
    GK_CHECK_EQ_INT(gk_csys_ready(&c), 1);
    GK_CHECK(gk_csys_reset_overtravel(&c, &axis) == GK_OK);
    GK_CHECK_EQ_INT(axis, -1);
    GK_CHECK(gk_csys_shutdown(&c) == GK_OK);
    GK_CHECK(c.state == GK_CSYS_POWERED_OFF);
}

static void test_touchoff_trialcut(void)
{
    gk_csys c;
    double stored = 0.0;
    double result = 0.0;
    gk_csys_init(&c);
    gk_csys_cold_start(&c);
    gk_csys_load_params(&c, "p");
    gk_csys_plc_start(&c);
    gk_csys_enable_servos(&c);
    gk_csys_home(&c, 3);
    GK_CHECK(gk_csys_touch_off(&c, 123.456, &stored) == GK_OK);
    GK_CHECK(fabs(stored - 123.456) < 1e-9);
    GK_CHECK(gk_csys_trial_cut(&c, 10.0, &result) == GK_OK);
    GK_CHECK(result < 10.0 && result > 9.0);
    GK_CHECK(gk_csys_trial_cut(&c, 0.0, &result) == GK_ERR_INVALID_ARG);
}

static void test_backup_upgrade(void)
{
    gk_csys c;
    char out[64];
    double ver = 0.0;
    gk_csys_init(&c);
    gk_csys_cold_start(&c);
    gk_csys_load_params(&c, "p");
    GK_CHECK(gk_csys_backup_params(&c, out, sizeof(out)) == GK_OK);
    GK_CHECK_STR_EQ(out, "PARAMS_BACKUP_OK");
    GK_CHECK(gk_csys_backup_program("O1", out, sizeof(out)) == GK_OK);
    GK_CHECK_STR_EQ(out, "PROG:O1");
    GK_CHECK(gk_csys_backup_program(NULL, out, sizeof(out)) ==
             GK_ERR_INVALID_ARG);
    GK_CHECK(gk_csys_upgrade(&c, "2.15", &ver) == GK_OK);
    GK_CHECK(fabs(ver - 2.15) < 1e-9);
    GK_CHECK(gk_csys_upgrade(&c, "abc", &ver) == GK_ERR_PARSE);
    GK_CHECK(gk_csys_restore(&c, 0) == GK_ERR_NOT_FOUND);
    GK_CHECK(gk_csys_restore(&c, 1) == GK_OK);
}

static void test_resets_start(void)
{
    gk_csys c;
    gk_csys_init(&c);
    c.state = GK_CSYS_ESTOP;
    GK_CHECK(gk_csys_reset_estop(&c) == GK_OK);
    GK_CHECK(c.state == GK_CSYS_READY);
    c.state = GK_CSYS_FAULT;
    c.alarmed = 1;
    GK_CHECK(gk_csys_reset_alarm(&c) == GK_OK);
    GK_CHECK(c.state == GK_CSYS_READY);
    GK_CHECK(gk_csys_reset_servo(&c) == GK_OK);
    GK_CHECK_EQ_INT(c.servos_enabled, 1);
    GK_CHECK(gk_csys_reset_system(&c) == GK_OK);
    GK_CHECK(c.state == GK_CSYS_POWERED_OFF);
    GK_CHECK(gk_csys_cold_start(&c) == GK_OK);
    GK_CHECK_EQ_INT(c.selftest_passed, 1);
    GK_CHECK(gk_csys_warm_start(&c) == GK_OK);
    GK_CHECK(c.state == GK_CSYS_READY);
    GK_CHECK(gk_csys_plc_stop(&c) == GK_OK);
    GK_CHECK_EQ_INT(c.plc_running, 0);
}

int main(void)
{
    test_selftest_load();
    test_program();
    test_startup_sequence();
    test_touchoff_trialcut();
    test_backup_upgrade();
    test_resets_start();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
