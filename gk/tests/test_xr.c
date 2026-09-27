#include "gk_test.h"
#include "gk/gk_xr.h"

#include <math.h>

static int near(double a, double b, double tol)
{
    return fabs(a - b) <= tol;
}

static void test_session_devices(void)
{
    gk_xr_session s;
    gk_xr_vec3 p = {1.0f, 2.0f, 3.0f};
    gk_xr_quat q = {0.0f, 0.0f, 0.0f, 1.0f};
    int head, ctrl;
    gk_xr_vec3 rel;

    GK_CHECK_EQ_INT(gk_xr_init(&s), GK_OK);
    GK_CHECK_EQ_INT(s.initialized, 1);
    GK_CHECK_EQ_INT(s.session_active, 0);
    GK_CHECK_EQ_INT(gk_xr_frame_begin(&s, 1.0), GK_ERR_STATE);

    GK_CHECK_EQ_INT(gk_xr_session_begin(&s), GK_OK);
    GK_CHECK_EQ_INT(gk_xr_session_begin(&s), GK_OK);
    GK_CHECK_EQ_INT(gk_xr_frame_begin(&s, 1.5), GK_OK);
    GK_CHECK_EQ_INT(s.frame_count, 1);
    GK_CHECK(near(s.predicted_display_time, 1.5, 1e-12));

    head = gk_xr_device_add(&s, GK_XR_DEVICE_HEADSET, "Quest");
    ctrl = gk_xr_device_add(&s, GK_XR_DEVICE_CONTROLLER, "RightCtrl");
    GK_CHECK_EQ_INT(head, 1);
    GK_CHECK_EQ_INT(ctrl, 2);
    GK_CHECK_EQ_INT(gk_xr_device_add(&s, GK_XR_DEVICE_NONE, "bad"), -1);
    GK_CHECK_EQ_INT(gk_xr_device_count(&s, GK_XR_DEVICE_HEADSET), 1);
    GK_CHECK_EQ_INT(gk_xr_device_count(&s, GK_XR_DEVICE_CONTROLLER), 1);
    GK_CHECK_EQ_INT(gk_xr_device_count(&s, GK_XR_DEVICE_GLASSES), 0);

    GK_CHECK_EQ_INT(gk_xr_device_set_pose(&s, head, p, q), GK_OK);
    GK_CHECK_EQ_INT(gk_xr_device_find(&s, head)->tracked, 1);
    GK_CHECK_EQ_INT(gk_xr_device_set_pose(&s, 999, p, q), GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_xr_device_set_pose(NULL, head, p, q),
                    GK_ERR_INVALID_ARG);
    GK_CHECK(gk_xr_device_find(&s, 999) == NULL);
    GK_CHECK(gk_xr_device_find(NULL, 1) == NULL);

    {
        gk_xr_vec3 cp = {4.0f, 6.0f, 8.0f};
        GK_CHECK_EQ_INT(gk_xr_device_set_pose(&s, ctrl, cp, q), GK_OK);
    }
    GK_CHECK_EQ_INT(gk_xr_locate_relative(&s, ctrl, &rel), GK_OK);
    GK_CHECK(near(rel.x, 3.0, 1e-6));
    GK_CHECK(near(rel.y, 4.0, 1e-6));
    GK_CHECK(near(rel.z, 5.0, 1e-6));
    GK_CHECK_EQ_INT(gk_xr_locate_relative(NULL, ctrl, &rel),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_xr_locate_relative(&s, ctrl, NULL),
                    GK_ERR_INVALID_ARG);

    GK_CHECK_EQ_INT(gk_xr_device_disconnect(&s, ctrl), GK_OK);
    GK_CHECK_EQ_INT(gk_xr_device_count(&s, GK_XR_DEVICE_CONTROLLER), 0);
    GK_CHECK_EQ_INT(gk_xr_device_set_pose(&s, ctrl, p, q), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_xr_device_disconnect(&s, 999), GK_ERR_NOT_FOUND);

    GK_CHECK_EQ_INT(gk_xr_session_end(&s), GK_OK);
    GK_CHECK_EQ_INT(gk_xr_session_end(&s), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_xr_session_begin(NULL), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_xr_init(NULL), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_xr_frame_begin(NULL, 1.0), GK_ERR_INVALID_ARG);

    GK_CHECK_STR_EQ(gk_xr_device_name(GK_XR_DEVICE_HEADSET), "headset");
    GK_CHECK_STR_EQ(gk_xr_device_name(GK_XR_DEVICE_NONE), "none");
    GK_CHECK_STR_EQ(gk_xr_device_name(GK_XR_DEVICE_COUNT), "unknown");
    (void)gk_xr_device_name(GK_XR_DEVICE_GLASSES);
    (void)gk_xr_device_name(GK_XR_DEVICE_PHONE);
    (void)gk_xr_device_name(GK_XR_DEVICE_CONTROLLER);
}

