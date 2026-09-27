#ifndef GK_UX_H
#define GK_UX_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_UX_MAX_ITEMS 64
#define GK_UX_NAME 64
#define GK_UX_TEXT 256

/* ---- 750-754 peripheral output devices ---- */

typedef struct {
    int connected;
    float target_temp;
    float current_temp;
    float heat_rate;   /* deg/s toward target */
    int active;
} gk_ux_thermal_glove;      /* 750 */

void gk_ux_glove_init(gk_ux_thermal_glove *g);
gk_status gk_ux_glove_set(gk_ux_thermal_glove *g, float temp);
gk_status gk_ux_glove_update(gk_ux_thermal_glove *g, float dt);

typedef struct {
    int cartridge;      /* scent id */
    float intensity;    /* 0..1 */
    float remaining_s;
} gk_ux_scent;              /* 751 */

void gk_ux_scent_init(gk_ux_scent *s);
gk_status gk_ux_scent_emit(gk_ux_scent *s, int cartridge, float intensity,
                           float duration);
gk_status gk_ux_scent_update(gk_ux_scent *s, float dt);
int gk_ux_scent_active(const gk_ux_scent *s);

typedef struct {
    int pressed;        /* 752 foot switch */
    unsigned switches;  /* bitfield of foot pedals */
} gk_ux_foot_switch;

void gk_ux_foot_switch_init(gk_ux_foot_switch *f);
gk_status gk_ux_foot_switch_set(gk_ux_foot_switch *f, int index, int down);
int gk_ux_foot_switch_down(const gk_ux_foot_switch *f, int index);

typedef struct {
    float nozzle_temp;      /* 753 3D print head */
    float feed_rate;        /* mm/s */
    float filament_used_mm;
    int extruding;
} gk_ux_print_head;

void gk_ux_print_head_init(gk_ux_print_head *p);
gk_status gk_ux_print_head_extrude(gk_ux_print_head *p, float feed_rate,
                                   float dt);

typedef struct {
    float amplitude;        /* 754 haptic seat */
    float frequency;
    int channels;
    int active;
} gk_ux_haptic_seat;

void gk_ux_haptic_seat_init(gk_ux_haptic_seat *s, int channels);
gk_status gk_ux_haptic_seat_pulse(gk_ux_haptic_seat *s, float amp, float freq);
gk_status gk_ux_haptic_seat_stop(gk_ux_haptic_seat *s);

/* ---- 755-759 sonification / cross-modal mapping ---- */

typedef enum {
    GK_UX_MAP_LOAD_TONE = 0,     /* 756 load -> pitch */
    GK_UX_MAP_TEMP_COLOR,        /* 757 temperature -> light colour */
    GK_UX_MAP_TEXTURE_HAPTIC,    /* 758 texture -> haptics */
    GK_UX_MAP_GCODE_MUSIC,       /* 759 G-code -> score */
    GK_UX_MAP_COUNT
} gk_ux_mapping_kind;

const char *gk_ux_mapping_name(gk_ux_mapping_kind k);

/* linear normalisation of a value within [lo, hi] to [0, 1] */
float gk_ux_normalize(float value, float lo, float hi);

/* 756: spindle/axis load maps to a musical pitch (Hz), pentatonic scale */
float gk_ux_load_to_pitch(float load);          /* load 0..1 -> Hz */
/* 757: temperature maps to an RGB colour (blue -> red) */
gk_status gk_ux_temp_to_color(float temp_c, float cold, float hot,
                              unsigned *out_rgb);
/* 758: surface roughness maps to a vibration frequency & amplitude */
gk_status gk_ux_texture_to_haptic(float roughness_um, float *out_freq,
                                  float *out_amp);
/* 759: a G-code letter maps to a musical note (semitone number, 0..11) */
int gk_ux_gcode_to_note(char letter);
/* 759: feed rate maps to a tempo in BPM */
float gk_ux_feed_to_tempo(float feed);

typedef struct {
    float last_pitch_hz;
    float last_tempo;
    unsigned last_color;
    int notes_played;
} gk_ux_sonifier;

void gk_ux_sonifier_init(gk_ux_sonifier *s);
/* convert a machine state into a sonification frame */
gk_status gk_ux_sonify(gk_ux_sonifier *s, float load, float temperature,
                       float feed);

/* ---- 760-762 explanation ---- */

typedef struct {
    char text[GK_UX_TEXT];
    int step;
} gk_ux_narration;

void gk_ux_narration_init(gk_ux_narration *n);
gk_status gk_ux_narration_say(gk_ux_narration *n, const char *fmt,
                              const char *subject);
/* 761: report which chart series should highlight for a given data point */
int gk_ux_chart_link(int series_count, int point_index, int *out_series);
/* 762: order animation keyframes from a duration and frame count */
int gk_ux_animation_frames(float duration_s, float fps, float *out_times,
                           int max_out);

/* ---- 763-768 presentation cues ---- */

typedef enum {
    GK_UX_CUE_ANNOTATION = 0,  /* 763 3D annotation */
    GK_UX_CUE_ARROW,           /* 764 arrow */
    GK_UX_CUE_HIGHLIGHT,       /* 765 highlight */
    GK_UX_CUE_SPEECH,          /* 766 speech */
    GK_UX_CUE_VIBRATION,       /* 767 vibration */
    GK_UX_CUE_LIGHT,           /* 768 light effect */
    GK_UX_CUE_COUNT
} gk_ux_cue_kind;

const char *gk_ux_cue_name(gk_ux_cue_kind k);

typedef struct {
    int id;
    gk_ux_cue_kind kind;
    float x, y, z;
    char text[GK_UX_TEXT];
    unsigned color;
    int priority;
    int active;
} gk_ux_cue;

typedef struct {
    gk_ux_cue cues[GK_UX_MAX_ITEMS];
    int count;
} gk_ux_cue_list;

void gk_ux_cue_list_init(gk_ux_cue_list *l);
int gk_ux_cue_add(gk_ux_cue_list *l, gk_ux_cue_kind kind, const char *text,
                  float x, float y, float z);
gk_status gk_ux_cue_set_active(gk_ux_cue_list *l, int id, int active);
/* highest-priority active cue of a given kind, or -1 */
int gk_ux_cue_highest(const gk_ux_cue_list *l, gk_ux_cue_kind kind);
/* 766: return the speech text for a cue, or NULL if not a speech cue */
const char *gk_ux_cue_speech(const gk_ux_cue_list *l, int id);

#ifdef __cplusplus
}
#endif

#endif /* GK_UX_H */
