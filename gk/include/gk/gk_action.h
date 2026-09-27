#ifndef GK_ACTION_H
#define GK_ACTION_H

#include <stddef.h>
#include "gk/gk_error.h"
#include "gk/gk_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- spindle (232-236) ---- */

typedef struct {
    double target_rpm;
    double current_rpm;
    double accel_rate;   /* rpm per second ramp-up */
    double decel_rate;   /* rpm per second ramp-down */
    int direction;       /* +1 forward, -1 reverse, 0 stopped */
    int orient;          /* 1 if oriented */
    int orient_angle;    /* degrees 0/90/180/270 */
    int gear;            /* 1..4 spindle gear range */
} gk_spindle_axis;

void gk_spindle_init(gk_spindle_axis *s);
gk_status gk_spindle_start(gk_spindle_axis *s, double rpm, int direction);
gk_status gk_spindle_stop(gk_spindle_axis *s);
/* Advance the ramp model by dt seconds. Returns current rpm. */
double gk_spindle_update(gk_spindle_axis *s, double dt);
int gk_spindle_is_up_to_speed(const gk_spindle_axis *s, double tol);
/* Spindle gear selection based on the desired rpm. */
gk_status gk_spindle_select_gear(gk_spindle_axis *s, double rpm);
/* Oriented spindle stop. */
gk_status gk_spindle_orient(gk_spindle_axis *s, int angle_deg);

/* ---- tool magazine / ATC (237-244) ---- */

typedef enum {
    GK_MAGAZINE_DISC = 0,   /* 240 disc / umbrella tool magazine */
    GK_MAGAZINE_UMBRELLA,   /* 241 */
    GK_MAGAZINE_CHAIN       /* 242 chain */
} gk_magazine_kind;

const char *gk_magazine_kind_name(gk_magazine_kind k);

typedef enum {
    GK_ATC_IDLE = 0,
    GK_ATC_UNCLAMP,
    GK_ATC_TOOL_OUT,
    GK_ATC_ROTATE,
    GK_ATC_TOOL_IN,
    GK_ATC_CLAMP,
    GK_ATC_DONE,
    GK_ATC_ERROR
} gk_atc_state;

const char *gk_atc_state_name(gk_atc_state s);

typedef struct {
    gk_magazine_kind kind;
    int capacity;
    int current_pocket;   /* 0..capacity-1 */
    int target_pocket;
    double index_time;    /* seconds per division */
    double index_angle;   /* degrees, accumulated */
} gk_magazine;

void gk_magazine_init(gk_magazine *m, gk_magazine_kind kind, int capacity);
gk_status gk_magazine_select(gk_magazine *m, int pocket);
/* Rotate one step toward target; returns the pocket angle advanced. */
double gk_magazine_index_step(gk_magazine *m, double dt);
int gk_magazine_at_target(const gk_magazine *m);

typedef struct {
    gk_atc_state state;
    int spindle_tool;     /* tool in spindle, 0 = empty */
    int standby_tool;     /* tool in the changer arm */
    double progress;      /* 0..1 within the current step */
    double arm_angle;     /* degrees */
    int manual;           /* manual tool change mode (244) */
} gk_atc;

void gk_atc_init(gk_atc *a);
/* Start the automatic tool change sequence for pocket tool `tool`. */
gk_status gk_atc_start(gk_atc *a, int tool);
/* Advance the state machine by dt seconds. Returns current state. */
gk_atc_state gk_atc_update(gk_atc *a, double dt);
/* Manual tool change (item 244). */
gk_status gk_atc_manual_change(gk_atc *a, int new_tool);

/* Tool setting (245-247). */
typedef enum {
    GK_PROBE_CONTACT = 0,  /* 246 contact tool setter */
    GK_PROBE_LASER,        /* 247 laser tool setter */
    GK_PROBE_SPINDLE        /* 245 on-machine touch probe */
} gk_probe_kind;

typedef struct {
    gk_probe_kind kind;
    double tip_diameter;
    double resolution;     /* mm */
    double trigger_x;
    double trigger_y;
    double trigger_z;
} gk_tool_setter;

