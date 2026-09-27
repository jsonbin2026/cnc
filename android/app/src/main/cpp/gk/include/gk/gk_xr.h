#ifndef GK_XR_H
#define GK_XR_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_XR_MAX_DEVICES 16
#define GK_XR_MAX_ANCHORS 64
#define GK_XR_MAX_CHANNELS 8
#define GK_XR_MAX_GAZE_POINTS 128
#define GK_XR_NAME 64

/* ---- 731 OpenXR session / 732 headset / 735 AR glasses ---- */

typedef enum {
    GK_XR_DEVICE_NONE = 0,
    GK_XR_DEVICE_HEADSET,      /* 732 VR headset */
    GK_XR_DEVICE_GLASSES,      /* 735 AR glasses */
    GK_XR_DEVICE_PHONE,        /* 736 phone AR */
    GK_XR_DEVICE_CONTROLLER,   /* 733 VR controller */
    GK_XR_DEVICE_COUNT
} gk_xr_device_type;

const char *gk_xr_device_name(gk_xr_device_type t);

typedef struct {
    float x, y, z;
} gk_xr_vec3;

typedef struct {
    float x, y, z, w;
} gk_xr_quat;

typedef struct {
    int id;
    gk_xr_device_type type;
    char name[GK_XR_NAME];
    int connected;
    int tracked;
    gk_xr_vec3 position;
    gk_xr_quat orientation;
} gk_xr_device;

typedef struct {
    int initialized;
    int session_active;
    int frame_count;
    double predicted_display_time;
    gk_xr_device devices[GK_XR_MAX_DEVICES];
    int device_count;
} gk_xr_session;

/* 731: initialise the runtime (OpenXR-style) and begin a session */
gk_status gk_xr_init(gk_xr_session *s);
gk_status gk_xr_session_begin(gk_xr_session *s);
gk_status gk_xr_session_end(gk_xr_session *s);
gk_status gk_xr_frame_begin(gk_xr_session *s, double predicted_time);

/* register a tracked device (headset / controller / glasses / phone) */
int gk_xr_device_add(gk_xr_session *s, gk_xr_device_type type,
                     const char *name);
gk_status gk_xr_device_set_pose(gk_xr_session *s, int id,
                                gk_xr_vec3 pos, gk_xr_quat rot);
gk_status gk_xr_device_disconnect(gk_xr_session *s, int id);
gk_xr_device *gk_xr_device_find(gk_xr_session *s, int id);
int gk_xr_device_count(const gk_xr_session *s, gk_xr_device_type type);
/* locate a device in the other eye / shared space relative to headset */
gk_status gk_xr_locate_relative(const gk_xr_session *s, int device_id,
                                gk_xr_vec3 *out_pos);

/* ---- 733 VR controller input / 742 Xbox / 743 PS pad ---- */

typedef enum {
    GK_XR_PAD_XBOX = 0,   /* 742 */
    GK_XR_PAD_PS,         /* 743 */
    GK_XR_PAD_VR,         /* 733 */
    GK_XR_PAD_GENERIC,
    GK_XR_PAD_COUNT
} gk_xr_pad_type;

const char *gk_xr_pad_name(gk_xr_pad_type t);

typedef struct {
    gk_xr_pad_type type;
    float axes[6];       /* thumbsticks + triggers */
    int axis_count;
    unsigned buttons;    /* bitfield */
    unsigned prev_buttons;
    float trigger_l;
    float trigger_r;
    float rumble_l;
    float rumble_r;
} gk_xr_gamepad;

void gk_xr_gamepad_init(gk_xr_gamepad *p, gk_xr_pad_type type);
gk_status gk_xr_gamepad_set_axis(gk_xr_gamepad *p, int index, float value);
gk_status gk_xr_gamepad_set_button(gk_xr_gamepad *p, int index, int down);
int gk_xr_gamepad_button_down(const gk_xr_gamepad *p, int index);
int gk_xr_gamepad_button_pressed(const gk_xr_gamepad *p, int index);
int gk_xr_gamepad_button_released(const gk_xr_gamepad *p, int index);
void gk_xr_gamepad_set_rumble(gk_xr_gamepad *p, float left, float right);
/* advance the edge-detection state for the next frame */
void gk_xr_gamepad_next_frame(gk_xr_gamepad *p);

