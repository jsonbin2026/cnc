#include "gk_test.h"
#include "gk/gk_svo.h"

#include <math.h>
#include <string.h>

static void test_axis_states(void)
{
    gk_svo_axis a;
    gk_svo_axis_init(&a);
    GK_CHECK_STR_EQ(gk_svo_state_name(a.state), "off");
    GK_CHECK(gk_svo_enable(&a) == GK_ERR_STATE);
    GK_CHECK(gk_svo_power_on(&a) == GK_OK);
    GK_CHECK(a.state == GK_SVO_READY);
    GK_CHECK(gk_svo_enable(&a) == GK_OK);
    GK_CHECK(a.state == GK_SVO_RUNNING);
    GK_CHECK_EQ_INT(a.enabled, 1);
    GK_CHECK(gk_svo_set_load(&a, 200.0) == GK_OK);
    GK_CHECK(a.state == GK_SVO_OVERLOAD);
    GK_CHECK_EQ_INT(a.enabled, 0);
    GK_CHECK_EQ_INT(gk_svo_in_alarm(&a), 1);
    GK_CHECK(gk_svo_reset_alarm(&a) == GK_OK);
    GK_CHECK(a.state == GK_SVO_READY);
    GK_CHECK_EQ_INT(gk_svo_in_alarm(&a), 0);
    a.state = GK_SVO_RUNNING;
    GK_CHECK(gk_svo_set_temperature(&a, 200.0) == GK_OK);
    GK_CHECK(a.state == GK_SVO_OVERHEAT);
    GK_CHECK(gk_svo_encoder_alarm(&a) == GK_OK);
    GK_CHECK(a.state == GK_SVO_ENCODER_FAULT);
    GK_CHECK(gk_svo_disable(&a) == GK_OK);
}

static void test_waves(void)
{
    gk_svo_wave w;
    gk_svo_wave_init(&w, 10.0, 50.0, 5.0);
    GK_CHECK(fabs(gk_svo_wave_at(&w, 0.0) - 5.0) < 1e-9);
    GK_CHECK(gk_svo_wave_at(&w, 0.005) > 5.0);
}

static void test_pid_loop(void)
{
    gk_svo_pid p;
    gk_svo_loop l;
    gk_svo_pid_init(&p, 2.0, 1.0, 0.0);
    GK_CHECK(fabs(gk_svo_pid_step(&p, 10.0, 1.0) - 30.0) < 1e-9);
    GK_CHECK(p.integral > 0.0);
    gk_svo_pid_reset(&p);
    GK_CHECK(fabs(p.integral) < 1e-12);
    gk_svo_loop_init(&l);
    GK_CHECK(gk_svo_current_loop(&l, 2.0) > 0.0);
    GK_CHECK(gk_svo_velocity_loop(&l, 2.0) > 0.0);
    GK_CHECK(gk_svo_position_loop(&l, 2.0) > 0.0);
    l.position_command = 10.0;
    l.position_feedback = 0.0;
    GK_CHECK(gk_svo_loop_step(&l, 1.0) > 0.0);
    GK_CHECK(fabs(gk_svo_loop_step(&l, 0.0)) < 1e-12);
}

static void test_comp_ff(void)
{
    gk_svo_comp c;
    gk_svo_ff f;
    gk_svo_comp_init(&c);
    GK_CHECK(gk_svo_feedforward(&c, 10.0, 1.0) > 0.0);
    GK_CHECK(gk_svo_friction_comp(&c, 10.0) > 0.0);
    GK_CHECK(gk_svo_friction_comp(&c, -10.0) < 0.0);
    GK_CHECK(fabs(gk_svo_friction_comp(&c, 0.0)) < 1e-12);
    gk_svo_ff_init(&f);
    GK_CHECK(fabs(gk_svo_ff_accel(&f, 2.0, 3.0) - 6.0) < 1e-9);
    GK_CHECK(fabs(gk_svo_ff_velocity(&f, 2.0, 3.0) - 6.0) < 1e-9);
    GK_CHECK(fabs(gk_svo_ff_torque(&f, 2.0, 3.0) - 6.0) < 1e-9);
}

static void test_backlash_pitch(void)
{
    gk_svo_backlash b;
    gk_svo_pitch p;
    gk_svo_geom g;
    gk_svo_backlash_init(&b, 0.02);
    GK_CHECK(fabs(gk_svo_backlash_compensate(&b, 1.0)) < 1e-12);
    GK_CHECK(fabs(gk_svo_backlash_compensate(&b, 1.0)) < 1e-12);
    GK_CHECK(fabs(gk_svo_backlash_compensate(&b, -1.0) - 0.02) < 1e-9);
    gk_svo_pitch_init(&p, 10, 100.0);
    GK_CHECK(fabs(gk_svo_pitch_compensate(&p, 0)) < 1e-12);
    GK_CHECK(gk_svo_pitch_compensate(&p, 9) > 0.0);
    GK_CHECK(fabs(gk_svo_pitch_total(&p) - gk_svo_pitch_compensate(&p, 9)) <
             1e-9);
    gk_svo_geom_init(&g, 1000.0);
    GK_CHECK(fabs(gk_svo_straightness_comp(&g, 0.5) - 0.0005) < 1e-9);
    GK_CHECK(fabs(gk_svo_squareness_comp(&g, 1.0) - 0.001) < 1e-9);
}

static void test_thermal_gain(void)
{
    gk_svo_thermal_comp t;
    gk_svo_tuning tn;
    gk_svo_thermal_comp_init(&t, 0.001, 20.0);
    GK_CHECK(fabs(gk_svo_thermal_compensate(&t, 30.0) - 0.01) < 1e-9);
    gk_svo_tuning_init(&tn);
    GK_CHECK(fabs(gk_svo_set_gain(&tn, 5.0) - 5.0) < 1e-9);
    GK_CHECK(fabs(gk_svo_set_rigidity(&tn, 20.0) - 20.0) < 1e-9);
    GK_CHECK(fabs(gk_svo_set_gain(&tn, -1.0)) < 1e-12);
}

static void test_filters(void)
{
    gk_svo_notch n;
    gk_svo_lowpass lp;
    gk_svo_smooth sm;
    gk_svo_notch_init(&n);
    GK_CHECK(gk_svo_notch_response(&n, 500.0) < 1.0);
    GK_CHECK(gk_svo_notch_response(&n, 2000.0) > 0.9);
    GK_CHECK_EQ_INT(gk_svo_notch_suppressed(&n, 1.0), 1);
    gk_svo_lowpass_init(&lp, 100.0, 1000.0);
    GK_CHECK(lp.alpha > 0.0 && lp.alpha < 1.0);
    GK_CHECK(fabs(gk_svo_lowpass_apply(&lp, 0.0)) < 1e-12);
    GK_CHECK(gk_svo_lowpass_apply(&lp, 100.0) > 0.0);
    gk_svo_smooth_init(&sm, 0.5);
    GK_CHECK(fabs(gk_svo_smooth_apply(&sm, 0.0)) < 1e-12);
    GK_CHECK(fabs(gk_svo_smooth_apply(&sm, 10.0) - 5.0) < 1e-9);
}

int main(void)
{
    test_axis_states();
    test_waves();
    test_pid_loop();
    test_comp_ff();
    test_backlash_pitch();
    test_thermal_gain();
    test_filters();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
