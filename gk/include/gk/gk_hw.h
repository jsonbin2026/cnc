#ifndef GK_HW_H
#define GK_HW_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_HW_NAME 64

/* ===================================================================
 * Batch 39: machine hardware details (1001-1018)
 *            real machine actions (1019-1050)
 * =================================================================== */

/* 1001 buzzer */
typedef struct {
    double frequency_hz;
    double duration_s;
    int active;
} gk_hw_buzzer;

void gk_hw_buzzer_init(gk_hw_buzzer *b);
gk_status gk_hw_buzzer_beep(gk_hw_buzzer *b, double frequency_hz,
                            double duration_s);
double gk_hw_buzzer_remaining(const gk_hw_buzzer *b, double elapsed_s);

/* 1002 emergency stop button */
typedef struct {
    int pressed;
    int latched;
    double travel_mm;
} gk_hw_estop;

void gk_hw_estop_init(gk_hw_estop *e);
gk_status gk_hw_estop_press(gk_hw_estop *e, double force_n);
gk_status gk_hw_estop_release(gk_hw_estop *e, int twist);
int gk_hw_estop_engaged(const gk_hw_estop *e);

/* 1003 door handle */
typedef struct {
    double length_mm;
    double angle_deg;
    int open;
} gk_hw_handle;

void gk_hw_handle_init(gk_hw_handle *h, double length_mm);
gk_status gk_hw_handle_pull(gk_hw_handle *h, double angle_deg);

/* 1004 observation window */
typedef struct {
    double width_mm;
    double height_mm;
    double transparency;
} gk_hw_window;

void gk_hw_window_init(gk_hw_window *w, double width_mm, double height_mm);
double gk_hw_window_view_area(const gk_hw_window *w);

/* 1005 window wiper */
typedef struct {
    int running;
    double sweep_deg;
    unsigned int strokes;
} gk_hw_wiper;

void gk_hw_wiper_init(gk_hw_wiper *w);
gk_status gk_hw_wiper_start(gk_hw_wiper *w);
gk_status gk_hw_wiper_stroke(gk_hw_wiper *w);
gk_status gk_hw_wiper_stop(gk_hw_wiper *w);

/* 1006 door lock */
typedef struct {
    int locked;
    char key_id[GK_HW_NAME];
} gk_hw_lock;

void gk_hw_lock_init(gk_hw_lock *l, const char *key_id);
gk_status gk_hw_lock_engage(gk_hw_lock *l);
gk_status gk_hw_lock_disengage(gk_hw_lock *l, const char *key_id);

/* 1007 door magnetic switch */
typedef struct {
    int closed;
    int triggered;
} gk_hw_door_sensor;

void gk_hw_door_sensor_init(gk_hw_door_sensor *s);
gk_status gk_hw_door_sensor_update(gk_hw_door_sensor *s, int closed);

/* 1008 safety door switch */
typedef struct {
    int channel_count;
    int channel_a;
    int channel_b;
} gk_hw_safety_switch;

void gk_hw_safety_switch_init(gk_hw_safety_switch *s);
gk_status gk_hw_safety_switch_set(gk_hw_safety_switch *s, int channel_a,
                                  int channel_b);
int gk_hw_safety_switch_consistent(const gk_hw_safety_switch *s);
int gk_hw_safety_switch_ok(const gk_hw_safety_switch *s);

/* 1009-1011 enclosure panels (side / rear / top) */
typedef enum {
    GK_HW_PANEL_SIDE = 0,   /* 1009 */
    GK_HW_PANEL_REAR,       /* 1010 */
    GK_HW_PANEL_TOP         /* 1011 */
} gk_hw_panel_kind;

const char *gk_hw_panel_name(gk_hw_panel_kind k);

typedef struct {
    gk_hw_panel_kind kind;
    double width_mm;
    double height_mm;
    double thickness_mm;
    int attached;
} gk_hw_panel;

void gk_hw_panel_init(gk_hw_panel *p, gk_hw_panel_kind kind, double width_mm,
                      double height_mm, double thickness_mm);
double gk_hw_panel_mass(const gk_hw_panel *p, double density_kg_m3);
gk_status gk_hw_panel_attach(gk_hw_panel *p, int attached);

/* 1012 base */
typedef struct {
    double length_mm;
    double width_mm;
    double height_mm;
} gk_hw_base;

void gk_hw_base_init(gk_hw_base *b, double length_mm, double width_mm,
                     double height_mm);
double gk_hw_base_footprint(const gk_hw_base *b);

/* 1013 machine feet */
typedef struct {
    int count;
    double diameter_mm;
    double load_capacity_kg;
} gk_hw_feet;

void gk_hw_feet_init(gk_hw_feet *f, int count);
int gk_hw_feet_can_support(const gk_hw_feet *f, double machine_mass_kg);

