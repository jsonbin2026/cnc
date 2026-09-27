#include "gk_test.h"

#include "gk/gk_material.h"

#include <math.h>

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

static void test_coolant(void)
{
    gk_coolant_jet j;
    GK_CHECK_STR_EQ(gk_coolant_name(GK_COOLANT_FLOOD), "FLOOD");
    GK_CHECK_STR_EQ(gk_coolant_name(GK_COOLANT_MIST), "MIST");
    GK_CHECK_STR_EQ(gk_coolant_name((gk_coolant_type)99), "unknown");

    GK_CHECK_EQ_INT(gk_coolant_jet_init(&j, GK_COOLANT_FLOOD,
                                        gk_vec3_make(0, 0, -1), 20.0, 10.0),
                    GK_OK);
    GK_CHECK(near(gk_vec3_length(j.nozzle_dir), 1.0, 1e-9));

    /* aim directly at cut point -> high effectiveness */
    {
        double e = gk_coolant_effectiveness(&j, gk_vec3_make(0, 0, 5),
                                            gk_vec3_make(0, 0, 0));
        GK_CHECK(e > 0.8 && e <= 1.0);
    }
    /* tool through-coolant is best */
    {
        gk_coolant_jet t;
        gk_coolant_jet_init(&t, GK_COOLANT_THROUGH, gk_vec3_make(0, 0, -1),
                            40, 20);
        GK_CHECK(gk_coolant_effectiveness(&t, gk_vec3_make(0, 0, 5),
                                          gk_vec3_make(0, 0, 0)) >
                 gk_coolant_effectiveness(&j, gk_vec3_make(0, 0, 5),
                                          gk_vec3_make(0, 0, 0)));
    }
    /* off is zero */
    {
        gk_coolant_jet o;
        gk_coolant_jet_init(&o, GK_COOLANT_OFF, gk_vec3_make(0, 0, -1),
                            0, 0);
        GK_CHECK(near(gk_coolant_effectiveness(&o, gk_vec3_make(0, 0, 5),
                                               gk_vec3_make(0, 0, 0)),
                      0.0, 1e-12));
    }
}

static void test_height_map(void)
{
    gk_height_map h;
    double v = 0.0;
    GK_CHECK_EQ_INT(gk_height_map_init(&h, 10, 10, 1.0, 5.0, NULL), GK_OK);
    GK_CHECK_EQ_INT(gk_height_map_at(&h, 0, 0, &v), GK_OK);
    GK_CHECK(near(v, 5.0, 1e-12));
    GK_CHECK_EQ_INT(gk_height_map_at(&h, 10, 0, &v), GK_ERR_OUT_OF_RANGE);

    GK_CHECK_EQ_INT(gk_height_map_cut(&h, 2, 3, 1.0), 1);
    GK_CHECK_EQ_INT(gk_height_map_cut(&h, 2, 3, 3.0), 0);
    gk_height_map_at(&h, 2, 3, &v);
    GK_CHECK(near(v, 1.0, 1e-12));

    GK_CHECK_EQ_INT(gk_height_map_cut_segment(&h, gk_vec3_make(0, 0, 2),
                                              gk_vec3_make(9, 0, 2),
                                              1.0),
                    GK_OK);
    gk_height_map_at(&h, 5, 0, &v);
    GK_CHECK(near(v, 2.0, 1e-12));
    gk_height_map_destroy(&h);
}

static void test_csg(void)
{
    gk_voxel_grid a, b;
    size_t removed;
    GK_CHECK_EQ_INT(gk_voxel_init(&a, 4, 4, 4, 1.0, gk_vec3_make(0, 0, 0),
                                  NULL),
                    GK_OK);
    GK_CHECK_EQ_INT(gk_voxel_init(&b, 4, 4, 4, 1.0, gk_vec3_make(0, 0, 0),
                                  NULL),
                    GK_OK);
    /* b emptied except center */
    gk_voxel_cut_point(&b, gk_vec3_make(-10, -10, -10), 100.0, &removed);
    GK_CHECK_EQ_INT(gk_voxel_present_count(&b), 0);

    /* difference: a - b = a when b empty */
    GK_CHECK_EQ_INT(gk_voxel_csg(&a, &b, GK_CSG_DIFFERENCE), GK_OK);
    GK_CHECK_EQ_INT(gk_voxel_present_count(&a), gk_voxel_count(&a));

    /* union with empty = same */
    GK_CHECK_EQ_INT(gk_voxel_csg(&a, &b, GK_CSG_UNION), GK_OK);

    /* intersect with empty = empty */
    GK_CHECK_EQ_INT(gk_voxel_csg(&a, &b, GK_CSG_INTERSECT), GK_OK);
    GK_CHECK_EQ_INT(gk_voxel_present_count(&a), 0);

    gk_voxel_destroy(&a);
    gk_voxel_destroy(&b);
}

