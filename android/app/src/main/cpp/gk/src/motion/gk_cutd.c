#include "gk/gk_cutd.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ===================================================================
 * Cutting entry / exit (1061-1062)
 * =================================================================== */

void gk_cutd_entry_init(gk_cutd_entry *e, double material_factor)
{
    if (e == NULL) {
        return;
    }
    memset(e, 0, sizeof(*e));
    e->material_factor = material_factor > 0.0 ? material_factor : 1.0;
}

gk_status gk_cutd_entry_engage(gk_cutd_entry *e, double approach_speed,
                               double engagement_area)
{
    if (e == NULL || approach_speed < 0.0 || engagement_area < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    e->approach_speed = approach_speed;
    /* impulsive force proportional to speed, area and material */
    e->impact_force = approach_speed * engagement_area * e->material_factor;
    e->registered = e->impact_force > 0.0;
    return GK_OK;
}

double gk_cutd_entry_signal(const gk_cutd_entry *e)
{
    if (e == NULL || !e->registered) {
        return 0.0;
    }
    return e->impact_force;
}

void gk_cutd_exit_init(gk_cutd_exit *e)
{
    if (e == NULL) {
        return;
    }
    memset(e, 0, sizeof(*e));
    e->support_factor = 1.0;
}

gk_status gk_cutd_exit_leave(gk_cutd_exit *e, double edge_angle_deg,
                             double support_factor)
{
    if (e == NULL || edge_angle_deg < 0.0 || edge_angle_deg > 180.0 ||
        support_factor < 0.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    e->edge_angle_deg = edge_angle_deg;
    e->support_factor = support_factor;
    /* lower support and small exit angle create a larger burr */
    e->burr_height_um = (1.0 + (90.0 - edge_angle_deg) / 90.0) /
                        (support_factor + 0.1) * 10.0;
    return GK_OK;
}

double gk_cutd_exit_burr_height(const gk_cutd_exit *e)
{
    if (e == NULL) {
        return 0.0;
    }
    return e->burr_height_um;
}

int gk_cutd_exit_chips(const gk_cutd_exit *e, double threshold_um)
{
    if (e == NULL) {
        return 0;
    }
    return e->burr_height_um >= threshold_um;
}

/* ===================================================================
 * Cutting force behaviour (1063-1065)
 * =================================================================== */

void gk_cutd_force_wave_init(gk_cutd_force_wave *w, double mean_n,
                             double amplitude_n, double frequency_hz)
{
    if (w == NULL) {
        return;
    }
    memset(w, 0, sizeof(*w));
    w->mean_n = mean_n;
    w->amplitude_n = amplitude_n;
    w->frequency_hz = frequency_hz;
}

double gk_cutd_force_wave_at(const gk_cutd_force_wave *w, double t)
{
    if (w == NULL) {
        return 0.0;
    }
    return w->mean_n + w->amplitude_n * sin(2.0 * M_PI * w->frequency_hz * t);
}

void gk_cutd_force_jump_init(gk_cutd_force_jump *j)
{
    if (j == NULL) {
        return;
    }
    memset(j, 0, sizeof(*j));
}

gk_status gk_cutd_force_jump_update(gk_cutd_force_jump *j, double force_n,
                                    double threshold_n)
{
    if (j == NULL || threshold_n < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    j->previous_n = j->current_n;
    j->current_n = force_n;
    j->jump_n = fabs(j->current_n - j->previous_n);
    j->detected = (j->jump_n > threshold_n) ? 1 : 0;
    return GK_OK;
}

int gk_cutd_force_jump_detected(const gk_cutd_force_jump *j)
{
    if (j == NULL) {
        return 0;
    }
    return j->detected;
}

void gk_cutd_chatter_init(gk_cutd_chatter *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    c->natural_hz = 500.0;
    c->damping = 0.05;
}

gk_status gk_cutd_chatter_excite(gk_cutd_chatter *c, double excitation_hz,
                                 double depth_mm)
{
    if (c == NULL || excitation_hz <= 0.0 || depth_mm < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    c->excitation_hz = excitation_hz;
    /* resonance response grows when the excitation is near the natural
       frequency; a deeper cut raises the amplitude */
    c->amplitude = depth_mm / (fabs(excitation_hz - c->natural_hz) + 1.0);
    return GK_OK;
}

double gk_cutd_chatter_gain(const gk_cutd_chatter *c)
{
    if (c == NULL || c->damping <= 0.0) {
        return 0.0;
    }
    return 1.0 / (2.0 * c->damping);
}

int gk_cutd_chatter_unstable(const gk_cutd_chatter *c, double limit)
{
    if (c == NULL) {
        return 0;
    }
    return c->amplitude * gk_cutd_chatter_gain(c) >= limit;
}

/* ===================================================================
 * Noise, sparks, smoke, smell (1066-1069)
 * =================================================================== */

void gk_cutd_noise_init(gk_cutd_noise *n)
{
    if (n == NULL) {
        return;
    }
    memset(n, 0, sizeof(*n));
    n->base_db = 60.0;
    n->level_db = 60.0;
}

double gk_cutd_noise_from_speed(gk_cutd_noise *n, double cutting_speed)
{
    if (n == NULL || cutting_speed < 0.0) {
        return 0.0;
    }
    /* 20*log10 reference scaling; speed in m/min */
    n->level_db = n->base_db + 20.0 * log10(1.0 + cutting_speed);
    return n->level_db;
}

double gk_cutd_noise_from_force(gk_cutd_noise *n, double force_n)
{
    if (n == NULL || force_n < 0.0) {
        return 0.0;
    }
    n->level_db = n->base_db + 10.0 * log10(1.0 + force_n);
    return n->level_db;
}

void gk_cutd_spark_init(gk_cutd_spark *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
}

gk_status gk_cutd_spark_emit(gk_cutd_spark *s, double material_hardness,
                             double cutting_speed)
{
    if (s == NULL || material_hardness < 0.0 || cutting_speed < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    /* sparks need both hard material and high speed */
    s->rate_per_s = material_hardness * cutting_speed / 100.0;
    s->intensity = s->rate_per_s / (1.0 + s->rate_per_s);
    s->active = s->rate_per_s > 0.0;
    return GK_OK;
}

int gk_cutd_spark_visible(const gk_cutd_spark *s, double threshold)
{
    if (s == NULL || !s->active) {
        return 0;
    }
    return s->intensity >= threshold;
}

void gk_cutd_smoke_init(gk_cutd_smoke *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
}

double gk_cutd_smoke_generate(gk_cutd_smoke *s, double heat_index,
                              double coolant_flow)
{
    if (s == NULL || heat_index < 0.0 || coolant_flow < 0.0) {
        return 0.0;
    }
    /* coolant suppresses smoke */
    s->coolant_effect = 1.0 / (1.0 + coolant_flow);
    s->density = heat_index * s->coolant_effect;
    return s->density;
}

const char *gk_cutd_smell_name(gk_cutd_smell s)
{
    switch (s) {
    case GK_CUTD_SMELL_NONE: return "none";
    case GK_CUTD_SMELL_OIL: return "oil";
    case GK_CUTD_SMELL_BURN: return "burn";
    case GK_CUTD_SMELL_COOLANT: return "coolant";
    default: return "unknown";
    }
}

gk_cutd_smell gk_cutd_smell_classify(double temperature_c, int coolant_used)
{
    if (temperature_c > 500.0) {
        return GK_CUTD_SMELL_BURN;
    }
    if (coolant_used && temperature_c > 100.0) {
        return GK_CUTD_SMELL_COOLANT;
    }
    if (temperature_c > 200.0) {
        return GK_CUTD_SMELL_OIL;
    }
    return GK_CUTD_SMELL_NONE;
}

/* ===================================================================
 * Chips (1070-1074)
 * =================================================================== */

void gk_cutd_chip_fly_init(gk_cutd_chip_fly *f)
{
    if (f == NULL) {
        return;
    }
    memset(f, 0, sizeof(*f));
    f->angle_deg = 45.0;
}

double gk_cutd_chip_fly_compute(gk_cutd_chip_fly *f, double spindle_rpm,
                                double diameter_mm)
{
    double omega;
    if (f == NULL || spindle_rpm < 0.0 || diameter_mm < 0.0) {
        return 0.0;
    }
    omega = spindle_rpm * 2.0 * M_PI / 60.0;
    f->velocity_m_s = omega * (diameter_mm / 2000.0);
    /* ballistic range for launch at the stored angle, ignoring drag */
    f->range_mm = f->velocity_m_s * f->velocity_m_s *
                  sin(2.0 * f->angle_deg * M_PI / 180.0) / 9.81 * 1000.0;
    return f->range_mm;
}

void gk_cutd_chip_pile_init(gk_cutd_chip_pile *p)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->produced_g_s = 0.5;
    p->removed_g_s = 0.4;
}

gk_status gk_cutd_chip_pile_update(gk_cutd_chip_pile *p, double dt)
{
    if (p == NULL || dt < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    p->accumulated_g += (p->produced_g_s - p->removed_g_s) * dt;
    if (p->accumulated_g < 0.0) {
        p->accumulated_g = 0.0;
    }
    return GK_OK;
}

int gk_cutd_chip_pile_clear(gk_cutd_chip_pile *p, double capacity_g)
{
    if (p == NULL || capacity_g <= 0.0) {
        return 0;
    }
    if (p->accumulated_g >= capacity_g) {
        p->accumulated_g = 0.0;
        return 1;
    }
    return 0;
}

void gk_cutd_chip_tangle_init(gk_cutd_chip_tangle *t)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->curl_ratio = 1.0;
}

gk_status gk_cutd_chip_tangle_grow(gk_cutd_chip_tangle *t, double feed_mm)
{
    if (t == NULL || feed_mm < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    t->length_mm += feed_mm / t->curl_ratio;
    return GK_OK;
}

int gk_cutd_chip_tangle_check(const gk_cutd_chip_tangle *t, double limit_mm)
{
    if (t == NULL) {
        return 0;
    }
    return t->length_mm >= limit_mm;
}

void gk_cutd_chip_break_init(gk_cutd_chip_break *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    c->critical_depth_mm = 0.2;
}

int gk_cutd_chip_break_check(gk_cutd_chip_break *c, double depth_mm,
                             double feed_mm_rev)
{
    if (c == NULL || depth_mm < 0.0 || feed_mm_rev < 0.0) {
        return 0;
    }
    c->feed_mm_rev = feed_mm_rev;
    /* chips break when the depth of cut exceeds a feed-scaled threshold */
    c->broken = (depth_mm * feed_mm_rev >= c->critical_depth_mm) ? 1 : 0;
    return c->broken;
}

gk_status gk_cutd_chip_colour(double temperature_c, char *out, size_t out_cap)
{
    int n;
    const char *colour;
    if (out == NULL || out_cap == 0 || temperature_c < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    if (temperature_c < 200.0) {
        colour = "silver";
    } else if (temperature_c < 350.0) {
        colour = "straw";
    } else if (temperature_c < 500.0) {
        colour = "brown";
    } else if (temperature_c < 650.0) {
        colour = "blue";
    } else {
        colour = "purple";
    }
    n = snprintf(out, out_cap, "%s", colour);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

/* ===================================================================
 * Tool wear (1075-1079)
 * =================================================================== */

const char *gk_cutd_wear_name(gk_cutd_wear_kind k)
{
    switch (k) {
    case GK_CUTD_WEAR_NORMAL: return "normal";
    case GK_CUTD_WEAR_RAPID: return "rapid";
    case GK_CUTD_WEAR_CHIP: return "chip";
    case GK_CUTD_WEAR_BREAK: return "break";
    default: return "unknown";
    }
}

void gk_cutd_tool_wear_init(gk_cutd_tool_wear *w, double normal_rate)
{
    if (w == NULL) {
        return;
    }
    memset(w, 0, sizeof(*w));
    w->normal_rate = normal_rate > 0.0 ? normal_rate : 0.001;
}

gk_status gk_cutd_tool_wear_advance(gk_cutd_tool_wear *w, double dt_min,
                                    double load_factor)
{
    if (w == NULL || dt_min < 0.0 || load_factor < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    w->wear_mm += w->normal_rate * dt_min * load_factor;
    w->time_min += dt_min;
    return GK_OK;
}

gk_status gk_cutd_tool_wear_shock(gk_cutd_tool_wear *w, double shock_mm)
{
    if (w == NULL || shock_mm < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    w->wear_mm += shock_mm;
    return GK_OK;
}

int gk_cutd_tool_wear_break_check(gk_cutd_tool_wear *w, double limit_mm)
{
    if (w == NULL || limit_mm <= 0.0) {
        return 0;
    }
    if (w->wear_mm >= limit_mm) {
        w->broken = 1;
        return 1;
    }
    return 0;
}

int gk_cutd_tool_wear_chipped(const gk_cutd_tool_wear *w, double jump_mm)
{
    if (w == NULL) {
        return 0;
    }
    /* a sudden wear increment indicates a chipped edge */
    return w->wear_mm >= jump_mm;
}

void gk_cutd_redheat_init(gk_cutd_redheat *r, double critical_c)
{
    if (r == NULL) {
        return;
    }
    memset(r, 0, sizeof(*r));
    r->critical_c = critical_c;
    r->temperature_c = 20.0;
}

double gk_cutd_redheat_update(gk_cutd_redheat *r, double heat_input,
                              double cooling, double dt)
{
    if (r == NULL || dt < 0.0) {
        return 0.0;
    }
    r->temperature_c += (heat_input - cooling) * dt;
    if (r->temperature_c < 20.0) {
        r->temperature_c = 20.0;
    }
    return r->temperature_c;
}

int gk_cutd_redheat_critical(const gk_cutd_redheat *r)
{
    if (r == NULL) {
        return 0;
    }
    return r->temperature_c >= r->critical_c;
}

/* ===================================================================
 * Surface (1080-1084)
 * =================================================================== */

void gk_cutd_pattern_init(gk_cutd_pattern *p)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->feed_mm_rev = 0.1;
    p->tool_radius_mm = 0.8;
}

double gk_cutd_pattern_pitch(gk_cutd_pattern *p, double feed_mm_rev)
{
    if (p == NULL) {
        return 0.0;
    }
    p->feed_mm_rev = feed_mm_rev;
    p->pitch_mm = feed_mm_rev;
    return p->pitch_mm;
}

double gk_cutd_pattern_ra(const gk_cutd_pattern *p)
{
    if (p == NULL || p->tool_radius_mm <= 0.0) {
        return 0.0;
    }
    /* theoretical peak-to-valley roughness for a round tool */
    return (p->feed_mm_rev * p->feed_mm_rev) / (8.0 * p->tool_radius_mm) * 1000.0;
}

const char *gk_cutd_defect_name(gk_cutd_defect d)
{
    switch (d) {
    case GK_CUTD_DEFECT_NONE: return "none";
    case GK_CUTD_DEFECT_BURN: return "burn";
    case GK_CUTD_DEFECT_SCRATCH: return "scratch";
    case GK_CUTD_DEFECT_CHATTER: return "chatter";
    case GK_CUTD_DEFECT_BURR: return "burr";
    default: return "unknown";
    }
}

void gk_cutd_surface_init(gk_cutd_surface *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->defect = GK_CUTD_DEFECT_NONE;
}

gk_status gk_cutd_surface_detect(gk_cutd_surface *s, double temperature_c,
                                 double vibration, double burr_um,
                                 double scratch_um)
{
    if (s == NULL || temperature_c < 0.0 || vibration < 0.0 || burr_um < 0.0 ||
        scratch_um < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    s->detected = 1;
    if (temperature_c > 600.0) {
        s->defect = GK_CUTD_DEFECT_BURN;
        s->severity = temperature_c / 600.0;
    } else if (scratch_um > 5.0) {
        s->defect = GK_CUTD_DEFECT_SCRATCH;
        s->severity = scratch_um / 5.0;
    } else if (vibration > 10.0) {
        s->defect = GK_CUTD_DEFECT_CHATTER;
        s->severity = vibration / 10.0;
    } else if (burr_um > 50.0) {
        s->defect = GK_CUTD_DEFECT_BURR;
        s->severity = burr_um / 50.0;
    } else {
        s->defect = GK_CUTD_DEFECT_NONE;
        s->severity = 0.0;
    }
    return GK_OK;
}

/* ===================================================================
 * Drift, thermal, stress, deflection (1085-1090)
 * =================================================================== */

void gk_cutd_drift_init(gk_cutd_drift *d, double nominal_mm)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    d->nominal_mm = nominal_mm;
    d->drift_rate = 0.001;
}

double gk_cutd_drift_update(gk_cutd_drift *d, double dt_min, double wear_source)
{
    if (d == NULL || dt_min < 0.0 || wear_source < 0.0) {
        return 0.0;
    }
    d->drift_mm += d->drift_rate * dt_min * wear_source;
    return d->drift_mm;
}

double gk_cutd_drift_deviation(const gk_cutd_drift *d)
{
    if (d == NULL) {
        return 0.0;
    }
    return d->drift_mm;
}

void gk_cutd_thermal_init(gk_cutd_thermal *t, double length_mm,
                          double alpha_per_c)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->length_mm = length_mm;
    t->alpha_per_c = alpha_per_c;
}

double gk_cutd_thermal_expansion(gk_cutd_thermal *t, double temp_delta_c)
{
    if (t == NULL) {
        return 0.0;
    }
    t->temp_delta_c = temp_delta_c;
    return t->length_mm * t->alpha_per_c * temp_delta_c;
}

double gk_cutd_thermal_contraction(gk_cutd_thermal *t, double temp_delta_c)
{
    /* cooling is simply negative expansion */
    return gk_cutd_thermal_expansion(t, -temp_delta_c);
}

void gk_cutd_stress_init(gk_cutd_stress *s, double modulus_mpa,
                         double length_mm)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->modulus_mpa = modulus_mpa;
    s->length_mm = length_mm;
}

double gk_cutd_stress_distortion(gk_cutd_stress *s, double stress_mpa)
{
    if (s == NULL || s->modulus_mpa <= 0.0) {
        return 0.0;
    }
    s->stress_mpa = stress_mpa;
    return s->length_mm * stress_mpa / s->modulus_mpa;
}

void gk_cutd_deflect_init(gk_cutd_deflect *d, double stiffness_n_mm)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    d->stiffness_n_mm = stiffness_n_mm;
}

double gk_cutd_deflect_apply(gk_cutd_deflect *d, double force_n)
{
    if (d == NULL || d->stiffness_n_mm <= 0.0) {
        return 0.0;
    }
    d->force_n = force_n;
    d->deflection_mm = force_n / d->stiffness_n_mm;
    return d->deflection_mm;
}

double gk_cutd_deflect_release(gk_cutd_deflect *d, double recovery_ratio)
{
    if (d == NULL) {
        return 0.0;
    }
    if (recovery_ratio < 0.0) {
        recovery_ratio = 0.0;
    }
    if (recovery_ratio > 1.0) {
        recovery_ratio = 1.0;
    }
    d->recovered_mm = d->deflection_mm * recovery_ratio;
    d->force_n = 0.0;
    d->deflection_mm -= d->recovered_mm;
    return d->recovered_mm;
}