/* 1014 anti-vibration pad */
typedef struct {
    double stiffness_n_mm;
    double damping_ratio;
    double natural_hz;
} gk_hw_pad;

void gk_hw_pad_init(gk_hw_pad *p, double stiffness_n_mm, double damping_ratio);
double gk_hw_pad_transmissibility(const gk_hw_pad *p, double forcing_hz);

/* 1015 levelling bolt */
typedef struct {
    double thread_pitch_mm;
    double turns;
    double height_offset_mm;
} gk_hw_level_bolt;

void gk_hw_level_bolt_init(gk_hw_level_bolt *b, double thread_pitch_mm);
double gk_hw_level_bolt_turn(gk_hw_level_bolt *b, double turns);

/* 1016 lifting eye */
typedef struct {
    double thread_size_mm;
    double rated_load_kg;
    int count;
} gk_hw_lifting_eye;

void gk_hw_lifting_eye_init(gk_hw_lifting_eye *e, double thread_size_mm,
                            double rated_load_kg);
int gk_hw_lifting_eye_ok(const gk_hw_lifting_eye *e, double machine_mass_kg);

/* 1017 forklift pocket */
typedef struct {
    double width_mm;
    double height_mm;
    double depth_mm;
    int count;
} gk_hw_fork_pocket;

void gk_hw_fork_pocket_init(gk_hw_fork_pocket *p, double width_mm,
                            double height_mm, double depth_mm);
int gk_hw_fork_pocket_accepts(const gk_hw_fork_pocket *p, double fork_width_mm,
                              double fork_thickness_mm);

/* 1018 nameplate bracket */
typedef struct {
    double width_mm;
    double height_mm;
    double angle_deg;
    int locked;
} gk_hw_plate_bracket;

void gk_hw_plate_bracket_init(gk_hw_plate_bracket *b, double width_mm,
                              double height_mm);
gk_status gk_hw_plate_bracket_tilt(gk_hw_plate_bracket *b, double angle_deg);

/* ===================================================================
 * Real machine actions (1019-1050)
 * =================================================================== */

/* 1019-1023 spindle behaviour */
typedef struct {
    double speed_rpm;
    double target_rpm;
    double inertia_kgm2;
    double ramp_s;
    int running;
    int airborne;
} gk_hw_spindle;

void gk_hw_spindle_init(gk_hw_spindle *s);
gk_status gk_hw_spindle_start(gk_hw_spindle *s, double target_rpm,
                              double ramp_s);
gk_status gk_hw_spindle_stop(gk_hw_spindle *s);
double gk_hw_spindle_start_jitter(const gk_hw_spindle *s);
double gk_hw_spindle_coast_distance(const gk_hw_spindle *s, double torque_nm);
gk_status gk_hw_spindle_shift(gk_hw_spindle *s, int gear);
gk_status gk_hw_spindle_orient(gk_hw_spindle *s);
gk_status gk_hw_spindle_air_blast(gk_hw_spindle *s);

/* 1024-1025 taper / pull stud */
typedef struct {
    double drawbar_force_n;
    int clamped;
} gk_hw_drawbar;

void gk_hw_drawbar_init(gk_hw_drawbar *d, double drawbar_force_n);
gk_status gk_hw_drawbar_clamp(gk_hw_drawbar *d);
gk_status gk_hw_drawbar_release(gk_hw_drawbar *d);

/* 1026-1029 tool change arm */
typedef enum {
    GK_HW_ATC_IDLE = 0,
    GK_HW_ATC_EXTEND,
    GK_HW_ATC_GRIP,
    GK_HW_ATC_ROTATE,
    GK_HW_ATC_RETRACT,
    GK_HW_ATC_DONE,
    GK_HW_ATC_FAULT
} gk_hw_atc_state;

const char *gk_hw_atc_state_name(gk_hw_atc_state s);

typedef struct {
    gk_hw_atc_state state;
    int tool_current;
    int tool_target;
    double arm_angle_deg;
    int gripped;
} gk_hw_atc;

void gk_hw_atc_init(gk_hw_atc *a);
gk_status gk_hw_atc_request(gk_hw_atc *a, int tool_target);
gk_status gk_hw_atc_step(gk_hw_atc *a);
gk_status gk_hw_atc_confirm(gk_hw_atc *a);

/* 1030-1034 tool magazine */
typedef struct {
    int slots;
    int position;
    double index_time_s;
    int locked;
    int count;
} gk_hw_magazine;

void gk_hw_magazine_init(gk_hw_magazine *m, int slots);
gk_status gk_hw_magazine_index(gk_hw_magazine *m, int slot);
gk_status gk_hw_magazine_lock(gk_hw_magazine *m);
int gk_hw_magazine_count(const gk_hw_magazine *m);

