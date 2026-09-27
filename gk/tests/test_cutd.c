#include "gk_test.h"
#include "gk/gk_cutd.h"

#include <math.h>
#include <string.h>

static void test_entry_exit(void)
{
    gk_cutd_entry e;
    gk_cutd_exit x;
    gk_cutd_entry_init(&e, 1.5);
    GK_CHECK(fabs(gk_cutd_entry_signal(&e)) < 1e-12);
    GK_CHECK(gk_cutd_entry_engage(&e, 2.0, 3.0) == GK_OK);
    GK_CHECK(fabs(gk_cutd_entry_signal(&e) - 9.0) < 1e-9);
    GK_CHECK(gk_cutd_entry_engage(&e, -1.0, 3.0) == GK_ERR_INVALID_ARG);
    gk_cutd_exit_init(&x);
    GK_CHECK(gk_cutd_exit_leave(&x, 30.0, 0.5) == GK_OK);
    GK_CHECK(gk_cutd_exit_burr_height(&x) > 0.0);
    GK_CHECK_EQ_INT(gk_cutd_exit_chips(&x, 1.0), 1);
    GK_CHECK(gk_cutd_exit_leave(&x, 200.0, 1.0) == GK_ERR_OUT_OF_RANGE);
}

static void test_force_chatter(void)
{
    gk_cutd_force_wave w;
    gk_cutd_force_jump j;
    gk_cutd_chatter c;
    gk_cutd_force_wave_init(&w, 100.0, 10.0, 5.0);
    GK_CHECK(fabs(gk_cutd_force_wave_at(&w, 0.0) - 100.0) < 1e-9);
    GK_CHECK(fabs(gk_cutd_force_wave_at(&w, 0.05) - 110.0) < 1e-9);
    gk_cutd_force_jump_init(&j);
    GK_CHECK_EQ_INT(gk_cutd_force_jump_detected(&j), 0);
    GK_CHECK(gk_cutd_force_jump_update(&j, 50.0, 80.0) == GK_OK);
    GK_CHECK_EQ_INT(gk_cutd_force_jump_detected(&j), 0);
    GK_CHECK(gk_cutd_force_jump_update(&j, 60.0, 80.0) == GK_OK);
    GK_CHECK_EQ_INT(gk_cutd_force_jump_detected(&j), 0);
    GK_CHECK(gk_cutd_force_jump_update(&j, 200.0, 80.0) == GK_OK);
    GK_CHECK_EQ_INT(gk_cutd_force_jump_detected(&j), 1);
    gk_cutd_chatter_init(&c);
    GK_CHECK(gk_cutd_chatter_excite(&c, 500.0, 1.0) == GK_OK);
    GK_CHECK(c.amplitude > 0.0);
    GK_CHECK(gk_cutd_chatter_gain(&c) > 0.0);
    GK_CHECK(gk_cutd_chatter_excite(&c, 800.0, 0.5) == GK_OK);
    GK_CHECK(gk_cutd_chatter_unstable(&c, 0.1) == 0 ||
             gk_cutd_chatter_unstable(&c, 0.001) == 1);
}

static void test_noise_spark_smoke(void)
{
    gk_cutd_noise n;
    gk_cutd_spark sp;
    gk_cutd_smoke sm;
    gk_cutd_noise_init(&n);
    GK_CHECK(gk_cutd_noise_from_speed(&n, 100.0) > 60.0);
    GK_CHECK(gk_cutd_noise_from_force(&n, 100.0) > 60.0);
    gk_cutd_spark_init(&sp);
    GK_CHECK(gk_cutd_spark_emit(&sp, 10.0, 200.0) == GK_OK);
    GK_CHECK_EQ_INT(sp.active, 1);
    GK_CHECK(gk_cutd_spark_visible(&sp, 0.5));
    GK_CHECK(gk_cutd_spark_emit(&sp, -1.0, 200.0) == GK_ERR_INVALID_ARG);
    gk_cutd_smoke_init(&sm);
    GK_CHECK(gk_cutd_smoke_generate(&sm, 10.0, 0.0) > 0.0);
}

static void test_smell(void)
{
    GK_CHECK_STR_EQ(gk_cutd_smell_name(GK_CUTD_SMELL_BURN), "burn");
    GK_CHECK(gk_cutd_smell_classify(700.0, 0) == GK_CUTD_SMELL_BURN);
    GK_CHECK(gk_cutd_smell_classify(150.0, 1) == GK_CUTD_SMELL_COOLANT);
    GK_CHECK(gk_cutd_smell_classify(250.0, 1) == GK_CUTD_SMELL_COOLANT);
    GK_CHECK(gk_cutd_smell_classify(250.0, 0) == GK_CUTD_SMELL_OIL);
    GK_CHECK(gk_cutd_smell_classify(50.0, 0) == GK_CUTD_SMELL_NONE);
}