/* ---- 734 AR overlay / 736 phone AR ---- */

typedef struct {
    int id;
    char label[GK_XR_NAME];
    gk_xr_vec3 world_pos;
    float size;
    int visible;
} gk_xr_anchor;

typedef struct {
    gk_xr_anchor anchors[GK_XR_MAX_ANCHORS];
    int count;
    int overlay_enabled;
} gk_xr_ar_layer;

void gk_xr_ar_init(gk_xr_ar_layer *a);
int gk_xr_ar_anchor_add(gk_xr_ar_layer *a, const char *label,
                        gk_xr_vec3 pos, float size);
gk_status gk_xr_ar_anchor_set_visible(gk_xr_ar_layer *a, int id, int visible);
/* project a world anchor to normalised screen coords of a given viewport;
   returns 0 if the anchor is behind the camera */
int gk_xr_ar_project(const gk_xr_ar_layer *a, int id, float fov_deg, float aspect,
                     float *out_x, float *out_y);
/* phone-AR: estimate pose from a planar marker of known size */
gk_status gk_xr_phone_ar_pose(float marker_pixel_size, float known_size_mm,
                              float focal_px, gk_xr_vec3 *out_translation);

/* ---- 737 haptics / 738 force handwheel / 739 force pad ---- */

typedef struct {
    float frequency_hz;
    float amplitude;    /* 0..1 */
    float duration_s;
} gk_xr_haptic_pulse;

typedef struct {
    float motor_l;
    float motor_r;
    int active;
    float remaining_s;
    gk_xr_haptic_pulse current;
} gk_xr_haptic;

void gk_xr_haptic_init(gk_xr_haptic *h);
gk_status gk_xr_haptic_play(gk_xr_haptic *h, float freq, float amp,
                            float duration);
gk_status gk_xr_haptic_update(gk_xr_haptic *h, float dt);
int gk_xr_haptic_active(const gk_xr_haptic *h);

/* 738/739: force feedback wheel/pad modelled by a spring-damper */
typedef struct {
    float inertia;
    float damping;
    float stiffness;
    float angle;
    float omega;
    float torque_limit;
} gk_xr_force_feedback;

void gk_xr_force_feedback_init(gk_xr_force_feedback *f, float inertia,
                               float damping, float stiffness);
gk_status gk_xr_force_feedback_step(gk_xr_force_feedback *f, float applied,
                                    float dt);
/* detent torque: snaps to the nearest of `detents` positions */
float gk_xr_force_feedback_detent(const gk_xr_force_feedback *f, int detents,
                                  float strength);

/* ---- 740 HRTF / 741 3D positional audio ---- */

typedef struct {
    float azimuth_deg;
    float elevation_deg;
    float itd_ms;      /* interaural time difference */
    float ild_db;      /* interaural level difference */
    float distance_m;
} gk_xr_hrtf;

void gk_xr_hrtf_compute(gk_xr_hrtf *h, float listener_x, float listener_z,
                        float source_x, float source_z, float source_y);

typedef struct {
    float gain;
    float delay_left_ms;
    float delay_right_ms;
    float distance;
} gk_xr_spatial_audio;

void gk_xr_spatial_audio_compute(gk_xr_spatial_audio *out,
                                 const gk_xr_hrtf *h, float gain_at_1m,
                                 float ref_distance);

/* ---- 744 real handwheel / 745 USB handwheel ---- */

typedef struct {
    int connected;
    unsigned vendor_id;
    unsigned product_id;
    int axis_count;
    double counts_per_rev;
    double position;
    double last_position;
    int has_baseline;
    float axis[GK_XR_MAX_CHANNELS];
    int emergency_stop;
} gk_xr_handwheel;

