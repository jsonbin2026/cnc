#include "gk/gk_motion.h"

#include <math.h>
#include <string.h>

void gk_motion_config_default(gk_motion_config *cfg)
{
    int i;
    if (cfg == NULL) {
        return;
    }
    memset(cfg, 0, sizeof(*cfg));
    for (i = 0; i < (int)GK_AXIS_COUNT; ++i) {
        cfg->axes[i].max_velocity = 10000.0;
        cfg->axes[i].max_accel = 1000.0;
        cfg->axes[i].max_jerk = 100000.0;
        cfg->axes[i].max_travel = 1000.0;
        cfg->axes[i].min_travel = -1000.0;
        cfg->axes[i].enabled = 1;
    }
    cfg->rapid_feed = 8000.0;   /* mm/min */
    cfg->feed_override = 1.0;
    cfg->rapid_override = 1.0;
    cfg->spindle_override = 1.0;
    cfg->accel_profile = GK_ACCEL_TRAPEZOID;
    cfg->dry_run = 0;
    cfg->machine_lock = 0;
}

gk_status gk_motion_config_validate(const gk_motion_config *cfg)
{
    int i;
    if (cfg == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (cfg->feed_override < 0.0 || cfg->rapid_override < 0.0 ||
        cfg->spindle_override < 0.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    for (i = 0; i < (int)GK_AXIS_COUNT; ++i) {
        if (cfg->axes[i].max_velocity < 0.0 ||
            cfg->axes[i].max_accel < 0.0) {
            return GK_ERR_OUT_OF_RANGE;
        }
    }
    return GK_OK;
}

int gk_axis_in_range(const gk_motion_config *cfg, gk_axis axis, double v)
{
    if (cfg == NULL || (int)axis < 0 || (int)axis >= (int)GK_AXIS_COUNT) {
        return 0;
    }
    if (!cfg->axes[(int)axis].enabled) {
        return 0;
    }
    return v >= cfg->axes[(int)axis].min_travel &&
           v <= cfg->axes[(int)axis].max_travel;
}
