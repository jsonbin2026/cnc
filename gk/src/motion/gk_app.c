#include "gk/gk_app.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void gk__copy(char *dst, size_t cap, const char *src)
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
 * Part A: surface detail & markings
 * =================================================================== */

void gk_app_texture_init(gk_app_texture *t, double roughness_um,
                         double metallic)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->roughness_um = roughness_um;
    t->metallic = metallic;
    t->scale = 1.0;
    t->seed = 1u;
}

double gk_app_texture_height(const gk_app_texture *t, double u, double v)
{
    unsigned int h;
    double noise;
    if (t == NULL) {
        return 0.0;
    }
    /* deterministic hash-based pseudo noise in [-0.5, 0.5] */
    h = t->seed * 2654435761u;
    h ^= (unsigned int)((u * 97.0 + v * 131.0) * 1000.0);
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= (h >> 16);
    noise = ((double)(h & 0xFFFFu) / 65535.0) - 0.5;
    return noise * t->roughness_um * t->scale;
}

void gk_app_seam_init(gk_app_seam *s, double gap_mm, double depth_mm)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->gap_mm = gap_mm;
    s->depth_mm = depth_mm;
    s->visible = 1;
}

double gk_app_seam_area(const gk_app_seam *s, double length_mm)
{
    if (s == NULL || length_mm < 0.0) {
        return 0.0;
    }
    /* the visible seam is the gap plus twice the depth wall */
    return length_mm * (s->gap_mm + 2.0 * s->depth_mm);
}

const char *gk_app_screw_name(gk_app_screw_head h)
{
    switch (h) {
    case GK_APP_SCREW_SOCKET: return "socket";
    case GK_APP_SCREW_HEX: return "hex";
    case GK_APP_SCREW_PHILLIPS: return "phillips";
    default: return "unknown";
    }
}

void gk_app_screw_init(gk_app_screw *s, gk_app_screw_head head,
                       double diameter_mm, double length_mm)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->head = head;
    s->diameter_mm = diameter_mm;
    s->length_mm = length_mm;
    s->count = 1;
}

double gk_app_screw_total_length(const gk_app_screw *s)
{
    if (s == NULL) {
        return 0.0;
    }
    return s->length_mm * (double)s->count;
}

void gk_app_nameplate_init(gk_app_nameplate *p, double width_mm,
                           double height_mm, const char *material)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->width_mm = width_mm;
    p->height_mm = height_mm;
    gk__copy(p->material, sizeof(p->material), material);
    p->rivets = 4;
}

double gk_app_nameplate_area(const gk_app_nameplate *p)
{
    if (p == NULL) {
        return 0.0;
    }
    return p->width_mm * p->height_mm;
}

