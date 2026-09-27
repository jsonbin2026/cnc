#include "gk/gk_mscale.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define GK_MS_DEG2RAD (3.14159265358979323846 / 180.0)

static void gk_ms_copy(char *dst, const char *src, size_t cap)
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

static double clampd(double v, double lo, double hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

/* ================= scale descriptors ================= */

const char *gk_ms_scale_name(gk_ms_scale s)
{
    switch (s) {
    case GK_MS_SCALE_SERVO: return "microsecond-servo";
    case GK_MS_SCALE_NANO: return "nanosecond-cutting";
    case GK_MS_SCALE_ATOM: return "atomic";
    case GK_MS_SCALE_GRAIN: return "grain";
    case GK_MS_SCALE_CHIP: return "micro-chip";
    case GK_MS_SCALE_TOOL_TIP: return "tool-tip";
    case GK_MS_SCALE_WORKPIECE: return "workpiece";
    case GK_MS_SCALE_MACHINE: return "machine";
    case GK_MS_SCALE_SHOP: return "workshop";
    case GK_MS_SCALE_FACTORY: return "factory";
    case GK_MS_SCALE_SUPPLY: return "supply-chain";
    default: return "unknown";
    }
}

double gk_ms_scale_length(gk_ms_scale s)
{
    switch (s) {
    case GK_MS_SCALE_SERVO: return 1e-4;    /* 0.1 mm motion per tick */
    case GK_MS_SCALE_NANO: return 1e-9;
    case GK_MS_SCALE_ATOM: return 1e-10;    /* ~0.1 nm lattice */
    case GK_MS_SCALE_GRAIN: return 1e-6;    /* 1 um grain */
    case GK_MS_SCALE_CHIP: return 1e-4;     /* 0.1 mm chip */
    case GK_MS_SCALE_TOOL_TIP: return 1e-5;
    case GK_MS_SCALE_WORKPIECE: return 1e-1; /* 100 mm */
    case GK_MS_SCALE_MACHINE: return 1.0;    /* 1 m */
    case GK_MS_SCALE_SHOP: return 1e1;
    case GK_MS_SCALE_FACTORY: return 1e2;
    case GK_MS_SCALE_SUPPLY: return 1e6;     /* 1000 km */
    default: return 0.0;
    }
}

double gk_ms_scale_time(gk_ms_scale s)
{
    switch (s) {
    case GK_MS_SCALE_SERVO: return 1e-6;   /* microsecond control loop */
    case GK_MS_SCALE_NANO: return 1e-9;
    case GK_MS_SCALE_ATOM: return 1e-12;
    case GK_MS_SCALE_GRAIN: return 1e-3;
    case GK_MS_SCALE_CHIP: return 1e-3;
    case GK_MS_SCALE_TOOL_TIP: return 1e0;
    case GK_MS_SCALE_WORKPIECE: return 1e0;
    case GK_MS_SCALE_MACHINE: return 1e1;
    case GK_MS_SCALE_SHOP: return 1e2;
    case GK_MS_SCALE_FACTORY: return 1e3;
    case GK_MS_SCALE_SUPPLY: return 1e5;
    default: return 0.0;
    }
}

/* ================= 789 microsecond servo ================= */

void gk_ms_servo_init(gk_ms_servo *s, double kp, double kv, double dt)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->kp = kp;
    s->kv = kv;
    s->dt = (dt > 0.0) ? dt : 1e-6;
    s->max_velocity = 1e4;
    s->max_accel = 1e6;
}

gk_status gk_ms_servo_step(gk_ms_servo *s)
{
    double error;
    double accel;
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    error = s->setpoint - s->position;
    s->integral += error * s->dt;
    /* PD control of a double-integrator plant: for the critically damped
       response choose kv = 2*sqrt(kp). The stored integral is retained for
       diagnostics but PD already drives the position error to zero. */
    accel = s->kp * error - s->kv * s->velocity;
    s->velocity += accel * s->dt;
    if (s->velocity > s->max_velocity) {
        s->velocity = s->max_velocity;
        s->saturated = 1;
    } else if (s->velocity < -s->max_velocity) {
        s->velocity = -s->max_velocity;
        s->saturated = 1;
    } else {
        s->saturated = 0;
    }
    s->position += s->velocity * s->dt;
    return GK_OK;
}

