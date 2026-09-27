#include "gk/gk_move.h"

#include <math.h>
#include <string.h>

/* Convert a feed in units/min to units/s, applying overrides and dry-run. */
static double effective_feed(const gk_motion_config *cfg, const gk_move *m)
{
    double feed = m->feed;
    if (m->mode == GK_MOTION_RAPID) {
        feed = cfg->rapid_feed;
    }
    if (m->mode == GK_MOTION_RAPID) {
        feed *= cfg->rapid_override;
    } else {
        feed *= cfg->feed_override;
    }
    if (cfg->dry_run) {
        feed *= 2.0;
    }
    if (feed < 0.0) {
        feed = 0.0;
    }
    return feed / 60.0; /* units/s */
}

double gk_axis_limited_velocity(const gk_motion_config *cfg,
                                const gk_move *m, double feed)
{
    double dx, dy, dz;
    double total;
    double limited;
    gk_vec3 delta;
    int i;

    if (cfg == NULL || m == NULL) {
        return 0.0;
    }
    delta = gk_vec3_sub(m->end, m->start);
    dx = fabs(delta.x);
    dy = fabs(delta.y);
    dz = fabs(delta.z);
    total = sqrt(dx * dx + dy * dy + dz * dz);
    limited = feed;
    if (total <= GK_EPS) {
        return 0.0;
    }
    for (i = 0; i < (int)GK_AXIS_COUNT; ++i) {
        double axis_delta = 0.0;
        double axis_max;
        switch ((gk_axis)i) {
        case GK_AXIS_X: axis_delta = dx; break;
        case GK_AXIS_Y: axis_delta = dy; break;
        case GK_AXIS_Z: axis_delta = dz; break;
        default: continue;
        }
        if (!cfg->axes[i].enabled) {
            continue;
        }
        axis_max = cfg->axes[i].max_velocity;
        if (axis_max <= 0.0) {
            continue;
        }
        if (axis_delta > GK_EPS) {
            double max_path_v = axis_max * total / axis_delta;
            if (max_path_v < limited) {
                limited = max_path_v;
            }
        }
    }
    return limited;
}

/* Determine acceleration time from v: t = v/a, capped by path length. */
static double accel_time_for(double cruise, double accel)
{
    if (accel <= GK_EPS) {
        return 0.0;
    }
    return cruise / accel;
}