gk_status gk_app_serial_render(const char *serial, char *out, size_t out_cap)
{
    int n;
    if (serial == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "SN:%s", serial);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

const char *gk_app_warning_text(gk_app_warning w)
{
    switch (w) {
    case GK_APP_WARN_ELECTRICAL: return "DANGER - ELECTRICAL HAZARD";
    case GK_APP_WARN_HOT: return "WARNING - HOT SURFACE";
    case GK_APP_WARN_CRUSH: return "WARNING - CRUSH HAZARD";
    case GK_APP_WARN_ENTANGLE: return "WARNING - ROTATING PARTS";
    case GK_APP_WARN_LASER: return "DANGER - LASER RADIATION";
    default: return "WARNING";
    }
}

gk_status gk_app_warning_label(gk_app_warning w, char *out, size_t out_cap)
{
    int n;
    if (out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "[%s]", gk_app_warning_text(w));
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

void gk_app_sticker_init(gk_app_sticker *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
}

gk_status gk_app_sticker_add(gk_app_sticker *s, const char *step_text)
{
    size_t used;
    if (s == NULL || step_text == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (s->step != 0) {
        used = strlen(s->text);
        if (used + 1 < sizeof(s->text)) {
            s->text[used] = '\n';
            s->text[used + 1] = '\0';
        }
    }
    used = strlen(s->text);
    {
        size_t remain = sizeof(s->text) - used;
        gk__copy(s->text + used, remain, step_text);
    }
    s->step++;
    return GK_OK;
}

int gk_app_sticker_count(const gk_app_sticker *s)
{
    if (s == NULL) {
        return 0;
    }
    return s->step;
}

void gk_app_logo_init(gk_app_logo *l, const char *brand)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
    gk__copy(l->brand, sizeof(l->brand), brand);
    l->width_mm = 60.0;
    l->height_mm = 20.0;
    l->monochrome = 1;
}

gk_status gk_app_logo_render(const gk_app_logo *l, char *out, size_t out_cap)
{
    int n;
    if (l == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "[LOGO %s %.0fx%.0f]", l->brand, l->width_mm,
                 l->height_mm);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

gk_status gk_app_model_render(const char *series, int number, char *out,
                              size_t out_cap)
{
    int n;
    if (series == NULL || out == NULL || out_cap == 0 || number < 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "%s-%04d", series, number);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

gk_status gk_app_date_render(int year, int month, int day, char *out,
                             size_t out_cap)
{
    int n;
    if (out == NULL || out_cap == 0 || month < 1 || month > 12 || day < 1 ||
        day > 31) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "%04d-%02d-%02d", year, month, day);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

/* ===================================================================
 * Part B: covers, routing, accessories
 * =================================================================== */

const char *gk_app_cover_name(gk_app_cover_kind k)
{
    switch (k) {
    case GK_APP_COVER_DUST: return "dust-cover";
    case GK_APP_COVER_TELESCOPIC: return "telescopic";
    case GK_APP_COVER_BELLOWS: return "bellows";
    case GK_APP_COVER_SPIRAL: return "spiral-band";
    default: return "unknown";
    }
}

void gk_app_cover_init(gk_app_cover *c, gk_app_cover_kind kind,
                       double stroke_mm, double closed_length_mm)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    c->kind = kind;
    c->stroke_mm = stroke_mm;
    c->closed_length_mm = closed_length_mm;
    c->section_pitch_mm = 5.0;
}

double gk_app_cover_extended(const gk_app_cover *c)
{
    if (c == NULL) {
        return 0.0;
    }
    return c->closed_length_mm + c->stroke_mm;
}

int gk_app_cover_folds(const gk_app_cover *c)
{
    if (c == NULL || c->section_pitch_mm <= 0.0) {
        return 0;
    }
    return (int)ceil(c->stroke_mm / c->section_pitch_mm);
}

void gk_app_dragchain_init(gk_app_dragchain *d, double link_pitch_mm,
                           double bend_radius_mm)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    d->link_pitch_mm = link_pitch_mm;
    d->bend_radius_mm = bend_radius_mm;
}

int gk_app_dragchain_links(const gk_app_dragchain *d, double travel_mm)
{
    double needed;
    if (d == NULL || d->link_pitch_mm <= 0.0) {
        return 0;
    }
    /* the chain folds back on itself, so it is about half the travel */
    needed = travel_mm / 2.0 + M_PI * d->bend_radius_mm;
    return (int)ceil(needed / d->link_pitch_mm);
}

double gk_app_dragchain_length(const gk_app_dragchain *d, double travel_mm)
{
    if (d == NULL) {
        return 0.0;
    }
    return (double)gk_app_dragchain_links(d, travel_mm) * d->link_pitch_mm;
}

const char *gk_app_route_name(gk_app_route_kind k)
{
    switch (k) {
    case GK_APP_ROUTE_CABLE: return "cable";
    case GK_APP_ROUTE_AIR: return "air";
    case GK_APP_ROUTE_OIL: return "oil";
    default: return "unknown";
    }
}

void gk_app_route_init(gk_app_route *r, gk_app_route_kind kind,
                       double diameter_mm, double length_mm)
{
    if (r == NULL) {
        return;
    }
    memset(r, 0, sizeof(*r));
    r->kind = kind;
    r->diameter_mm = diameter_mm;
    r->length_mm = length_mm;
    r->bend_radius_mm = 3.0 * diameter_mm;
    r->segments = 1;
}

double gk_app_route_total(const gk_app_route *r)
{
    if (r == NULL) {
        return 0.0;
    }
    return r->length_mm + (double)r->segments * 0.5 * M_PI * r->bend_radius_mm;
}

void gk_app_hydraulic_init(gk_app_hydraulic *h, double pressure_mpa,
                           double flow_lpm, double tank_l)
{
    if (h == NULL) {
        return;
    }
    memset(h, 0, sizeof(*h));
    h->pressure_mpa = pressure_mpa;
    h->flow_lpm = flow_lpm;
    h->tank_l = tank_l;
    h->running = 1;
}

double gk_app_hydraulic_power_kw(const gk_app_hydraulic *h)
{
    double p_pa;
    double q_m3s;
    if (h == NULL || !h->running) {
        return 0.0;
    }
    p_pa = h->pressure_mpa * 1.0e6;
    q_m3s = h->flow_lpm / 60000.0;
    return p_pa * q_m3s / 1000.0;
}

void gk_app_pneumatic_init(gk_app_pneumatic *p, double pressure_mpa)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->pressure_mpa = pressure_mpa;
    p->filter_um = 5.0;
    p->lubricator_ml = 100.0;
    p->regulator_ok = 1;
}

int gk_app_pneumatic_ok(const gk_app_pneumatic *p)
{
    if (p == NULL) {
        return 0;
    }
    return p->regulator_ok && p->pressure_mpa > 0.0 && p->filter_um > 0.0;
}

void gk_app_chiller_init(gk_app_chiller *c, double capacity_kw,
                         double setpoint_c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    c->capacity_kw = capacity_kw;
    c->setpoint_c = setpoint_c;
    c->actual_c = setpoint_c;
}

double gk_app_chiller_error(const gk_app_chiller *c)
{
    if (c == NULL) {
        return 0.0;
    }
    return c->actual_c - c->setpoint_c;
}

void gk_app_cabinet_init(gk_app_cabinet *c, double width_mm, double height_mm)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    c->width_mm = width_mm;
    c->height_mm = height_mm;
    c->fan_cfm = 100.0;
    c->vent_area_cm2 = 80.0;
}

