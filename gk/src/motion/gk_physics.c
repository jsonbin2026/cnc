#include "gk/gk_physics.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define GK_PI 3.14159265358979323846

/* ---- structure ---- */

void gk_structure_init(gk_structure *s, double mass, double stiffness,
                       double damping)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->mass = mass;
    s->stiffness = stiffness;
    s->damping = damping;
    s->natural_freq = gk_structure_natural_freq(s);
}

double gk_structure_natural_freq(const gk_structure *s)
{
    if (s == NULL || s->mass <= 0.0 || s->stiffness <= 0.0) {
        return 0.0;
    }
    return sqrt(s->stiffness / s->mass) / (2.0 * GK_PI);
}

double gk_structure_damping_ratio(const gk_structure *s)
{
    double ccrit;
    if (s == NULL) {
        return 0.0;
    }
    ccrit = 2.0 * sqrt(s->stiffness * s->mass);
    return ccrit > 0.0 ? s->damping / ccrit : 0.0;
}

double gk_chatter_stability_limit(const gk_structure *s, double cutting_coeff,
                                  double spindle_rpm, int teeth,
                                  gk_chatter_kind kind)
{
    double zeta, wn, limit;
    (void)spindle_rpm;
    if (s == NULL || cutting_coeff <= 0.0 || teeth <= 0) {
        return 0.0;
    }
    zeta = gk_structure_damping_ratio(s);
    wn = gk_structure_natural_freq(s) * 2.0 * GK_PI;
    if (wn <= 0.0) {
        return 0.0;
    }
    /* Tlusty: blim = 1 / (2 * Kc * Re[G]) with Re[G] ~ zeta / (k * ...) */
    limit = 2.0 * zeta * (1.0 - zeta * zeta);
    limit = limit / cutting_coeff;
    limit = limit * s->stiffness / 1000.0;   /* N/m -> N/mm */
    if (kind == GK_CHATTER_MILLING) {
        limit *= (double)teeth;
    }
    (void)wn;
    return limit;
}

double gk_chatter_growth_rate(const gk_structure *s, double depth,
                              double cutting_coeff, double width)
{
    double wn, zeta, drive;
    if (s == NULL) {
        return 0.0;
    }
    wn = gk_structure_natural_freq(s) * 2.0 * GK_PI;
    zeta = gk_structure_damping_ratio(s);
    /* negative damping produced by the regenerative effect */
    drive = depth * cutting_coeff * width / s->stiffness;
    return wn * (drive - 2.0 * zeta) * 0.5;
}

/* ---- lobe ---- */

void gk_lobe_init(gk_lobe *l)
{
    if (l != NULL) {
        memset(l, 0, sizeof(*l));
    }
}

int gk_lobe_generate(gk_lobe *l, const gk_structure *s, double cutting_coeff,
                     int teeth, int lobe_number)
{
    int i;
    double fn;
    if (l == NULL || s == NULL || teeth <= 0 || lobe_number < 1) {
        return 0;
    }
    fn = gk_structure_natural_freq(s);
    if (fn <= 0.0) {
        return 0;
    }
    memset(l, 0, sizeof(*l));
    for (i = 0; i < GK_PHYS_MAX_LOBE; ++i) {
        double rpm_lo = (double)(i + 1) * 100.0;
        double rpm_hi = rpm_lo + 100.0;
        double n = lobe_number;
        double rpm = 60.0 * fn / (n * teeth) *
                     (1.0 + (double)i / GK_PHYS_MAX_LOBE);
        double depth;
        if (rpm <= 0.0) {
            continue;
        }
        /* lobes rise with rpm; approximate with a harmonic envelope */
        depth = gk_chatter_stability_limit(s, cutting_coeff, rpm, teeth,
                                           GK_CHATTER_MILLING);
        depth *= 0.5 + 0.5 * (double)(i % 20) / 20.0;
        (void)rpm_lo;
        (void)rpm_hi;
        l->rpm[l->count] = rpm;
        l->depth[l->count] = depth;
        l->count++;
    }
    return l->count;
}

