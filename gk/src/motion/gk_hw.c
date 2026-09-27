#include "gk/gk_hw.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void gk__hw_copy(char *dst, size_t cap, const char *src)
{
    size_t n;
    if (dst == NULL || cap == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    n = strlen(src);
    if (n >= cap) {
        n = cap - 1;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
}

/* ===================================================================
 * Hardware details (1001-1018)
 * =================================================================== */

void gk_hw_buzzer_init(gk_hw_buzzer *b)
{
    if (b == NULL) {
        return;
    }
    memset(b, 0, sizeof(*b));
    b->frequency_hz = 2000.0;
}

gk_status gk_hw_buzzer_beep(gk_hw_buzzer *b, double frequency_hz,
                            double duration_s)
{
    if (b == NULL || frequency_hz <= 0.0 || duration_s <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    b->frequency_hz = frequency_hz;
    b->duration_s = duration_s;
    b->active = 1;
    return GK_OK;
}

double gk_hw_buzzer_remaining(const gk_hw_buzzer *b, double elapsed_s)
{
    double remain;
    if (b == NULL || !b->active) {
        return 0.0;
    }
    remain = b->duration_s - elapsed_s;
    return remain > 0.0 ? remain : 0.0;
}

void gk_hw_estop_init(gk_hw_estop *e)
{
    if (e == NULL) {
        return;
    }
    memset(e, 0, sizeof(*e));
    e->travel_mm = 8.0;
}

gk_status gk_hw_estop_press(gk_hw_estop *e, double force_n)
{
    if (e == NULL || force_n <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    if (force_n >= 5.0) {
        e->pressed = 1;
        e->latched = 1;
    }
    return GK_OK;
}

gk_status gk_hw_estop_release(gk_hw_estop *e, int twist)
{
    if (e == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!twist) {
        return GK_ERR_STATE;   /* must twist to release a latched e-stop */
    }
    e->pressed = 0;
    e->latched = 0;
    return GK_OK;
}

int gk_hw_estop_engaged(const gk_hw_estop *e)
{
    if (e == NULL) {
        return 0;
    }
    return e->latched;
}

void gk_hw_handle_init(gk_hw_handle *h, double length_mm)
{
    if (h == NULL) {
        return;
    }
    memset(h, 0, sizeof(*h));
    h->length_mm = length_mm;
}

gk_status gk_hw_handle_pull(gk_hw_handle *h, double angle_deg)
{
    if (h == NULL || angle_deg < 0.0 || angle_deg > 90.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    h->angle_deg = angle_deg;
    h->open = angle_deg >= 30.0;
    return GK_OK;
}

void gk_hw_window_init(gk_hw_window *w, double width_mm, double height_mm)
{
    if (w == NULL) {
        return;
    }
    memset(w, 0, sizeof(*w));
    w->width_mm = width_mm;
    w->height_mm = height_mm;
    w->transparency = 0.85;
}

double gk_hw_window_view_area(const gk_hw_window *w)
{
    if (w == NULL) {
        return 0.0;
    }
    return w->width_mm * w->height_mm;
}

void gk_hw_wiper_init(gk_hw_wiper *w)
{
    if (w == NULL) {
        return;
    }
    memset(w, 0, sizeof(*w));
    w->sweep_deg = 120.0;
}

gk_status gk_hw_wiper_start(gk_hw_wiper *w)
{
    if (w == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    w->running = 1;
    return GK_OK;
}

gk_status gk_hw_wiper_stroke(gk_hw_wiper *w)
{
    if (w == NULL || !w->running) {
        return GK_ERR_STATE;
    }
    w->strokes++;
    return GK_OK;
}

gk_status gk_hw_wiper_stop(gk_hw_wiper *w)
{
    if (w == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    w->running = 0;
    return GK_OK;
}

void gk_hw_lock_init(gk_hw_lock *l, const char *key_id)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
    gk__hw_copy(l->key_id, sizeof(l->key_id), key_id);
}

gk_status gk_hw_lock_engage(gk_hw_lock *l)
{
    if (l == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    l->locked = 1;
    return GK_OK;
}

gk_status gk_hw_lock_disengage(gk_hw_lock *l, const char *key_id)
{
    if (l == NULL || key_id == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (strcmp(l->key_id, key_id) != 0) {
        return GK_ERR_UNSUPPORTED;
    }
    l->locked = 0;
    return GK_OK;
}

void gk_hw_door_sensor_init(gk_hw_door_sensor *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->closed = 1;
}

gk_status gk_hw_door_sensor_update(gk_hw_door_sensor *s, int closed)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    s->triggered = (s->closed != closed) ? 1 : 0;
    s->closed = closed ? 1 : 0;
    return GK_OK;
}

void gk_hw_safety_switch_init(gk_hw_safety_switch *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->channel_count = 2;
}

gk_status gk_hw_safety_switch_set(gk_hw_safety_switch *s, int channel_a,
                                  int channel_b)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    s->channel_a = channel_a ? 1 : 0;
    s->channel_b = channel_b ? 1 : 0;
    return GK_OK;
}

int gk_hw_safety_switch_consistent(const gk_hw_safety_switch *s)
{
    if (s == NULL) {
        return 0;
    }
    return s->channel_a == s->channel_b;
}

int gk_hw_safety_switch_ok(const gk_hw_safety_switch *s)
{
    if (s == NULL) {
        return 0;
    }
    return gk_hw_safety_switch_consistent(s) && s->channel_a == 1;
}

const char *gk_hw_panel_name(gk_hw_panel_kind k)
{
    switch (k) {
    case GK_HW_PANEL_SIDE: return "side-panel";
    case GK_HW_PANEL_REAR: return "rear-panel";
    case GK_HW_PANEL_TOP: return "top-cover";
    default: return "unknown";
    }
}

void gk_hw_panel_init(gk_hw_panel *p, gk_hw_panel_kind kind, double width_mm,
                      double height_mm, double thickness_mm)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->kind = kind;
    p->width_mm = width_mm;
    p->height_mm = height_mm;
    p->thickness_mm = thickness_mm;
    p->attached = 1;
}

double gk_hw_panel_mass(const gk_hw_panel *p, double density_kg_m3)
{
    double vol_m3;
    if (p == NULL || density_kg_m3 <= 0.0) {
        return 0.0;
    }
    vol_m3 = (p->width_mm / 1000.0) * (p->height_mm / 1000.0) *
             (p->thickness_mm / 1000.0);
    return vol_m3 * density_kg_m3;
}

gk_status gk_hw_panel_attach(gk_hw_panel *p, int attached)
{
    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    p->attached = attached ? 1 : 0;
    return GK_OK;
}

void gk_hw_base_init(gk_hw_base *b, double length_mm, double width_mm,
                     double height_mm)
{
    if (b == NULL) {
        return;
    }
    memset(b, 0, sizeof(*b));
    b->length_mm = length_mm;
    b->width_mm = width_mm;
    b->height_mm = height_mm;
}

double gk_hw_base_footprint(const gk_hw_base *b)
{
    if (b == NULL) {
        return 0.0;
    }
    return b->length_mm * b->width_mm;
}

void gk_hw_feet_init(gk_hw_feet *f, int count)
{
    if (f == NULL) {
        return;
    }
    memset(f, 0, sizeof(*f));
    f->count = count > 0 ? count : 4;
    f->diameter_mm = 80.0;
    f->load_capacity_kg = 800.0;
}

int gk_hw_feet_can_support(const gk_hw_feet *f, double machine_mass_kg)
{
    if (f == NULL || f->count <= 0) {
        return 0;
    }
    return (double)f->count * f->load_capacity_kg >= machine_mass_kg;
}

void gk_hw_pad_init(gk_hw_pad *p, double stiffness_n_mm, double damping_ratio)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->stiffness_n_mm = stiffness_n_mm;
    p->damping_ratio = damping_ratio;
    /* natural frequency for unit mass: sqrt(k)/2pi, k in N/m */
    p->natural_hz = sqrt(stiffness_n_mm * 1000.0) / (2.0 * M_PI);
}

double gk_hw_pad_transmissibility(const gk_hw_pad *p, double forcing_hz)
{
    double r;
    double num;
    double den;
    if (p == NULL || p->natural_hz <= 0.0) {
        return 1.0;
    }
    r = forcing_hz / p->natural_hz;
    num = 1.0 + 4.0 * p->damping_ratio * p->damping_ratio * r * r;
    den = (1.0 - r * r) * (1.0 - r * r) +
          4.0 * p->damping_ratio * p->damping_ratio * r * r;
    if (den <= 0.0) {
        return 1.0;
    }
    return sqrt(num / den);
}

void gk_hw_level_bolt_init(gk_hw_level_bolt *b, double thread_pitch_mm)
{
    if (b == NULL) {
        return;
    }
    memset(b, 0, sizeof(*b));
    b->thread_pitch_mm = thread_pitch_mm;
}

double gk_hw_level_bolt_turn(gk_hw_level_bolt *b, double turns)
{
    if (b == NULL) {
        return 0.0;
    }
    b->turns += turns;
    b->height_offset_mm = b->turns * b->thread_pitch_mm;
    return b->height_offset_mm;
}

void gk_hw_lifting_eye_init(gk_hw_lifting_eye *e, double thread_size_mm,
                            double rated_load_kg)
{
    if (e == NULL) {
        return;
    }
    memset(e, 0, sizeof(*e));
    e->thread_size_mm = thread_size_mm;
    e->rated_load_kg = rated_load_kg;
    e->count = 4;
}

int gk_hw_lifting_eye_ok(const gk_hw_lifting_eye *e, double machine_mass_kg)
{
    double total;
    if (e == NULL || e->count <= 0) {
        return 0;
    }
    total = (double)e->count * e->rated_load_kg;
    /* require a safety factor of 2 */
    return total >= 2.0 * machine_mass_kg;
}

void gk_hw_fork_pocket_init(gk_hw_fork_pocket *p, double width_mm,
                            double height_mm, double depth_mm)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->width_mm = width_mm;
    p->height_mm = height_mm;
    p->depth_mm = depth_mm;
    p->count = 2;
}

int gk_hw_fork_pocket_accepts(const gk_hw_fork_pocket *p, double fork_width_mm,
                              double fork_thickness_mm)
{
    if (p == NULL) {
        return 0;
    }
    return fork_width_mm <= p->width_mm && fork_thickness_mm <= p->height_mm &&
           p->depth_mm > 0.0;
}

void gk_hw_plate_bracket_init(gk_hw_plate_bracket *b, double width_mm,
                              double height_mm)
{
    if (b == NULL) {
        return;
    }
    memset(b, 0, sizeof(*b));
    b->width_mm = width_mm;
    b->height_mm = height_mm;
    b->angle_deg = 30.0;
    b->locked = 1;
}

gk_status gk_hw_plate_bracket_tilt(gk_hw_plate_bracket *b, double angle_deg)
{
    if (b == NULL || angle_deg < -90.0 || angle_deg > 90.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    b->angle_deg = angle_deg;
    return GK_OK;
}

/* ===================================================================
 * Real machine actions (1019-1050)
 * =================================================================== */

void gk_hw_spindle_init(gk_hw_spindle *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->inertia_kgm2 = 0.02;
}

gk_status gk_hw_spindle_start(gk_hw_spindle *s, double target_rpm,
                              double ramp_s)
{
    if (s == NULL || target_rpm < 0.0 || ramp_s < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    s->target_rpm = target_rpm;
    s->ramp_s = ramp_s;
    s->running = target_rpm > 0.0 ? 1 : 0;
    return GK_OK;
}

gk_status gk_hw_spindle_stop(gk_hw_spindle *s)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    s->target_rpm = 0.0;
    s->running = 0;
    return GK_OK;
}

double gk_hw_spindle_start_jitter(const gk_hw_spindle *s)
{
    if (s == NULL || s->inertia_kgm2 <= 0.0) {
        return 0.0;
    }
    /* lighter spindles jitter more during ramp-up */
    return 1.0 / s->inertia_kgm2;
}

double gk_hw_spindle_coast_distance(const gk_hw_spindle *s, double torque_nm)
{
    double omega;
    double alpha;
    double t;
    if (s == NULL || torque_nm <= 0.0 || s->inertia_kgm2 <= 0.0) {
        return 0.0;
    }
    omega = s->speed_rpm * 2.0 * M_PI / 60.0;
    alpha = torque_nm / s->inertia_kgm2;
    t = omega / alpha;
    /* distance (radians) = omega*t - 0.5*alpha*t^2 = 0.5*omega*t */
    return 0.5 * omega * t;
}

gk_status gk_hw_spindle_shift(gk_hw_spindle *s, int gear)
{
    if (s == NULL || gear < 1 || gear > 4) {
        return GK_ERR_OUT_OF_RANGE;
    }
    /* gear change requires a brief stop; represent as a speed ratio */
    s->speed_rpm = s->target_rpm / (double)gear;
    return GK_OK;
}

gk_status gk_hw_spindle_orient(gk_hw_spindle *s)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    /* orientation is a low-speed positioning stop */
    s->speed_rpm = 0.0;
    s->target_rpm = 0.0;
    s->running = 0;
    return GK_OK;
}

gk_status gk_hw_spindle_air_blast(gk_hw_spindle *s)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    /* air blast cleans the taper; only meaningful when not cutting */
    s->airborne = 1;
    return s->running ? GK_OK : GK_OK;
}

void gk_hw_drawbar_init(gk_hw_drawbar *d, double drawbar_force_n)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    d->drawbar_force_n = drawbar_force_n;
}

gk_status gk_hw_drawbar_clamp(gk_hw_drawbar *d)
{
    if (d == NULL || d->drawbar_force_n <= 0.0) {
        return GK_ERR_STATE;
    }
    d->clamped = 1;
    return GK_OK;
}

gk_status gk_hw_drawbar_release(gk_hw_drawbar *d)
{
    if (d == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    d->clamped = 0;
    return GK_OK;
}

const char *gk_hw_atc_state_name(gk_hw_atc_state s)
{
    switch (s) {
    case GK_HW_ATC_IDLE: return "idle";
    case GK_HW_ATC_EXTEND: return "extend";
    case GK_HW_ATC_GRIP: return "grip";
    case GK_HW_ATC_ROTATE: return "rotate";
    case GK_HW_ATC_RETRACT: return "retract";
    case GK_HW_ATC_DONE: return "done";
    case GK_HW_ATC_FAULT: return "fault";
    default: return "unknown";
    }
}

void gk_hw_atc_init(gk_hw_atc *a)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    a->tool_current = -1;
    a->tool_target = -1;
}

gk_status gk_hw_atc_request(gk_hw_atc *a, int tool_target)
{
    if (a == NULL || tool_target < 0) {
        return GK_ERR_INVALID_ARG;
    }
    if (a->state != GK_HW_ATC_IDLE && a->state != GK_HW_ATC_DONE) {
        return GK_ERR_STATE;
    }
    a->tool_target = tool_target;
    a->state = GK_HW_ATC_EXTEND;
    return GK_OK;
}

gk_status gk_hw_atc_step(gk_hw_atc *a)
{
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    switch (a->state) {
    case GK_HW_ATC_EXTEND:
        a->state = GK_HW_ATC_GRIP;
        a->gripped = 1;
        return GK_OK;
    case GK_HW_ATC_GRIP:
        a->state = GK_HW_ATC_ROTATE;
        a->arm_angle_deg = 90.0;
        return GK_OK;
    case GK_HW_ATC_ROTATE:
        a->state = GK_HW_ATC_RETRACT;
        return GK_OK;
    case GK_HW_ATC_RETRACT:
        a->state = GK_HW_ATC_DONE;
        a->gripped = 0;
        return GK_OK;
    default:
        return GK_ERR_STATE;
    }
}

gk_status gk_hw_atc_confirm(gk_hw_atc *a)
{
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (a->state != GK_HW_ATC_DONE) {
        a->state = GK_HW_ATC_FAULT;
        return GK_ERR_STATE;
    }
    a->tool_current = a->tool_target;
    return GK_OK;
}

void gk_hw_magazine_init(gk_hw_magazine *m, int slots)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    m->slots = slots > 0 ? slots : 12;
    m->index_time_s = 0.5;
    m->count = m->slots;
}

