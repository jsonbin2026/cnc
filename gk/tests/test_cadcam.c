#include "gk_test.h"
#include "gk/gk_phys.h"
#include "gk/gk_cadcam.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void test_daily_drift(void)
{
    gk_phys_daily_drift d;
    double v0, v1;
    gk_phys_daily_init(&d, 0.01, 0.001);
    v0 = gk_phys_daily_advance(&d, 1.0, 0.0);
    v1 = gk_phys_daily_advance(&d, 1.0, 0.0);
    GK_CHECK(fabs(v0 - 0.01) < 1e-12);
    GK_CHECK(fabs(v1 - 0.02) < 1e-12);
    GK_CHECK(v1 > v0);
    /* negative days return current without advancing */
    GK_CHECK(fabs(gk_phys_daily_advance(&d, -5.0, 0.0) - 0.02) < 1e-12);
}

static void test_monthly_wear(void)
{
    gk_phys_monthly_wear w;
    gk_phys_monthly_wear_init(&w, 0.1, 100.0);
    w.parts_per_month = 100.0;
    GK_CHECK(!gk_phys_monthly_wear_expired(&w));
    GK_CHECK(gk_phys_monthly_wear_run(&w, 5) == GK_OK);
    GK_CHECK_EQ_INT(w.months, 5);
    GK_CHECK(fabs(w.vb_um - 50.0) < 1e-9);
    GK_CHECK(gk_phys_monthly_wear_run(&w, 5) == GK_OK);
    GK_CHECK(gk_phys_monthly_wear_expired(&w));
}

static void test_ageing(void)
{
    gk_phys_ageing a;
    double r_new, r_aged;
    gk_phys_ageing_init(&a, 10000.0, 0.1);
    r_new = gk_phys_ageing_reliability(&a, 1000.0);
    GK_CHECK(r_new > 0.9 && r_new < 1.0);
    GK_CHECK(gk_phys_ageing_advance(&a, 5.0) == GK_OK);
    r_aged = gk_phys_ageing_reliability(&a, 1000.0);
    GK_CHECK(r_aged < r_new);
    GK_CHECK(gk_phys_ageing_reliability(&a, 0.0) > 0.0);
}

static void test_life(void)
{
    gk_phys_life l;
    gk_phys_life_init(&l, 10.0, 4000.0);
    GK_CHECK(fabs(gk_phys_life_remaining_years(&l) - 10.0) < 1e-12);
    GK_CHECK(gk_phys_life_advance(&l, 4.0) == GK_OK);
    GK_CHECK(fabs(l.hours_used - 16000.0) < 1e-9);
    GK_CHECK(fabs(gk_phys_life_remaining_years(&l) - 6.0) < 1e-12);
    GK_CHECK(!gk_phys_life_expired(&l));
    GK_CHECK(gk_phys_life_advance(&l, 7.0) == GK_OK);
    GK_CHECK(gk_phys_life_expired(&l));
    GK_CHECK(fabs(gk_phys_life_remaining_years(&l)) < 1e-12);
}

static void test_history(void)
{
    gk_phys_history h;
    gk_phys_history_frame f;
    const gk_phys_history_frame *got;
    int i;
    gk_phys_history_init(&h);
    for (i = 0; i < 5; i++) {
        memset(&f, 0, sizeof(f));
        f.time_s = (double)i;
        f.tool_id = i;
        f.x = (double)i * 10.0;
        GK_CHECK_EQ_INT(gk_phys_history_record(&h, &f), i + 1);
    }
    GK_CHECK_EQ_INT(h.count, 5);
    GK_CHECK(fabs(gk_phys_history_total_time(&h) - 4.0) < 1e-12);
    got = gk_phys_history_next(&h);
    GK_CHECK(got != NULL);
    GK_CHECK_EQ_INT(got->tool_id, 0);
    GK_CHECK(gk_phys_history_seek(&h, 3) == GK_OK);
    got = gk_phys_history_next(&h);
    GK_CHECK_EQ_INT(got->tool_id, 3);
    GK_CHECK(gk_phys_history_seek(&h, 99) == GK_ERR_OUT_OF_RANGE);
}