double gk_lobe_max_depth(const gk_lobe *l)
{
    double best = 0.0;
    int i;
    if (l == NULL) {
        return 0.0;
    }
    for (i = 0; i < l->count; ++i) {
        if (l->depth[i] > best) best = l->depth[i];
    }
    return best;
}

int gk_lobe_safe_at(const gk_lobe *l, double rpm, double depth)
{
    int i;
    double best_rpm = -1.0;
    double best_diff = 1e30;
    if (l == NULL || l->count == 0) {
        return 1;
    }
    /* find the nearest lobe point in rpm */
    for (i = 0; i < l->count; ++i) {
        double diff = fabs(l->rpm[i] - rpm);
        if (diff < best_diff) {
            best_diff = diff;
            best_rpm = l->depth[i];
        }
    }
    return depth <= best_rpm;
}

/* ---- workpiece ---- */

void gk_workpiece_init(gk_workpiece *w, double l, double wd, double h,
                       double e, double nu)
{
    if (w == NULL) {
        return;
    }
    memset(w, 0, sizeof(*w));
    w->length = l;
    w->width = wd;
    w->height = h;
    w->youngs = e;
    w->poisson = nu;
}

double gk_workpiece_stiffness(const gk_workpiece *w)
{
    double I;
    if (w == NULL || w->length <= 0.0) {
        return 0.0;
    }
    /* rectangular cantilever, I = b*h^3/12 (in m^4 using mm->m) */
    I = (w->width / 1000.0) * pow(w->height / 1000.0, 3.0) / 12.0;
    return 3.0 * w->youngs * 1e9 * I / pow(w->length / 1000.0, 3.0);
}

double gk_workpiece_deflection(const gk_workpiece *w, double force,
                               double overhang)
{
    double I;
    if (w == NULL || w->length <= 0.0 || overhang <= 0.0) {
        return 0.0;
    }
    I = (w->width / 1000.0) * pow(w->height / 1000.0, 3.0) / 12.0;
    /* cantilever load at distance L: d = F L^3 / (3 E I) */
    return force * pow(overhang / 1000.0, 3.0) /
           (3.0 * w->youngs * 1e9 * I);
}

/* ---- thermal ---- */

void gk_thermo_init(gk_thermo *t, double alpha, double length,
                    double ref_temp)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->alpha = alpha;
    t->length = length;
    t->ref_temp = ref_temp;
    t->temp = ref_temp;
}

double gk_thermo_expansion(const gk_thermo *t)
{
    if (t == NULL) {
        return 0.0;
    }
    return t->alpha * t->length * (t->temp - t->ref_temp);
}

double gk_thermo_coupled_strain(const gk_thermo *t, double stress,
                                double youngs)
{
    double thermal;
    if (t == NULL || youngs <= 0.0) {
        return 0.0;
    }
    thermal = t->alpha * (t->temp - t->ref_temp);
    return thermal + stress / youngs;
}

double gk_spindle_growth(double max_growth, double tau, double time)
{
    if (tau <= 0.0 || time < 0.0) {
        return 0.0;
    }
    return max_growth * (1.0 - exp(-time / tau));
}

double gk_screw_deformation(double alpha, double length, double delta_temp,
                            double preload, double area, double youngs)
{
    double thermal = alpha * length * delta_temp;
    double elastic = (area > 0.0 && youngs > 0.0) ? preload * length /
                                                    (area * youngs) : 0.0;
    return thermal + elastic;
}

/* ---- modal ---- */

void gk_modal_init(gk_modal *m)
{
    if (m != NULL) {
        memset(m, 0, sizeof(*m));
    }
}

gk_status gk_modal_add_mode(gk_modal *m, double freq, double damping)
{
    if (m == NULL || freq <= 0.0 || m->count >= GK_PHYS_MAX_MODES) {
        return GK_ERR_OUT_OF_RANGE;
    }
    m->frequencies[m->count] = freq;
    m->damping[m->count] = damping;
    m->count++;
    return GK_OK;
}

