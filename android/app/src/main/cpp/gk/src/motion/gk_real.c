#include "gk/gk_real.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ================= PRNG ================= */

void gk_rng_seed(gk_rng *r, unsigned long long seed)
{
    if (r == NULL) {
        return;
    }
    if (seed == 0ULL) {
        seed = 0x9E3779B97F4A7C15ULL;
    }
    r->state = seed;
}

unsigned long long gk_rng_next(gk_rng *r)
{
    unsigned long long x;
    if (r == NULL) {
        return 0ULL;
    }
    x = r->state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    r->state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

double gk_rng_uniform(gk_rng *r, double lo, double hi)
{
    double u;
    if (r == NULL) {
        return lo;
    }
    u = (double)(gk_rng_next(r) >> 11) / 9007199254740992.0; /* [0,1) */
    return lo + u * (hi - lo);
}

double gk_rng_normal(gk_rng *r, double mean, double stddev)
{
    double u1, u2, z;
    if (r == NULL) {
        return mean;
    }
    u1 = (double)(gk_rng_next(r) >> 11) / 9007199254740992.0;
    u2 = (double)(gk_rng_next(r) >> 11) / 9007199254740992.0;
    if (u1 < 1e-12) {
        u1 = 1e-12;
    }
    z = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
    return mean + stddev * z;
}

int gk_rng_chance(gk_rng *r, double probability)
{
    double u;
    if (r == NULL) {
        return 0;
    }
    if (probability <= 0.0) {
        return 0;
    }
    if (probability >= 1.0) {
        return 1;
    }
    u = (double)(gk_rng_next(r) >> 11) / 9007199254740992.0;
    return u < probability;
}

/* ================= variation ================= */

void gk_variation_init(gk_variation *v, double nominal, double sigma,
                       double min_limit, double max_limit)
{
    if (v == NULL) {
        return;
    }
    v->nominal = nominal;
    v->sigma = sigma;
    v->min_limit = min_limit;
    v->max_limit = max_limit;
}

double gk_variation_sample(const gk_variation *v, gk_rng *r)
{
    double s;
    if (v == NULL) {
        return 0.0;
    }
    s = gk_rng_normal(r, v->nominal, v->sigma);
    if (s < v->min_limit) {
        s = v->min_limit;
    }
    if (s > v->max_limit) {
        s = v->max_limit;
    }
    return s;
}

void gk_blank_variation_init(gk_blank_variation *b)
{
    if (b == NULL) {
        return;
    }
    gk_variation_init(&b->blank_length, 100.0, 0.3, 99.0, 101.0);
    gk_variation_init(&b->blank_width, 80.0, 0.3, 79.0, 81.0);
    gk_variation_init(&b->blank_height, 50.0, 0.2, 49.0, 51.0);
    gk_variation_init(&b->hardness, 200.0, 8.0, 170.0, 240.0);
}

void gk_tool_batch_init(gk_tool_batch *t, int batch_id, gk_rng *r)
{
    if (t == NULL) {
        return;
    }
    t->batch_id = batch_id;
    t->diameter_offset = gk_rng_normal(r, 0.0, 0.005);
    t->runout = fabs(gk_rng_normal(r, 0.0, 0.010));
    t->wear_factor = 1.0 + gk_rng_normal(r, 0.0, 0.05);
    if (t->wear_factor < 0.5) {
        t->wear_factor = 0.5;
    }
    if (t->wear_factor > 2.0) {
        t->wear_factor = 2.0;
    }
}

/* ================= environment ================= */

void gk_fluctuation_init(gk_fluctuation *f, double nominal, double amplitude,
                         double period, double phase)
{
    if (f == NULL) {
        return;
    }
    f->nominal = nominal;
    f->amplitude = amplitude;
    f->period = (period <= 0.0) ? 1.0 : period;
    f->phase = phase;
}

double gk_fluctuation_at(const gk_fluctuation *f, double time)
{
    if (f == NULL) {
        return 0.0;
    }
    return f->nominal + f->amplitude * sin(2.0 * M_PI * time / f->period +
                                           f->phase);
}

void gk_environment_init(gk_environment *e)
{
    if (e == NULL) {
        return;
    }
    gk_fluctuation_init(&e->grid_voltage, 380.0, 6.0, 24.0, 0.0);
    gk_fluctuation_init(&e->air_pressure, 0.6, 0.02, 6.0, 1.0);
    gk_fluctuation_init(&e->hydraulic, 6.0, 0.15, 8.0, 2.0);
    gk_fluctuation_init(&e->temperature, 22.0, 3.0, 24.0, 0.0);
    gk_fluctuation_init(&e->humidity, 50.0, 8.0, 24.0, 3.0);
}

int gk_environment_kind_count(void)
{
    return 5;
}

double gk_environment_sample(const gk_environment *e, double time,
                             int kind_index)
{
    if (e == NULL) {
        return 0.0;
    }
    switch (kind_index) {
    case 0: return gk_fluctuation_at(&e->grid_voltage, time);
    case 1: return gk_fluctuation_at(&e->air_pressure, time);
    case 2: return gk_fluctuation_at(&e->hydraulic, time);
    case 3: return gk_fluctuation_at(&e->temperature, time);
    case 4: return gk_fluctuation_at(&e->humidity, time);
    default: return 0.0;
    }
}

/* ================= operator / clamp ================= */

void gk_operator_model_init(gk_operator_model *o, double skill)
{
    if (o == NULL) {
        return;
    }
    if (skill < 0.0) {
        skill = 0.0;
    }
    if (skill > 1.0) {
        skill = 1.0;
    }
    o->skill = skill;
    o->fatigue = 0.0;
    o->reaction_sigma = 0.15 + 0.5 * (1.0 - skill);
}

double gk_operator_setting_error(const gk_operator_model *o, gk_rng *r)
{
    double base;
    if (o == NULL) {
        return 0.0;
    }
    base = (1.0 - o->skill) * 0.2;
    base += o->fatigue * 0.1;
    return fabs(gk_rng_normal(r, 0.0, base > 0.0 ? base : 0.001));
}

double gk_operator_reaction_time(const gk_operator_model *o, gk_rng *r)
{
    double mean;
    if (o == NULL) {
        return 0.0;
    }
    mean = 0.4 + 0.6 * (1.0 - o->skill) + 0.5 * o->fatigue;
    return fabs(gk_rng_normal(r, mean, o->reaction_sigma));
}

void gk_clamp_force_init(gk_clamp_force *c, double nominal, double sigma,
                         double max_force)
{
    if (c == NULL) {
        return;
    }
    c->nominal_force = nominal;
    c->sigma = sigma;
    c->max_force = max_force;
}

double gk_clamp_force_sample(const gk_clamp_force *c, gk_rng *r)
{
    double f;
    if (c == NULL) {
        return 0.0;
    }
    f = gk_rng_normal(r, c->nominal_force, c->sigma);
    if (f < 0.0) {
        f = 0.0;
    }
    return f;
}

int gk_clamp_force_ok(const gk_clamp_force *c, double force)
{
    if (c == NULL) {
        return 0;
    }
    return force > 0.0 && force <= c->max_force;
}

/* ================= faults ================= */

const char *gk_real_fault_name(gk_real_fault f)
{
    switch (f) {
    case GK_REAL_FAULT_NONE: return "none";
    case GK_REAL_FAULT_TOOL_BREAK: return "tool breakage";
    case GK_REAL_FAULT_POWER_LOSS: return "power loss";
    case GK_REAL_FAULT_SPINDLE_STALL: return "spindle stall";
    case GK_REAL_FAULT_COOLANT_LOSS: return "coolant loss";
    case GK_REAL_FAULT_CHIP_JAM: return "chip jam";
    default: return "unknown";
    }
}

void gk_fault_model_init(gk_fault_model *m, double base_rate)
{
    if (m == NULL) {
        return;
    }
    m->count = 0;
    (void)gk_fault_model_set(m, GK_REAL_FAULT_TOOL_BREAK, base_rate);
    (void)gk_fault_model_set(m, GK_REAL_FAULT_SPINDLE_STALL, base_rate * 0.5);
    (void)gk_fault_model_set(m, GK_REAL_FAULT_COOLANT_LOSS, base_rate * 0.3);
    (void)gk_fault_model_set(m, GK_REAL_FAULT_CHIP_JAM, base_rate * 0.8);
}

gk_status gk_fault_model_set(gk_fault_model *m, gk_real_fault f, double rate)
{
    int i;
    if (m == NULL || f == GK_REAL_FAULT_NONE || f >= GK_REAL_FAULT_COUNT) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < m->count; i++) {
        if (m->rules[i].fault == f) {
            m->rules[i].probability_per_hour = rate;
            return GK_OK;
        }
    }
    if (m->count >= GK_REAL_FAULT_COUNT) {
        return GK_ERR_OUT_OF_RANGE;
    }
    m->rules[m->count].fault = f;
    m->rules[m->count].probability_per_hour = rate;
    m->count++;
    return GK_OK;
}