static void test_field_basic(void)
{
    gk_phys_field f;
    GK_CHECK(gk_phys_field_init(&f, GK_PHYS_THERMAL, 8, 8) == GK_OK);
    GK_CHECK(gk_phys_field_set(&f, 2, 3, 5.0) == GK_OK);
    GK_CHECK(fabs(gk_phys_field_get(&f, 2, 3) - 5.0) < 1e-12);
    GK_CHECK(gk_phys_field_set(&f, 99, 0, 1.0) == GK_ERR_OUT_OF_RANGE);
    GK_CHECK(gk_phys_field_get(&f, -1, 0) == 0.0);
    GK_CHECK(gk_phys_field_source(&f, 3.5, 3.5, 10.0, 1.5) == GK_OK);
    GK_CHECK(gk_phys_field_mean(&f) > 0.0);
    GK_CHECK(f.max_value > 0.0);
    GK_CHECK(gk_phys_field_update_stats(&f) == GK_OK);
}

static void test_field_names(void)
{
    int k;
    for (k = 0; k < GK_PHYS_FIELD_COUNT; k++) {
        GK_CHECK(gk_phys_field_name((gk_phys_field_kind)k) != NULL);
    }
    GK_CHECK_STR_EQ(gk_phys_field_name(GK_PHYS_FORCE), "force");
    GK_CHECK_STR_EQ(gk_phys_field_name(GK_PHYS_FLOW), "flow");
}

static void test_coupling(void)
{
    gk_phys_coupling c;
    gk_phys_field a, b;
    int ia, ib, dom;
    gk_phys_coupling_init(&c);
    GK_CHECK(gk_phys_field_init(&a, GK_PHYS_FORCE, 8, 8) == GK_OK);
    GK_CHECK(gk_phys_field_init(&b, GK_PHYS_THERMAL, 8, 8) == GK_OK);
    GK_CHECK(gk_phys_field_source(&a, 4.0, 4.0, 10.0, 2.0) == GK_OK);
    ia = gk_phys_coupling_add(&c, &a);
    ib = gk_phys_coupling_add(&c, &b);
    GK_CHECK_EQ_INT(ia, 1);
    GK_CHECK_EQ_INT(ib, 2);
    GK_CHECK(gk_phys_coupling_set(&c, 0, 1, 5.0) == GK_OK);
    GK_CHECK(gk_phys_coupling_set_strength(&c, 2.0) == GK_OK);
    GK_CHECK(gk_phys_coupling_show_single(&c, 1) == GK_OK);
    GK_CHECK_EQ_INT(c.fields[0].visible, 0);
    GK_CHECK_EQ_INT(c.fields[1].visible, 1);
    GK_CHECK(gk_phys_coupling_overlay_all(&c) == GK_OK);
    GK_CHECK_EQ_INT(c.fields[0].visible, 1);
    GK_CHECK(gk_phys_coupling_step(&c, 0, 1) > 0.0);
    dom = gk_phys_coupling_dominant(&c);
    GK_CHECK(dom == 1);
    GK_CHECK(gk_phys_coupling_set(&c, 0, 1, 0.5) == GK_OK);
    GK_CHECK(gk_phys_coupling_show_single(&c, 9) == GK_ERR_OUT_OF_RANGE);
}

