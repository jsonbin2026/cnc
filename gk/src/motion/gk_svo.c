#include "gk/gk_svo.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ===================================================================
 * Axis state and protection (1091-1096)
 * =================================================================== */

const char *gk_svo_state_name(gk_svo_state s)
{
    switch (s) {
    case GK_SVO_OFF: return "off";
    case GK_SVO_READY: return "ready";
    case GK_SVO_RUNNING: return "running";
    case GK_SVO_ALARM: return "alarm";
    case GK_SVO_OVERLOAD: return "overload";
    case GK_SVO_OVERHEAT: return "overheat";
    case GK_SVO_ENCODER_FAULT: return "encoder-fault";
    default: return "unknown";
    }
}

void gk_svo_axis_init(gk_svo_axis *a)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    a->state = GK_SVO_OFF;
    a->max_temperature_c = 85.0;
    a->max_load_pct = 120.0;
}

gk_status gk_svo_power_on(gk_svo_axis *a)
{
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    a->state = GK_SVO_READY;
    return GK_OK;
}

gk_status gk_svo_enable(gk_svo_axis *a)
{
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (a->state == GK_SVO_OFF) {
        return GK_ERR_STATE;
    }
    if (gk_svo_in_alarm(a)) {
        return GK_ERR_STATE;
    }
    a->enabled = 1;
    a->state = GK_SVO_RUNNING;
    return GK_OK;
}

gk_status gk_svo_disable(gk_svo_axis *a)
{
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    a->enabled = 0;
    if (!gk_svo_in_alarm(a)) {
        a->state = GK_SVO_READY;
    }
    return GK_OK;
}

gk_status gk_svo_set_load(gk_svo_axis *a, double load_pct)
{
    if (a == NULL || load_pct < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    a->load_pct = load_pct;
    if (load_pct > a->max_load_pct) {
        a->state = GK_SVO_OVERLOAD;
        a->enabled = 0;
    }
    return GK_OK;
}

gk_status gk_svo_set_temperature(gk_svo_axis *a, double temperature_c)
{
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    a->temperature_c = temperature_c;
    if (temperature_c > a->max_temperature_c) {
        a->state = GK_SVO_OVERHEAT;
        a->enabled = 0;
    }
    return GK_OK;
}

gk_status gk_svo_encoder_alarm(gk_svo_axis *a)
{
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    a->state = GK_SVO_ENCODER_FAULT;
    a->enabled = 0;
    return GK_OK;
}

gk_status gk_svo_reset_alarm(gk_svo_axis *a)
{
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (gk_svo_in_alarm(a)) {
        a->state = GK_SVO_READY;
        a->enabled = 0;
    }
    return GK_OK;
}

int gk_svo_in_alarm(const gk_svo_axis *a)
{
    if (a == NULL) {
        return 0;
    }
    return a->state == GK_SVO_ALARM || a->state == GK_SVO_OVERLOAD ||
           a->state == GK_SVO_OVERHEAT ||
           a->state == GK_SVO_ENCODER_FAULT;
}

/* ===================================================================
 * Waveforms (1097-1099)
 * =================================================================== */

void gk_svo_wave_init(gk_svo_wave *w, double amplitude, double frequency_hz,
                      double offset)
{
    if (w == NULL) {
        return;
    }
    memset(w, 0, sizeof(*w));
    w->amplitude = amplitude;
    w->frequency_hz = frequency_hz;
    w->offset = offset;
}

double gk_svo_wave_at(const gk_svo_wave *w, double t)
{
    if (w == NULL) {
        return 0.0;
    }
    return w->offset + w->amplitude * sin(2.0 * M_PI * w->frequency_hz * t);
}

/* ===================================================================
 * PID and three-loop control (1100-1104)
 * =================================================================== */

void gk_svo_pid_init(gk_svo_pid *p, double kp, double ki, double kd)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->kp = kp;
    p->ki = ki;
    p->kd = kd;
}

double gk_svo_pid_step(gk_svo_pid *p, double error, double dt)
{
    double derivative;
    double out;
    if (p == NULL || dt <= 0.0) {
        return 0.0;
    }
    p->integral += error * dt;
    derivative = (error - p->previous_error) / dt;
    p->previous_error = error;
    out = p->kp * error + p->ki * p->integral + p->kd * derivative;
    return out;
}