gk_real_fault gk_fault_model_sample(const gk_fault_model *m, gk_rng *r,
                                    double hours)
{
    int i;
    if (m == NULL || r == NULL || hours <= 0.0) {
        return GK_REAL_FAULT_NONE;
    }
    for (i = 0; i < m->count; i++) {
        double p = 1.0 - exp(-m->rules[i].probability_per_hour * hours);
        if (gk_rng_chance(r, p)) {
            return m->rules[i].fault;
        }
    }
    return GK_REAL_FAULT_NONE;
}

/* ================= random events ================= */

void gk_event_model_init(gk_event_model *m)
{
    if (m == NULL) {
        return;
    }
    m->count = 0;
    memset(m->events, 0, sizeof(m->events));
}

int gk_event_model_add(gk_event_model *m, const char *name, double rate,
                       double impact)
{
    gk_random_event *e;
    size_t n;
    if (m == NULL || name == NULL || m->count >= GK_REAL_MAX_EVENTS) {
        return -1;
    }
    e = &m->events[m->count];
    e->id = m->count + 1;
    n = strlen(name);
    if (n >= GK_REAL_NAME) {
        n = GK_REAL_NAME - 1;
    }
    memcpy(e->name, name, n);
    e->name[n] = '\0';
    e->probability_per_hour = rate;
    e->impact = impact;
    m->count++;
    return e->id;
}

