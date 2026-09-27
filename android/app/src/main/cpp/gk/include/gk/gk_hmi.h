#ifndef GK_HMI_H
#define GK_HMI_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===================================================================
 * Batch 43: real human-machine interaction details (1151-1175)
 * Prefix: gk_hmi_
 * =================================================================== */

/* 1151 handwheel detents */
typedef struct {
    int detents_per_rev;
    double angle_deg;
    int detent_index;
    double notch_force_n;
} gk_hmi_handwheel;

void gk_hmi_handwheel_init(gk_hmi_handwheel *h, int detents_per_rev);
gk_status gk_hmi_handwheel_turn(gk_hmi_handwheel *h, double delta_deg);
int gk_hmi_handwheel_detent(const gk_hmi_handwheel *h);

/* 1152 handwheel scale alignment */
double gk_hmi_handwheel_scale_error(const gk_hmi_handwheel *h);
int gk_hmi_handwheel_scale_aligned(const gk_hmi_handwheel *h,
                                   double tolerance_deg);

/* 1153 handwheel gear change click */
gk_status gk_hmi_handwheel_gear(gk_hmi_handwheel *h, int gear,
                                double *click_db);

/* 1154-1156 push button */
typedef struct {
    double travel_mm;
    double press_depth_mm;
    int pressed;
    int backlight;
    double r_backlight;
} gk_hmi_button;

void gk_hmi_button_init(gk_hmi_button *b, double travel_mm);
double gk_hmi_button_press(gk_hmi_button *b, double force_n);
double gk_hmi_button_release(gk_hmi_button *b);
gk_status gk_hmi_button_backlight(gk_hmi_button *b, double level);

/* 1157-1159 rotary knob */
typedef struct {
    double angle_deg;
    double damping;
    int positions;
    int position;
} gk_hmi_knob;

void gk_hmi_knob_init(gk_hmi_knob *k, int positions, double damping);
double gk_hmi_knob_rotate(gk_hmi_knob *k, double delta_deg);
double gk_hmi_knob_damping_torque(const gk_hmi_knob *k, double speed_dps);
int gk_hmi_knob_position(const gk_hmi_knob *k);

/* 1160-1165 display */
typedef struct {
    int scanlines;
    double glare;
    double aging;
    int dead_pixels;
    double refresh_hz;
    double flicker;
    int on;
} gk_hmi_display;

void gk_hmi_display_init(gk_hmi_display *d);
gk_status gk_hmi_display_set_refresh(gk_hmi_display *d, double refresh_hz);
double gk_hmi_display_glare(const gk_hmi_display *d, double viewing_angle_deg);
double gk_hmi_display_brightness(const gk_hmi_display *d);
int gk_hmi_display_dead(const gk_hmi_display *d, int x, int y);

/* 1166-1169 keyboard / mouse */
typedef struct {
    double key_volume_db;
    double backlight;
} gk_hmi_keyboard;

void gk_hmi_keyboard_init(gk_hmi_keyboard *k);
double gk_hmi_key_press(gk_hmi_keyboard *k, double key_mass);
gk_status gk_hmi_keyboard_backlight(gk_hmi_keyboard *k, double level);

typedef struct {
    double click_db;
    double wheel_db;
} gk_hmi_mouse;

void gk_hmi_mouse_init(gk_hmi_mouse *m);
double gk_hmi_mouse_click(gk_hmi_mouse *m);
double gk_hmi_mouse_wheel(gk_hmi_mouse *m, double scroll_steps);

/* 1170-1171 touch / vibration feedback */
typedef struct {
    double pressure_n;
    double threshold_n;
    int triggered;
} gk_hmi_touch;

void gk_hmi_touch_init(gk_hmi_touch *t, double threshold_n);
gk_status gk_hmi_touch_press(gk_hmi_touch *t, double pressure_n);
double gk_hmi_touch_feedback(const gk_hmi_touch *t);

typedef struct {
    double amplitude_mm;
    double duration_ms;
    double frequency_hz;
} gk_hmi_haptic;

void gk_hmi_haptic_init(gk_hmi_haptic *h);
gk_status gk_hmi_haptic_pulse(gk_hmi_haptic *h, double amplitude_mm,
                              double duration_ms);

/* 1172-1175 notification sounds */
typedef enum {
    GK_HMI_SOUND_PROMPT = 0,  /* 1172 */
    GK_HMI_SOUND_WARNING,     /* 1173 */
    GK_HMI_SOUND_ERROR,       /* 1174 */
    GK_HMI_SOUND_SUCCESS      /* 1175 */
} gk_hmi_sound;

const char *gk_hmi_sound_name(gk_hmi_sound s);

typedef struct {
    double volume;
    unsigned int plays;
} gk_hmi_speaker;

void gk_hmi_speaker_init(gk_hmi_speaker *s);
gk_status gk_hmi_speaker_play(gk_hmi_speaker *s, gk_hmi_sound sound,
                              double *frequency_hz, double *duration_ms);
double gk_hmi_sound_level(gk_hmi_sound s);

#ifdef __cplusplus
}
#endif

#endif /* GK_HMI_H */
