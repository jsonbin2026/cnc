#include "gk_test.h"
#include "gk/gk_app.h"

#include <math.h>
#include <string.h>

static void test_texture_seam_screw(void)
{
    gk_app_texture t;
    gk_app_seam s;
    gk_app_screw sc;
    double h1, h2;
    gk_app_texture_init(&t, 6.4, 0.8);
    h1 = gk_app_texture_height(&t, 0.1, 0.2);
    h2 = gk_app_texture_height(&t, 0.1, 0.2);
    GK_CHECK(fabs(h1 - h2) < 1e-12);
    GK_CHECK(fabs(h1) <= 6.4);
    gk_app_seam_init(&s, 1.0, 0.5);
    GK_CHECK_EQ_INT(s.visible, 1);
    GK_CHECK(fabs(gk_app_seam_area(&s, 100.0) - 200.0) < 1e-9);
    gk_app_screw_init(&sc, GK_APP_SCREW_SOCKET, 6.0, 20.0);
    sc.count = 8;
    GK_CHECK_STR_EQ(gk_app_screw_name(sc.head), "socket");
    GK_CHECK(fabs(gk_app_screw_total_length(&sc) - 160.0) < 1e-9);
}

static void test_plates_markings(void)
{
    gk_app_nameplate p;
    char out[128];
    gk_app_nameplate_init(&p, 100.0, 50.0, "aluminium");
    GK_CHECK(fabs(gk_app_nameplate_area(&p) - 5000.0) < 1e-9);
    GK_CHECK_STR_EQ(p.material, "aluminium");
    GK_CHECK_EQ_INT(p.rivets, 4);
    GK_CHECK(gk_app_serial_render("ABC123", out, sizeof(out)) == GK_OK);
    GK_CHECK_STR_EQ(out, "SN:ABC123");
    GK_CHECK_STR_EQ(gk_app_warning_text(GK_APP_WARN_HOT),
                    "WARNING - HOT SURFACE");
    GK_CHECK(gk_app_warning_label(GK_APP_WARN_ELECTRICAL, out, sizeof(out)) ==
             GK_OK);
    GK_CHECK(strstr(out, "ELECTRICAL") != NULL);
    GK_CHECK(gk_app_model_render("VMC", 850, out, sizeof(out)) == GK_OK);
    GK_CHECK_STR_EQ(out, "VMC-0850");
    GK_CHECK(gk_app_date_render(2026, 3, 9, out, sizeof(out)) == GK_OK);
    GK_CHECK_STR_EQ(out, "2026-03-09");
    GK_CHECK(gk_app_date_render(2026, 13, 9, out, sizeof(out)) ==
             GK_ERR_INVALID_ARG);
}

static void test_sticker_logo(void)
{
    gk_app_sticker s;
    gk_app_logo l;
    char out[128];
    gk_app_sticker_init(&s);
    GK_CHECK_EQ_INT(gk_app_sticker_count(&s), 0);
    GK_CHECK(gk_app_sticker_add(&s, "1. Power on") == GK_OK);
    GK_CHECK(gk_app_sticker_add(&s, "2. Home all axes") == GK_OK);
    GK_CHECK_EQ_INT(gk_app_sticker_count(&s), 2);
    GK_CHECK(strstr(s.text, "Home all axes") != NULL);
    gk_app_logo_init(&l, "GK");
    GK_CHECK(gk_app_logo_render(&l, out, sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "GK") != NULL);
}

static void test_covers(void)
{
    gk_app_cover c;
    gk_app_dragchain d;
    gk_app_cover_init(&c, GK_APP_COVER_BELLOWS, 400.0, 50.0);
    GK_CHECK_STR_EQ(gk_app_cover_name(c.kind), "bellows");
    GK_CHECK(fabs(gk_app_cover_extended(&c) - 450.0) < 1e-9);
    GK_CHECK_EQ_INT(gk_app_cover_folds(&c), 80);
    gk_app_dragchain_init(&d, 20.0, 30.0);
    GK_CHECK(gk_app_dragchain_links(&d, 1000.0) > 0);
    GK_CHECK(gk_app_dragchain_length(&d, 1000.0) >= 1000.0 / 2.0);
}