gk_status gk_hw_magazine_index(gk_hw_magazine *m, int slot)
{
    if (m == NULL || slot < 0 || slot >= m->slots) {
        return GK_ERR_OUT_OF_RANGE;
    }
    if (m->locked) {
        return GK_ERR_STATE;
    }
    m->position = slot;
    return GK_OK;
}

gk_status gk_hw_magazine_lock(gk_hw_magazine *m)
{
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    m->locked = 1;
    return GK_OK;
}

int gk_hw_magazine_count(const gk_hw_magazine *m)
{
    if (m == NULL) {
        return 0;
    }
    return m->count;
}

void gk_hw_setter_init(gk_hw_setter *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->contact_force_n = 2.0;
    s->retract_mm = 3.0;
}

gk_status gk_hw_setter_contact(gk_hw_setter *s, double measured_mm)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    s->measured_mm = measured_mm;
    s->signalled = 1;
    return GK_OK;
}

gk_status gk_hw_setter_retract(gk_hw_setter *s)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    s->signalled = 0;
    return GK_OK;
}

double gk_hw_setter_offset(const gk_hw_setter *s, double nominal_mm)
{
    if (s == NULL) {
        return 0.0;
    }
    return s->measured_mm - nominal_mm;
}

void gk_hw_clamp_init(gk_hw_clamp *c, double required_n)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    c->required_n = required_n;
}

