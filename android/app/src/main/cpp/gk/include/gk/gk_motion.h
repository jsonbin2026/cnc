#ifndef GK_MOTION_H
#define GK_MOTION_H

#include <stddef.h>
#include "gk/gk_error.h"
#include "gk/gk_math.h"
#include "gk/gk_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GK_ACCEL_TRAPEZOID = 0,
    GK_ACCEL_S_CURVE,
    GK_ACCEL_EXPONENTIAL
} gk_accel_profile;

typedef struct {
    double max_velocity;   /* units/s */
    double max_accel;      /* units/s^2 */
    double max_jerk;       /* units/s^3, used by S-curve */
    double max_travel;
    double min_travel;
    int    enabled;
} gk_axis_limit;

typedef struct {
    gk_axis_limit axes[GK_AXIS_COUNT];
    double rapid_feed;         /* rapid traverse rate, units/min */
    double feed_override;      /* 0.0 .. 2.0 */
    double rapid_override;     /* 0.0 .. 1.0 */
    double spindle_override;   /* 0.0 .. 2.0 */
    gk_accel_profile accel_profile;
    int dry_run;
    int machine_lock;
} gk_motion_config;

typedef struct {
    gk_point3 start;
    gk_point3 end;
    gk_point3 center;
    double radius;
    double feed;
    gk_motion_mode mode;
    gk_plane plane;
    int use_radius;      /* G02/G03 R form */
    int radius_negative; /* R < 0 selects the long arc */
    double normal_angle; /* path angle for circle */
} gk_move;

typedef struct {
    gk_point3 position;   /* commanded position */
    double path_position; /* distance travelled along the path */
    double velocity;      /* current path velocity */
    size_t step;
} gk_interp_sample;

typedef struct {
    gk_move move;
    double length;        /* total path length, arc length for arcs */
    double duration;      /* total time given accel profile and limits */
    double cruise_velocity;
    double accel_time;
    double cruise_time;
    double decel_time;
    double distance;
    size_t steps;
} gk_motion_plan;

void gk_motion_config_default(gk_motion_config *cfg);
gk_status gk_motion_config_validate(const gk_motion_config *cfg);

gk_status gk_move_linear(gk_move *m, gk_point3 from, gk_point3 to,
                         double feed, gk_motion_mode mode);
gk_status gk_move_arc_ijk(gk_move *m, gk_point3 from, gk_point3 to,
                          gk_point3 center, double feed, gk_motion_mode mode,
                          gk_plane plane);
gk_status gk_move_arc_radius(gk_move *m, gk_point3 from, gk_point3 to,
                             double radius, double feed,
                             gk_motion_mode mode, gk_plane plane);

double gk_move_length(const gk_move *m);
gk_status gk_plan_move(const gk_motion_config *cfg, const gk_move *m,
                       gk_motion_plan *out);
gk_status gk_plan_velocity_at(const gk_motion_plan *plan, double t,
                              double *out_velocity);
gk_status gk_plan_sample(const gk_motion_plan *plan, size_t step,
                         size_t total_steps, gk_interp_sample *out);
gk_status gk_interpolate_arc_point(const gk_move *m, double t,
                                   gk_point3 *out);

/* Axis scaling: returns the limiting velocity for a move. */
double gk_axis_limited_velocity(const gk_motion_config *cfg,
                                const gk_move *m, double feed);
int gk_axis_in_range(const gk_motion_config *cfg, gk_axis axis, double v);

/* Servo models */
typedef struct {
    double time_constant;   /* first order tau */
    double natural_freq;    /* second order wn */
    double damping;         /* second order zeta */
    double backlash;
    double pitch_error;
    double following_error;
} gk_servo_params;

typedef struct {
    gk_servo_params params;
    double position;
    double velocity;
    double internal;
    double backlash_state;
    int use_second_order;
} gk_servo;

void gk_servo_init(gk_servo *s, const gk_servo_params *p, int second_order);
double gk_servo_step(gk_servo *s, double command, double dt);
double gk_servo_following_error(const gk_servo *s, double command);
void gk_servo_compensate_backlash(gk_servo *s, double dir);

#ifdef __cplusplus
}
#endif

#endif
