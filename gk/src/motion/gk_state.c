#include "gk/gk_state.h"

#include <string.h>

void gk_state_init(gk_machine_state *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->plane = GK_PLANE_XY;
    s->unit = GK_UNIT_MM;
    s->distance = GK_DIST_ABSOLUTE;
    s->feed_mode = GK_FEED_PER_MIN;
    s->spindle_mode = GK_SPEED_CONST_RPM;
    s->motion = GK_MOTION_NONE;
    s->last_motion = GK_MOTION_NONE;
    s->feed = 0.0;
    s->spindle_rpm = 0.0;
    s->spindle_max_rpm = 24000.0;
    s->tool = 1;
}

void gk_state_reset(gk_machine_state *s)
{
    if (s == NULL) {
        return;
    }
    gk_state_init(s);
}

gk_status gk_state_select_plane(gk_machine_state *s, gk_plane plane)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if ((int)plane < 0 || (int)plane > (int)GK_PLANE_YZ) {
        return GK_ERR_OUT_OF_RANGE;
    }
    s->plane = plane;
    return GK_OK;
}

gk_status gk_state_set_unit(gk_machine_state *s, gk_unit unit)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (unit != GK_UNIT_MM && unit != GK_UNIT_INCH) {
        return GK_ERR_OUT_OF_RANGE;
    }
    s->unit = unit;
    return GK_OK;
}

gk_status gk_state_set_distance(gk_machine_state *s, gk_distance_mode mode)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (mode != GK_DIST_ABSOLUTE && mode != GK_DIST_INCREMENTAL) {
        return GK_ERR_OUT_OF_RANGE;
    }
    s->distance = mode;
    return GK_OK;
}

gk_status gk_state_set_feed_mode(gk_machine_state *s, gk_feed_mode mode)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (mode != GK_FEED_PER_MIN && mode != GK_FEED_PER_REV) {
        return GK_ERR_OUT_OF_RANGE;
    }
    s->feed_mode = mode;
    return GK_OK;
}

gk_status gk_state_set_work_offset(gk_machine_state *s, int gcode)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (gcode < 54 || gcode > 59) {
        return GK_ERR_OUT_OF_RANGE;
    }
    s->work_offset = gcode - 54;
    return GK_OK;
}

gk_status gk_state_set_spindle_mode(gk_machine_state *s, gk_spindle_mode m)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (m != GK_SPEED_CONST_SURFACE && m != GK_SPEED_CONST_RPM) {
        return GK_ERR_OUT_OF_RANGE;
    }
    s->spindle_mode = m;
    return GK_OK;
}

double gk_state_unit_scale(const gk_machine_state *s)
{
    if (s == NULL) {
        return 1.0;
    }
    return s->unit == GK_UNIT_INCH ? 25.4 : 1.0;
}

void gk_state_set_axis(gk_machine_state *s, gk_axis axis, double value)
{
    if (s == NULL || (int)axis < 0 || (int)axis >= (int)GK_AXIS_COUNT) {
        return;
    }
    s->coord[(int)axis] = value;
}

double gk_state_get_axis(const gk_machine_state *s, gk_axis axis)
{
    if (s == NULL || (int)axis < 0 || (int)axis >= (int)GK_AXIS_COUNT) {
        return 0.0;
    }
    return s->coord[(int)axis];
}
