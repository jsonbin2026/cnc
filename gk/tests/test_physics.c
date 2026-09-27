#include "gk_test.h"

#include "gk/gk_physics.h"

#include <math.h>
#include <string.h>

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

static void test_structure(void)
{
    gk_structure s;

    gk_structure_init(&s, 10.0, 1000.0, 20.0);
    GK_CHECK(near(gk_structure_natural_freq(&s),
                  0.5 * sqrt(1000.0 / 10.0) / 3.14159265358979323846, 1e-6));
    GK_CHECK(gk_structure_damping_ratio(&s) > 0.0);
    GK_CHECK(gk_structure_natural_freq(NULL) == 0.0);
    gk_structure z;
    gk_structure_init(&z, 0.0, 0.0, 0.0);
    GK_CHECK(gk_structure_natural_freq(&z) == 0.0);
    GK_CHECK(gk_structure_damping_ratio(&z) == 0.0);
}

static void test_chatter(void)
{
    gk_structure s;
    double lim;

    gk_structure_init(&s, 50.0, 2.0e7, 500.0);
    lim = gk_chatter_stability_limit(&s, 2000.0, 3000.0, 4,
                                     GK_CHATTER_MILLING);
    GK_CHECK(lim > 0.0);
    GK_CHECK_EQ_INT(gk_chatter_stability_limit(NULL, 1.0, 1.0, 1,
                                               GK_CHATTER_TURNING), 0.0);
    GK_CHECK(gk_chatter_growth_rate(&s, 2.0, 2000.0, 5.0) >
             gk_chatter_growth_rate(&s, 0.1, 2000.0, 5.0));
}

static void test_lobe(void)
{
    gk_structure s;
    gk_lobe l;

    gk_structure_init(&s, 50.0, 2.0e7, 500.0);
    gk_lobe_init(&l);
    GK_CHECK(gk_lobe_generate(&l, &s, 2000.0, 4, 1) > 0);
    GK_CHECK(l.count > 0);
    GK_CHECK(gk_lobe_max_depth(&l) > 0.0);
    GK_CHECK_EQ_INT(gk_lobe_safe_at(&l, l.rpm[0], l.depth[0] * 0.5), 1);
    GK_CHECK_EQ_INT(gk_lobe_safe_at(&l, l.rpm[0], l.depth[0] * 10.0), 0);
    gk_lobe empty;
    gk_lobe_init(&empty);
    GK_CHECK_EQ_INT(gk_lobe_safe_at(&empty, 0.0, 100.0), 1);
}

static void test_workpiece(void)
{
    gk_workpiece w;
    double older;

    gk_workpiece_init(&w, 200.0, 50.0, 20.0, 210.0, 0.3);
    older = gk_workpiece_deflection(&w, 1000.0, 100.0);
    GK_CHECK(older > 0.0);
    GK_CHECK(gk_workpiece_deflection(&w, 2000.0, 100.0) > older);
    GK_CHECK(gk_workpiece_stiffness(&w) > 0.0);
    GK_CHECK_EQ_INT(gk_workpiece_deflection(&w, 1000.0, 0.0), 0.0);
}

static void test_thermal(void)
{
    gk_thermo t;
    double g1, g2;

    gk_thermo_init(&t, 12e-6, 500.0, 20.0);
    t.temp = 50.0;
    GK_CHECK(near(gk_thermo_expansion(&t), 12e-6 * 500.0 * 30.0, 1e-9));
    GK_CHECK(near(gk_thermo_coupled_strain(&t, 0.0, 210e9),
                  12e-6 * 30.0, 1e-9));
    GK_CHECK(gk_spindle_growth(100.0, 60.0, 0.0) == 0.0);
    g1 = gk_spindle_growth(100.0, 60.0, 60.0);
    g2 = gk_spindle_growth(100.0, 60.0, 600.0);
    GK_CHECK(g2 > g1 && g2 < 100.0);
    GK_CHECK(gk_screw_deformation(12e-6, 1000.0, 10.0, 0.0, 1.0, 1.0) > 0.0);
}