static void test_routing(void)
{
    gk_app_route r;
    gk_app_route_init(&r, GK_APP_ROUTE_CABLE, 5.0, 1000.0);
    GK_CHECK_STR_EQ(gk_app_route_name(r.kind), "cable");
    GK_CHECK(fabs(r.bend_radius_mm - 15.0) < 1e-9);
    GK_CHECK(gk_app_route_total(&r) > 1000.0);
}

static void test_hydraulic_pneumatic_chiller(void)
{
    gk_app_hydraulic h;
    gk_app_pneumatic p;
    gk_app_chiller c;
    gk_app_hydraulic_init(&h, 10.0, 60.0, 100.0);
    GK_CHECK(gk_app_hydraulic_power_kw(&h) > 0.0);
    h.running = 0;
    GK_CHECK(fabs(gk_app_hydraulic_power_kw(&h)) < 1e-12);
    gk_app_pneumatic_init(&p, 0.6);
    GK_CHECK(gk_app_pneumatic_ok(&p));
    p.regulator_ok = 0;
    GK_CHECK(!gk_app_pneumatic_ok(&p));
    gk_app_chiller_init(&c, 5.0, 20.0);
    GK_CHECK(fabs(gk_app_chiller_error(&c)) < 1e-12);
    c.actual_c = 22.5;
    GK_CHECK(fabs(gk_app_chiller_error(&c) - 2.5) < 1e-9);
}

static void test_cabinet(void)
{
    gk_app_cabinet c;
    gk_app_cabinet_init(&c, 600.0, 2000.0);
    GK_CHECK_EQ_INT(gk_app_cabinet_fan_needed(&c, 45.0, 25.0), 1);
    GK_CHECK_EQ_INT(gk_app_cabinet_fan_needed(&c, 30.0, 25.0), 0);
    GK_CHECK(gk_app_cabinet_door(&c, 1) == GK_OK);
    GK_CHECK_EQ_INT(gk_app_cabinet_fan_needed(&c, 60.0, 25.0), 0);
    GK_CHECK(gk_app_cabinet_vent_ratio(&c) > 0.0);
}

static void test_cooling(void)
{
    gk_app_cool c;
    gk_app_cool_init(&c, GK_APP_COOL_NOZZLE, 4.0);
    GK_CHECK_STR_EQ(gk_app_cool_name(c.kind), "universal-nozzle");
    GK_CHECK(gk_app_cool_velocity(&c) > 0.0);
    GK_CHECK(gk_app_cool_aim(&c, 45.0) == GK_OK);
    GK_CHECK(fabs(c.angle_deg - 45.0) < 1e-9);
    GK_CHECK(gk_app_cool_aim(&c, 400.0) == GK_ERR_OUT_OF_RANGE);
}

static void test_lights(void)
{
    gk_app_light l;
    gk_app_stack s;
    gk_app_light_init(&l, 1000.0, 5000.0);
    GK_CHECK(fabs(gk_app_light_illuminance(&l, 1.0)) < 1e-9);
    l.on = 1;
    GK_CHECK(gk_app_light_illuminance(&l, 2.0) > 0.0);
    gk_app_stack_init(&s);
    GK_CHECK_STR_EQ(gk_app_stack_name(s.state), "off");
    GK_CHECK(gk_app_stack_set(&s, GK_APP_STACK_RED) == GK_OK);
    GK_CHECK_EQ_INT(s.buzzer, 1);
    GK_CHECK(gk_app_stack_set(&s, GK_APP_STACK_GREEN) == GK_OK);
    GK_CHECK_EQ_INT(s.buzzer, 0);
    GK_CHECK(gk_app_stack_from_alarm(3) == GK_APP_STACK_RED);
    GK_CHECK(gk_app_stack_from_alarm(2) == GK_APP_STACK_AMBER);
    GK_CHECK(gk_app_stack_from_alarm(1) == GK_APP_STACK_GREEN);
    GK_CHECK(gk_app_stack_from_alarm(0) == GK_APP_STACK_OFF);
}

int main(void)
{
    test_texture_seam_screw();
    test_plates_markings();
    test_sticker_logo();
    test_covers();
    test_routing();
    test_hydraulic_pneumatic_chiller();
    test_cabinet();
    test_cooling();
    test_lights();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