static void test_cad_sketch(void)
{
    gk_cad_sketch s;
    double len;
    gk_cad_sketch_init(&s);
    GK_CHECK_EQ_INT(gk_cad_add_line(&s, 0, 0, 3, 4), 1);
    len = gk_cad_sketch_length(&s);
    GK_CHECK(fabs(len - 5.0) < 1e-12);
    GK_CHECK_EQ_INT(gk_cad_add_arc(&s, 0, 0, 2.0, 0), 2);
    GK_CHECK(gk_cad_add_fillet(&s, 1.0) == GK_OK);
    GK_CHECK(gk_cad_add_chamfer(&s, 1.0) == GK_OK);
    GK_CHECK(gk_cad_add_fillet(&s, -1.0) == GK_ERR_INVALID_ARG);
    GK_CHECK(gk_cad_sketch_length(&s) > 5.0);
    GK_CHECK_EQ_INT(gk_cad_add_arc(&s, 0, 0, -2.0, 0), -1);
}

static void test_cad_solid(void)
{
    gk_cad_solid s;
    gk_cad_box box;
    double vol = 0.0;
    gk_cad_solid_init(&s);
    s.profile_count = 1;
    s.profile[0].x1 = 0;
    s.profile[0].y1 = 0;
    s.profile[0].x2 = 10;
    s.profile[0].y2 = 20;
    GK_CHECK(gk_cad_extrude(&s, 5.0, &box) == GK_OK);
    GK_CHECK(fabs(box.x2 - 10.0) < 1e-12);
    GK_CHECK(fabs(gk_cad_box_volume(&box) - 1000.0) < 1e-9);
    GK_CHECK(gk_cad_revolve(&s, 360.0, 3.0, &vol) == GK_OK);
    GK_CHECK(fabs(vol - M_PI * 9.0 * 5.0) < 1e-6);
    GK_CHECK(gk_cad_revolve(&s, 720.0, 3.0, &vol) == GK_ERR_OUT_OF_RANGE);
}

static void test_cad_boolean(void)
{
    gk_cad_box a = {0, 0, 0, 10, 10, 10};
    gk_cad_box b = {5, 5, 5, 15, 15, 15};
    gk_cad_box u, it;
    GK_CHECK(gk_cad_boolean(&a, &b, GK_CAD_BOOL_UNION, &u) == GK_OK);
    GK_CHECK(fabs(u.x1 - 0.0) < 1e-12 && fabs(u.x2 - 15.0) < 1e-12);
    GK_CHECK(fabs(gk_cad_box_volume(&u) - 3375.0) < 1e-9);
    GK_CHECK(gk_cad_boolean(&a, &b, GK_CAD_BOOL_INTERSECT, &it) == GK_OK);
    GK_CHECK(fabs(gk_cad_box_volume(&it) - 125.0) < 1e-9);
}

static void test_cam(void)
{
    gk_cam_job j;
    gk_cam_op op;
    double t;
    memset(&op, 0, sizeof(op));
    op.kind = GK_CAM_POCKET;
    op.x = 10;
    op.y = 20;
    op.z_top = 0;
    op.z_bottom = -5;
    op.tool_diameter = 6;
    op.feed = 300;
    gk_cam_job_init(&j);
    GK_CHECK_EQ_INT(gk_cam_add_op(&j, &op), 1);
    t = gk_cam_op_time(&op);
    GK_CHECK(t > 0.0);
    GK_CHECK(gk_cam_generate(&j) == GK_OK);
    GK_CHECK(strstr(j.program, "POCKET") != NULL);
    GK_CHECK(strstr(j.program, "M30") != NULL);
    GK_CHECK_STR_EQ(gk_cam_op_name(GK_CAM_DRILL), "drill");
}

static void test_cam_verify(void)
{
    const char *prog = "G21 G90\nG0 X0 Y0\nG1 Z-5 F100\nG1 X10 Y10\nG0 Z0\nM30\n";
    gk_cam_bounds bounds;
    gk_cam_verify v;
    GK_CHECK(gk_cam_verify_run(prog, &bounds, &v) == GK_OK);
    GK_CHECK(fabs(bounds.max_x - 10.0) < 1e-9);
    GK_CHECK(fabs(bounds.min_z + 5.0) < 1e-9);
    GK_CHECK(v.rapid_distance > 0.0);
    GK_CHECK(v.cutting_distance > 0.0);
}

