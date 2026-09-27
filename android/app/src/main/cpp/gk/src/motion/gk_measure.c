#include "gk/gk_measure.h"

#include <math.h>
#include <string.h>
#include <stdio.h>

const char *gk_gauge_name(gk_gauge_kind k)
{
    switch (k) {
    case GK_GAUGE_CALIPER:
        return "caliper";
    case GK_GAUGE_MICROMETER:
        return "micrometer";
    case GK_GAUGE_HEIGHT:
        return "height-gauge";
    case GK_GAUGE_BORE_MICROMETER:
        return "bore-micrometer";
    case GK_GAUGE_CMM:
        return "CMM";
    case GK_GAUGE_ROUGHNESS:
        return "roughness-tester";
    case GK_GAUGE_PROFILOMETER:
        return "profilometer";
    case GK_GAUGE_ROUNDNESS:
        return "roundness-tester";
    case GK_GAUGE_VISION:
        return "vision-measuring";
    case GK_GAUGE_LASER_SCAN:
        return "laser-scanner";
    case GK_GAUGE_ONLINE_PROBE:
        return "online-probe";
    default:
        return "unknown";
    }
}

gk_status gk_gauge_init(gk_gauge *g, gk_gauge_kind kind)
{
    if (g == NULL || kind < 0 || kind >= GK_GAUGE_COUNT) {
        return GK_ERR_INVALID_ARG;
    }
    memset(g, 0, sizeof(*g));
    g->kind = kind;
    g->points = 1;
    switch (kind) {
    case GK_GAUGE_CALIPER:
        g->resolution = 0.01;
        g->accuracy = 0.02;
        g->range_min = 0.0;
        g->range_max = 150.0;
        break;
    case GK_GAUGE_MICROMETER:
        g->resolution = 0.001;
        g->accuracy = 0.004;
        g->range_min = 0.0;
        g->range_max = 25.0;
        break;
    case GK_GAUGE_HEIGHT:
        g->resolution = 0.01;
        g->accuracy = 0.03;
        g->range_min = 0.0;
        g->range_max = 600.0;
        break;
    case GK_GAUGE_BORE_MICROMETER:
        g->resolution = 0.001;
        g->accuracy = 0.005;
        g->range_min = 5.0;
        g->range_max = 100.0;
        break;
    case GK_GAUGE_CMM:
        g->resolution = 0.0001;
        g->accuracy = 0.002;
        g->range_min = 0.0;
        g->range_max = 1000.0;
        g->points = 5;
        break;
    case GK_GAUGE_ROUGHNESS:
        g->resolution = 0.001;
        g->accuracy = 0.01;
        g->range_min = 0.0;
        g->range_max = 100.0; /* um */
        g->points = 5;
        break;
    case GK_GAUGE_PROFILOMETER:
        g->resolution = 0.0001;
        g->accuracy = 0.002;
        g->range_min = -500.0;
        g->range_max = 500.0;
        g->points = 100;
        break;
    case GK_GAUGE_ROUNDNESS:
        g->resolution = 0.0001;
        g->accuracy = 0.002;
        g->range_min = 0.0;
        g->range_max = 200.0;
        g->points = 36;
        break;
    case GK_GAUGE_VISION:
        g->resolution = 0.001;
        g->accuracy = 0.005;
        g->range_min = 0.0;
        g->range_max = 300.0;
        g->points = 10;
        break;
    case GK_GAUGE_LASER_SCAN:
        g->resolution = 0.0005;
        g->accuracy = 0.003;
        g->range_min = 0.0;
        g->range_max = 400.0;
        g->points = 50;
        break;
    case GK_GAUGE_ONLINE_PROBE:
        g->resolution = 0.001;
        g->accuracy = 0.005;
        g->range_min = 0.0;
        g->range_max = 1000.0;
        g->points = 1;
        break;
    default:
        return GK_ERR_INVALID_ARG;
    }
    return GK_OK;
}

int gk_gauge_is_in_range(const gk_gauge *g, double reading)
{
    if (g == NULL) {
        return 0;
    }
    return reading >= g->range_min && reading <= g->range_max;
}

double gk_gauge_read(const gk_gauge *g, double true_value)
{
    double q;
    if (g == NULL || g->resolution <= 0.0) {
        return true_value;
    }
    q = floor(true_value / g->resolution + 0.5) * g->resolution;
    return q;
}

gk_measure_result gk_measure_eval(const gk_gauge *g, double nominal,
                                  double actual, double tolerance,
                                  const char *label)
{
    gk_measure_result r;
    memset(&r, 0, sizeof(r));
    r.nominal = nominal;
    r.true_value = actual;
    r.measured = gk_gauge_read(g, actual);
    r.error = r.measured - nominal;
    r.tolerance = tolerance;
    r.out_of_tolerance = (fabs(r.error) > tolerance) ? 1 : 0;
    if (label != NULL) {
        size_t i;
        for (i = 0; i < sizeof(r.label) - 1 && label[i] != '\0'; ++i) {
            r.label[i] = label[i];
        }
        r.label[i] = '\0';
    }
    return r;
}