static void test_surface(void)
{
    gk_surface_quality q;
    double s = gk_scallop_height(5.0, 2.0);
    GK_CHECK(near(s, 5.0 - sqrt(25.0 - 1.0), 1e-9));
    GK_CHECK(near(gk_scallop_height(0.0, 1.0), 0.0, 1e-12));
    /* wide step over clamps to radius */
    GK_CHECK(near(gk_scallop_height(5.0, 20.0), 5.0, 1e-12));

    GK_CHECK_EQ_INT(gk_surface_roughness(0.1, 5.0, 1.0, 0.4, &q), GK_OK);
    GK_CHECK(q.ra > 0.0);
    GK_CHECK(q.rz > q.ra);
    GK_CHECK(near(q.step_over, 1.0, 1e-12));

    GK_CHECK(gk_burr_height(90.0, 2.0, 5.0) > gk_burr_height(10.0, 2.0, 5.0));
}

static void test_quality_detect(void)
{
    gk_cut_check c = gk_cut_check_eval(10.0, 8.5, 0.5);
    GK_CHECK(near(c.error, -1.5, 1e-12));
    GK_CHECK(gk_overcut_detect(10.0, 8.5, 0.5));
    GK_CHECK(!gk_overcut_detect(10.0, 9.8, 0.5));
    GK_CHECK(gk_undercut_detect(10.0, 11.0, 0.5));
    GK_CHECK(!gk_undercut_detect(10.0, 10.2, 0.5));
}

static void test_force(void)
{
    gk_cut_force_params p;
    gk_cut_force f;
    p.kc = 2000.0;
    p.mc = 0.25;
    p.feed_per_tooth = 0.1;
    p.depth_of_cut = 2.0;
    p.width_of_cut = 10.0;
    p.diameter = 10.0;
    p.flutes = 4;
    GK_CHECK_EQ_INT(gk_cut_force_model(&p, 5000.0, &f), GK_OK);
    GK_CHECK(f.tangential > 0.0);
    GK_CHECK(near(f.tangential, 2000.0 * 0.1 * 2.0, 1e-9));
    GK_CHECK(f.power_kw > 0.0);
    GK_CHECK(f.torque > 0.0);
    p.diameter = 0.0;
    GK_CHECK_EQ_INT(gk_cut_force_model(&p, 5000.0, &f), GK_ERR_INVALID_ARG);
}

static void test_wear(void)
{
    gk_wear_model m;
    m.initial_wear = 0.02;
    m.rate = 0.01;
    m.max_wear = 0.30;
    GK_CHECK(near(gk_tool_wear(&m, 10.0, 1.0), 0.12, 1e-12));
    GK_CHECK(near(gk_tool_wear(&m, 100.0, 1.0), 0.30, 1e-12)); /* clamped */
    GK_CHECK(!gk_tool_life_alarm(0.12, &m));
    GK_CHECK(gk_tool_life_alarm(0.30, &m));

    /* Taylor: V=100, C=1000, n=0.25 -> T=10000 min */
    GK_CHECK(near(gk_taylor_life(100.0, 1000.0, 0.25), 10000.0, 1e-6));
    GK_CHECK(near(gk_taylor_life(0.0, 1000.0, 0.25), 0.0, 1e-12));

    GK_CHECK(gk_tool_break_detect(30.0, 10.0, 2.0));
    GK_CHECK(!gk_tool_break_detect(15.0, 10.0, 2.0));
}