gk_status gk_plan_move(const gk_motion_config *cfg, const gk_move *m,
                       gk_motion_plan *out)
{
    double feed;
    double cruise;
    double accel;
    double ta;
    double dist_accel_decel;
    if (cfg == NULL || m == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (gk_motion_config_validate(cfg) != GK_OK) {
        return GK_ERR_INVALID_ARG;
    }

    memset(out, 0, sizeof(*out));
    out->move = *m;
    out->length = gk_move_length(m);
    out->distance = out->length;

    if (out->length <= GK_EPS) {
        out->duration = 0.0;
        out->cruise_velocity = 0.0;
        out->steps = 0;
        return GK_OK;
    }

    feed = effective_feed(cfg, m);
    cruise = gk_axis_limited_velocity(cfg, m, feed);
    /* Use the most conservative enabled axis acceleration. */
    accel = cfg->axes[GK_AXIS_X].max_accel;
    {
        int i;
        for (i = 0; i < (int)GK_AXIS_COUNT; ++i) {
            if (cfg->axes[i].enabled && cfg->axes[i].max_accel > GK_EPS &&
                cfg->axes[i].max_accel < accel) {
                accel = cfg->axes[i].max_accel;
            }
        }
    }
    if (accel <= GK_EPS || cruise <= GK_EPS) {
        out->cruise_velocity = 0.0;
        out->duration = 0.0;
        out->steps = 0;
        return GK_OK;
    }

    ta = accel_time_for(cruise, accel);
    dist_accel_decel = cruise * ta; /* accel + decel distance */

    if (dist_accel_decel >= out->length) {
        /* Triangular profile: never reaches cruise. */
        cruise = sqrt(out->length * accel);
        ta = cruise / accel;
        out->cruise_time = 0.0;
    } else {
        double d_cruise = out->length - dist_accel_decel;
        out->cruise_time = d_cruise / cruise;
    }
    out->cruise_velocity = cruise;
    out->accel_time = ta;
    out->decel_time = ta;
    out->duration = 2.0 * ta + out->cruise_time;

    /* Sampling resolution: at least 100 steps, else 1 step per 0.1 unit. */
    out->steps = (size_t)(out->length / 0.1) + 1;
    if (out->steps < 100) {
        out->steps = 100;
    }
    return GK_OK;
}

gk_status gk_plan_velocity_at(const gk_motion_plan *plan, double t,
                              double *out_velocity)
{
    double v;
    if (plan == NULL || out_velocity == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (plan->duration <= GK_EPS) {
        *out_velocity = 0.0;
        return GK_OK;
    }
    if (t <= 0.0 || t >= plan->duration) {
        *out_velocity = 0.0;
        return GK_OK;
    }
    if (t < plan->accel_time && plan->accel_time > GK_EPS) {
        double a = plan->cruise_velocity / plan->accel_time;
        v = a * t;
    } else if (t > plan->duration - plan->decel_time &&
               plan->decel_time > GK_EPS) {
        double remaining = plan->duration - t;
        double a = plan->cruise_velocity / plan->decel_time;
        v = a * remaining;
    } else {
        v = plan->cruise_velocity;
    }
    *out_velocity = gk_clamp(v, 0.0, plan->cruise_velocity);
    return GK_OK;
}

gk_status gk_plan_sample(const gk_motion_plan *plan, size_t step,
                         size_t total_steps, gk_interp_sample *out)
{
    double u;
    if (plan == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (total_steps == 0) {
        total_steps = 1;
    }
    if (step >= total_steps) {
        step = total_steps - 1;
    }
    u = (double)step / (double)(total_steps - 1 == 0 ? 1
                                                        : total_steps - 1);

    out->step = step;
    out->path_position = u * plan->length;
    out->velocity = 0.0;
    if (plan->duration > GK_EPS) {
        gk_plan_velocity_at(plan, u * plan->duration, &out->velocity);
    }

    if (plan->move.mode == GK_MOTION_RAPID ||
        plan->move.mode == GK_MOTION_LINEAR) {
        out->position.x = gk_lerp(plan->move.start.x, plan->move.end.x, u);
        out->position.y = gk_lerp(plan->move.start.y, plan->move.end.y, u);
        out->position.z = gk_lerp(plan->move.start.z, plan->move.end.z, u);
    } else {
        return gk_interpolate_arc_point(&plan->move, u, &out->position);
    }
    return GK_OK;
}

void gk_servo_init(gk_servo *s, const gk_servo_params *p, int second_order)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    if (p != NULL) {
        s->params = *p;
    } else {
        s->params.time_constant = 0.02;
        s->params.natural_freq = 50.0;
        s->params.damping = 0.7;
    }
    s->use_second_order = second_order ? 1 : 0;
}

double gk_servo_step(gk_servo *s, double command, double dt)
{
    if (s == NULL || dt <= 0.0) {
        return 0.0;
    }
    if (s->use_second_order) {
        /* Discretized 2nd order: x'' + 2*zeta*wn*x' + wn^2*(x - cmd) = 0 */
        double wn = s->params.natural_freq;
        double zeta = s->params.damping;
        if (wn <= GK_EPS) {
            s->position = command;
            return s->position;
        }
        {
            double accel = -2.0 * zeta * wn * s->velocity
                           - wn * wn * (s->position - command);
            s->velocity += accel * dt;
            s->position += s->velocity * dt;
        }
    } else {
        double tau = s->params.time_constant;
        if (tau <= GK_EPS) {
            s->position = command;
        } else {
            double alpha = dt / (tau + dt);
            s->position += (command - s->position) * alpha;
        }
    }
    s->internal = command - s->position;
    return s->position;
}

double gk_servo_following_error(const gk_servo *s, double command)
{
    if (s == NULL) {
        return 0.0;
    }
    return command - s->position;
}

void gk_servo_compensate_backlash(gk_servo *s, double dir)
{
    if (s == NULL) {
        return;
    }
    if (dir > 0.0 && s->backlash_state < 0.0) {
        s->position += s->params.backlash;
    } else if (dir < 0.0 && s->backlash_state > 0.0) {
        s->position -= s->params.backlash;
    }
    s->backlash_state = dir;
}
