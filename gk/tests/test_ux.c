#include "gk_test.h"
#include "gk/gk_ux.h"

#include <math.h>

static int near(double a, double b, double tol)
{
    return fabs(a - b) <= tol;
}

static void test_devices(void)
{
    gk_ux_thermal_glove g;
    gk_ux_scent s;
    gk_ux_foot_switch f;
    gk_ux_print_head p;
    gk_ux_haptic_seat seat;

    gk_ux_glove_init(&g);
    GK_CHECK(near(g.current_temp, 25.0, 1e-6));
    GK_CHECK_EQ_INT(gk_ux_glove_set(&g, 40.0f), GK_OK);
    GK_CHECK_EQ_INT(g.active, 1);
    GK_CHECK_EQ_INT(gk_ux_glove_set(&g, 100.0f), GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_ux_glove_set(&g, -1.0f), GK_ERR_OUT_OF_RANGE);
    /* heat toward a lower target of 30 deg at 4 deg/s */
    (void)gk_ux_glove_set(&g, 30.0f);
    g.current_temp = 34.0f;
    GK_CHECK_EQ_INT(gk_ux_glove_update(&g, 2.0f), GK_OK);
    GK_CHECK(near(g.current_temp, 30.0, 1e-6));
    GK_CHECK_EQ_INT(g.active, 0);
    GK_CHECK_EQ_INT(gk_ux_glove_update(NULL, 1.0f), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_ux_glove_set(NULL, 30.0f), GK_ERR_INVALID_ARG);
    gk_ux_glove_init(NULL);

    gk_ux_scent_init(&s);
    GK_CHECK_EQ_INT(gk_ux_scent_active(&s), 0);
    GK_CHECK_EQ_INT(gk_ux_scent_emit(&s, 3, 2.0f, 0.5f), GK_OK);
    GK_CHECK(near(s.intensity, 1.0, 1e-6)); /* clamped */
    GK_CHECK_EQ_INT(s.cartridge, 3);
    GK_CHECK_EQ_INT(gk_ux_scent_active(&s), 1);
    GK_CHECK_EQ_INT(gk_ux_scent_update(&s, 0.6f), GK_OK);
    GK_CHECK_EQ_INT(gk_ux_scent_active(&s), 0);
    GK_CHECK_EQ_INT(gk_ux_scent_update(&s, 0.1f), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_ux_scent_emit(&s, -1, 1.0f, 1.0f),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_ux_scent_emit(&s, 1, 1.0f, 0.0f),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_ux_scent_active(NULL), 0);
    gk_ux_scent_init(NULL);

    gk_ux_foot_switch_init(&f);
    GK_CHECK_EQ_INT(gk_ux_foot_switch_down(&f, 0), 0);
    GK_CHECK_EQ_INT(gk_ux_foot_switch_set(&f, 0, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_ux_foot_switch_down(&f, 0), 1);
    GK_CHECK_EQ_INT(f.pressed, 1);
    GK_CHECK_EQ_INT(gk_ux_foot_switch_set(&f, 1, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_ux_foot_switch_set(&f, 0, 0), GK_OK);
    GK_CHECK_EQ_INT(gk_ux_foot_switch_down(&f, 0), 0);
    GK_CHECK_EQ_INT(f.pressed, 1); /* pedal 1 still held */
    GK_CHECK_EQ_INT(gk_ux_foot_switch_set(&f, 1, 0), GK_OK);
    GK_CHECK_EQ_INT(f.pressed, 0);
    GK_CHECK_EQ_INT(gk_ux_foot_switch_set(&f, 8, 1), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_ux_foot_switch_down(&f, -1), 0);
    GK_CHECK_EQ_INT(gk_ux_foot_switch_set(NULL, 0, 1), GK_ERR_INVALID_ARG);
    gk_ux_foot_switch_init(NULL);

    gk_ux_print_head_init(&p);
    GK_CHECK_EQ_INT(gk_ux_print_head_extrude(&p, 50.0f, 2.0f), GK_OK);
    GK_CHECK(near(p.filament_used_mm, 100.0, 1e-5));
    GK_CHECK_EQ_INT(p.extruding, 1);
    GK_CHECK_EQ_INT(gk_ux_print_head_extrude(&p, 0.0f, 1.0f), GK_OK);
    GK_CHECK_EQ_INT(p.extruding, 0);
    GK_CHECK_EQ_INT(gk_ux_print_head_extrude(&p, 500.0f, 1.0f),
                    GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_ux_print_head_extrude(NULL, 1.0f, 1.0f),
                    GK_ERR_INVALID_ARG);
    gk_ux_print_head_init(NULL);

    gk_ux_haptic_seat_init(&seat, 4);
    GK_CHECK_EQ_INT(seat.channels, 4);
    GK_CHECK_EQ_INT(gk_ux_haptic_seat_pulse(&seat, 0.8f, 40.0f), GK_OK);
    GK_CHECK_EQ_INT(seat.active, 1);
    GK_CHECK_EQ_INT(gk_ux_haptic_seat_pulse(&seat, 1.0f, 0.0f),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_ux_haptic_seat_stop(&seat), GK_OK);
    GK_CHECK_EQ_INT(seat.active, 0);
    GK_CHECK_EQ_INT(gk_ux_haptic_seat_stop(NULL), GK_ERR_INVALID_ARG);
    gk_ux_haptic_seat_init(&seat, 0);
    GK_CHECK_EQ_INT(seat.channels, 1);
    gk_ux_haptic_seat_init(NULL, 1);
}

static void test_sonification(void)
{
    gk_ux_sonifier s;
    unsigned rgb;
    float f, a;

    GK_CHECK(near(gk_ux_normalize(5.0f, 0.0f, 10.0f), 0.5, 1e-6));
    GK_CHECK(near(gk_ux_normalize(-5.0f, 0.0f, 10.0f), 0.0, 1e-6));
    GK_CHECK(near(gk_ux_normalize(50.0f, 0.0f, 10.0f), 1.0, 1e-6));
    GK_CHECK(near(gk_ux_normalize(1.0f, 5.0f, 5.0f), 0.0, 1e-6));

    /* pitch is non-decreasing with load (quantised to a pentatonic scale) */
    {
        float prev = gk_ux_load_to_pitch(0.0f);
        int i;
        for (i = 1; i <= 10; i++) {
            float cur = gk_ux_load_to_pitch((float)i / 10.0f);
            GK_CHECK(cur >= prev);
            prev = cur;
        }
        /* full-range endpoints differ */
        GK_CHECK(gk_ux_load_to_pitch(1.0f) > gk_ux_load_to_pitch(0.0f));
    }
    GK_CHECK(gk_ux_load_to_pitch(0.0f) > 0.0f);

    /* temperature color: cold is blue-dominant, hot is red-dominant */
    GK_CHECK_EQ_INT(gk_ux_temp_to_color(20.0f, 20.0f, 120.0f, &rgb), GK_OK);
    GK_CHECK(((rgb >> 16) & 0xFF) < (rgb & 0xFF));
    GK_CHECK_EQ_INT(gk_ux_temp_to_color(120.0f, 20.0f, 120.0f, &rgb), GK_OK);
    GK_CHECK(((rgb >> 16) & 0xFF) > (rgb & 0xFF));
    GK_CHECK_EQ_INT(gk_ux_temp_to_color(0.0f, 20.0f, 20.0f, &rgb),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_ux_temp_to_color(0.0f, 20.0f, 120.0f, NULL),
                    GK_ERR_INVALID_ARG);

    /* texture haptics */
    GK_CHECK_EQ_INT(gk_ux_texture_to_haptic(1.0f, &f, &a), GK_OK);
    GK_CHECK(f > 0.0f && a > 0.0f);
    {
        float f2, a2;
        gk_ux_texture_to_haptic(9.0f, &f2, &a2);
        GK_CHECK(f2 > f && a2 > a);
    }
    GK_CHECK_EQ_INT(gk_ux_texture_to_haptic(1.0f, NULL, &a),
                    GK_ERR_INVALID_ARG);

    /* gcode letter to note */
    GK_CHECK_EQ_INT(gk_ux_gcode_to_note('X'), 0);
    GK_CHECK_EQ_INT(gk_ux_gcode_to_note('x'), 0);
    GK_CHECK_EQ_INT(gk_ux_gcode_to_note('Y'), 2);
    GK_CHECK_EQ_INT(gk_ux_gcode_to_note('Z'), 4);
    GK_CHECK_EQ_INT(gk_ux_gcode_to_note('?'), -1);

    /* tempo rises with feed */
    GK_CHECK(gk_ux_feed_to_tempo(1000.0f) > gk_ux_feed_to_tempo(0.0f));
    GK_CHECK(near(gk_ux_feed_to_tempo(0.0f), 40.0, 1e-5));
    GK_CHECK(near(gk_ux_feed_to_tempo(1000.0f), 220.0, 1e-5));

    gk_ux_sonifier_init(&s);
    GK_CHECK_EQ_INT(gk_ux_sonify(&s, 0.9f, 100.0f, 800.0f), GK_OK);
    GK_CHECK(s.last_pitch_hz > 200.0f);
    GK_CHECK_EQ_INT(s.notes_played, 1);
    /* load 0.1 still lands on the 220 Hz base tone, so the counter advances */
    GK_CHECK_EQ_INT(gk_ux_sonify(&s, 0.1f, 30.0f, 100.0f), GK_OK);
    GK_CHECK_EQ_INT(s.notes_played, 2);
    GK_CHECK_EQ_INT(gk_ux_sonify(NULL, 0.1f, 30.0f, 100.0f),
                    GK_ERR_INVALID_ARG);
    gk_ux_sonifier_init(NULL);

    GK_CHECK_STR_EQ(gk_ux_mapping_name(GK_UX_MAP_LOAD_TONE), "load-tone");
    GK_CHECK_STR_EQ(gk_ux_mapping_name(GK_UX_MAP_GCODE_MUSIC), "gcode-music");
    GK_CHECK_STR_EQ(gk_ux_mapping_name(GK_UX_MAP_COUNT), "unknown");
}

static void test_explanation(void)
{
    gk_ux_narration n;
    float times[10];
    int series = -1;

    gk_ux_narration_init(&n);
    GK_CHECK_EQ_INT(gk_ux_narration_say(&n, "Now cutting %s", "hole 3"),
                    GK_OK);
    GK_CHECK_STR_EQ(n.text, "Now cutting hole 3");
    GK_CHECK_EQ_INT(n.step, 1);
    GK_CHECK_EQ_INT(gk_ux_narration_say(&n, "No placeholder", "ignored"),
                    GK_OK);
    GK_CHECK_STR_EQ(n.text, "No placeholder");
    GK_CHECK_EQ_INT(gk_ux_narration_say(&n, NULL, "x"), GK_ERR_INVALID_ARG);
    gk_ux_narration_init(&n);
    GK_CHECK_EQ_INT(n.step, 0);
    gk_ux_narration_init(NULL);

    GK_CHECK_EQ_INT(gk_ux_chart_link(4, 0, &series), 0);
    GK_CHECK_EQ_INT(series, 0);
    GK_CHECK_EQ_INT(gk_ux_chart_link(4, 5, &series), 1);
    GK_CHECK_EQ_INT(series, 1);
    GK_CHECK_EQ_INT(gk_ux_chart_link(0, 1, &series), -1);
    GK_CHECK_EQ_INT(gk_ux_chart_link(2, -1, &series), -1);
    (void)gk_ux_chart_link(3, 4, NULL);

    GK_CHECK_EQ_INT(gk_ux_animation_frames(1.0f, 10.0f, times, 11), 11);
    GK_CHECK(near(times[0], 0.0, 1e-6));
    GK_CHECK(near(times[10], 1.0, 1e-6));
    GK_CHECK_EQ_INT(gk_ux_animation_frames(1.0f, 10.0f, times, 3), 11);
    GK_CHECK_EQ_INT(gk_ux_animation_frames(0.0f, 10.0f, times, 10), 0);
    GK_CHECK_EQ_INT(gk_ux_animation_frames(1.0f, 0.0f, times, 10), 0);
}

static void test_cues(void)
{
    gk_ux_cue_list l;
    int a, b, c, d;

    gk_ux_cue_list_init(&l);
    a = gk_ux_cue_add(&l, GK_UX_CUE_ANNOTATION, "note", 1.0f, 2.0f, 3.0f);
    b = gk_ux_cue_add(&l, GK_UX_CUE_ARROW, "->", 0.0f, 0.0f, 0.0f);
    c = gk_ux_cue_add(&l, GK_UX_CUE_SPEECH, "Check the tool", 0.0f, 0.0f, 0.0f);
    d = gk_ux_cue_add(&l, GK_UX_CUE_SPEECH, "Slow down", 0.0f, 0.0f, 0.0f);
    GK_CHECK_EQ_INT(a, 1);
    GK_CHECK_EQ_INT(b, 2);
    GK_CHECK_EQ_INT(c, 3);
    GK_CHECK_EQ_INT(d, 4);
    GK_CHECK_EQ_INT(l.count, 4);
    GK_CHECK_EQ_INT(gk_ux_cue_add(NULL, GK_UX_CUE_ARROW, "x", 0, 0, 0), -1);
    GK_CHECK_EQ_INT(gk_ux_cue_add(&l, GK_UX_CUE_COUNT, "x", 0, 0, 0), -1);

    /* highest priority: default 0, first match wins */
    GK_CHECK_EQ_INT(gk_ux_cue_highest(&l, GK_UX_CUE_SPEECH), 3);
    l.cues[3].priority = 5;
    GK_CHECK_EQ_INT(gk_ux_cue_highest(&l, GK_UX_CUE_SPEECH), 4);
    GK_CHECK_EQ_INT(gk_ux_cue_highest(NULL, GK_UX_CUE_SPEECH), -1);
    GK_CHECK_EQ_INT(gk_ux_cue_highest(&l, GK_UX_CUE_LIGHT), -1);

    GK_CHECK_EQ_INT(gk_ux_cue_set_active(&l, 4, 0), GK_OK);
    GK_CHECK_EQ_INT(gk_ux_cue_highest(&l, GK_UX_CUE_SPEECH), 3);
    GK_CHECK_EQ_INT(gk_ux_cue_set_active(&l, 99, 0), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_ux_cue_set_active(&l, 0, 0), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_ux_cue_set_active(NULL, 1, 0), GK_ERR_INVALID_ARG);

    GK_CHECK_STR_EQ(gk_ux_cue_speech(&l, 3), "Check the tool");
    GK_CHECK(gk_ux_cue_speech(&l, 1) == NULL); /* not a speech cue */
    GK_CHECK(gk_ux_cue_speech(&l, 99) == NULL);
    GK_CHECK(gk_ux_cue_speech(NULL, 3) == NULL);

    GK_CHECK_STR_EQ(gk_ux_cue_name(GK_UX_CUE_ANNOTATION), "annotation");
    GK_CHECK_STR_EQ(gk_ux_cue_name(GK_UX_CUE_LIGHT), "light");
    GK_CHECK_STR_EQ(gk_ux_cue_name(GK_UX_CUE_COUNT), "unknown");
    gk_ux_cue_list_init(NULL);
}

int main(void)
{
    test_devices();
    test_sonification();
    test_explanation();
    test_cues();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