double gk_modal_frf(const gk_modal *m, double freq)
{
    double sum = 0.0;
    int i;
    if (m == NULL || freq <= 0.0) {
        return 0.0;
    }
    for (i = 0; i < m->count; ++i) {
        double fn = m->frequencies[i];
        double zeta = m->damping[i];
        double r = freq / fn;
        double denom = pow(1.0 - r * r, 2.0) + pow(2.0 * zeta * r, 2.0);
        if (denom > 0.0) {
            sum += 1.0 / denom;
        }
    }
    return sum;
}

/* ---- servo ---- */

void gk_axis_servo_init(gk_axis_servo *s, double bandwidth, double resonance)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->bandwidth = bandwidth;
    s->resonance = resonance;
    s->inertia = 1.0;
    s->gain = 1.0;
}

double gk_axis_servo_phase_lag(const gk_axis_servo *s, double freq)
{
    if (s == NULL || s->bandwidth <= 0.0) {
        return 0.0;
    }
    return atan2(freq, s->bandwidth) * 180.0 / GK_PI;
}

int gk_axis_servo_is_stable(const gk_axis_servo *s)
{
    if (s == NULL) {
        return 0;
    }
    /* unstable when the resonance sits below the bandwidth */
    return !(s->resonance > 0.0 && s->resonance < s->bandwidth);
}

/* ---- drives ---- */

const char *gk_drive_name(gk_drive_kind k)
{
    switch (k) {
    case GK_DRIVE_AIR_BEARING: return "air-bearing";
    case GK_DRIVE_HYDROSTATIC: return "hydrostatic";
    case GK_DRIVE_MAGNETIC: return "magnetic";
    case GK_DRIVE_LINEAR_MOTOR: return "linear-motor";
    case GK_DRIVE_VOICE_COIL: return "voice-coil";
    case GK_DRIVE_PIEZO: return "piezo";
    default: return "unknown";
    }
}

gk_status gk_drive_init(gk_drive *d, gk_drive_kind kind)
{
    if (d == NULL || kind < 0 || kind >= GK_DRIVE_COUNT) {
        return GK_ERR_INVALID_ARG;
    }
    memset(d, 0, sizeof(*d));
    d->kind = kind;
    switch (kind) {
    case GK_DRIVE_AIR_BEARING:
        d->load_capacity = 500.0;
        d->stiffness = 200.0;
        d->max_speed = 100000.0;
        break;
    case GK_DRIVE_HYDROSTATIC:
        d->load_capacity = 2000.0;
        d->stiffness = 800.0;
        d->max_speed = 30000.0;
        break;
    case GK_DRIVE_MAGNETIC:
        d->load_capacity = 1000.0;
        d->stiffness = 300.0;
        d->max_speed = 200000.0;
        break;
    case GK_DRIVE_LINEAR_MOTOR:
        d->load_capacity = 5000.0;
        d->stiffness = 1500.0;
        d->force = 3000.0;
        d->max_speed = 5.0;
        break;
    case GK_DRIVE_VOICE_COIL:
        d->load_capacity = 200.0;
        d->stiffness = 100.0;
        d->force = 500.0;
        d->max_speed = 2.0;
        break;
    case GK_DRIVE_PIEZO:
        d->load_capacity = 50.0;
        d->stiffness = 2000.0;
        d->force = 1000.0;
        d->max_speed = 0.1;
        break;
    default:
        break;
    }
    d->enabled = 1;
    return GK_OK;
}

double gk_drive_stiffness(const gk_drive *d)
{
    return d != NULL ? d->stiffness : 0.0;
}

/* ---- assisted machining ---- */

const char *gk_assist_name(gk_assist_kind k)
{
    switch (k) {
    case GK_ASSIST_ULTRASONIC: return "ultrasonic";
    case GK_ASSIST_LASER: return "laser";
    case GK_ASSIST_CRYOGENIC: return "cryogenic";
    case GK_ASSIST_MQL: return "mql";
    case GK_ASSIST_HIGH_PRESSURE: return "high-pressure";
    default: return "unknown";
    }
}

