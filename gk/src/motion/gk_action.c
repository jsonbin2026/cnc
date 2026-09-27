#include "gk/gk_action.h"

#include <string.h>
#include <math.h>

/* ---- spindle ---- */

static const double g_gear_ranges[4][2] = {
    {0.0, 2000.0}, {2000.0, 5000.0}, {5000.0, 8000.0}, {8000.0, 24000.0}
};

void gk_spindle_init(gk_spindle_axis *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->gear = 1;
}

gk_status gk_spindle_start(gk_spindle_axis *s, double rpm, int direction)
{
    if (s == NULL || rpm < 0.0 || (direction != 1 && direction != -1)) {
        return GK_ERR_INVALID_ARG;
    }
    s->target_rpm = rpm;
    s->direction = direction;
    if (s->accel_rate <= 0.0) {
        s->accel_rate = rpm > 0.0 ? rpm / 1.5 : 1000.0;
    }
    return GK_OK;
}

gk_status gk_spindle_stop(gk_spindle_axis *s)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    s->target_rpm = 0.0;
    return GK_OK;
}

double gk_spindle_update(gk_spindle_axis *s, double dt)
{
    double err;
    if (s == NULL || dt < 0.0) {
        return 0.0;
    }
    err = s->target_rpm - s->current_rpm;
    if (err > 0.0) {
        double rate = s->accel_rate > 0.0 ? s->accel_rate : 1000.0;
        s->current_rpm += rate * dt;
        if (s->current_rpm > s->target_rpm) {
            s->current_rpm = s->target_rpm;
        }
    } else if (err < 0.0) {
        double rate = s->decel_rate > 0.0 ? s->decel_rate : 1500.0;
        s->current_rpm -= rate * dt;
        if (s->current_rpm < s->target_rpm) {
            s->current_rpm = s->target_rpm;
        }
    }
    if (s->current_rpm <= 0.0) {
        s->current_rpm = 0.0;
    }
    return s->current_rpm;
}

int gk_spindle_is_up_to_speed(const gk_spindle_axis *s, double tol)
{
    if (s == NULL) {
        return 0;
    }
    return fabs(s->target_rpm - s->current_rpm) <= tol;
}

gk_status gk_spindle_select_gear(gk_spindle_axis *s, double rpm)
{
    int i;
    if (s == NULL || rpm < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < 4; ++i) {
        if (rpm >= g_gear_ranges[i][0] && rpm <= g_gear_ranges[i][1]) {
            s->gear = i + 1;
            return GK_OK;
        }
    }
    return GK_ERR_OUT_OF_RANGE;
}

gk_status gk_spindle_orient(gk_spindle_axis *s, int angle_deg)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (angle_deg != 0 && angle_deg != 90 && angle_deg != 180 &&
        angle_deg != 270) {
        return GK_ERR_OUT_OF_RANGE;
    }
    s->orient = 1;
    s->orient_angle = angle_deg;
    s->target_rpm = 0.0;
    s->current_rpm = 0.0;
    return GK_OK;
}

/* ---- magazine ---- */

const char *gk_magazine_kind_name(gk_magazine_kind k)
{
    switch (k) {
    case GK_MAGAZINE_DISC:
        return "disc";
    case GK_MAGAZINE_UMBRELLA:
        return "umbrella";
    case GK_MAGAZINE_CHAIN:
        return "chain";
    default:
        return "unknown";
    }
}

void gk_magazine_init(gk_magazine *m, gk_magazine_kind kind, int capacity)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    m->kind = kind;
    m->capacity = capacity > 0 ? capacity : 1;
    m->current_pocket = 0;
    m->target_pocket = 0;
    m->index_time = 0.5;
}

gk_status gk_magazine_select(gk_magazine *m, int pocket)
{
    if (m == NULL || pocket < 0 || pocket >= m->capacity) {
        return GK_ERR_OUT_OF_RANGE;
    }
    if (m->kind == GK_MAGAZINE_CHAIN) {
        /* chain magazines require sequential indexing: disallow random */
        int diff = pocket - m->current_pocket;
        if (diff != 0 && pocket != (m->current_pocket + 1) % m->capacity) {
            return GK_ERR_UNSUPPORTED;
        }
    }
    m->target_pocket = pocket;
    return GK_OK;
}

double gk_magazine_index_step(gk_magazine *m, double dt)
{
    if (m == NULL || dt <= 0.0) {
        return 0.0;
    }
    if (m->current_pocket == m->target_pocket) {
        return 0.0;
    }
    {
        double step_deg = 360.0 / (double)m->capacity;
        double moved = (step_deg / m->index_time) * dt;
        m->index_angle += moved;
        m->current_pocket = (m->current_pocket + 1) % m->capacity;
        return moved;
    }
}

int gk_magazine_at_target(const gk_magazine *m)
{
    if (m == NULL) {
        return 0;
    }
    return m->current_pocket == m->target_pocket;
}