int gk_event_model_sample(const gk_event_model *m, gk_rng *r, double hours,
                          int *out_ids, int max_out)
{
    int i;
    int fired = 0;
    if (m == NULL || r == NULL) {
        return 0;
    }
    for (i = 0; i < m->count; i++) {
        double p = 1.0 - exp(-m->events[i].probability_per_hour * hours);
        if (gk_rng_chance(r, p)) {
            if (out_ids != NULL && fired < max_out) {
                out_ids[fired] = m->events[i].id;
            }
            fired++;
        }
    }
    return fired;
}

/* ================= measurement noise ================= */

void gk_measure_noise_init(gk_measure_noise *m, double sigma, double bias,
                           double resolution)
{
    if (m == NULL) {
        return;
    }
    m->sigma = sigma;
    m->bias = bias;
    m->resolution = resolution;
}

double gk_measure_noise_apply(const gk_measure_noise *m, double value,
                              gk_rng *r)
{
    double v;
    if (m == NULL) {
        return value;
    }
    v = value + m->bias + gk_rng_normal(r, 0.0, m->sigma);
    if (m->resolution > 0.0) {
        v = floor(v / m->resolution + 0.5) * m->resolution;
    }
    return v;
}

/* ================= statistics / Monte-Carlo ================= */

void gk_stats_init(gk_stats *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->minimum = 1e300;
    s->maximum = -1e300;
}

gk_status gk_stats_add(gk_stats *s, double value)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    s->samples++;
    if (s->samples == 1) {
        s->minimum = value;
        s->maximum = value;
        s->mean = value;
        return GK_OK;
    }
    if (value < s->minimum) {
        s->minimum = value;
    }
    if (value > s->maximum) {
        s->maximum = value;
    }
    s->mean += value;
    return GK_OK;
}

static int cmp_double(const void *a, const void *b)
{
    double da = *(const double *)a;
    double db = *(const double *)b;
    if (da < db) {
        return -1;
    }
    if (da > db) {
        return 1;
    }
    return 0;
}

gk_status gk_stats_finalize(gk_stats *s)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (s->samples <= 0) {
        return GK_ERR_STATE;
    }
    s->mean /= (double)s->samples;
    return GK_OK;
}