gk_status gk_assist_init(gk_assist *a, gk_assist_kind kind)
{
    if (a == NULL || kind < 0 || kind >= GK_ASSIST_COUNT) {
        return GK_ERR_INVALID_ARG;
    }
    memset(a, 0, sizeof(*a));
    a->kind = kind;
    a->enabled = 1;
    switch (kind) {
    case GK_ASSIST_ULTRASONIC:
        a->frequency = 20000.0;
        break;
    case GK_ASSIST_LASER:
        a->power = 1000.0;
        break;
    case GK_ASSIST_CRYOGENIC:
        a->temperature = -196.0;
        break;
    case GK_ASSIST_MQL:
        a->flow = 50.0;
        break;
    case GK_ASSIST_HIGH_PRESSURE:
        a->pressure = 70.0;
        break;
    default:
        break;
    }
    return GK_OK;
}

double gk_assist_benefit(const gk_assist *a, double baseline)
{
    if (a == NULL || !a->enabled) {
        return baseline;
    }
    switch (a->kind) {
    case GK_ASSIST_ULTRASONIC:
        return baseline * 1.3;
    case GK_ASSIST_LASER:
        return baseline * 1.5;
    case GK_ASSIST_CRYOGENIC:
        return baseline * 1.2;
    case GK_ASSIST_MQL:
        return baseline * 1.1;
    case GK_ASSIST_HIGH_PRESSURE:
        return baseline * 1.25;
    default:
        return baseline;
    }
}

/* ---- materials ---- */

static const gk_material g_materials[GK_MAT_COUNT] = {
    { GK_MAT_CARBON_STEEL, 7850, 150, 210, 120, 0.08, 50 },
    { GK_MAT_ALLOY_STEEL, 7850, 250, 210, 100, 0.07, 45 },
    { GK_MAT_STAINLESS, 8000, 180, 200, 90, 0.06, 16 },
    { GK_MAT_ALUMINUM, 2700, 60, 70, 300, 0.15, 200 },
    { GK_MAT_COPPER, 8900, 80, 120, 250, 0.12, 390 },
    { GK_MAT_TITANIUM, 4500, 200, 110, 45, 0.05, 7 },
    { GK_MAT_SUPERALLOY, 8200, 300, 200, 30, 0.04, 11 },
    { GK_MAT_CAST_IRON, 7200, 200, 120, 100, 0.10, 45 },
    { GK_MAT_PLASTIC, 1200, 20, 3, 500, 0.20, 0.2 },
    { GK_MAT_COMPOSITE, 1600, 100, 70, 200, 0.10, 1 },
    { GK_MAT_CERAMIC, 3200, 1500, 350, 60, 0.03, 30 },
};

const char *gk_material_name(gk_material_kind k)
{
    switch (k) {
    case GK_MAT_CARBON_STEEL: return "carbon-steel";
    case GK_MAT_ALLOY_STEEL: return "alloy-steel";
    case GK_MAT_STAINLESS: return "stainless";
    case GK_MAT_ALUMINUM: return "aluminum";
    case GK_MAT_COPPER: return "copper";
    case GK_MAT_TITANIUM: return "titanium";
    case GK_MAT_SUPERALLOY: return "superalloy";
    case GK_MAT_CAST_IRON: return "cast-iron";
    case GK_MAT_PLASTIC: return "plastic";
    case GK_MAT_COMPOSITE: return "composite";
    case GK_MAT_CERAMIC: return "ceramic";
    default: return "unknown";
    }
}

const gk_material *gk_material_get(gk_material_kind k)
{
    if (k < 0 || k >= GK_MAT_COUNT) {
        return NULL;
    }
    return &g_materials[k];
}

/* ---- Johnson-Cook ---- */

void gk_jc_init(gk_johnson_cook *jc, double A, double B, double n, double C,
                double m, double melting)
{
    if (jc == NULL) {
        return;
    }
    memset(jc, 0, sizeof(*jc));
    jc->A = A;
    jc->B = B;
    jc->n = n;
    jc->C = C;
    jc->m = m;
    jc->melting = melting;
}