void gk_xr_handwheel_init(gk_xr_handwheel *h, double counts_per_rev);
gk_status gk_xr_handwheel_connect(gk_xr_handwheel *h, unsigned vid,
                                  unsigned pid);
gk_status gk_xr_handwheel_report(gk_xr_handwheel *h, const float *axes,
                                 int axes_count);
/* signed angular delta (degrees) since the previous report */
double gk_xr_handwheel_delta_deg(gk_xr_handwheel *h);
int gk_xr_handwheel_is_usb(const gk_xr_handwheel *h);

/* ---- 746 multi-screen sync ---- */

typedef enum {
    GK_XR_SCREEN_MASTER = 0,
    GK_XR_SCREEN_SLAVE,
    GK_XR_SCREEN_MIRROR
} gk_xr_screen_role;

typedef struct {
    int id;
    gk_xr_screen_role role;
    int width;
    int height;
    double latency_ms;
} gk_xr_screen;

typedef struct {
    gk_xr_screen screens[GK_XR_MAX_DEVICES];
    int count;
    int swap_barrier;
} gk_xr_multiscreen;

void gk_xr_multiscreen_init(gk_xr_multiscreen *m);
int gk_xr_multiscreen_add(gk_xr_multiscreen *m, gk_xr_screen_role role,
                          int width, int height);
gk_status gk_xr_multiscreen_sync(gk_xr_multiscreen *m, double frame_time);
double gk_xr_multiscreen_max_latency(const gk_xr_multiscreen *m);

/* ---- 747 eye tracking ---- */

typedef struct {
    float x, y;
    int valid;
} gk_xr_gaze_point;

typedef struct {
    gk_xr_gaze_point points[GK_XR_MAX_GAZE_POINTS];
    int count;
    float fixation_x;
    float fixation_y;
    float dwell_s;
    int blink;
} gk_xr_eye_tracker;

void gk_xr_eye_init(gk_xr_eye_tracker *e);
gk_status gk_xr_eye_add_point(gk_xr_eye_tracker *e, float x, float y);
gk_status gk_xr_eye_update(gk_xr_eye_tracker *e, double dt);
/* returns the anchor id being fixated on for at least `dwell` seconds, or -1 */
int gk_xr_eye_select(const gk_xr_eye_tracker *e, float threshold, double dwell);

/* ---- 748 EEG headband ---- */

typedef enum {
    GK_XR_EEG_DELTA = 0,
    GK_XR_EEG_THETA,
    GK_XR_EEG_ALPHA,
    GK_XR_EEG_BETA,
    GK_XR_EEG_GAMMA,
    GK_XR_EEG_BAND_COUNT
} gk_xr_eeg_band;

const char *gk_xr_eeg_band_name(gk_xr_eeg_band b);

typedef struct {
    float bands[GK_XR_EEG_BAND_COUNT];
    float attention;   /* 0..1 */
    float meditation;  /* 0..1 */
    int signal_quality;
} gk_xr_eeg;

void gk_xr_eeg_init(gk_xr_eeg *e);
gk_status gk_xr_eeg_set_band(gk_xr_eeg *e, gk_xr_eeg_band b, float value);
gk_status gk_xr_eeg_evaluate(gk_xr_eeg *e);
/* cognitive load from beta/(alpha+theta) ratio */
float gk_xr_eeg_cognitive_load(const gk_xr_eeg *e);

/* ---- 749 heart-rate strap ---- */

typedef struct {
    int connected;
    int samples;
    double last_beat_s;
    double avg_bpm;
    int bpm;
    double hrv_ms;
} gk_xr_heart;

void gk_xr_heart_init(gk_xr_heart *h);
gk_status gk_xr_heart_connect(gk_xr_heart *h);
gk_status gk_xr_heart_report(gk_xr_heart *h, int bpm, double rr_interval_ms);
/* stress index derived from elevated heart rate and low HRV */
float gk_xr_heart_stress(const gk_xr_heart *h);

#ifdef __cplusplus
}
#endif

#endif /* GK_XR_H */