gk_status gk_hw_clamp_close(gk_hw_clamp *c, double force_n)
{
    if (c == NULL || force_n < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    c->clamp_force_n = force_n;
    c->clamped = 1;
    return GK_OK;
}

gk_status gk_hw_clamp_open(gk_hw_clamp *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    c->clamp_force_n = 0.0;
    c->clamped = 0;
    return GK_OK;
}

int gk_hw_clamp_ok(const gk_hw_clamp *c)
{
    if (c == NULL || !c->clamped) {
        return 0;
    }
    return c->clamp_force_n >= c->required_n;
}

void gk_hw_tailstock_init(gk_hw_tailstock *t)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
}

gk_status gk_hw_tailstock_advance(gk_hw_tailstock *t, double quill_mm,
                                  double pressure_n)
{
    if (t == NULL || quill_mm < 0.0 || pressure_n < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    t->quill_mm = quill_mm;
    t->pressure_n = pressure_n;
    t->engaged = quill_mm > 0.0;
    return GK_OK;
}

gk_status gk_hw_tailstock_retract(gk_hw_tailstock *t)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    t->quill_mm = 0.0;
    t->pressure_n = 0.0;
    t->engaged = 0;
    return GK_OK;
}

void gk_hw_steady_init(gk_hw_steady *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->jaws = 3;
}