int gk_monte_carlo(gk_real_model_fn fn, void *ctx, gk_rng *r, int samples,
                   double *out_values, int max_out, gk_stats *out_stats)
{
    int i;
    int stored = 0;
    gk_stats local;
    gk_stats *st = out_stats ? out_stats : &local;

    if (fn == NULL || r == NULL || samples <= 0) {
        return 0;
    }
    if (samples > GK_REAL_MAX_SAMPLES) {
        samples = GK_REAL_MAX_SAMPLES;
    }
    gk_stats_init(st);
    for (i = 0; i < samples; i++) {
        double v = fn(ctx, r);
        (void)gk_stats_add(st, v);
        if (out_values != NULL && stored < max_out) {
            out_values[stored++] = v;
        }
    }
    (void)gk_stats_finalize(st);

    if (out_values != NULL && stored > 0) {
        /* percentile estimates over the stored sample */
        int n = stored;
        qsort(out_values, (size_t)n, sizeof(double), cmp_double);
        st->p05 = out_values[(int)(0.05 * (n - 1))];
        st->p50 = out_values[(int)(0.50 * (n - 1))];
        st->p95 = out_values[(int)(0.95 * (n - 1))];
    } else {
        st->p05 = st->minimum;
        st->p50 = st->mean;
        st->p95 = st->maximum;
    }
    return stored > 0 ? stored : st->samples;
}

/* ================= disruptions ================= */

const char *gk_disruption_name(gk_disruption_kind k)
{
    switch (k) {
    case GK_DR_POWER_RESUME: return "power-loss resume";
    case GK_DR_TOOL_CHANGE_BROKEN: return "broken tool change";
    case GK_DR_CRASH_RECOVERY: return "crash recovery";
    case GK_DR_BATCH_SCRAP: return "batch scrap";
    case GK_DR_CUSTOMER_COMPLAINT: return "customer complaint";
    case GK_DR_DEADLINE_SQUEEZE: return "deadline squeeze";
    case GK_DR_MACHINE_BREAKDOWN: return "machine breakdown";
    case GK_DR_MATERIAL_REJECT: return "material reject";
    case GK_DR_STAFF_SHORTAGE: return "staff shortage";
    case GK_DR_OVERNIGHT_RUSH: return "overnight rush";
    case GK_DR_FATIGUE_OPERATION: return "fatigue operation";
    case GK_DR_SAFETY_ACCIDENT: return "safety accident";
    case GK_DR_FIRE: return "fire";
    case GK_DR_ELECTRICAL_LEAK: return "electrical leak";
    default: return "unknown";
    }
}

void gk_disruption_log_init(gk_disruption_log *l)
{
    if (l == NULL) {
        return;
    }
    l->count = 0;
    memset(l->events, 0, sizeof(l->events));
}

static double disruption_downtime(gk_disruption_kind k)
{
    switch (k) {
    case GK_DR_POWER_RESUME: return 0.5;
    case GK_DR_TOOL_CHANGE_BROKEN: return 0.3;
    case GK_DR_CRASH_RECOVERY: return 4.0;
    case GK_DR_BATCH_SCRAP: return 1.0;
    case GK_DR_CUSTOMER_COMPLAINT: return 0.0;
    case GK_DR_DEADLINE_SQUEEZE: return 0.0;
    case GK_DR_MACHINE_BREAKDOWN: return 8.0;
    case GK_DR_MATERIAL_REJECT: return 2.0;
    case GK_DR_STAFF_SHORTAGE: return 1.5;
    case GK_DR_OVERNIGHT_RUSH: return 0.0;
    case GK_DR_FATIGUE_OPERATION: return 0.0;
    case GK_DR_SAFETY_ACCIDENT: return 16.0;
    case GK_DR_FIRE: return 24.0;
    case GK_DR_ELECTRICAL_LEAK: return 6.0;
    default: return 0.0;
    }
}