int gk_measure_out_of_tolerance(const gk_measure_result *r)
{
    return r == NULL ? 0 : r->out_of_tolerance;
}

int gk_measure_is_conforming(const gk_measure_result *r)
{
    return r == NULL ? 0 : !r->out_of_tolerance;
}

/* ---- auto cycle ---- */

void gk_measure_cycle_init(gk_measure_cycle *c, double clearance, double depth,
                           double feed, int total)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    c->state = GK_CYCLE_IDLE;
    c->clearance = clearance;
    c->depth = depth;
    c->feed = feed;
    c->total = total > 0 ? total : 1;
}

gk_measure_cycle_state gk_measure_cycle_step(gk_measure_cycle *c, double dt,
                                             double *out_reading)
{
    if (c == NULL) {
        return GK_CYCLE_IDLE;
    }
    c->elapsed += dt;
    switch (c->state) {
    case GK_CYCLE_IDLE:
        c->state = GK_CYCLE_APPROACH;
        break;
    case GK_CYCLE_APPROACH:
        c->state = GK_CYCLE_PROBE;
        break;
    case GK_CYCLE_PROBE:
        if (out_reading != NULL) {
            *out_reading = c->depth;
        }
        c->state = GK_CYCLE_RETRACT;
        break;
    case GK_CYCLE_RETRACT:
        c->state = GK_CYCLE_RECORD;
        break;
    case GK_CYCLE_RECORD:
        c->index += 1;
        if (c->index >= c->total) {
            c->state = GK_CYCLE_DONE;
        } else {
            c->state = GK_CYCLE_APPROACH;
        }
        break;
    case GK_CYCLE_DONE:
    default:
        break;
    }
    return c->state;
}

/* ---- GD&T ---- */

gk_gdt_result gk_gdt_check(const gk_gdt_tolerance *t, double measured)
{
    gk_gdt_result r;
    memset(&r, 0, sizeof(r));
    r.measured = measured;
    if (t == NULL) {
        return r;
    }
    r.deviation = measured - t->nominal;
    r.allowed = r.deviation >= 0.0 ? t->upper_tol : t->lower_tol;
    if (t->is_positional) {
        r.within = fabs(r.deviation) <= fabs(r.allowed) ? 1 : 0;
    } else {
        r.within = (r.deviation <= t->upper_tol &&
                    r.deviation >= -t->lower_tol)
                       ? 1
                       : 0;
    }
    return r;
}

double gk_gdt_worst_case(const double *tol_plus, const double *tol_minus,
                         size_t n)
{
    double sum = 0.0;
    size_t i;
    if (tol_plus == NULL || tol_minus == NULL) {
        return 0.0;
    }
    for (i = 0; i < n; ++i) {
        sum += fabs(tol_plus[i]) + fabs(tol_minus[i]);
    }
    return sum;
}

double gk_gdt_rss(const double *tols, size_t n)
{
    double sum = 0.0;
    size_t i;
    if (tols == NULL) {
        return 0.0;
    }
    for (i = 0; i < n; ++i) {
        sum += tols[i] * tols[i];
    }
    return sqrt(sum);
}

/* ---- samples / SPC ---- */

void gk_sample_set_init(gk_sample_set *s, double target)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->target = target;
}

gk_status gk_sample_add(gk_sample_set *s, double value)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (s->count >= GK_MEASURE_MAX_SAMPLES) {
        return GK_ERR_OVERFLOW;
    }
    s->samples[s->count++] = value;
    return GK_OK;
}

double gk_sample_mean(const gk_sample_set *s)
{
    double sum = 0.0;
    size_t i;
    if (s == NULL || s->count == 0) {
        return 0.0;
    }
    for (i = 0; i < s->count; ++i) {
        sum += s->samples[i];
    }
    return sum / (double)s->count;
}

double gk_sample_stddev(const gk_sample_set *s, int sampled)
{
    double mean;
    double sum = 0.0;
    size_t i;
    size_t denom;
    if (s == NULL || s->count < 2) {
        return 0.0;
    }
    mean = gk_sample_mean(s);
    for (i = 0; i < s->count; ++i) {
        double d = s->samples[i] - mean;
        sum += d * d;
    }
    denom = sampled ? s->count - 1 : s->count;
    return sqrt(sum / (double)denom);
}

double gk_sample_min(const gk_sample_set *s)
{
    double v;
    size_t i;
    if (s == NULL || s->count == 0) {
        return 0.0;
    }
    v = s->samples[0];
    for (i = 1; i < s->count; ++i) {
        if (s->samples[i] < v) {
            v = s->samples[i];
        }
    }
    return v;
}

double gk_sample_max(const gk_sample_set *s)
{
    double v;
    size_t i;
    if (s == NULL || s->count == 0) {
        return 0.0;
    }
    v = s->samples[0];
    for (i = 1; i < s->count; ++i) {
        if (s->samples[i] > v) {
            v = s->samples[i];
        }
    }
    return v;
}