gk_status gk_ms_servo_run(gk_ms_servo *s, double duration)
{
    int steps;
    int i;
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (duration <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    steps = (int)(duration / s->dt);
    if (steps <= 0) {
        steps = 1;
    }
    for (i = 0; i < steps; i++) {
        (void)gk_ms_servo_step(s);
    }
    return GK_OK;
}

/* ================= 790 nanosecond cutting ================= */

void gk_ms_nano_cut_init(gk_ms_nano_cut *n, double frequency_hz,
                         double amplitude_nm)
{
    if (n == NULL) {
        return;
    }
    memset(n, 0, sizeof(*n));
    n->vibration_hz = frequency_hz;
    n->amplitude_nm = amplitude_nm;
    n->period_ns = (frequency_hz > 0.0) ? 1e9 / frequency_hz : 0.0;
}

double gk_ms_nano_cut_time(const gk_ms_nano_cut *n, double feed_mm_s)
{
    double amplitude_mm;
    if (n == NULL || feed_mm_s <= 0.0 || n->vibration_hz <= 0.0) {
        return 0.0;
    }
    /* one full vibration period in ns, with the tool in contact while the
       displacement is within +-amplitude of the peak, i.e. a fraction of
       the period proportional to feed*period / (2*amplitude) */
    amplitude_mm = n->amplitude_nm * 1e-6;
    {
        double travel = feed_mm_s * (n->period_ns * 1e-9);
        double ratio = travel / (2.0 * amplitude_mm);
        return n->period_ns * clampd(ratio, 0.0, 1.0);
    }
}

/* ================= 791 atomic ================= */

void gk_ms_atom_init(gk_ms_atom *a, double lattice_nm)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    a->lattice_constant_nm = (lattice_nm > 0.0) ? lattice_nm : 0.361;
    a->formation_energy_ev = 1.5;
    a->temperature_k = 300.0;
}

double gk_ms_atom_dislocation(const gk_ms_atom *a, double strain)
{
    if (a == NULL || strain < 0.0) {
        return 0.0;
    }
    /* density proportional to strain over the Burgers vector (lattice) */
    return strain / a->lattice_constant_nm * 1e10;
}

double gk_ms_atom_activation(const gk_ms_atom *a)
{
    double kb_ev_per_k = 8.617333262e-5;
    if (a == NULL) {
        return 0.0;
    }
    if (a->temperature_k <= 0.0) {
        return 0.0;
    }
    return exp(-a->formation_energy_ev / (kb_ev_per_k * a->temperature_k));
}

/* ================= 792 grain ================= */

void gk_ms_grain_init(gk_ms_grain *g, double mean_diameter_um)
{
    if (g == NULL) {
        return;
    }
    memset(g, 0, sizeof(*g));
    g->mean_diameter_um = (mean_diameter_um > 0.0) ? mean_diameter_um : 10.0;
    g->sigma = 1.0;
    g->phase_count = 1;
    g->grain_count = 1;
}