gk_status gk_app_cabinet_door(gk_app_cabinet *c, int open)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    c->door_open = open ? 1 : 0;
    return GK_OK;
}

int gk_app_cabinet_fan_needed(const gk_app_cabinet *c, double internal_temp_c,
                              double ambient_temp_c)
{
    if (c == NULL) {
        return 0;
    }
    if (c->door_open) {
        return 0;
    }
    return (internal_temp_c - ambient_temp_c) > 10.0;
}

double gk_app_cabinet_vent_ratio(const gk_app_cabinet *c)
{
    double face;
    if (c == NULL || c->width_mm <= 0.0 || c->height_mm <= 0.0) {
        return 0.0;
    }
    face = c->width_mm * c->height_mm;   /* mm^2 */
    return (c->vent_area_cm2 * 100.0) / face;
}

const char *gk_app_cool_name(gk_app_cool_kind k)
{
    switch (k) {
    case GK_APP_COOL_SPINDLE: return "spindle-cooling";
    case GK_APP_COOL_TOOL: return "tool-cooling";
    case GK_APP_COOL_NOZZLE: return "universal-nozzle";
    case GK_APP_COOL_INTERNAL: return "internal-channel";
    default: return "unknown";
    }
}

void gk_app_cool_init(gk_app_cool *c, gk_app_cool_kind kind, double diameter_mm)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    c->kind = kind;
    c->diameter_mm = diameter_mm;
    c->pressure_mpa = 0.5;
    c->flow_lpm = 10.0;
    c->angle_deg = 0.0;
}

double gk_app_cool_velocity(const gk_app_cool *c)
{
    double area;
    double q_m3s;
    if (c == NULL || c->diameter_mm <= 0.0) {
        return 0.0;
    }
    area = M_PI * (c->diameter_mm / 2000.0) * (c->diameter_mm / 2000.0);
    q_m3s = c->flow_lpm / 60000.0;
    return q_m3s / area;
}

gk_status gk_app_cool_aim(gk_app_cool *c, double angle_deg)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (angle_deg < -180.0 || angle_deg > 180.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    c->angle_deg = angle_deg;
    return GK_OK;
}

void gk_app_light_init(gk_app_light *l, double brightness_lm, double colour_k)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
    l->brightness_lm = brightness_lm;
    l->colour_k = colour_k;
}

double gk_app_light_illuminance(const gk_app_light *l, double distance_m)
{
    if (l == NULL || !l->on || distance_m <= 0.0) {
        return 0.0;
    }
    return l->brightness_lm / (4.0 * M_PI * distance_m * distance_m);
}

const char *gk_app_stack_name(gk_app_stack_state s)
{
    switch (s) {
    case GK_APP_STACK_RED: return "red";
    case GK_APP_STACK_AMBER: return "amber";
    case GK_APP_STACK_GREEN: return "green";
    case GK_APP_STACK_OFF: return "off";
    default: return "unknown";
    }
}

void gk_app_stack_init(gk_app_stack *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->state = GK_APP_STACK_OFF;
}

gk_status gk_app_stack_set(gk_app_stack *s, gk_app_stack_state state)
{
    if (s == NULL || state > GK_APP_STACK_OFF) {
        return GK_ERR_INVALID_ARG;
    }
    s->state = state;
    s->buzzer = (state == GK_APP_STACK_RED) ? 1 : 0;
    return GK_OK;
}

gk_app_stack_state gk_app_stack_from_alarm(int severity)
{
    if (severity >= 3) {
        return GK_APP_STACK_RED;
    }
    if (severity == 2) {
        return GK_APP_STACK_AMBER;
    }
    if (severity == 1) {
        return GK_APP_STACK_GREEN;
    }
    return GK_APP_STACK_OFF;
}