/* 1035-1037 tool setter */
typedef struct {
    double contact_force_n;
    double retract_mm;
    int signalled;
    double measured_mm;
} gk_hw_setter;

void gk_hw_setter_init(gk_hw_setter *s);
gk_status gk_hw_setter_contact(gk_hw_setter *s, double measured_mm);
gk_status gk_hw_setter_retract(gk_hw_setter *s);
double gk_hw_setter_offset(const gk_hw_setter *s, double nominal_mm);

/* 1038-1040 workpiece clamping */
typedef struct {
    double clamp_force_n;
    double required_n;
    int clamped;
} gk_hw_clamp;

void gk_hw_clamp_init(gk_hw_clamp *c, double required_n);
gk_status gk_hw_clamp_close(gk_hw_clamp *c, double force_n);
gk_status gk_hw_clamp_open(gk_hw_clamp *c);
int gk_hw_clamp_ok(const gk_hw_clamp *c);

/* 1041-1042 tailstock */
typedef struct {
    double quill_mm;
    double pressure_n;
    int engaged;
} gk_hw_tailstock;

void gk_hw_tailstock_init(gk_hw_tailstock *t);
gk_status gk_hw_tailstock_advance(gk_hw_tailstock *t, double quill_mm,
                                  double pressure_n);
gk_status gk_hw_tailstock_retract(gk_hw_tailstock *t);

/* 1043-1044 steady rest */
typedef struct {
    int jaws;
    double jaw_force_n;
    int clamped;
} gk_hw_steady;

void gk_hw_steady_init(gk_hw_steady *s);
gk_status gk_hw_steady_clamp(gk_hw_steady *s, double jaw_force_n);
gk_status gk_hw_steady_release(gk_hw_steady *s);

/* 1045-1047 chip conveyor */
typedef enum {
    GK_HW_CHIP_STOPPED = 0,
    GK_HW_CHIP_FORWARD,
    GK_HW_CHIP_REVERSE
} gk_hw_chip_state;

const char *gk_hw_chip_state_name(gk_hw_chip_state s);

typedef struct {
    gk_hw_chip_state state;
    double speed_m_min;
} gk_hw_conveyor;

void gk_hw_conveyor_init(gk_hw_conveyor *c);
gk_status gk_hw_conveyor_start(gk_hw_conveyor *c, double speed_m_min);
gk_status gk_hw_conveyor_stop(gk_hw_conveyor *c);
gk_status gk_hw_conveyor_reverse(gk_hw_conveyor *c);

/* 1048-1050 coolant */
typedef struct {
    int running;
    double flow_lpm;
    double target_flow_lpm;
} gk_hw_coolant;

void gk_hw_coolant_init(gk_hw_coolant *c);
gk_status gk_hw_coolant_start(gk_hw_coolant *c);
gk_status gk_hw_coolant_stop(gk_hw_coolant *c);
gk_status gk_hw_coolant_set_flow(gk_hw_coolant *c, double flow_lpm);

/* 1051 air blow gun */
gk_status gk_hw_air_gun_blow(double pressure_mpa, double duration_s,
                             double *chips_removed);

/* 1052-1055 automatic door */
typedef struct {
    double open_pct;
    double buffer_pct;
    int anti_pinch;
    int blocked;
} gk_hw_auto_door;

void gk_hw_auto_door_init(gk_hw_auto_door *d);
gk_status gk_hw_auto_door_open(gk_hw_auto_door *d, double target_pct);
gk_status gk_hw_auto_door_close(gk_hw_auto_door *d, double target_pct);
int gk_hw_auto_door_stop_on_obstacle(gk_hw_auto_door *d, double obstacle_pct);

/* 1056-1058 rotary table */
typedef struct {
    double angle_deg;
    int locked;
    double max_speed_rpm;
} gk_hw_rotary;

void gk_hw_rotary_init(gk_hw_rotary *r, double max_speed_rpm);
gk_status gk_hw_rotary_rotate(gk_hw_rotary *r, double delta_deg);
gk_status gk_hw_rotary_lock(gk_hw_rotary *r);
gk_status gk_hw_rotary_unlock(gk_hw_rotary *r);

/* 1059-1060 indexing table */
typedef struct {
    int stations;
    int position;
    double index_time_s;
} gk_hw_indexer;

const char *gk_hw_indexer_name(void);

void gk_hw_indexer_init(gk_hw_indexer *i, int stations);
gk_status gk_hw_indexer_index(gk_hw_indexer *i, int station);
gk_status gk_hw_indexer_lock(gk_hw_indexer *i);

#ifdef __cplusplus
}
#endif

#endif /* GK_HW_H */
