#include "gk_test.h"
#include "gk/gk_envd.h"

#include <math.h>
#include <string.h>

static void test_shop(void)
{
    gk_envd_shop s;
    gk_envd_shop_init(&s, 50.0, 30.0, 8.0);
    GK_CHECK(fabs(gk_envd_shop_volume(&s) - 12000.0) < 1e-6);
    GK_CHECK(fabs(gk_envd_shop_floor_area(&s) - 1500.0) < 1e-6);
    GK_CHECK(fabs(s.ambient_c - 25.0) < 1e-9);
}

static void test_sounds(void)
{
    gk_envd_sound s;
    gk_envd_sound near;
    gk_envd_sound far;
    GK_CHECK_STR_EQ(gk_envd_sound_name(GK_ENVD_SND_MACHINE), "machine");
    GK_CHECK(gk_envd_sound_base_db(GK_ENVD_SND_MACHINE) >
             gk_envd_sound_base_db(GK_ENVD_SND_CABINET_FAN));
    gk_envd_sound_init(&near, GK_ENVD_SND_MACHINE, 1.0);
    gk_envd_sound_init(&far, GK_ENVD_SND_MACHINE, 10.0);
    GK_CHECK(gk_envd_sound_level(&near) > gk_envd_sound_level(&far));
    far.enabled = 0;
    GK_CHECK(fabs(gk_envd_sound_level(&far)) < 1e-12);
    gk_envd_sound_init(&s, GK_ENVD_SND_COMPRESSOR, 2.0);
    GK_CHECK(gk_envd_sound_level(&s) > 0.0);
}

static void test_floor(void)
{
    gk_envd_floor f;
    gk_envd_floor_init(&f);
    GK_CHECK_STR_EQ(gk_envd_floor_name(f.state), "clean");
    GK_CHECK_EQ_INT(gk_envd_floor_slip_risk(&f, 0.5), 0);
    GK_CHECK(gk_envd_floor_soil(&f, GK_ENVD_FLOOR_OIL, 1.0) == GK_OK);
    GK_CHECK(f.friction < 0.7);
    GK_CHECK_EQ_INT(gk_envd_floor_slip_risk(&f, 0.5), 1);
    GK_CHECK(gk_envd_floor_soil(&f, GK_ENVD_FLOOR_WATER, 2.0) ==
             GK_ERR_OUT_OF_RANGE);
}

static void test_sticker_aisle(void)
{
    char out[128];
    gk_envd_aisle a;
    gk_envd_fire_equipment fe;
    GK_CHECK(gk_envd_wall_sticker("SAFETY FIRST", out, sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "SAFETY FIRST") != NULL);
    gk_envd_aisle_init(&a, 1000.0);
    GK_CHECK_EQ_INT(gk_envd_aisle_ok(&a), 1);
    GK_CHECK(gk_envd_aisle_block(&a, 1) == GK_OK);
    GK_CHECK_EQ_INT(gk_envd_aisle_ok(&a), 0);
    gk_envd_aisle_init(&a, 500.0);
    GK_CHECK_EQ_INT(gk_envd_aisle_ok(&a), 0);
    gk_envd_fire_init(&fe, "extinguisher", 4);
    GK_CHECK_EQ_INT(fe.count, 4);
    GK_CHECK_STR_EQ(fe.kind, "extinguisher");
    GK_CHECK(gk_envd_fire_inspect(&fe, 1) == GK_OK);
    GK_CHECK_EQ_INT(fe.inspected, 1);
    GK_CHECK(gk_envd_fire_inspect(&fe, 0) == GK_ERR_STATE);
}

