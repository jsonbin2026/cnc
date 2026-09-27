#include "gk/gk_hmi.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ===================================================================
 * Handwheel (1151-1153)
 * =================================================================== */

void gk_hmi_handwheel_init(gk_hmi_handwheel *h, int detents_per_rev)
{
    if (h == NULL) {
        return;
    }
    memset(h, 0, sizeof(*h));
    h->detents_per_rev = detents_per_rev > 0 ? detents_per_rev : 100;
    h->notch_force_n = 0.5;
}

gk_status gk_hmi_handwheel_turn(gk_hmi_handwheel *h, double delta_deg)
{
    if (h == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    h->angle_deg += delta_deg;
    if (h->angle_deg >= 360.0) {
        h->angle_deg -= 360.0;
    }
    if (h->angle_deg < 0.0) {
        h->angle_deg += 360.0;
    }
    h->detent_index = (int)(h->angle_deg *
                            (double)h->detents_per_rev / 360.0);
    return GK_OK;
}

int gk_hmi_handwheel_detent(const gk_hmi_handwheel *h)
{
    if (h == NULL) {
        return 0;
    }
    return h->detent_index;
}

double gk_hmi_handwheel_scale_error(const gk_hmi_handwheel *h)
{
    double step;
    double nearest;
    if (h == NULL || h->detents_per_rev <= 0) {
        return 0.0;
    }
    step = 360.0 / (double)h->detents_per_rev;
    nearest = round(h->angle_deg / step) * step;
    return fabs(h->angle_deg - nearest);
}

int gk_hmi_handwheel_scale_aligned(const gk_hmi_handwheel *h,
                                   double tolerance_deg)
{
    return gk_hmi_handwheel_scale_error(h) <= tolerance_deg;
}

gk_status gk_hmi_handwheel_gear(gk_hmi_handwheel *h, int gear,
                                double *click_db)
{
    if (h == NULL || click_db == NULL || gear < 1 || gear > 4) {
        return GK_ERR_OUT_OF_RANGE;
    }
    /* higher gears give a sharper detent, so a louder click */
    *click_db = 40.0 + 5.0 * (double)gear;
    h->notch_force_n = 0.3 + 0.2 * (double)gear;
    return GK_OK;
}

/* ===================================================================
 * Push button (1154-1156)
 * =================================================================== */

void gk_hmi_button_init(gk_hmi_button *b, double travel_mm)
{
    if (b == NULL) {
        return;
    }
    memset(b, 0, sizeof(*b));
    b->travel_mm = travel_mm;
    b->backlight = 0.5;
}

double gk_hmi_button_press(gk_hmi_button *b, double force_n)
{
    if (b == NULL || force_n < 0.0) {
        return 0.0;
    }
    /* a button gives in proportionally to the applied force */
    b->pressed = force_n > 0.0 ? 1 : 0;
    b->press_depth_mm = b->travel_mm * (force_n / (force_n + 1.0));
    if (b->pressed && b->press_depth_mm > b->travel_mm) {
        b->press_depth_mm = b->travel_mm;
    }
    return b->press_depth_mm;
}

double gk_hmi_button_release(gk_hmi_button *b)
{
    if (b == NULL) {
        return 0.0;
    }
    b->pressed = 0;
    b->press_depth_mm = 0.0;
    return b->press_depth_mm;
}

gk_status gk_hmi_button_backlight(gk_hmi_button *b, double level)
{
    if (b == NULL || level < 0.0 || level > 1.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    b->backlight = level;
    b->r_backlight = level * 255.0;
    return GK_OK;
}

/* ===================================================================
 * Rotary knob (1157-1159)
 * =================================================================== */

void gk_hmi_knob_init(gk_hmi_knob *k, int positions, double damping)
{
    if (k == NULL) {
        return;
    }
    memset(k, 0, sizeof(*k));
    k->positions = positions > 0 ? positions : 12;
    k->damping = damping;
}

double gk_hmi_knob_rotate(gk_hmi_knob *k, double delta_deg)
{
    double step;
    if (k == NULL) {
        return 0.0;
    }
    k->angle_deg += delta_deg;
    while (k->angle_deg >= 360.0) {
        k->angle_deg -= 360.0;
    }
    while (k->angle_deg < 0.0) {
        k->angle_deg += 360.0;
    }
    step = 360.0 / (double)k->positions;
    k->position = (int)(k->angle_deg / step) % k->positions;
    return k->angle_deg;
}

double gk_hmi_knob_damping_torque(const gk_hmi_knob *k, double speed_dps)
{
    if (k == NULL) {
        return 0.0;
    }
    return k->damping * speed_dps;
}

int gk_hmi_knob_position(const gk_hmi_knob *k)
{
    if (k == NULL) {
        return 0;
    }
    return k->position;
}

/* ===================================================================
 * Display (1160-1165)
 * =================================================================== */

void gk_hmi_display_init(gk_hmi_display *d)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    d->scanlines = 1080;
    d->refresh_hz = 60.0;
    d->on = 1;
}

gk_status gk_hmi_display_set_refresh(gk_hmi_display *d, double refresh_hz)
{
    if (d == NULL || refresh_hz <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    d->refresh_hz = refresh_hz;
    /* low refresh rates flicker visibly */
    d->flicker = refresh_hz < 50.0 ? (50.0 - refresh_hz) / 50.0 : 0.0;
    return GK_OK;
}

double gk_hmi_display_glare(const gk_hmi_display *d, double viewing_angle_deg)
{
    if (d == NULL) {
        return 0.0;
    }
    /* glare is highest for grazing angles */
    return d->glare * fabs(sin(viewing_angle_deg * M_PI / 180.0));
}

double gk_hmi_display_brightness(const gk_hmi_display *d)
{
    if (d == NULL || !d->on) {
        return 0.0;
    }
    /* aging reduces brightness over time */
    return 1.0 - d->aging;
}

int gk_hmi_display_dead(const gk_hmi_display *d, int x, int y)
{
    int code;
    if (d == NULL || d->dead_pixels <= 0) {
        return 0;
    }
    /* deterministic scattering of the dead pixels */
    code = (x * 73856093) ^ (y * 19349663);
    if (code < 0) {
        code = -code;
    }
    return (code % 1000) < d->dead_pixels;
}

/* ===================================================================
 * Keyboard / mouse (1166-1169)
 * =================================================================== */

void gk_hmi_keyboard_init(gk_hmi_keyboard *k)
{
    if (k == NULL) {
        return;
    }
    memset(k, 0, sizeof(*k));
    k->key_volume_db = 45.0;
    k->backlight = 0.5;
}

double gk_hmi_key_press(gk_hmi_keyboard *k, double key_mass)
{
    if (k == NULL || key_mass < 0.0) {
        return 0.0;
    }
    return k->key_volume_db + 10.0 * log10(1.0 + key_mass);
}

gk_status gk_hmi_keyboard_backlight(gk_hmi_keyboard *k, double level)
{
    if (k == NULL || level < 0.0 || level > 1.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    k->backlight = level;
    return GK_OK;
}

void gk_hmi_mouse_init(gk_hmi_mouse *m)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    m->click_db = 50.0;
    m->wheel_db = 35.0;
}

double gk_hmi_mouse_click(gk_hmi_mouse *m)
{
    if (m == NULL) {
        return 0.0;
    }
    return m->click_db;
}

double gk_hmi_mouse_wheel(gk_hmi_mouse *m, double scroll_steps)
{
    if (m == NULL || scroll_steps < 0.0) {
        return 0.0;
    }
    /* every detent gives one click */
    return m->wheel_db + 3.0 * scroll_steps;
}

/* ===================================================================
 * Touch / haptic (1170-1171)
 * =================================================================== */

void gk_hmi_touch_init(gk_hmi_touch *t, double threshold_n)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->threshold_n = threshold_n;
}

gk_status gk_hmi_touch_press(gk_hmi_touch *t, double pressure_n)
{
    if (t == NULL || pressure_n < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    t->pressure_n = pressure_n;
    t->triggered = pressure_n >= t->threshold_n ? 1 : 0;
    return GK_OK;
}

double gk_hmi_touch_feedback(const gk_hmi_touch *t)
{
    if (t == NULL || !t->triggered) {
        return 0.0;
    }
    return t->pressure_n;
}

void gk_hmi_haptic_init(gk_hmi_haptic *h)
{
    if (h == NULL) {
        return;
    }
    memset(h, 0, sizeof(*h));
    h->frequency_hz = 200.0;
}

gk_status gk_hmi_haptic_pulse(gk_hmi_haptic *h, double amplitude_mm,
                              double duration_ms)
{
    if (h == NULL || amplitude_mm < 0.0 || duration_ms < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    h->amplitude_mm = amplitude_mm;
    h->duration_ms = duration_ms;
    return GK_OK;
}

/* ===================================================================
 * Notification sounds (1172-1175)
 * =================================================================== */

const char *gk_hmi_sound_name(gk_hmi_sound s)
{
    switch (s) {
    case GK_HMI_SOUND_PROMPT: return "prompt";
    case GK_HMI_SOUND_WARNING: return "warning";
    case GK_HMI_SOUND_ERROR: return "error";
    case GK_HMI_SOUND_SUCCESS: return "success";
    default: return "unknown";
    }
}

double gk_hmi_sound_level(gk_hmi_sound s)
{
    switch (s) {
    case GK_HMI_SOUND_PROMPT: return 0.3;
    case GK_HMI_SOUND_WARNING: return 0.6;
    case GK_HMI_SOUND_ERROR: return 0.9;
    case GK_HMI_SOUND_SUCCESS: return 0.5;
    default: return 0.0;
    }
}

void gk_hmi_speaker_init(gk_hmi_speaker *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->volume = 1.0;
}

gk_status gk_hmi_speaker_play(gk_hmi_speaker *s, gk_hmi_sound sound,
                              double *frequency_hz, double *duration_ms)
{
    if (s == NULL || frequency_hz == NULL || duration_ms == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    switch (sound) {
    case GK_HMI_SOUND_PROMPT:
        *frequency_hz = 1000.0;
        *duration_ms = 100.0;
        break;
    case GK_HMI_SOUND_WARNING:
        *frequency_hz = 800.0;
        *duration_ms = 400.0;
        break;
    case GK_HMI_SOUND_ERROR:
        *frequency_hz = 400.0;
        *duration_ms = 800.0;
        break;
    case GK_HMI_SOUND_SUCCESS:
        *frequency_hz = 1500.0;
        *duration_ms = 200.0;
        break;
    default:
        return GK_ERR_OUT_OF_RANGE;
    }
    s->plays++;
    return GK_OK;
}