void gk_tool_setter_init(gk_tool_setter *t, gk_probe_kind kind);
/* Simulate a touch: returns measured offset along the axis (mm). */
double gk_tool_setter_measure(const gk_tool_setter *t, int axis, double approach);

/* ---- table / tailstock / steady (248-251) ---- */

typedef struct {
    double rotation;       /* degrees, rotary table (248) */
    double tilt;           /* degrees, tilting table (249) */
    double max_tilt;
} gk_table;

void gk_table_init(gk_table *t);
gk_status gk_table_rotate(gk_table *t, double delta_deg);
gk_status gk_table_tilt(gk_table *t, double angle_deg);

typedef struct {
    double position;       /* mm along Z (250) */
    double min_pos;
    double max_pos;
    int clamped;
} gk_tailstock;

void gk_tailstock_init(gk_tailstock *t);
gk_status gk_tailstock_move(gk_tailstock *t, double to_mm);
gk_status gk_tailstock_clamp(gk_tailstock *t, int clamped);

typedef struct {
    double open;           /* mm opening diameter (251) */
    int engaged;
} gk_steady_rest;

void gk_steady_rest_init(gk_steady_rest *r);
gk_status gk_steady_rest_engage(gk_steady_rest *r, double diameter);
gk_status gk_steady_rest_release(gk_steady_rest *r);

/* ---- auxiliaries (252-257) ---- */

typedef struct {
    double speed;          /* rpm of conveyor (252) */
    double running_time;
} gk_chip_conveyor;

void gk_chip_conveyor_init(gk_chip_conveyor *c);
void gk_chip_conveyor_set(gk_chip_conveyor *c, double rpm);
double gk_chip_conveyor_update(gk_chip_conveyor *c, double dt);

typedef struct {
    int open;
    double position;       /* 0 closed .. 1 open */
    double speed;          /* fraction per second */
} gk_auto_door;

void gk_auto_door_init(gk_auto_door *d);
gk_status gk_auto_door_set(gk_auto_door *d, int open);
double gk_auto_door_update(gk_auto_door *d, double dt);

typedef struct {
    int clamped;
    double clamp_force;    /* N */
    double stroke;         /* 0..1 */
} gk_auto_fixture;

void gk_auto_fixture_init(gk_auto_fixture *f);
gk_status gk_auto_fixture_set(gk_auto_fixture *f, int clamped, double force);

typedef struct {
    int on;
    double flow;
    gk_vec3 dir;
} gk_coolant_valve;

void gk_coolant_valve_init(gk_coolant_valve *v);
void gk_coolant_valve_set(gk_coolant_valve *v, int on, double flow);

typedef struct {
    int on;
    double pressure;       /* bar (256) */
} gk_air_blast;

void gk_air_blast_init(gk_air_blast *a);
void gk_air_blast_set(gk_air_blast *a, int on, double pressure);

typedef struct {
    int on;
    double intensity;      /* 0..1 (257) */
} gk_work_light;

void gk_work_light_init(gk_work_light *l);
void gk_work_light_set(gk_work_light *l, int on, double intensity);

/* ---- alarm beacon & buzzer (258-259) ---- */

typedef enum {
    GK_BEACON_OFF = 0,
    GK_BEACON_GREEN,
    GK_BEACON_YELLOW,
    GK_BEACON_RED
} gk_beacon_color;

const char *gk_beacon_color_name(gk_beacon_color c);

typedef struct {
    gk_beacon_color color;
    int blink;
    double blink_hz;
} gk_beacon;

void gk_beacon_init(gk_beacon *b);
void gk_beacon_set(gk_beacon *b, gk_beacon_color color, int blink);

typedef struct {
    int on;
    double frequency;      /* Hz */
    double volume;         /* 0..1 */
} gk_buzzer;

void gk_buzzer_init(gk_buzzer *z);
void gk_buzzer_set(gk_buzzer *z, int on, double hz, double volume);

#ifdef __cplusplus
}
#endif

#endif