static void test_cam_post(void)
{
    char out[128];
    GK_CHECK(gk_cam_post_transform(GK_CAM_POST_FANUC, "G1 X0\n", out,
                                   sizeof(out)) == GK_OK);
    GK_CHECK_STR_EQ(out, "G1 X0\n");
    GK_CHECK(gk_cam_post_transform(GK_CAM_POST_SIEMENS, "G1 X0\n", out,
                                   sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "SIEMENS") != NULL);
    GK_CHECK_STR_EQ(gk_cam_post_name(GK_CAM_POST_HAAS), "haas");
}

static void test_drawing(void)
{
    gk_cad_drawing d;
    gk_cam_bounds b = {0, 0, 0, 10, 20, 30};
    char out[256];
    gk_cad_drawing_init(&d, "PART-A");
    GK_CHECK_STR_EQ(d.title, "PART-A");
    GK_CHECK(gk_cad_drawing_generate(&d, &b, out, sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "PART-A") != NULL);
    GK_CHECK(strstr(out, "third") != NULL);
}

static void test_import(void)
{
    int n = 0;
    GK_CHECK(gk_cad_import_detect("part.step") == GK_CAD_IMPORT_STEP);
    GK_CHECK(gk_cad_import_detect("a.DXF") == GK_CAD_IMPORT_DXF);
    GK_CHECK(gk_cad_import_detect("m.stl") == GK_CAD_IMPORT_STL);
    GK_CHECK(gk_cad_import_detect("x.obj") == GK_CAD_IMPORT_OBJ);
    GK_CHECK(gk_cad_import_detect("y.3mf") == GK_CAD_IMPORT_3MF);
    GK_CHECK(gk_cad_import_detect("z.x_t") == GK_CAD_IMPORT_PARASOLID);
    GK_CHECK(gk_cad_import_detect("nope") == GK_CAD_IMPORT_COUNT);
    GK_CHECK(gk_cad_import_probe("solid s\nfacet normal\nfacet normal\n",
                                 GK_CAD_IMPORT_STL, &n) == GK_OK);
    GK_CHECK_EQ_INT(n, 2);
    GK_CHECK_STR_EQ(gk_cad_import_name(GK_CAD_IMPORT_IGES), "iges");
}

static void test_platform(void)
{
    gk_cad_platform p = gk_cad_platform_current();
    GK_CHECK(gk_cad_platform_supported(p));
    GK_CHECK_STR_EQ(gk_cad_platform_name(GK_CAD_PLATFORM_WINDOWS), "windows");
    GK_CHECK_STR_EQ(gk_cad_platform_name(GK_CAD_PLATFORM_LINUX), "linux");
    GK_CHECK_STR_EQ(gk_cad_platform_name(GK_CAD_PLATFORM_MACOS), "macos");
}

static int mock_pass(void)
{
    return 0;
}

static int mock_fail(void)
{
    return 1;
}

static void test_suite(void)
{
    gk_cad_test_suite t;
    gk_cad_test_suite_init(&t);
    GK_CHECK_EQ_INT(gk_cad_test_add(&t, "pass", mock_pass), 1);
    GK_CHECK_EQ_INT(gk_cad_test_add(&t, "fail", mock_fail), 2);
    GK_CHECK_EQ_INT(gk_cad_test_run(&t), 1);
    GK_CHECK_EQ_INT(t.passed, 1);
    GK_CHECK_EQ_INT(t.failed, 1);
    GK_CHECK_STR_EQ(t.cases[0].name, "pass");
}

int main(void)
{
    test_daily_drift();
    test_monthly_wear();
    test_ageing();
    test_life();
    test_history();
    test_field_basic();
    test_field_names();
    test_coupling();
    test_cad_sketch();
    test_cad_solid();
    test_cad_boolean();
    test_cam();
    test_cam_verify();
    test_cam_post();
    test_drawing();
    test_import();
    test_platform();
    test_suite();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