static void test_gamepad(void)
{
    gk_xr_gamepad p;

    gk_xr_gamepad_init(&p, GK_XR_PAD_XBOX);
    GK_CHECK_EQ_INT(p.type, GK_XR_PAD_XBOX);
    GK_CHECK_EQ_INT(gk_xr_gamepad_set_axis(&p, 0, 0.5f), GK_OK);
    GK_CHECK(near(p.axes[0], 0.5, 1e-6));
    GK_CHECK_EQ_INT(gk_xr_gamepad_set_axis(&p, 0, 5.0f), GK_OK);
    GK_CHECK(near(p.axes[0], 1.0, 1e-6)); /* clamped */
    GK_CHECK_EQ_INT(gk_xr_gamepad_set_axis(&p, -1, 0.0f),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_xr_gamepad_set_axis(&p, 99, 0.0f),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_xr_gamepad_set_axis(NULL, 0, 0.0f),
                    GK_ERR_INVALID_ARG);

    gk_xr_gamepad_set_axis(&p, 4, 1.0f);
    gk_xr_gamepad_set_axis(&p, 5, 0.0f);
    GK_CHECK(near(p.trigger_l, 1.0, 1e-6));
    GK_CHECK(near(p.trigger_r, 0.5, 1e-6));

    GK_CHECK_EQ_INT(gk_xr_gamepad_set_button(&p, 0, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_xr_gamepad_button_down(&p, 0), 1);
    GK_CHECK_EQ_INT(gk_xr_gamepad_button_pressed(&p, 0), 1);
    GK_CHECK_EQ_INT(gk_xr_gamepad_button_released(&p, 0), 0);
    gk_xr_gamepad_next_frame(&p);
    GK_CHECK_EQ_INT(gk_xr_gamepad_button_pressed(&p, 0), 0);
    gk_xr_gamepad_set_button(&p, 0, 0);
    GK_CHECK_EQ_INT(gk_xr_gamepad_button_released(&p, 0), 1);
    gk_xr_gamepad_next_frame(&p);
    GK_CHECK_EQ_INT(gk_xr_gamepad_button_released(&p, 0), 0);

    GK_CHECK_EQ_INT(gk_xr_gamepad_set_button(&p, 99, 1),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_xr_gamepad_button_down(&p, -1), 0);
    GK_CHECK_EQ_INT(gk_xr_gamepad_button_pressed(NULL, 0), 0);
    GK_CHECK_EQ_INT(gk_xr_gamepad_button_released(NULL, 0), 0);

    gk_xr_gamepad_set_rumble(&p, 0.7f, 2.0f);
    GK_CHECK(near(p.rumble_l, 0.7, 1e-6));
    GK_CHECK(near(p.rumble_r, 1.0, 1e-6));

    gk_xr_gamepad_init(NULL, GK_XR_PAD_PS);
    gk_xr_gamepad_set_axis(NULL, 0, 1.0f);
    gk_xr_gamepad_set_button(NULL, 0, 1);
    gk_xr_gamepad_set_rumble(NULL, 0.0f, 0.0f);
    gk_xr_gamepad_next_frame(NULL);

    GK_CHECK_STR_EQ(gk_xr_pad_name(GK_XR_PAD_XBOX), "xbox");
    GK_CHECK_STR_EQ(gk_xr_pad_name(GK_XR_PAD_PS), "playstation");
    GK_CHECK_STR_EQ(gk_xr_pad_name(GK_XR_PAD_VR), "vr-controller");
    GK_CHECK_STR_EQ(gk_xr_pad_name(GK_XR_PAD_GENERIC), "generic");
    GK_CHECK_STR_EQ(gk_xr_pad_name(GK_XR_PAD_COUNT), "unknown");
}

static void test_ar(void)
{
    gk_xr_ar_layer a;
    gk_xr_vec3 pos = {0.1f, 0.0f, 1.0f};
    gk_xr_vec3 behind = {0.0f, 0.0f, -1.0f};
    int id, id2;
    float x, y;
    gk_xr_vec3 t;

    gk_xr_ar_init(&a);
    GK_CHECK_EQ_INT(a.overlay_enabled, 1);
    id = gk_xr_ar_anchor_add(&a, "hole1", pos, 0.05f);
    id2 = gk_xr_ar_anchor_add(&a, "hole2", behind, 0.05f);
    GK_CHECK_EQ_INT(id, 1);
    GK_CHECK_EQ_INT(id2, 2);
    GK_CHECK_EQ_INT(a.count, 2);

    GK_CHECK_EQ_INT(gk_xr_ar_project(&a, id, 90.0f, 1.0f, &x, &y), 1);
    GK_CHECK_EQ_INT(gk_xr_ar_project(&a, id2, 90.0f, 1.0f, &x, &y), 0);
    GK_CHECK_EQ_INT(gk_xr_ar_project(&a, 999, 90.0f, 1.0f, &x, &y), 0);
    GK_CHECK_EQ_INT(gk_xr_ar_project(NULL, id, 90.0f, 1.0f, &x, &y), 0);
    GK_CHECK_EQ_INT(gk_xr_ar_project(&a, id, 0.0f, 1.0f, &x, &y), 0);
    GK_CHECK_EQ_INT(gk_xr_ar_project(&a, id, 90.0f, 1.0f, NULL, &y), 0);

    GK_CHECK_EQ_INT(gk_xr_ar_anchor_set_visible(&a, id, 0), GK_OK);
    GK_CHECK_EQ_INT(a.anchors[0].visible, 0);
    GK_CHECK_EQ_INT(gk_xr_ar_anchor_set_visible(&a, 999, 0),
                    GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_xr_ar_anchor_set_visible(NULL, id, 0),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_xr_ar_anchor_add(NULL, "x", pos, 1.0f), -1);

    /* phone AR pose: farther marker (smaller pixels) sits farther away */
    GK_CHECK_EQ_INT(gk_xr_phone_ar_pose(100.0f, 50.0f, 800.0f, &t), GK_OK);
    {
        gk_xr_vec3 t2;
        GK_CHECK_EQ_INT(gk_xr_phone_ar_pose(50.0f, 50.0f, 800.0f, &t2), GK_OK);
        GK_CHECK(t2.z > t.z);
    }
    GK_CHECK_EQ_INT(gk_xr_phone_ar_pose(0.0f, 50.0f, 800.0f, &t),
                    GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_xr_phone_ar_pose(100.0f, 50.0f, 0.0f, &t),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_xr_phone_ar_pose(100.0f, 50.0f, 800.0f, NULL),
                    GK_ERR_INVALID_ARG);
    gk_xr_ar_init(NULL);
}

static void test_haptics_force(void)
{
    gk_xr_haptic h;
    gk_xr_force_feedback f;
    int i;

    gk_xr_haptic_init(&h);
    GK_CHECK_EQ_INT(gk_xr_haptic_active(&h), 0);
    GK_CHECK_EQ_INT(gk_xr_haptic_play(&h, 0.0f, 0.5f, 0.1f),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_xr_haptic_play(&h, 200.0f, 2.0f, 0.1f), GK_OK);
    GK_CHECK(near(h.current.amplitude, 1.0, 1e-6)); /* clamped */
    GK_CHECK_EQ_INT(gk_xr_haptic_active(&h), 1);

    for (i = 0; i < 20 && gk_xr_haptic_active(&h); i++) {
        GK_CHECK_EQ_INT(gk_xr_haptic_update(&h, 0.01f), GK_OK);
    }
    GK_CHECK_EQ_INT(gk_xr_haptic_active(&h), 0);
    GK_CHECK_EQ_INT(gk_xr_haptic_update(&h, 0.01f), GK_ERR_STATE);

    GK_CHECK_EQ_INT(gk_xr_haptic_play(NULL, 1.0f, 1.0f, 1.0f),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_xr_haptic_update(NULL, 0.01f), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_xr_haptic_active(NULL), 0);
    gk_xr_haptic_init(NULL);

    gk_xr_force_feedback_init(&f, 1.0f, 0.5f, 2.0f);
    GK_CHECK(near(f.angle, 0.0, 1e-6));
    /* constant torque spins the wheel up */
    for (i = 0; i < 50; i++) {
        GK_CHECK_EQ_INT(gk_xr_force_feedback_step(&f, 5.0f, 0.01f), GK_OK);
    }
    GK_CHECK(f.angle > 0.0f);
    GK_CHECK_EQ_INT(gk_xr_force_feedback_step(&f, 0.0f, 0.0f),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_xr_force_feedback_step(NULL, 0.0f, 0.01f),
                    GK_ERR_INVALID_ARG);
    gk_xr_force_feedback_init(NULL, 1.0f, 1.0f, 1.0f);
    GK_CHECK(near(gk_xr_force_feedback_detent(&f, 0, 1.0f), 0.0, 1e-9));
    (void)gk_xr_force_feedback_detent(&f, 24, 0.5f);
}

static void test_hrtf_audio(void)
{
    gk_xr_hrtf h;
    gk_xr_spatial_audio a;

    /* source directly to the right */
    gk_xr_hrtf_compute(&h, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    GK_CHECK(near(h.azimuth_deg, 90.0, 1.0));
    GK_CHECK(near(h.distance_m, 1.0, 1e-5));
    GK_CHECK(h.itd_ms > 0.0f);   /* right ear earlier */
    GK_CHECK(h.ild_db > 0.0f);

    /* source directly ahead */
    gk_xr_hrtf_compute(&h, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    GK_CHECK(near(h.azimuth_deg, 0.0, 1e-5));
    GK_CHECK(near(h.itd_ms, 0.0, 1e-5));

    /* elevated source above the listener */
    gk_xr_hrtf_compute(&h, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    GK_CHECK(h.elevation_deg > 0.0f);

    /* coincident source-safe distance handling */
    gk_xr_hrtf_compute(&h, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    GK_CHECK(near(h.distance_m, 0.0, 1e-6));
    GK_CHECK(near(h.elevation_deg, 0.0, 1e-6));

    gk_xr_hrtf_compute(NULL, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    gk_xr_hrtf_compute(&h, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f);
    gk_xr_spatial_audio_compute(&a, &h, 1.0f, 1.0f);
    GK_CHECK(near(a.distance, 2.0, 1e-5));
    GK_CHECK(near(a.gain, 0.5, 1e-5));
    gk_xr_spatial_audio_compute(&a, &h, 2.0f, 1.0f);
    GK_CHECK(near(a.gain, 1.0, 1e-6)); /* capped at unity */

    /* right-side source delays the left ear */
    gk_xr_hrtf_compute(&h, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    gk_xr_spatial_audio_compute(&a, &h, 1.0f, 1.0f);
    GK_CHECK(a.delay_left_ms > 0.0f);
    GK_CHECK(near(a.delay_right_ms, 0.0, 1e-6));

    gk_xr_spatial_audio_compute(NULL, &h, 1.0f, 1.0f);
    gk_xr_spatial_audio_compute(&a, NULL, 1.0f, 1.0f);
    gk_xr_spatial_audio_compute(&a, &h, 1.0f, 0.0f);
}

static void test_handwheel(void)
{
    gk_xr_handwheel h;
    float axes[2] = {100.0f, 0.0f};
    float axes2[2] = {150.0f, 0.0f};

    gk_xr_handwheel_init(&h, 100.0);
    GK_CHECK(near(h.counts_per_rev, 100.0, 1e-9));
    GK_CHECK_EQ_INT(gk_xr_handwheel_is_usb(&h), 0);
    GK_CHECK_EQ_INT(gk_xr_handwheel_report(&h, axes, 2), GK_ERR_STATE);

    GK_CHECK_EQ_INT(gk_xr_handwheel_connect(&h, 0x1234, 0x5678), GK_OK);
    GK_CHECK_EQ_INT(gk_xr_handwheel_is_usb(&h), 1);
    GK_CHECK_EQ_INT(gk_xr_handwheel_report(&h, axes, 2), GK_OK);
    GK_CHECK(near(h.position, 100.0, 1e-9));
    GK_CHECK(near(gk_xr_handwheel_delta_deg(&h), 0.0, 1e-9));

    GK_CHECK_EQ_INT(gk_xr_handwheel_report(&h, axes2, 2), GK_OK);
    GK_CHECK(near(gk_xr_handwheel_delta_deg(&h), 180.0, 1e-9));

    GK_CHECK_EQ_INT(gk_xr_handwheel_report(&h, NULL, 2),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_xr_handwheel_report(NULL, axes, 2),
                    GK_ERR_INVALID_ARG);
    gk_xr_handwheel_init(NULL, 1.0);
    GK_CHECK_EQ_INT(gk_xr_handwheel_connect(NULL, 1, 1), GK_ERR_INVALID_ARG);
    GK_CHECK(near(gk_xr_handwheel_delta_deg(NULL), 0.0, 1e-9));
    GK_CHECK_EQ_INT(gk_xr_handwheel_is_usb(NULL), 0);

    /* default counts per rev when zero passed */
    gk_xr_handwheel_init(&h, 0.0);
    GK_CHECK(near(h.counts_per_rev, 100.0, 1e-9));
}

static void test_multiscreen(void)
{
    gk_xr_multiscreen m;

    gk_xr_multiscreen_init(&m);
    GK_CHECK_EQ_INT(m.count, 0);
    GK_CHECK_EQ_INT(gk_xr_multiscreen_sync(&m, 0.016), GK_ERR_STATE);

    GK_CHECK_EQ_INT(gk_xr_multiscreen_add(&m, GK_XR_SCREEN_SLAVE, 1920, 1080),
                    1);
    /* exactly one master required */
    GK_CHECK_EQ_INT(gk_xr_multiscreen_sync(&m, 0.016), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_xr_multiscreen_add(&m, GK_XR_SCREEN_MASTER, 1920, 1080),
                    2);
    GK_CHECK_EQ_INT(gk_xr_multiscreen_sync(&m, 0.016), GK_OK);
    /* swap barrier equalises latency */
    GK_CHECK(near(m.screens[0].latency_ms, m.screens[1].latency_ms, 1e-9));
    GK_CHECK(near(gk_xr_multiscreen_max_latency(&m), m.screens[0].latency_ms,
                  1e-9));

    m.swap_barrier = 0;
    GK_CHECK_EQ_INT(gk_xr_multiscreen_add(&m, GK_XR_SCREEN_MIRROR, 3840, 2160),
                    3);
    GK_CHECK_EQ_INT(gk_xr_multiscreen_sync(&m, 0.016), GK_OK);
    GK_CHECK(m.screens[2].latency_ms > m.screens[0].latency_ms);

    GK_CHECK_EQ_INT(gk_xr_multiscreen_add(&m, GK_XR_SCREEN_SLAVE, 0, 1080), -1);
    GK_CHECK_EQ_INT(gk_xr_multiscreen_add(NULL, GK_XR_SCREEN_SLAVE, 1, 1), -1);
    GK_CHECK_EQ_INT(gk_xr_multiscreen_sync(NULL, 0.016), GK_ERR_INVALID_ARG);
    GK_CHECK(near(gk_xr_multiscreen_max_latency(NULL), 0.0, 1e-12));
    gk_xr_multiscreen_init(NULL);
}

static void test_eye_eeg_heart(void)
{
    gk_xr_eye_tracker e;
    gk_xr_eeg g;
    gk_xr_heart h;

    gk_xr_eye_init(&e);
    GK_CHECK_EQ_INT(gk_xr_eye_update(&e, 0.01), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_xr_eye_add_point(&e, 0.5f, 0.5f), GK_OK);
    GK_CHECK_EQ_INT(gk_xr_eye_add_point(&e, 0.6f, 0.4f), GK_OK);
    GK_CHECK_EQ_INT(gk_xr_eye_update(&e, 0.1), GK_OK);
    GK_CHECK(near(e.fixation_x, 0.55, 1e-5));
    GK_CHECK(near(e.dwell_s, 0.1, 1e-6));
    GK_CHECK_EQ_INT(gk_xr_eye_select(&e, 0.1f, 0.05), 1);
    GK_CHECK_EQ_INT(gk_xr_eye_select(&e, 0.1f, 1.0), -1);
    GK_CHECK_EQ_INT(gk_xr_eye_select(&e, 0.0f, 0.05), -1);
    GK_CHECK_EQ_INT(gk_xr_eye_select(NULL, 0.1f, 0.05), -1);
    GK_CHECK_EQ_INT(gk_xr_eye_add_point(NULL, 0.0f, 0.0f),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_xr_eye_update(NULL, 0.01), GK_ERR_INVALID_ARG);
    /* window overflow keeps working */
    {
        int i;
        for (i = 0; i < GK_XR_MAX_GAZE_POINTS + 10; i++) {
            gk_xr_eye_add_point(&e, 0.1f, 0.1f);
        }
        GK_CHECK_EQ_INT(e.count, GK_XR_MAX_GAZE_POINTS);
    }
    gk_xr_eye_init(NULL);

    gk_xr_eeg_init(&g);
    GK_CHECK_EQ_INT(g.signal_quality, 100);
    GK_CHECK_EQ_INT(gk_xr_eeg_set_band(&g, GK_XR_EEG_BETA, 0.6f), GK_OK);
    GK_CHECK_EQ_INT(gk_xr_eeg_set_band(&g, GK_XR_EEG_ALPHA, 0.2f), GK_OK);
    GK_CHECK_EQ_INT(gk_xr_eeg_set_band(&g, GK_XR_EEG_THETA, 0.2f), GK_OK);
    GK_CHECK_EQ_INT(gk_xr_eeg_set_band(&g, GK_XR_EEG_BAND_COUNT, 0.1f),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_xr_eeg_set_band(&g, GK_XR_EEG_BETA, -1.0f),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_xr_eeg_evaluate(&g), GK_OK);
    GK_CHECK(near(g.attention, 0.6 / 0.8, 1e-5));
    GK_CHECK(near(g.meditation, 0.5, 1e-5));
    GK_CHECK(near(gk_xr_eeg_cognitive_load(&g), 0.6 / 0.4, 1e-5));
    GK_CHECK_EQ_INT(gk_xr_eeg_evaluate(NULL), GK_ERR_INVALID_ARG);
    GK_CHECK(near(gk_xr_eeg_cognitive_load(NULL), 0.0, 1e-9));
    gk_xr_eeg_init(NULL);
    GK_CHECK_STR_EQ(gk_xr_eeg_band_name(GK_XR_EEG_DELTA), "delta");
    GK_CHECK_STR_EQ(gk_xr_eeg_band_name(GK_XR_EEG_GAMMA), "gamma");
    GK_CHECK_STR_EQ(gk_xr_eeg_band_name(GK_XR_EEG_BAND_COUNT), "unknown");

    gk_xr_heart_init(&h);
    GK_CHECK_EQ_INT(gk_xr_heart_report(&h, 70, 850.0), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_xr_heart_connect(&h), GK_OK);
    GK_CHECK_EQ_INT(gk_xr_heart_report(&h, 70, 850.0), GK_OK);
    GK_CHECK_EQ_INT(gk_xr_heart_report(&h, 90, 600.0), GK_OK);
    GK_CHECK_EQ_INT(h.samples, 2);
    GK_CHECK(near(h.avg_bpm, 80.0, 1e-9));
    GK_CHECK_EQ_INT(gk_xr_heart_report(&h, 0, 800.0), GK_ERR_INVALID_ARG);
    GK_CHECK(gk_xr_heart_stress(&h) > 0.0f);
    /* a racing heart with erratic RR intervals scores a high stress index */
    {
        gk_xr_heart stressed;
        gk_xr_heart_init(&stressed);
        gk_xr_heart_connect(&stressed);
        gk_xr_heart_report(&stressed, 130, 400.0);
        gk_xr_heart_report(&stressed, 140, 450.0);
        GK_CHECK(gk_xr_heart_stress(&stressed) > gk_xr_heart_stress(&h));
        GK_CHECK(gk_xr_heart_stress(&stressed) <= 1.0f);
    }
    GK_CHECK(near(gk_xr_heart_stress(NULL), 0.0, 1e-9));
    GK_CHECK_EQ_INT(gk_xr_heart_connect(NULL), GK_ERR_INVALID_ARG);
}

int main(void)
{
    test_session_devices();
    test_gamepad();
    test_ar();
    test_haptics_force();
    test_hrtf_audio();
    test_handwheel();
    test_multiscreen();
    test_eye_eeg_heart();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