/* ---- ATC ---- */

const char *gk_atc_state_name(gk_atc_state s)
{
    switch (s) {
    case GK_ATC_IDLE:
        return "idle";
    case GK_ATC_UNCLAMP:
        return "unclamp";
    case GK_ATC_TOOL_OUT:
        return "tool-out";
    case GK_ATC_ROTATE:
        return "rotate";
    case GK_ATC_TOOL_IN:
        return "tool-in";
    case GK_ATC_CLAMP:
        return "clamp";
    case GK_ATC_DONE:
        return "done";
    case GK_ATC_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

void gk_atc_init(gk_atc *a)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    a->state = GK_ATC_IDLE;
}

gk_status gk_atc_start(gk_atc *a, int tool)
{
    if (a == NULL || tool < 0) {
        return GK_ERR_INVALID_ARG;
    }
    if (a->state != GK_ATC_IDLE && a->state != GK_ATC_DONE) {
        return GK_ERR_STATE;
    }
    a->state = GK_ATC_UNCLAMP;
    a->progress = 0.0;
    a->standby_tool = tool;
    a->arm_angle = 0.0;
    return GK_OK;
}

gk_atc_state gk_atc_update(gk_atc *a, double dt)
{
    if (a == NULL) {
        return GK_ATC_ERROR;
    }
    if (a->state == GK_ATC_IDLE || a->state == GK_ATC_DONE ||
        a->state == GK_ATC_ERROR) {
        return a->state;
    }
    a->progress += dt * 2.0;
    while (a->progress >= 1.0) {
        a->progress -= 1.0;
        switch (a->state) {
        case GK_ATC_UNCLAMP:
            a->state = GK_ATC_TOOL_OUT;
            break;
        case GK_ATC_TOOL_OUT:
            a->arm_angle = 90.0;
            a->state = GK_ATC_ROTATE;
            break;
        case GK_ATC_ROTATE:
            /* swap tools in the arm */
            {
                int tmp = a->spindle_tool;
                a->spindle_tool = a->standby_tool;
                a->standby_tool = tmp;
            }
            a->state = GK_ATC_TOOL_IN;
            break;
        case GK_ATC_TOOL_IN:
            a->arm_angle = 0.0;
            a->state = GK_ATC_CLAMP;
            break;
        case GK_ATC_CLAMP:
            a->state = GK_ATC_DONE;
            break;
        default:
            a->state = GK_ATC_DONE;
            break;
        }
        if (a->state == GK_ATC_DONE) {
            break;
        }
    }
    return a->state;
}

gk_status gk_atc_manual_change(gk_atc *a, int new_tool)
{
    if (a == NULL || new_tool < 0) {
        return GK_ERR_INVALID_ARG;
    }
    a->manual = 1;
    a->spindle_tool = new_tool;
    a->state = GK_ATC_DONE;
    return GK_OK;
}

/* ---- tool setter ---- */

void gk_tool_setter_init(gk_tool_setter *t, gk_probe_kind kind)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->kind = kind;
    t->tip_diameter = 1.0;
    t->resolution = 0.001;
    if (kind == GK_PROBE_LASER) {
        t->resolution = 0.0001;
    }
}

double gk_tool_setter_measure(const gk_tool_setter *t, int axis,
                              double approach)
{
    double base;
    if (t == NULL || axis < 0 || axis > 2) {
        return 0.0;
    }
    base = approach - t->tip_diameter * 0.5;
    /* quantize to resolution */
    if (t->resolution > 0.0) {
        base = floor(base / t->resolution + 0.5) * t->resolution;
    }
    return base;
}

/* ---- table ---- */

void gk_table_init(gk_table *t)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->max_tilt = 120.0;
}

gk_status gk_table_rotate(gk_table *t, double delta_deg)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    t->rotation += delta_deg;
    while (t->rotation >= 360.0) {
        t->rotation -= 360.0;
    }
    while (t->rotation < 0.0) {
        t->rotation += 360.0;
    }
    return GK_OK;
}

gk_status gk_table_tilt(gk_table *t, double angle_deg)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (fabs(angle_deg) > t->max_tilt) {
        return GK_ERR_OUT_OF_RANGE;
    }
    t->tilt = angle_deg;
    return GK_OK;
}

/* ---- tailstock ---- */

void gk_tailstock_init(gk_tailstock *t)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->min_pos = 0.0;
    t->max_pos = 500.0;
}

gk_status gk_tailstock_move(gk_tailstock *t, double to_mm)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (t->clamped) {
        return GK_ERR_STATE;
    }
    if (to_mm < t->min_pos || to_mm > t->max_pos) {
        return GK_ERR_OUT_OF_RANGE;
    }
    t->position = to_mm;
    return GK_OK;
}

gk_status gk_tailstock_clamp(gk_tailstock *t, int clamped)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    t->clamped = clamped ? 1 : 0;
    return GK_OK;
}