gk_status gk_ms_grain_grow(gk_ms_grain *g, double time_s, double temp_c)
{
    double factor;
    if (g == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (time_s < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    /* parabolic grain-growth law accelerated by temperature above 600 C */
    factor = 1.0 + 0.001 * sqrt(time_s) * (1.0 + (temp_c - 600.0) / 1000.0);
    if (factor < 1.0) {
        factor = 1.0;
    }
    g->mean_diameter_um *= factor;
    if (g->mean_diameter_um > 0.0) {
        g->grain_count = (int)(1e4 / g->mean_diameter_um) + 1;
    }
    return GK_OK;
}

double gk_ms_grain_hall_petch(const gk_ms_grain *g, double sigma0, double k)
{
    if (g == NULL || g->mean_diameter_um <= 0.0) {
        return 0.0;
    }
    return sigma0 + k / sqrt(g->mean_diameter_um);
}

/* ================= 793 microscopic chip ================= */

void gk_ms_chip_init(gk_ms_chip *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
}

double gk_ms_chip_shear_angle(double rake_deg, double friction_deg)
{
    /* Merchant: phi = 45 + rake/2 - friction/2 */
    return 45.0 + rake_deg * 0.5 - friction_deg * 0.5;
}

gk_status gk_ms_chip_estimate(gk_ms_chip *c, double depth_of_cut_um,
                              double feed_um, double rake_deg,
                              double friction_deg, double shear_strength)
{
    double phi;
    double shear_plane;
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (depth_of_cut_um <= 0.0 || feed_um <= 0.0 || shear_strength < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    c->thickness_um = feed_um;
    c->width_um = depth_of_cut_um;
    phi = gk_ms_chip_shear_angle(rake_deg, friction_deg);
    if (phi <= 0.0 || phi >= 90.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    c->shear_angle_deg = phi;
    /* shear-plane area = (t * w) / sin(phi) */
    shear_plane = (feed_um * depth_of_cut_um) * 1e-6 / sin(phi * GK_MS_DEG2RAD);
    c->force_n = shear_strength * shear_plane;
    c->curl_radius_um = feed_um * 2.0;
    return GK_OK;
}

/* ================= 794 tool tip ================= */

void gk_ms_tool_tip_init(gk_ms_tool_tip *t)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->edge_radius_um = 10.0;
    t->coating_thickness_um = 3.0;
    t->temperature_c = 25.0;
}

double gk_ms_tool_tip_wear_rate(const gk_ms_tool_tip *t, double cutting_speed)
{
    double temp_term;
    if (t == NULL || cutting_speed < 0.0) {
        return 0.0;
    }
    /* Arrhenius-like: wear grows with speed and temperature */
    temp_term = exp((t->temperature_c - 25.0) / 200.0);
    return cutting_speed * temp_term *
           (t->coating_thickness_um > 0.0 ? 1.0 / t->coating_thickness_um
                                          : 1.0) *
           1e-5;
}

gk_status gk_ms_tool_tip_update(gk_ms_tool_tip *t, double cutting_speed,
                                double dt)
{
    if (t == NULL || dt <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    if (cutting_speed < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    t->wear_vb_um += gk_ms_tool_tip_wear_rate(t, cutting_speed) * dt;
    /* friction heats the edge */
    t->temperature_c += 0.01 * cutting_speed * dt;
    return GK_OK;
}

/* ================= 795 workpiece ================= */

void gk_ms_workpiece_init(gk_ms_workpiece *w, double l, double wd, double h)
{
    if (w == NULL) {
        return;
    }
    memset(w, 0, sizeof(*w));
    w->length_mm = l;
    w->width_mm = wd;
    w->height_mm = h;
    w->youngs_modulus_gpa = 200.0;
    w->poisson = 0.3;
}

double gk_ms_workpiece_deflection(const gk_ms_workpiece *w, double force_n)
{
    double e_pa, i_m4, l_m, base_um;
    if (w == NULL || force_n < 0.0) {
        return 0.0;
    }
    /* cantilever thin-wall of height h (bending about the width) */
    l_m = w->height_mm * 1e-3;
    e_pa = w->youngs_modulus_gpa * 1e9;
    /* second moment of area for a rectangular section: b*h^3/12 */
    i_m4 = (w->width_mm * 1e-3) * pow(l_m, 3.0) / 12.0;
    if (e_pa <= 0.0 || i_m4 <= 0.0) {
        return 0.0;
    }
    base_um = (force_n * pow(l_m, 3.0) / (3.0 * e_pa * i_m4)) * 1e6;
    return base_um;
}

/* ================= 796-799 hierarchy ================= */

void gk_ms_machine_init(gk_ms_machine *m, const char *name, int spindles)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    gk_ms_copy(m->name, name, sizeof(m->name));
    m->spindles = (spindles > 0) ? spindles : 1;
    m->drives = m->spindles + 3;
    m->power_kw = m->drives * 1.5;
}

void gk_ms_shop_init(gk_ms_shop *s, const char *name)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    gk_ms_copy(s->name, name, sizeof(s->name));
}

int gk_ms_shop_add_machine(gk_ms_shop *s, const gk_ms_machine *m)
{
    if (s == NULL || m == NULL || s->machine_count >= GK_MS_MAX_ITEMS) {
        return -1;
    }
    s->machines[s->machine_count] = *m;
    s->machine_count++;
    return s->machine_count;
}

double gk_ms_shop_oee(gk_ms_shop *s, double availability, double performance,
                      double quality)
{
    if (s == NULL) {
        return 0.0;
    }
    availability = clampd(availability, 0.0, 1.0);
    performance = clampd(performance, 0.0, 1.0);
    quality = clampd(quality, 0.0, 1.0);
    s->oee = availability * performance * quality;
    return s->oee;
}

void gk_ms_factory_init(gk_ms_factory_ms *f, const char *name)
{
    if (f == NULL) {
        return;
    }
    memset(f, 0, sizeof(*f));
    gk_ms_copy(f->name, name, sizeof(f->name));
}

int gk_ms_factory_add_shop(gk_ms_factory_ms *f, const gk_ms_shop *s)
{
    if (f == NULL || s == NULL || f->shop_count >= GK_MS_MAX_ITEMS) {
        return -1;
    }
    f->shops[f->shop_count] = *s;
    f->shop_count++;
    return f->shop_count;
}

double gk_ms_factory_output(gk_ms_factory_ms *f)
{
    int i;
    double total = 0.0;
    if (f == NULL) {
        return 0.0;
    }
    for (i = 0; i < f->shop_count; i++) {
        int j;
        double util = 0.0;
        for (j = 0; j < f->shops[i].machine_count; j++) {
            util += f->shops[i].machines[j].utilization;
        }
        total += util * f->shops[i].oee * 10.0;
    }
    f->daily_output = total;
    return total;
}

/* ================= supply chain ================= */

void gk_ms_supply_init(gk_ms_supply *s, int nodes)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->nodes = (nodes > 0) ? nodes : 1;
    s->lead_time_days = 7.0;
    s->inventory_days = 14.0;
    s->service_level = 0.95;
    s->cost_per_unit = 10.0;
}

double gk_ms_supply_bullwhip(const gk_ms_supply *s, double demand_variance)
{
    double nodes;
    if (s == NULL || demand_variance < 0.0) {
        return 0.0;
    }
    /* amplification grows with the number of echelons (square of nodes) */
    nodes = (double)s->nodes;
    return demand_variance * (1.0 + nodes * nodes * 0.1);
}

double gk_ms_supply_landed_cost(const gk_ms_supply *s, double procurement,
                                double holding_rate, double stockout_cost)
{
    double holding;
    double stockout;
    if (s == NULL) {
        return 0.0;
    }
    holding = s->cost_per_unit * holding_rate * s->inventory_days / 365.0;
    stockout = stockout_cost * (1.0 - clampd(s->service_level, 0.0, 1.0));
    return procurement + holding + stockout;
}

/* ================= 800 hourly thermal deformation ================= */

void gk_ms_thermal_drift_init(gk_ms_thermal_drift *t, double coeff,
                              double length_mm)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->coefficient = (coeff != 0.0) ? coeff : 12e-6; /* steel ~12 ppm/K */
    t->length_mm = length_mm;
    t->reference_temp_c = 20.0;
    t->ambient_c = 20.0;
    t->heating_rate_c_per_h = 3.0;
    t->time_constant_h = 2.0;
    t->temperature_c = 20.0;
}

gk_status gk_ms_thermal_drift_update(gk_ms_thermal_drift *t, double dt_h)
{
    double target;
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (dt_h <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    /* first-order approach to a steady state elevated by the heating rate */
    target = t->ambient_c + t->heating_rate_c_per_h * t->time_constant_h;
    t->temperature_c += (target - t->temperature_c) *
                        (1.0 - exp(-dt_h / t->time_constant_h));
    t->elapsed_h += dt_h;
    /* thermal growth = alpha * L * dT */
    t->growth_um = t->coefficient * t->length_mm * 1e3 *
                   (t->temperature_c - t->reference_temp_c);
    return GK_OK;
}

double gk_ms_thermal_drift_steady(const gk_ms_thermal_drift *t)
{
    double delta_t;
    if (t == NULL) {
        return 0.0;
    }
    delta_t = t->heating_rate_c_per_h * t->time_constant_h;
    return t->coefficient * t->length_mm * 1e3 * delta_t;
}