double gk_jc_flow_stress(const gk_johnson_cook *jc, double strain,
                         double strain_rate, double temperature)
{
    double hard, rate, thermal, tstar;
    if (jc == NULL || jc->melting <= 0.0) {
        return 0.0;
    }
    hard = jc->A + jc->B * pow(strain > 0.0 ? strain : 0.0, jc->n);
    rate = 1.0 + jc->C * log(strain_rate > 0.0 ? strain_rate : 1e-6);
    tstar = (temperature) / jc->melting;
    if (tstar < 0.0) tstar = 0.0;
    if (tstar > 1.0) tstar = 1.0;
    thermal = 1.0 - pow(tstar, jc->m);
    if (thermal < 0.0) thermal = 0.0;
    return hard * rate * thermal;
}

/* ---- hardness / heat treatment ---- */

const char *gk_heat_name(gk_heat_treatment h)
{
    switch (h) {
    case GK_HEAT_ANNEALED: return "annealed";
    case GK_HEAT_NORMALIZED: return "normalized";
    case GK_HEAT_QUENCHED: return "quenched";
    case GK_HEAT_TEMPERED: return "tempered";
    default: return "unknown";
    }
}

double gk_hardness_at_depth(double surface_hb, double core_hb, double depth,
                            double case_depth)
{
    if (case_depth <= 0.0) {
        return core_hb;
    }
    if (depth >= case_depth) {
        return core_hb;
    }
    return surface_hb - (surface_hb - core_hb) * (depth / case_depth);
}

int gk_heat_affects_hardness(gk_heat_treatment h)
{
    return h == GK_HEAT_QUENCHED || h == GK_HEAT_TEMPERED;
}

/* ---- recommendations ---- */

gk_status gk_recommend_speed(const gk_cut_input *in, gk_cut_params *out)
{
    const gk_material *m;
    if (in == NULL || out == NULL || in->tool_diameter <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    m = gk_material_get(in->material);
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    out->speed = m->vc * 1000.0 / (GK_PI * in->tool_diameter);
    return GK_OK;
}

gk_status gk_recommend_feed(const gk_cut_input *in, gk_cut_params *out)
{
    const gk_material *m;
    double rpm;
    gk_cut_params tmp;
    if (in == NULL || out == NULL || in->flutes <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    m = gk_material_get(in->material);
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    memset(&tmp, 0, sizeof(tmp));
    gk_recommend_speed(in, &tmp);
    rpm = tmp.speed;
    out->feed = rpm * m->feed_per_tooth * in->flutes;
    return GK_OK;
}

gk_status gk_recommend_depth(const gk_cut_input *in, gk_cut_params *out)
{
    if (in == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    out->depth = in->tool_diameter * 0.5;
    out->width = in->tool_diameter * 0.7;
    return GK_OK;
}

int gk_recommend_tool(const gk_cut_input *in, char *buf, size_t len)
{
    const gk_material *m;
    if (in == NULL || buf == NULL) {
        return 0;
    }
    m = gk_material_get(in->material);
    if (m == NULL) {
        return 0;
    }
    if (in->hardness > 300.0 || in->material == GK_MAT_SUPERALLOY ||
        in->material == GK_MAT_TITANIUM) {
        return snprintf(buf, len, "carbide grade C with TiAlN coating");
    }
    if (in->material == GK_MAT_ALUMINUM) {
        return snprintf(buf, len, "uncoated carbide, polished flutes");
    }
    return snprintf(buf, len, "carbide grade P with TiN coating");
}

int gk_recommend_cooling(const gk_cut_input *in, char *buf, size_t len)
{
    if (in == NULL || buf == NULL) {
        return 0;
    }
    if (in->material == GK_MAT_TITANIUM ||
        in->material == GK_MAT_SUPERALLOY) {
        return snprintf(buf, len, "high-pressure coolant (70 bar)");
    }
    if (in->material == GK_MAT_ALUMINUM) {
        return snprintf(buf, len, "MQL or flood mist");
    }
    if (in->material == GK_MAT_CAST_IRON) {
        return snprintf(buf, len, "dry machining with air blast");
    }
    return snprintf(buf, len, "flood coolant");
}