static void test_chip(void)
{
    GK_CHECK_STR_EQ(gk_chip_form_name(GK_CHIP_CONTINUOUS), "continuous");
    GK_CHECK_EQ_INT(gk_chip_form_for(0.8, 40.0, 0.1), GK_CHIP_BUILT_UP);
    GK_CHECK_EQ_INT(gk_chip_form_for(0.7, 200.0, 0.1), GK_CHIP_CONTINUOUS);
    GK_CHECK_EQ_INT(gk_chip_form_for(0.2, 100.0, 0.1), GK_CHIP_DISCONTINUOUS);

    {
        double r = gk_chip_curl_radius(1.0, 10.0);
        GK_CHECK(r > 0.0);
        GK_CHECK(gk_chip_breaks(0.5, 0.2, 0.1));
        GK_CHECK(!gk_chip_breaks(5.0, 0.2, 0.1));
        GK_CHECK(gk_chip_breaks(0.0, 0.2, 0.1));
    }
    GK_CHECK(gk_bue_tendency(10.0, 300.0, 0.9) >
             gk_bue_tendency(200.0, 100.0, 0.5));
    GK_CHECK(gk_edge_radius_effect(0.05, 0.1, 15.0) < GK_DEG2RAD(15.0));
    GK_CHECK(near(gk_hardness_at(30.0, gk_vec3_make(0, 0, 0),
                                 gk_vec3_make(0, 0, 0), 1.0),
                  45.0, 1e-9));
    GK_CHECK(near(gk_hardness_at(30.0, gk_vec3_make(100, 0, 0),
                                 gk_vec3_make(0, 0, 0), 1.0),
                  30.0, 0.01));
}

static void test_thermal(void)
{
    gk_thermal_model m;
    m.alpha = 12e-6;
    m.delta_t = 50.0;
    m.length = 1000.0;
    GK_CHECK(near(gk_thermal_expansion(&m), 0.6, 1e-9));

    GK_CHECK(near(gk_spindle_thermal_growth(50.0, 200.0, 12e-6),
                  0.12, 1e-9));
    GK_CHECK(near(gk_ballscrew_thermal_growth(30.0, 1000.0, 11e-6),
                  0.33, 1e-9));

    GK_CHECK(near(gk_thermal_lag(0.0, 100.0, 10.0, 0.0), 0.0, 1e-9));
    GK_CHECK(near(gk_thermal_lag(0.0, 100.0, 10.0, 1000.0), 100.0, 1e-3));

    /* center of simply-supported beam deflection */
    {
        double d = gk_beam_deflection(1000.0, 100.0, 200000.0, 1000.0, 50.0);
        GK_CHECK(d > 0.0);
        GK_CHECK(near(gk_beam_deflection(1.0, 1.0, 0.0, 1.0, 0.5), 0.0,
                      1e-12));
    }
    GK_CHECK(near(gk_thermo_mech_strain(12e-6, 50.0, 200.0, 200000.0),
                  0.0006 + 0.001, 1e-9));
}

static void test_dynamics(void)
{
    gk_modal_mode m;
    m.mass = 1.0;
    m.stiffness = 4.0 * GK_PI * GK_PI; /* wn=2pi -> f=1 */
    m.damping = 0.05;
    m.frequency = 0.0;
    GK_CHECK(near(gk_natural_freq(&m), 1.0, 1e-6));
    GK_CHECK(gk_mode_response(&m, 1.0, 100.0) > gk_mode_response(&m, 0.0, 100.0));

    GK_CHECK(gk_stability_lobe_depth(1000.0, 0.05, 1e6, 500.0) > 0.0);
    GK_CHECK(near(gk_stability_lobe_depth(0.0, 0.05, 1e6, 500.0), 0.0, 1e-12));
    GK_CHECK(gk_chatter_detect(0.5, 0.1, 1.0));
    GK_CHECK(!gk_chatter_detect(0.15, 0.1, 1.0));
    GK_CHECK(near(gk_servo_flex_error(100.0, 1000.0, 0.0, 0.0), 0.1, 1e-9));
}

static void test_effects(void)
{
    gk_spark_effect s = gk_spark_generate(50.0, 8000.0, 3.0);
    GK_CHECK(s.intensity > 0.0 && s.intensity <= 1.0);
    GK_CHECK(s.particle_count > 0.0);
    GK_CHECK(s.lifetime >= 0.3);
    GK_CHECK(gk_smoke_density(0.0, 600.0, 40.0) >
             gk_smoke_density(1.0, 100.0, 1.0));
}

int main(void)
{
    test_coolant();
    test_height_map();
    test_csg();
    test_surface();
    test_quality_detect();
    test_force();
    test_wear();
    test_chip();
    test_thermal();
    test_dynamics();
    test_effects();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
