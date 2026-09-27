#ifndef GK_SVO_H
#define GK_SVO_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===================================================================
 * Batch 41: real servo details (1091-1120)
 * Prefix: gk_svo_
 * =================================================================== */

/* 1091-1096 servo state, alarms and protection */
typedef enum {
    GK_SVO_OFF = 0,       /* 1091 powered down */
    GK_SVO_READY,         /* 1092 enabled */
    GK_SVO_RUNNING,
    GK_SVO_ALARM,         /* 1093 */
    GK_SVO_OVERLOAD,      /* 1094 */
    GK_SVO_OVERHEAT,      /* 1095 */
    GK_SVO_ENCODER_FAULT  /* 1096 */
} gk_svo_state;

const char *gk_svo_state_name(gk_svo_state s);

typedef struct {
    gk_svo_state state;
    double temperature_c;
    double max_temperature_c;
    double load_pct;
    double max_load_pct;
    int enabled;
} gk_svo_axis;

void gk_svo_axis_init(gk_svo_axis *a);
gk_status gk_svo_power_on(gk_svo_axis *a);
gk_status gk_svo_enable(gk_svo_axis *a);
gk_status gk_svo_disable(gk_svo_axis *a);
gk_status gk_svo_set_load(gk_svo_axis *a, double load_pct);
gk_status gk_svo_set_temperature(gk_svo_axis *a, double temperature_c);
gk_status gk_svo_encoder_alarm(gk_svo_axis *a);
gk_status gk_svo_reset_alarm(gk_svo_axis *a);
int gk_svo_in_alarm(const gk_svo_axis *a);

/* 1097-1099 current / speed / position waveforms */
typedef struct {
    double amplitude;
    double frequency_hz;
    double offset;
} gk_svo_wave;

void gk_svo_wave_init(gk_svo_wave *w, double amplitude, double frequency_hz,
                      double offset);
double gk_svo_wave_at(const gk_svo_wave *w, double t);

/* 1100-1103 three-loop control */
typedef struct {
    double kp;
    double ki;
    double kd;
    double integral;
    double previous_error;
} gk_svo_pid;

void gk_svo_pid_init(gk_svo_pid *p, double kp, double ki, double kd);
double gk_svo_pid_step(gk_svo_pid *p, double error, double dt);
void gk_svo_pid_reset(gk_svo_pid *p);

typedef struct {
    gk_svo_pid position;
    gk_svo_pid velocity;
    gk_svo_pid current;
    double position_command;
    double position_feedback;
} gk_svo_loop;

void gk_svo_loop_init(gk_svo_loop *l);
double gk_svo_loop_step(gk_svo_loop *l, double dt);
double gk_svo_current_loop(const gk_svo_loop *l, double error);
double gk_svo_velocity_loop(const gk_svo_loop *l, double error);
double gk_svo_position_loop(const gk_svo_loop *l, double error);

/* 1105-1106 feedforward and friction compensation */
typedef struct {
    double velocity_ff_gain;
    double accel_ff_gain;
    double friction_coulomb;
    double friction_viscous;
} gk_svo_comp;

void gk_svo_comp_init(gk_svo_comp *c);
double gk_svo_feedforward(const gk_svo_comp *c, double velocity, double accel);
double gk_svo_friction_comp(const gk_svo_comp *c, double velocity);

/* 1107 backlash compensation */
typedef struct {
    double backlash_mm;
    double last_dir;
    int applied;
} gk_svo_backlash;

void gk_svo_backlash_init(gk_svo_backlash *b, double backlash_mm);
double gk_svo_backlash_compensate(gk_svo_backlash *b, double direction);

/* 1108 pitch error compensation */
typedef struct {
    int points;
    double pitch_mm;
    double error_per_mm;
} gk_svo_pitch;

void gk_svo_pitch_init(gk_svo_pitch *p, int points, double pitch_mm);
double gk_svo_pitch_compensate(const gk_svo_pitch *p, int index);
double gk_svo_pitch_total(const gk_svo_pitch *p);

/* 1109-1110 straightness / squareness compensation */
typedef struct {
    double deviation_mm;
    double travel_mm;
} gk_svo_geom;

void gk_svo_geom_init(gk_svo_geom *g, double travel_mm);
double gk_svo_straightness_comp(gk_svo_geom *g, double deviation_mm);
double gk_svo_squareness_comp(gk_svo_geom *g, double angle_err_mm);

/* 1111 thermal deformation compensation */
typedef struct {
    double coefficient;
    double reference_c;
} gk_svo_thermal_comp;

void gk_svo_thermal_comp_init(gk_svo_thermal_comp *t, double coefficient,
                              double reference_c);
double gk_svo_thermal_compensate(const gk_svo_thermal_comp *t,
                                 double temperature_c);

/* 1112-1113 gain / rigidity */
typedef struct {
    double gain;
    double rigidity;
} gk_svo_tuning;

void gk_svo_tuning_init(gk_svo_tuning *t);
double gk_svo_set_gain(gk_svo_tuning *t, double gain);
double gk_svo_set_rigidity(gk_svo_tuning *t, double rigidity);

/* 1114-1117 vibration suppression / filters */
typedef struct {
    double frequency_hz;
    double depth;
    double width_hz;
} gk_svo_notch;

void gk_svo_notch_init(gk_svo_notch *n);
double gk_svo_notch_response(const gk_svo_notch *n, double frequency_hz);
int gk_svo_notch_suppressed(const gk_svo_notch *n, double vibration);

typedef struct {
    double cutoff_hz;
    double alpha;
    double state;
} gk_svo_lowpass;

void gk_svo_lowpass_init(gk_svo_lowpass *l, double cutoff_hz, double sample_hz);
double gk_svo_lowpass_apply(gk_svo_lowpass *l, double sample);

typedef struct {
    double previous;
    double alpha;
} gk_svo_smooth;

void gk_svo_smooth_init(gk_svo_smooth *s, double alpha);
double gk_svo_smooth_apply(gk_svo_smooth *s, double value);

/* 1118-1120 accel / velocity / torque feedforward */
typedef struct {
    double accel_ff;      /* 1118 */
    double velocity_ff;   /* 1119 */
    double torque_ff;     /* 1120 */
} gk_svo_ff;

void gk_svo_ff_init(gk_svo_ff *f);
double gk_svo_ff_accel(gk_svo_ff *f, double accel, double gain);
double gk_svo_ff_velocity(gk_svo_ff *f, double velocity, double gain);
double gk_svo_ff_torque(gk_svo_ff *f, double accel, double inertia);

#ifdef __cplusplus
}
#endif

#endif /* GK_SVO_H */