static void test_storage(void)
{
    gk_envd_store s;
    GK_CHECK_STR_EQ(gk_envd_store_name(GK_ENVD_STORE_WASTE), "waste-bin");
    gk_envd_store_init(&s, GK_ENVD_STORE_MATERIAL, 100);
    GK_CHECK_EQ_INT(gk_envd_store_count(&s), 0);
    GK_CHECK(gk_envd_store_put(&s, 60) == GK_OK);
    GK_CHECK_EQ_INT(gk_envd_store_count(&s), 60);
    GK_CHECK(gk_envd_store_put(&s, 50) == GK_ERR_OUT_OF_RANGE);
    GK_CHECK(gk_envd_store_take(&s, 10) == GK_OK);
    GK_CHECK_EQ_INT(gk_envd_store_count(&s), 50);
    GK_CHECK(gk_envd_store_take(&s, 100) == GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_envd_store_full(&s), 0);
    GK_CHECK(gk_envd_store_put(&s, 50) == GK_OK);
    GK_CHECK_EQ_INT(gk_envd_store_full(&s), 1);
}

static void test_lighting(void)
{
    gk_envd_lighting l;
    gk_envd_emergency_light e;
    gk_envd_lighting_init(&l, 20, 40.0);
    GK_CHECK(fabs(gk_envd_lighting_power(&l) - 800.0) < 1e-9);
    l.failed = 2;
    GK_CHECK(fabs(gk_envd_lighting_power(&l) - 720.0) < 1e-9);
    GK_CHECK(gk_envd_lighting_set_level(&l, 0.5) == GK_OK);
    GK_CHECK(fabs(gk_envd_lighting_power(&l) - 360.0) < 1e-9);
    GK_CHECK(gk_envd_lighting_set_level(&l, 2.0) == GK_ERR_OUT_OF_RANGE);
    gk_envd_emergency_light_init(&e);
    GK_CHECK_EQ_INT(gk_envd_emergency_light_ok(&e), 0);
    GK_CHECK(gk_envd_emergency_light_test(&e, 120.0) == GK_OK);
    GK_CHECK_EQ_INT(gk_envd_emergency_light_ok(&e), 1);
    GK_CHECK(gk_envd_emergency_light_test(&e, 30.0) == GK_OK);
    GK_CHECK_EQ_INT(gk_envd_emergency_light_ok(&e), 0);
}

static void test_window_clock(void)
{
    gk_envd_window w;
    gk_envd_clock c;
    char out[64];
    gk_envd_window_init(&w, 1200.0, 800.0);
    GK_CHECK(fabs(gk_envd_window_area(&w) - 960000.0) < 1e-3);
    GK_CHECK(gk_envd_window_view("parking lot", out, sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "parking lot") != NULL);
    gk_envd_clock_init(&c);
    GK_CHECK(gk_envd_clock_set(&c, 14, 5, 9) == GK_OK);
    GK_CHECK(gk_envd_clock_render(&c, out, sizeof(out)) == GK_OK);
    GK_CHECK_STR_EQ(out, "14:05:09");
    GK_CHECK(gk_envd_clock_set(&c, 25, 0, 0) == GK_ERR_OUT_OF_RANGE);
}

static void test_board_handover(void)
{
    gk_envd_board b;
    gk_envd_handover h;
    gk_envd_board_init(&b, 100);
    GK_CHECK(fabs(gk_envd_board_achievement(&b)) < 1e-12);
    GK_CHECK(gk_envd_board_update(&b, 100, 80) == GK_OK);
    GK_CHECK(fabs(gk_envd_board_achievement(&b) - 0.8) < 1e-9);
    GK_CHECK(gk_envd_board_update(&b, -1, 0) == GK_ERR_INVALID_ARG);
    gk_envd_handover_init(&h);
    GK_CHECK(gk_envd_handover_sign(&h) == GK_ERR_STATE);
    GK_CHECK(gk_envd_handover_set(&h, "Alice", "Bob", "line stable") == GK_OK);
    GK_CHECK_STR_EQ(h.outgoing, "Alice");
    GK_CHECK(gk_envd_handover_sign(&h) == GK_OK);
    GK_CHECK_EQ_INT(h.signed_off, 1);
}

int main(void)
{
    test_shop();
    test_sounds();
    test_floor();
    test_sticker_aisle();
    test_storage();
    test_lighting();
    test_window_clock();
    test_board_handover();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