void gk_svo_pid_reset(gk_svo_pid *p)
{
    if (p == NULL) {
        return;
    }
    p->integral = 0.0;
    p->previous_error = 0.0;
}

void gk_svo_loop_init(gk_svo_loop *l)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
    gk_svo_pid_init(&l->current, 1.0, 0.1, 0.0);
    gk_svo_pid_init(&l->velocity, 2.0, 0.2, 0.01);
    gk_svo_pid_init(&l->position, 10.0, 0.0, 0.1);
}

double gk_svo_current_loop(const gk_svo_loop *l, double error)
{
    if (l == NULL) {
        return 0.0;
    }
    return l->current.kp * error;
}

double gk_svo_velocity_loop(const gk_svo_loop *l, double error)
{
    if (l == NULL) {
        return 0.0;
    }
    return l->velocity.kp * error;
}

double gk_svo_position_loop(const gk_svo_loop *l, double error)
{
    if (l == NULL) {
        return 0.0;
    }
    return l->position.kp * error;
}

double gk_svo_loop_step(gk_svo_loop *l, double dt)
{
    double pos_err;
    double vel_cmd;
    if (l == NULL || dt <= 0.0) {
        return 0.0;
    }
    pos_err = l->position_command - l->position_feedback;
    vel_cmd = gk_svo_pid_step(&l->position, pos_err, dt);
    /* cascade: velocity command drives the "velocity" until near target */
    gk_svo_pid_step(&l->velocity, vel_cmd, dt);
    return vel_cmd;
}

/* ===================================================================
 * Feedforward and friction (1105-1106)
 * =================================================================== */

void gk_svo_comp_init(gk_svo_comp *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    c->velocity_ff_gain = 1.0;
    c->accel_ff_gain = 1.0;
    c->friction_coulomb = 5.0;
    c->friction_viscous = 0.1;
}

double gk_svo_feedforward(const gk_svo_comp *c, double velocity, double accel)
{
    if (c == NULL) {
        return 0.0;
    }
    return c->velocity_ff_gain * velocity + c->accel_ff_gain * accel;
}

double gk_svo_friction_comp(const gk_svo_comp *c, double velocity)
{
    double sign;
    if (c == NULL) {
        return 0.0;
    }
    sign = velocity >= 0.0 ? 1.0 : -1.0;
    if (fabs(velocity) < 1e-12) {
        return 0.0;
    }
    return c->friction_coulomb * sign + c->friction_viscous * velocity;
}

/* ===================================================================
 * Backlash (1107)
 * =================================================================== */

void gk_svo_backlash_init(gk_svo_backlash *b, double backlash_mm)
{
    if (b == NULL) {
        return;
    }
    memset(b, 0, sizeof(*b));
    b->backlash_mm = backlash_mm;
}

double gk_svo_backlash_compensate(gk_svo_backlash *b, double direction)
{
    if (b == NULL) {
        return 0.0;
    }
    if (direction * b->last_dir < 0.0) {
        /* a direction reversal needs the full backlash take-up */
        b->applied = 1;
        b->last_dir = direction;
        return b->backlash_mm;
    }
    b->last_dir = direction;
    b->applied = 0;
    return 0.0;
}

/* ===================================================================
 * Pitch error (1108)
 * =================================================================== */

void gk_svo_pitch_init(gk_svo_pitch *p, int points, double pitch_mm)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->points = points;
    p->pitch_mm = pitch_mm;
    p->error_per_mm = 0.005;
}

double gk_svo_pitch_compensate(const gk_svo_pitch *p, int index)
{
    if (p == NULL || index < 0 || index >= p->points) {
        return 0.0;
    }
    /* compensation grows linearly with distance from the reference point */
    return (double)index * p->pitch_mm * p->error_per_mm;
}

double gk_svo_pitch_total(const gk_svo_pitch *p)
{
    if (p == NULL || p->points <= 1) {
        return 0.0;
    }
    return gk_svo_pitch_compensate(p, p->points - 1);
}

/* ===================================================================
 * Straightness / squareness (1109-1110)
 * =================================================================== */

void gk_svo_geom_init(gk_svo_geom *g, double travel_mm)
{
    if (g == NULL) {
        return;
    }
    memset(g, 0, sizeof(*g));
    g->travel_mm = travel_mm;
}

double gk_svo_straightness_comp(gk_svo_geom *g, double deviation_mm)
{
    if (g == NULL || g->travel_mm <= 0.0) {
        return 0.0;
    }
    g->deviation_mm = deviation_mm;
    return deviation_mm / g->travel_mm;
}