static double disruption_cost(gk_disruption_kind k)
{
    switch (k) {
    case GK_DR_POWER_RESUME: return 200.0;
    case GK_DR_TOOL_CHANGE_BROKEN: return 150.0;
    case GK_DR_CRASH_RECOVERY: return 5000.0;
    case GK_DR_BATCH_SCRAP: return 3000.0;
    case GK_DR_CUSTOMER_COMPLAINT: return 1000.0;
    case GK_DR_DEADLINE_SQUEEZE: return 2000.0;
    case GK_DR_MACHINE_BREAKDOWN: return 8000.0;
    case GK_DR_MATERIAL_REJECT: return 1200.0;
    case GK_DR_STAFF_SHORTAGE: return 800.0;
    case GK_DR_OVERNIGHT_RUSH: return 1500.0;
    case GK_DR_FATIGUE_OPERATION: return 600.0;
    case GK_DR_SAFETY_ACCIDENT: return 20000.0;
    case GK_DR_FIRE: return 50000.0;
    case GK_DR_ELECTRICAL_LEAK: return 9000.0;
    default: return 0.0;
    }
}

gk_status gk_disruption_add(gk_disruption_log *l, gk_disruption_kind kind,
                            double severity)
{
    gk_disruption *d;
    if (l == NULL || kind >= GK_DR_COUNT || l->count >= GK_REAL_MAX_EVENTS) {
        return GK_ERR_INVALID_ARG;
    }
    if (severity < 0.0) {
        severity = 0.0;
    }
    if (severity > 1.0) {
        severity = 1.0;
    }
    d = &l->events[l->count];
    d->kind = kind;
    d->severity = severity;
    d->downtime_hours = disruption_downtime(kind) * severity;
    d->cost_impact = disruption_cost(kind) * severity;
    d->handled = 0;
    l->count++;
    return GK_OK;
}

gk_status gk_disruption_handle(gk_disruption_log *l, int index)
{
    if (l == NULL || index < 0 || index >= l->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    if (l->events[index].handled) {
        return GK_ERR_STATE;
    }
    l->events[index].handled = 1;
    return GK_OK;
}

double gk_disruption_total_cost(const gk_disruption_log *l)
{
    int i;
    double total = 0.0;
    if (l == NULL) {
        return 0.0;
    }
    for (i = 0; i < l->count; i++) {
        total += l->events[i].cost_impact;
    }
    return total;
}

double gk_disruption_total_downtime(const gk_disruption_log *l)
{
    int i;
    double total = 0.0;
    if (l == NULL) {
        return 0.0;
    }
    for (i = 0; i < l->count; i++) {
        total += l->events[i].downtime_hours;
    }
    return total;
}

static int approx_equal(double a, double b)
{
    return fabs(a - b) < 1e-9;
}

int gk_power_resume(gk_disruption_log *l, const double *last_pos,
                    const double *safe_pos, int axes)
{
    int i;
    int moved = 0;
    if (last_pos == NULL || safe_pos == NULL || axes <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    if (l != NULL) {
        (void)gk_disruption_add(l, GK_DR_POWER_RESUME, 0.3);
    }
    /* count axes that need re-referencing (differ from safe position) */
    for (i = 0; i < axes; i++) {
        if (!approx_equal(last_pos[i], safe_pos[i])) {
            moved++;
        }
    }
    return moved;
}

/* ================= lifecycle ================= */

void gk_realism_init(gk_realism *r, unsigned long long seed)
{
    if (r == NULL) {
        return;
    }
    memset(r, 0, sizeof(*r));
    gk_rng_seed(&r->rng, seed);
    gk_environment_init(&r->environment);
    gk_operator_model_init(&r->operator_model, 0.8);
    gk_fault_model_init(&r->faults, 0.02);
    gk_measure_noise_init(&r->noise, 0.002, 0.0, 0.001);
    gk_disruption_log_init(&r->disruptions);
}

int gk_realism_step(gk_realism *r, double time, double dt)
{
    gk_real_fault f;
    int fired = 0;
    if (r == NULL || dt <= 0.0) {
        return 0;
    }
    /* temperature/humidity drift raises fault probability slightly */
    f = gk_fault_model_sample(&r->faults, &r->rng, dt);
    if (f != GK_REAL_FAULT_NONE) {
        (void)gk_disruption_add(&r->disruptions, GK_DR_MACHINE_BREAKDOWN, 0.4);
        fired++;
    }
    /* occasional defect/scrap events */
    if (gk_rng_chance(&r->rng, 1.0 - exp(-0.05 * dt))) {
        (void)gk_disruption_add(&r->disruptions, GK_DR_BATCH_SCRAP, 0.2);
        fired++;
    }
    (void)time;
    return fired;
}