static void test_chips(void)
{
    gk_cutd_chip_fly f;
    gk_cutd_chip_pile p;
    gk_cutd_chip_tangle t;
    gk_cutd_chip_break b;
    char out[32];
    gk_cutd_chip_fly_init(&f);
    GK_CHECK(gk_cutd_chip_fly_compute(&f, 1000.0, 10.0) > 0.0);
    gk_cutd_chip_pile_init(&p);
    p.produced_g_s = 1.0;
    p.removed_g_s = 0.5;
    GK_CHECK(gk_cutd_chip_pile_update(&p, 2.0) == GK_OK);
    GK_CHECK(fabs(p.accumulated_g - 1.0) < 1e-9);
    GK_CHECK_EQ_INT(gk_cutd_chip_pile_clear(&p, 0.5), 1);
    GK_CHECK(fabs(p.accumulated_g) < 1e-12);
    gk_cutd_chip_tangle_init(&t);
    GK_CHECK(gk_cutd_chip_tangle_grow(&t, 10.0) == GK_OK);
    GK_CHECK_EQ_INT(gk_cutd_chip_tangle_check(&t, 50.0), 0);
    GK_CHECK_EQ_INT(gk_cutd_chip_tangle_check(&t, 5.0), 1);
    gk_cutd_chip_break_init(&b);
    GK_CHECK_EQ_INT(gk_cutd_chip_break_check(&b, 1.0, 1.0), 1);
    GK_CHECK_EQ_INT(gk_cutd_chip_break_check(&b, 0.01, 0.01), 0);
    GK_CHECK(gk_cutd_chip_colour(100.0, out, sizeof(out)) == GK_OK);
    GK_CHECK_STR_EQ(out, "silver");
    GK_CHECK(gk_cutd_chip_colour(450.0, out, sizeof(out)) == GK_OK);
    GK_CHECK_STR_EQ(out, "brown");
    GK_CHECK(gk_cutd_chip_colour(700.0, out, sizeof(out)) == GK_OK);
    GK_CHECK_STR_EQ(out, "purple");
}

static void test_tool_wear(void)
{
    gk_cutd_tool_wear w;
    gk_cutd_redheat r;
    GK_CHECK_STR_EQ(gk_cutd_wear_name(GK_CUTD_WEAR_CHIP), "chip");
    gk_cutd_tool_wear_init(&w, 0.001);
    GK_CHECK(gk_cutd_tool_wear_advance(&w, 100.0, 1.0) == GK_OK);
    GK_CHECK(fabs(w.wear_mm - 0.1) < 1e-9);
    GK_CHECK_EQ_INT(gk_cutd_tool_wear_break_check(&w, 1.0), 0);
    GK_CHECK(gk_cutd_tool_wear_shock(&w, 0.95) == GK_OK);
    GK_CHECK_EQ_INT(gk_cutd_tool_wear_break_check(&w, 1.0), 1);
    GK_CHECK_EQ_INT(w.broken, 1);
    GK_CHECK_EQ_INT(gk_cutd_tool_wear_chipped(&w, 0.5), 1);
    gk_cutd_redheat_init(&r, 700.0);
    GK_CHECK(fabs(r.temperature_c - 20.0) < 1e-9);
    gk_cutd_redheat_update(&r, 500.0, 100.0, 2.0);
    GK_CHECK(r.temperature_c > 700.0);
    GK_CHECK_EQ_INT(gk_cutd_redheat_critical(&r), 1);
}

static void test_surface(void)
{
    gk_cutd_pattern p;
    gk_cutd_surface s;
    gk_cutd_pattern_init(&p);
    GK_CHECK(fabs(gk_cutd_pattern_pitch(&p, 0.2) - 0.2) < 1e-9);
    GK_CHECK(gk_cutd_pattern_ra(&p) > 0.0);
    gk_cutd_surface_init(&s);
    GK_CHECK_STR_EQ(gk_cutd_defect_name(s.defect), "none");
    GK_CHECK(gk_cutd_surface_detect(&s, 700.0, 0.0, 0.0, 0.0) == GK_OK);
    GK_CHECK(s.defect == GK_CUTD_DEFECT_BURN);
    GK_CHECK(gk_cutd_surface_detect(&s, 100.0, 0.0, 0.0, 10.0) == GK_OK);
    GK_CHECK(s.defect == GK_CUTD_DEFECT_SCRATCH);
    GK_CHECK(gk_cutd_surface_detect(&s, 100.0, 20.0, 0.0, 0.0) == GK_OK);
    GK_CHECK(s.defect == GK_CUTD_DEFECT_CHATTER);
    GK_CHECK(gk_cutd_surface_detect(&s, 100.0, 0.0, 100.0, 0.0) == GK_OK);
    GK_CHECK(s.defect == GK_CUTD_DEFECT_BURR);
    GK_CHECK(gk_cutd_surface_detect(&s, 100.0, 0.0, 0.0, 0.0) == GK_OK);
    GK_CHECK(s.defect == GK_CUTD_DEFECT_NONE);
}

static void test_drift_thermal(void)
{
    gk_cutd_drift d;
    gk_cutd_thermal t;
    gk_cutd_stress st;
    gk_cutd_deflect df;
    gk_cutd_drift_init(&d, 100.0);
    gk_cutd_drift_update(&d, 10.0, 1.0);
    GK_CHECK(d.drift_mm > 0.0);
    GK_CHECK(fabs(gk_cutd_drift_deviation(&d) - d.drift_mm) < 1e-12);
    gk_cutd_thermal_init(&t, 1000.0, 1.2e-5);
    GK_CHECK(fabs(gk_cutd_thermal_expansion(&t, 10.0) - 0.12) < 1e-9);
    GK_CHECK(fabs(gk_cutd_thermal_contraction(&t, 10.0) + 0.12) < 1e-9);
    gk_cutd_stress_init(&st, 200000.0, 1000.0);
    GK_CHECK(gk_cutd_stress_distortion(&st, 100.0) > 0.0);
    gk_cutd_deflect_init(&df, 1000.0);
    GK_CHECK(fabs(gk_cutd_deflect_apply(&df, 500.0) - 0.5) < 1e-9);
    GK_CHECK(fabs(gk_cutd_deflect_release(&df, 0.5) - 0.25) < 1e-9);
    GK_CHECK(fabs(df.deflection_mm - 0.25) < 1e-9);
}

int main(void)
{
    test_entry_exit();
    test_force_chatter();
    test_noise_spark_smoke();
    test_smell();
    test_chips();
    test_tool_wear();
    test_surface();
    test_drift_thermal();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