double gk_svo_squareness_comp(gk_svo_geom *g, double angle_err_mm)
{
    if (g == NULL || g->travel_mm <= 0.0) {
        return 0.0;
    }
    return angle_err_mm / g->travel_mm;
}

/* ===================================================================
 * Thermal compensation (1111)
 * =================================================================== */

void gk_svo_thermal_comp_init(gk_svo_thermal_comp *t, double coefficient,
                              double reference_c)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->coefficient = coefficient;
    t->reference_c = reference_c;
}

double gk_svo_thermal_compensate(const gk_svo_thermal_comp *t,
                                 double temperature_c)
{
    if (t == NULL) {
        return 0.0;
    }
    return t->coefficient * (temperature_c - t->reference_c);
}

/* ===================================================================
 * Gain / rigidity (1112-1113)
 * =================================================================== */

void gk_svo_tuning_init(gk_svo_tuning *t)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->gain = 1.0;
    t->rigidity = 10.0;
}

double gk_svo_set_gain(gk_svo_tuning *t, double gain)
{
    if (t == NULL || gain < 0.0) {
        return 0.0;
    }
    t->gain = gain;
    return t->gain;
}

double gk_svo_set_rigidity(gk_svo_tuning *t, double rigidity)
{
    if (t == NULL || rigidity < 0.0) {
        return 0.0;
    }
    t->rigidity = rigidity;
    return t->rigidity;
}

/* ===================================================================
 * Filters (1114-1117)
 * =================================================================== */

void gk_svo_notch_init(gk_svo_notch *n)
{
    if (n == NULL) {
        return;
    }
    memset(n, 0, sizeof(*n));
    n->frequency_hz = 500.0;
    n->depth = 0.1;
    n->width_hz = 20.0;
}

double gk_svo_notch_response(const gk_svo_notch *n, double frequency_hz)
{
    double d;
    if (n == NULL) {
        return 1.0;
    }
    d = fabs(frequency_hz - n->frequency_hz);
    if (d < n->width_hz) {
        return n->depth;
    }
    /* smooth roll-off outside the notch */
    return 1.0 - (1.0 - n->depth) / (1.0 + d / n->width_hz);
}

int gk_svo_notch_suppressed(const gk_svo_notch *n, double vibration)
{
    if (n == NULL) {
        return 0;
    }
    return vibration * gk_svo_notch_response(n, n->frequency_hz) < vibration;
}

void gk_svo_lowpass_init(gk_svo_lowpass *l, double cutoff_hz, double sample_hz)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
    l->cutoff_hz = cutoff_hz;
    if (sample_hz > 0.0 && cutoff_hz > 0.0) {
        l->alpha = 1.0 / (1.0 + sample_hz / (2.0 * M_PI * cutoff_hz));
    } else {
        l->alpha = 1.0;
    }
}

double gk_svo_lowpass_apply(gk_svo_lowpass *l, double sample)
{
    if (l == NULL) {
        return 0.0;
    }
    l->state = l->state + l->alpha * (sample - l->state);
    return l->state;
}

void gk_svo_smooth_init(gk_svo_smooth *s, double alpha)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->alpha = alpha;
}

double gk_svo_smooth_apply(gk_svo_smooth *s, double value)
{
    if (s == NULL) {
        return 0.0;
    }
    s->previous = s->previous + s->alpha * (value - s->previous);
    return s->previous;
}

/* ===================================================================
 * Feedforward (1118-1120)
 * =================================================================== */

void gk_svo_ff_init(gk_svo_ff *f)
{
    if (f == NULL) {
        return;
    }
    memset(f, 0, sizeof(*f));
}

double gk_svo_ff_accel(gk_svo_ff *f, double accel, double gain)
{
    if (f == NULL) {
        return 0.0;
    }
    f->accel_ff = accel * gain;
    return f->accel_ff;
}

double gk_svo_ff_velocity(gk_svo_ff *f, double velocity, double gain)
{
    if (f == NULL) {
        return 0.0;
    }
    f->velocity_ff = velocity * gain;
    return f->velocity_ff;
}

double gk_svo_ff_torque(gk_svo_ff *f, double accel, double inertia)
{
    if (f == NULL) {
        return 0.0;
    }
    f->torque_ff = accel * inertia;
    return f->torque_ff;
}