/* ---- steady rest ---- */

void gk_steady_rest_init(gk_steady_rest *r)
{
    if (r == NULL) {
        return;
    }
    memset(r, 0, sizeof(*r));
}

gk_status gk_steady_rest_engage(gk_steady_rest *r, double diameter)
{
    if (r == NULL || diameter <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    r->open = diameter;
    r->engaged = 1;
    return GK_OK;
}

gk_status gk_steady_rest_release(gk_steady_rest *r)
{
    if (r == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    r->engaged = 0;
    return GK_OK;
}

/* ---- auxiliaries ---- */

void gk_chip_conveyor_init(gk_chip_conveyor *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
}

void gk_chip_conveyor_set(gk_chip_conveyor *c, double rpm)
{
    if (c == NULL) {
        return;
    }
    c->speed = rpm < 0.0 ? 0.0 : rpm;
}

double gk_chip_conveyor_update(gk_chip_conveyor *c, double dt)
{
    if (c == NULL || dt < 0.0) {
        return 0.0;
    }
    c->running_time += dt;
    return c->running_time;
}

void gk_auto_door_init(gk_auto_door *d)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    d->speed = 1.0;
}

gk_status gk_auto_door_set(gk_auto_door *d, int open)
{
    if (d == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    d->open = open ? 1 : 0;
    return GK_OK;
}

double gk_auto_door_update(gk_auto_door *d, double dt)
{
    double target;
    double step;
    if (d == NULL || dt < 0.0) {
        return 0.0;
    }
    target = d->open ? 1.0 : 0.0;
    step = d->speed * dt;
    if (d->position < target) {
        d->position += step;
        if (d->position > target) {
            d->position = target;
        }
    } else if (d->position > target) {
        d->position -= step;
        if (d->position < target) {
            d->position = target;
        }
    }
    return d->position;
}

void gk_auto_fixture_init(gk_auto_fixture *f)
{
    if (f == NULL) {
        return;
    }
    memset(f, 0, sizeof(*f));
}

gk_status gk_auto_fixture_set(gk_auto_fixture *f, int clamped, double force)
{
    if (f == NULL || force < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    f->clamped = clamped ? 1 : 0;
    f->clamp_force = f->clamped ? force : 0.0;
    f->stroke = f->clamped ? 1.0 : 0.0;
    return GK_OK;
}

void gk_coolant_valve_init(gk_coolant_valve *v)
{
    if (v == NULL) {
        return;
    }
    memset(v, 0, sizeof(*v));
    v->dir = gk_vec3_make(0, 0, -1);
}

void gk_coolant_valve_set(gk_coolant_valve *v, int on, double flow)
{
    if (v == NULL) {
        return;
    }
    v->on = on ? 1 : 0;
    v->flow = v->on ? (flow < 0.0 ? 0.0 : flow) : 0.0;
}

void gk_air_blast_init(gk_air_blast *a)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
}

void gk_air_blast_set(gk_air_blast *a, int on, double pressure)
{
    if (a == NULL) {
        return;
    }
    a->on = on ? 1 : 0;
    a->pressure = a->on ? (pressure < 0.0 ? 0.0 : pressure) : 0.0;
}

void gk_work_light_init(gk_work_light *l)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
}

void gk_work_light_set(gk_work_light *l, int on, double intensity)
{
    if (l == NULL) {
        return;
    }
    l->on = on ? 1 : 0;
    if (!l->on) {
        l->intensity = 0.0;
    } else {
        l->intensity = intensity < 0.0 ? 0.0 : (intensity > 1.0 ? 1.0 : intensity);
    }
}

/* ---- beacon / buzzer ---- */

const char *gk_beacon_color_name(gk_beacon_color c)
{
    switch (c) {
    case GK_BEACON_OFF:
        return "off";
    case GK_BEACON_GREEN:
        return "green";
    case GK_BEACON_YELLOW:
        return "yellow";
    case GK_BEACON_RED:
        return "red";
    default:
        return "unknown";
    }
}

void gk_beacon_init(gk_beacon *b)
{
    if (b == NULL) {
        return;
    }
    memset(b, 0, sizeof(*b));
    b->blink_hz = 1.0;
}

void gk_beacon_set(gk_beacon *b, gk_beacon_color color, int blink)
{
    if (b == NULL) {
        return;
    }
    b->color = color;
    b->blink = blink ? 1 : 0;
}

void gk_buzzer_init(gk_buzzer *z)
{
    if (z == NULL) {
        return;
    }
    memset(z, 0, sizeof(*z));
    z->frequency = 1000.0;
}

void gk_buzzer_set(gk_buzzer *z, int on, double hz, double volume)
{
    if (z == NULL) {
        return;
    }
    z->on = on ? 1 : 0;
    z->frequency = hz;
    z->volume = volume < 0.0 ? 0.0 : (volume > 1.0 ? 1.0 : volume);
}
