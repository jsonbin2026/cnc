#ifndef GK_STATE_H
#define GK_STATE_H

#include "gk/gk_error.h"
#include "gk/gk_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GK_AXIS_X = 0,
    GK_AXIS_Y,
    GK_AXIS_Z,
    GK_AXIS_A,
    GK_AXIS_B,
    GK_AXIS_C,
    GK_AXIS_COUNT
} gk_axis;

typedef enum {
    GK_PLANE_XY = 0,
    GK_PLANE_ZX,
    GK_PLANE_YZ
} gk_plane;

typedef enum {
    GK_DIST_ABSOLUTE = 0,
    GK_DIST_INCREMENTAL
} gk_distance_mode;

typedef enum {
    GK_UNIT_MM = 0,
    GK_UNIT_INCH
} gk_unit;

typedef enum {
    GK_FEED_PER_MIN = 0,
    GK_FEED_PER_REV
} gk_feed_mode;

typedef enum {
    GK_SPEED_CONST_SURFACE = 0,
    GK_SPEED_CONST_RPM
} gk_spindle_mode;

typedef enum {
    GK_MOTION_NONE = 0,
    GK_MOTION_RAPID,
    GK_MOTION_LINEAR,
    GK_MOTION_CW,
    GK_MOTION_CCW
} gk_motion_mode;

typedef struct {
    double coord[GK_AXIS_COUNT];
    double feed;
    double spindle_rpm;
    double spindle_max_rpm;
    double surface_speed;
    int tool;
    int tool_offset;
    int coolant;
    int spindle_on;
    int spindle_dir;      /* +1 forward, -1 reverse, 0 stop */
    gk_plane plane;
    gk_unit unit;
    gk_distance_mode distance;
    gk_feed_mode feed_mode;
    gk_spindle_mode spindle_mode;
    gk_motion_mode motion;
    int work_offset;      /* 54..59 -> 0..5, 0 = none */
    gk_motion_mode last_motion;
} gk_machine_state;

void gk_state_init(gk_machine_state *s);
void gk_state_reset(gk_machine_state *s);

gk_status gk_state_select_plane(gk_machine_state *s, gk_plane plane);
gk_status gk_state_set_unit(gk_machine_state *s, gk_unit unit);
gk_status gk_state_set_distance(gk_machine_state *s, gk_distance_mode mode);
gk_status gk_state_set_feed_mode(gk_machine_state *s, gk_feed_mode mode);
gk_status gk_state_set_work_offset(gk_machine_state *s, int gcode);
gk_status gk_state_set_spindle_mode(gk_machine_state *s, gk_spindle_mode m);

double gk_state_unit_scale(const gk_machine_state *s);
void gk_state_set_axis(gk_machine_state *s, gk_axis axis, double value);
double gk_state_get_axis(const gk_machine_state *s, gk_axis axis);

#ifdef __cplusplus
}
#endif

#endif
