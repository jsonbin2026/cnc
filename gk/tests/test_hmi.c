#include "gk_test.h"
#include "gk/gk_hmi.h"

#include <math.h>
#include <string.h>

static void test_handwheel(void)
{
    gk_hmi_handwheel h;
    double click = 0.0;
    gk_hmi_handwheel_init(&h, 100);
    GK_CHECK(gk_hmi_handwheel_turn(&h, 3.6) == GK_OK);
    GK_CHECK(fabs(h.angle_deg - 3.6) < 1e-9);
    GK_CHECK_EQ_INT(gk_hmi_handwheel_detent(&h), 1);
    GK_CHECK(fabs(gk_hmi_handwheel_scale_error(&h)) < 1e-9);
    GK_CHECK_EQ_INT(gk_hmi_handwheel_scale_aligned(&h, 0.01), 1);
    gk_hmi_handwheel_turn(&h, 1.8);
    GK_CHECK(gk_hmi_handwheel_scale_error(&h) > 1.0);
    GK_CHECK_EQ_INT(gk_hmi_handwheel_scale_aligned(&h, 0.01), 0);
    GK_CHECK(gk_hmi_handwheel_gear(&h, 2, &click) == GK_OK);
    GK_CHECK(click > 40.0);
    GK_CHECK(gk_hmi_handwheel_gear(&h, 9, &click) == GK_ERR_OUT_OF_RANGE);
}

static void test_button_knob(void)
{
    gk_hmi_button b;
    gk_hmi_knob k;
    gk_hmi_button_init(&b, 4.0);
    GK_CHECK(gk_hmi_button_press(&b, 3.0) > 0.0);
    GK_CHECK_EQ_INT(b.pressed, 1);
    GK_CHECK(b.press_depth_mm <= 4.0);
    GK_CHECK(fabs(gk_hmi_button_release(&b)) < 1e-12);
    GK_CHECK_EQ_INT(b.pressed, 0);
    GK_CHECK(gk_hmi_button_backlight(&b, 0.8) == GK_OK);
    GK_CHECK(fabs(b.r_backlight - 204.0) < 1e-9);
    GK_CHECK(gk_hmi_button_backlight(&b, 2.0) == GK_ERR_OUT_OF_RANGE);
    gk_hmi_knob_init(&k, 12, 0.5);
    GK_CHECK(gk_hmi_knob_rotate(&k, 30.0) > 0.0);
    GK_CHECK_EQ_INT(gk_hmi_knob_position(&k), 1);
    GK_CHECK(gk_hmi_knob_damping_torque(&k, 10.0) > 0.0);
    GK_CHECK(gk_hmi_knob_rotate(&k, -400.0) >= 0.0);
}

static void test_display(void)
{
    gk_hmi_display d;
    gk_hmi_display_init(&d);
    GK_CHECK_EQ_INT(d.scanlines, 1080);
    GK_CHECK(fabs(d.refresh_hz - 60.0) < 1e-9);
    GK_CHECK(fabs(d.flicker) < 1e-12);
    GK_CHECK(gk_hmi_display_set_refresh(&d, 30.0) == GK_OK);
    GK_CHECK(d.flicker > 0.0);
    GK_CHECK(gk_hmi_display_set_refresh(&d, 0.0) == GK_ERR_INVALID_ARG);
    d.glare = 0.5;
    GK_CHECK(gk_hmi_display_glare(&d, 90.0) > gk_hmi_display_glare(&d, 0.0));
    d.aging = 0.2;
    GK_CHECK(fabs(gk_hmi_display_brightness(&d) - 0.8) < 1e-9);
    d.on = 0;
    GK_CHECK(fabs(gk_hmi_display_brightness(&d)) < 1e-12);
    d.on = 1;
    d.dead_pixels = 0;
    GK_CHECK_EQ_INT(gk_hmi_display_dead(&d, 10, 20), 0);
    d.dead_pixels = 5;
    {
        int dead = 0;
        int x;
        for (x = 0; x < 200; x++) {
            dead += gk_hmi_display_dead(&d, x, x * 7);
        }
        GK_CHECK(dead > 0);
    }
}

static void test_keyboard_mouse(void)
{
    gk_hmi_keyboard k;
    gk_hmi_mouse m;
    gk_hmi_keyboard_init(&k);
    GK_CHECK(gk_hmi_key_press(&k, 1.0) > k.key_volume_db);
    GK_CHECK(gk_hmi_keyboard_backlight(&k, 0.7) == GK_OK);
    GK_CHECK(fabs(k.backlight - 0.7) < 1e-9);
    GK_CHECK(gk_hmi_keyboard_backlight(&k, 5.0) == GK_ERR_OUT_OF_RANGE);
    gk_hmi_mouse_init(&m);
    GK_CHECK(fabs(gk_hmi_mouse_click(&m) - 50.0) < 1e-9);
    GK_CHECK(gk_hmi_mouse_wheel(&m, 3.0) > m.wheel_db);
}

static void test_touch_haptic(void)
{
    gk_hmi_touch t;
    gk_hmi_haptic hp;
    gk_hmi_touch_init(&t, 2.0);
    GK_CHECK(gk_hmi_touch_press(&t, 1.0) == GK_OK);
    GK_CHECK_EQ_INT(t.triggered, 0);
    GK_CHECK(fabs(gk_hmi_touch_feedback(&t)) < 1e-12);
    GK_CHECK(gk_hmi_touch_press(&t, 3.0) == GK_OK);
    GK_CHECK_EQ_INT(t.triggered, 1);
    GK_CHECK(fabs(gk_hmi_touch_feedback(&t) - 3.0) < 1e-9);
    gk_hmi_haptic_init(&hp);
    GK_CHECK(gk_hmi_haptic_pulse(&hp, 0.1, 50.0) == GK_OK);
    GK_CHECK(fabs(hp.amplitude_mm - 0.1) < 1e-9);
    GK_CHECK(gk_hmi_haptic_pulse(&hp, -1.0, 50.0) == GK_ERR_INVALID_ARG);
}

static void test_sounds(void)
{
    gk_hmi_speaker sp;
    double freq = 0.0;
    double dur = 0.0;
    GK_CHECK_STR_EQ(gk_hmi_sound_name(GK_HMI_SOUND_ERROR), "error");
    GK_CHECK(gk_hmi_sound_level(GK_HMI_SOUND_ERROR) >
             gk_hmi_sound_level(GK_HMI_SOUND_PROMPT));
    gk_hmi_speaker_init(&sp);
    GK_CHECK(gk_hmi_speaker_play(&sp, GK_HMI_SOUND_PROMPT, &freq, &dur) ==
             GK_OK);
    GK_CHECK(freq > 0.0 && dur > 0.0);
    GK_CHECK_EQ_INT((int)sp.plays, 1);
    GK_CHECK(gk_hmi_speaker_play(&sp, GK_HMI_SOUND_ERROR, &freq, &dur) ==
             GK_OK);
    GK_CHECK(freq < 500.0);
    GK_CHECK(gk_hmi_speaker_play(&sp, GK_HMI_SOUND_SUCCESS, &freq, &dur) ==
             GK_OK);
    GK_CHECK(gk_hmi_speaker_play(&sp, GK_HMI_SOUND_SUCCESS, &freq, NULL) ==
             GK_ERR_INVALID_ARG);
}

int main(void)
{
    test_handwheel();
    test_button_knob();
    test_display();
    test_keyboard_mouse();
    test_touch_haptic();
    test_sounds();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