gk_status gk_hw_steady_clamp(gk_hw_steady *s, double jaw_force_n)
{
    if (s == NULL || jaw_force_n < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    s->jaw_force_n = jaw_force_n;
    s->clamped = 1;
    return GK_OK;
}

gk_status gk_hw_steady_release(gk_hw_steady *s)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    s->jaw_force_n = 0.0;
    s->clamped = 0;
    return GK_OK;
}

const char *gk_hw_chip_state_name(gk_hw_chip_state s)
{
    switch (s) {
    case GK_HW_CHIP_STOPPED: return "stopped";
    case GK_HW_CHIP_FORWARD: return "forward";
    case GK_HW_CHIP_REVERSE: return "reverse";
    default: return "unknown";
    }
}

void gk_hw_conveyor_init(gk_hw_conveyor *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    c->state = GK_HW_CHIP_STOPPED;
}

gk_status gk_hw_conveyor_start(gk_hw_conveyor *c, double speed_m_min)
{
    if (c == NULL || speed_m_min < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    c->speed_m_min = speed_m_min;
    c->state = GK_HW_CHIP_FORWARD;
    return GK_OK;
}

gk_status gk_hw_conveyor_stop(gk_hw_conveyor *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    c->state = GK_HW_CHIP_STOPPED;
    c->speed_m_min = 0.0;
    return GK_OK;
}

gk_status gk_hw_conveyor_reverse(gk_hw_conveyor *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    c->state = GK_HW_CHIP_REVERSE;
    return GK_OK;
}

void gk_hw_coolant_init(gk_hw_coolant *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    c->target_flow_lpm = 20.0;
}

gk_status gk_hw_coolant_start(gk_hw_coolant *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    c->running = 1;
    c->flow_lpm = c->target_flow_lpm;
    return GK_OK;
}

gk_status gk_hw_coolant_stop(gk_hw_coolant *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    c->running = 0;
    c->flow_lpm = 0.0;
    return GK_OK;
}

gk_status gk_hw_coolant_set_flow(gk_hw_coolant *c, double flow_lpm)
{
    if (c == NULL || flow_lpm < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    c->target_flow_lpm = flow_lpm;
    if (c->running) {
        c->flow_lpm = flow_lpm;
    }
    return GK_OK;
}

gk_status gk_hw_air_gun_blow(double pressure_mpa, double duration_s,
                             double *chips_removed)
{
    if (chips_removed == NULL || pressure_mpa < 0.0 || duration_s < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    /* empirical removal rate: pressure times time */
    *chips_removed = pressure_mpa * duration_s * 10.0;
    return GK_OK;
}

void gk_hw_auto_door_init(gk_hw_auto_door *d)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    d->open_pct = 0.0;
    d->buffer_pct = 15.0;
    d->anti_pinch = 1;
}

gk_status gk_hw_auto_door_open(gk_hw_auto_door *d, double target_pct)
{
    if (d == NULL || target_pct < 0.0 || target_pct > 100.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    if (d->blocked) {
        return GK_ERR_STATE;
    }
    d->open_pct = target_pct;
    return GK_OK;
}

gk_status gk_hw_auto_door_close(gk_hw_auto_door *d, double target_pct)
{
    return gk_hw_auto_door_open(d, target_pct);
}

int gk_hw_auto_door_stop_on_obstacle(gk_hw_auto_door *d, double obstacle_pct)
{
    if (d == NULL) {
        return 0;
    }
    if (d->anti_pinch && obstacle_pct > 0.0 && obstacle_pct < 100.0) {
        d->blocked = 1;
        return 1;
    }
    return 0;
}

void gk_hw_rotary_init(gk_hw_rotary *r, double max_speed_rpm)
{
    if (r == NULL) {
        return;
    }
    memset(r, 0, sizeof(*r));
    r->max_speed_rpm = max_speed_rpm;
    r->locked = 1;
}

gk_status gk_hw_rotary_rotate(gk_hw_rotary *r, double delta_deg)
{
    if (r == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (r->locked) {
        return GK_ERR_STATE;
    }
    r->angle_deg += delta_deg;
    while (r->angle_deg >= 360.0) {
        r->angle_deg -= 360.0;
    }
    while (r->angle_deg < 0.0) {
        r->angle_deg += 360.0;
    }
    return GK_OK;
}

gk_status gk_hw_rotary_lock(gk_hw_rotary *r)
{
    if (r == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    r->locked = 1;
    return GK_OK;
}

gk_status gk_hw_rotary_unlock(gk_hw_rotary *r)
{
    if (r == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    r->locked = 0;
    return GK_OK;
}

const char *gk_hw_indexer_name(void)
{
    return "indexing-table";
}

void gk_hw_indexer_init(gk_hw_indexer *i, int stations)
{
    if (i == NULL) {
        return;
    }
    memset(i, 0, sizeof(*i));
    i->stations = stations > 0 ? stations : 8;
    i->index_time_s = 0.3;
}

gk_status gk_hw_indexer_index(gk_hw_indexer *i, int station)
{
    if (i == NULL || station < 0 || station >= i->stations) {
        return GK_ERR_OUT_OF_RANGE;
    }
    i->position = station;
    return GK_OK;
}

gk_status gk_hw_indexer_lock(gk_hw_indexer *i)
{
    if (i == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    return GK_OK;
}