double gk_sample_range(const gk_sample_set *s)
{
    return gk_sample_max(s) - gk_sample_min(s);
}

void gk_spc_limits(const gk_sample_set *s, double *ucl, double *lcl,
                   double *center)
{
    double mean = gk_sample_mean(s);
    double sd = gk_sample_stddev(s, 1);
    if (ucl != NULL) {
        *ucl = mean + 3.0 * sd;
    }
    if (lcl != NULL) {
        *lcl = mean - 3.0 * sd;
    }
    if (center != NULL) {
        *center = mean;
    }
}

int gk_spc_out_of_control(const gk_sample_set *s, size_t index)
{
    double ucl, lcl;
    if (s == NULL || index >= s->count) {
        return 0;
    }
    gk_spc_limits(s, &ucl, &lcl, NULL);
    if (s->count < 2) {
        return 0;
    }
    return (s->samples[index] > ucl || s->samples[index] < lcl) ? 1 : 0;
}

int gk_spc_out_of_control_count(const gk_sample_set *s)
{
    size_t i;
    int n = 0;
    if (s == NULL) {
        return 0;
    }
    for (i = 0; i < s->count; ++i) {
        n += gk_spc_out_of_control(s, i);
    }
    return n;
}

static gk_status capability(const gk_sample_set *s, double usl, double lsl,
                            int sampled, double *out)
{
    double mean, sd;
    if (s == NULL || out == NULL || s->count < 2 || usl <= lsl) {
        return GK_ERR_INVALID_ARG;
    }
    mean = gk_sample_mean(s);
    sd = gk_sample_stddev(s, sampled);
    if (sd <= 0.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    *out = (usl - mean < mean - lsl ? usl - mean : mean - lsl) /
           (3.0 * sd);
    return GK_OK;
}

gk_status gk_cpk(const gk_sample_set *s, double usl, double lsl, double *cpk)
{
    return capability(s, usl, lsl, 1, cpk);
}

gk_status gk_ppk(const gk_sample_set *s, double usl, double lsl, double *ppk)
{
    return capability(s, usl, lsl, 0, ppk);
}

gk_status gk_cp(const gk_sample_set *s, double usl, double lsl, double *cp)
{
    double sd;
    if (s == NULL || cp == NULL || s->count < 2 || usl <= lsl) {
        return GK_ERR_INVALID_ARG;
    }
    sd = gk_sample_stddev(s, 1);
    if (sd <= 0.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    *cp = (usl - lsl) / (6.0 * sd);
    return GK_OK;
}

/* ---- reporting ---- */

void gk_report_init(gk_report *r)
{
    if (r == NULL) {
        return;
    }
    memset(r, 0, sizeof(*r));
}

gk_status gk_report_add_line(gk_report *r, const gk_measure_result *result,
                             const char *unit)
{
    char line[160];
    int n;
    if (r == NULL || result == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(line, sizeof(line), "%s: %.4f %s [%s]\n", result->label,
                 result->measured, unit != NULL ? unit : "",
                 result->out_of_tolerance ? "FAIL" : "OK");
    if (n < 0 || r->length + (size_t)n >= sizeof(r->content)) {
        return GK_ERR_OVERFLOW;
    }
    memcpy(r->content + r->length, line, (size_t)n);
    r->length += (size_t)n;
    r->content[r->length] = '\0';
    if (result->out_of_tolerance) {
        r->failed += 1;
    } else {
        r->passed += 1;
    }
    return GK_OK;
}

const char *gk_report_text(const gk_report *r)
{
    if (r == NULL) {
        return "";
    }
    return r->content;
}

size_t gk_samples_to_csv(const gk_sample_set *s, char *out, size_t out_size)
{
    size_t i;
    size_t written = 0;
    if (s == NULL || out == NULL || out_size == 0) {
        return 0;
    }
    written += (size_t)snprintf(out + written, out_size - written,
                                "index,value\n");
    for (i = 0; i < s->count && written + 1 < out_size; ++i) {
        written += (size_t)snprintf(out + written, out_size - written,
                                    "%zu,%.6f\n", i, s->samples[i]);
    }
    return written;
}

/* ---- tolerance alarm ---- */

void gk_tolerance_alarm_init(gk_tolerance_alarm *a)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
}

gk_status gk_tolerance_alarm_check(gk_tolerance_alarm *a,
                                   const gk_measure_result *r)
{
    if (a == NULL || r == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    a->nominal = r->nominal;
    a->actual = r->measured;
    a->active = r->out_of_tolerance ? 1 : 0;
    a->overage = fabs(r->error) - r->tolerance;
    if (a->overage < 0.0) {
        a->overage = 0.0;
    }
    memcpy(a->label, r->label, sizeof(a->label) - 1);
    a->label[sizeof(a->label) - 1] = '\0';
    return GK_OK;
}