static void test_modal(void)
{
    gk_modal m;

    gk_modal_init(&m);
    GK_CHECK_EQ_INT(gk_modal_add_mode(&m, 100.0, 0.02), GK_OK);
    GK_CHECK_EQ_INT(gk_modal_add_mode(&m, 250.0, 0.03), GK_OK);
    GK_CHECK_EQ_INT(m.count, 2);
    GK_CHECK_EQ_INT(gk_modal_add_mode(&m, 0.0, 0.0), GK_ERR_OUT_OF_RANGE);
    GK_CHECK(gk_modal_frf(&m, 100.0) > gk_modal_frf(&m, 1000.0));
    GK_CHECK(gk_modal_frf(NULL, 100.0) == 0.0);
    while (m.count < GK_PHYS_MAX_MODES) {
        gk_modal_add_mode(&m, 300.0 + m.count, 0.01);
    }
    GK_CHECK_EQ_INT(gk_modal_add_mode(&m, 999.0, 0.01), GK_ERR_OUT_OF_RANGE);
}

static void test_servo(void)
{
    gk_axis_servo s;

    gk_axis_servo_init(&s, 100.0, 500.0);
    GK_CHECK(gk_axis_servo_phase_lag(&s, 100.0) > 0.0);
    GK_CHECK(gk_axis_servo_phase_lag(NULL, 100.0) == 0.0);
    GK_CHECK_EQ_INT(gk_axis_servo_is_stable(&s), 1);
    s.resonance = 50.0;
    GK_CHECK_EQ_INT(gk_axis_servo_is_stable(&s), 0);
}

static void test_drives(void)
{
    gk_drive d;
    int i;

    for (i = 0; i < GK_DRIVE_COUNT; ++i) {
        GK_CHECK_EQ_INT(gk_drive_init(&d, (gk_drive_kind)i), GK_OK);
        GK_CHECK(d.stiffness > 0.0);
        GK_CHECK_EQ_INT(d.enabled, 1);
        GK_CHECK(strlen(gk_drive_name((gk_drive_kind)i)) > 0);
        GK_CHECK(gk_drive_stiffness(&d) > 0.0);
    }
    GK_CHECK_EQ_INT(gk_drive_init(&d, (gk_drive_kind)99), GK_ERR_INVALID_ARG);
    GK_CHECK(gk_drive_init(NULL, GK_DRIVE_PIEZO) == GK_ERR_INVALID_ARG);
    GK_CHECK(gk_drive_stiffness(NULL) == 0.0);
    GK_CHECK_STR_EQ(gk_drive_name(GK_DRIVE_PIEZO), "piezo");
}

static void test_assist(void)
{
    gk_assist a;
    int i;

    for (i = 0; i < GK_ASSIST_COUNT; ++i) {
        GK_CHECK_EQ_INT(gk_assist_init(&a, (gk_assist_kind)i), GK_OK);
        GK_CHECK_EQ_INT(a.enabled, 1);
        GK_CHECK(gk_assist_benefit(&a, 100.0) > 100.0);
        GK_CHECK(strlen(gk_assist_name((gk_assist_kind)i)) > 0);
    }
    a.enabled = 0;
    GK_CHECK(near(gk_assist_benefit(&a, 100.0), 100.0, 1e-9));
    GK_CHECK_EQ_INT(gk_assist_init(&a, (gk_assist_kind)99),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_STR_EQ(gk_assist_name(GK_ASSIST_CRYOGENIC), "cryogenic");
}

static void test_materials(void)
{
    const gk_material *m;
    int i;

    for (i = 0; i < GK_MAT_COUNT; ++i) {
        m = gk_material_get((gk_material_kind)i);
        GK_CHECK(m != NULL);
        GK_CHECK(m->density > 0.0);
        GK_CHECK(m->vc > 0.0);
        GK_CHECK(strlen(gk_material_name((gk_material_kind)i)) > 0);
    }
    GK_CHECK(gk_material_get((gk_material_kind)99) == NULL);
    GK_CHECK(gk_material_get(GK_MAT_ALUMINUM)->vc >
             gk_material_get(GK_MAT_TITANIUM)->vc);
    GK_CHECK_STR_EQ(gk_material_name(GK_MAT_COMPOSITE), "composite");
}

static void test_johnson_cook(void)
{
    gk_johnson_cook jc;

    gk_jc_init(&jc, 500.0, 1000.0, 0.5, 0.02, 1.0, 1800.0);
    GK_CHECK(gk_jc_flow_stress(&jc, 0.0, 1.0, 20.0) > 0.0);
    GK_CHECK(gk_jc_flow_stress(&jc, 0.2, 1.0, 20.0) >
             gk_jc_flow_stress(&jc, 0.0, 1.0, 20.0));
    GK_CHECK(gk_jc_flow_stress(&jc, 0.2, 1.0, 1600.0) <
             gk_jc_flow_stress(&jc, 0.2, 1.0, 20.0));
    GK_CHECK(gk_jc_flow_stress(&jc, 0.2, 1000.0, 20.0) >
             gk_jc_flow_stress(&jc, 0.2, 1.0, 20.0));
    GK_CHECK(gk_jc_flow_stress(NULL, 0.2, 1.0, 20.0) == 0.0);
}

static void test_heat(void)
{
    int i;

    for (i = 0; i < GK_HEAT_COUNT; ++i) {
        GK_CHECK(strlen(gk_heat_name((gk_heat_treatment)i)) > 0);
    }
    GK_CHECK(near(gk_hardness_at_depth(60.0, 30.0, 0.0, 2.0), 60.0, 1e-9));
    GK_CHECK(near(gk_hardness_at_depth(60.0, 30.0, 2.0, 2.0), 30.0, 1e-9));
    GK_CHECK(near(gk_hardness_at_depth(60.0, 30.0, 1.0, 2.0), 45.0, 1e-9));
    GK_CHECK(near(gk_hardness_at_depth(60.0, 30.0, 5.0, 0.0), 30.0, 1e-9));
    GK_CHECK_EQ_INT(gk_heat_affects_hardness(GK_HEAT_QUENCHED), 1);
    GK_CHECK_EQ_INT(gk_heat_affects_hardness(GK_HEAT_ANNEALED), 0);
    GK_CHECK_STR_EQ(gk_heat_name(GK_HEAT_TEMPERED), "tempered");
}

static void test_recommend(void)
{
    gk_cut_input in;
    gk_cut_params out;
    char buf[64];

    memset(&in, 0, sizeof(in));
    in.material = GK_MAT_ALUMINUM;
    in.tool_diameter = 10.0;
    in.flutes = 2;
    in.hardness = 60.0;

    memset(&out, 0, sizeof(out));
    GK_CHECK_EQ_INT(gk_recommend_speed(&in, &out), GK_OK);
    GK_CHECK(out.speed > 0.0);
    GK_CHECK_EQ_INT(gk_recommend_feed(&in, &out), GK_OK);
    GK_CHECK(out.feed > 0.0);
    GK_CHECK_EQ_INT(gk_recommend_depth(&in, &out), GK_OK);
    GK_CHECK(near(out.depth, 5.0, 1e-9));
    GK_CHECK(gk_recommend_tool(&in, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "aluminum") != NULL ||
             strstr(buf, "polished") != NULL);
    GK_CHECK(gk_recommend_cooling(&in, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "MQL") != NULL);

    in.material = GK_MAT_TITANIUM;
    in.hardness = 200.0;
    GK_CHECK(gk_recommend_tool(&in, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "TiAlN") != NULL);
    GK_CHECK(gk_recommend_cooling(&in, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "high-pressure") != NULL);

    memset(&in, 0, sizeof(in));
    in.material = GK_MAT_CARBON_STEEL;
    in.tool_diameter = 0.0;
    GK_CHECK_EQ_INT(gk_recommend_speed(&in, &out), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_recommend_speed(NULL, &out), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_recommend_feed(&in, NULL), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_recommend_depth(NULL, &out), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_recommend_tool(NULL, buf, sizeof(buf)), 0);
    GK_CHECK_EQ_INT(gk_recommend_cooling(NULL, buf, sizeof(buf)), 0);
}

int main(void)
{
    test_structure();
    test_chatter();
    test_lobe();
    test_workpiece();
    test_thermal();
    test_modal();
    test_servo();
    test_drives();
    test_assist();
    test_materials();
    test_johnson_cook();
    test_heat();
    test_recommend();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
