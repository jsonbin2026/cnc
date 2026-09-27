#include "gk/gk_phys.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ===================================================================
 * Part A: drift & ageing
 * =================================================================== */

void gk_phys_daily_init(gk_phys_daily_drift *d, double drift_per_day,
                        double noise_sigma)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    d->drift_per_day = drift_per_day;
    d->noise_sigma = noise_sigma;
    d->reference = 0.0;
}

double gk_phys_daily_advance(gk_phys_daily_drift *d, double days,
                             double noise)
{
    double value;
    if (d == NULL) {
        return 0.0;
    }
    if (days < 0.0) {
        return d->reference + d->drift_per_day * d->elapsed_days;
    }
    d->elapsed_days += days;
    value = d->drift_per_day * d->elapsed_days + noise * d->noise_sigma;
    return value;
}

void gk_phys_monthly_wear_init(gk_phys_monthly_wear *w,
                               double wear_per_part, double vb_limit_um)
{
    if (w == NULL) {
        return;
    }
    memset(w, 0, sizeof(*w));
    w->wear_per_part = wear_per_part;
    w->vb_limit_um = (vb_limit_um > 0.0) ? vb_limit_um : 300.0;
    w->parts_per_month = 1000.0;
}

gk_status gk_phys_monthly_wear_run(gk_phys_monthly_wear *w, int months)
{
    int i;
    if (w == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (months < 0) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < months; i++) {
        w->vb_um += w->wear_per_part * w->parts_per_month;
        w->months++;
    }
    return GK_OK;
}

int gk_phys_monthly_wear_expired(const gk_phys_monthly_wear *w)
{
    if (w == NULL) {
        return 0;
    }
    return w->vb_um >= w->vb_limit_um;
}

void gk_phys_ageing_init(gk_phys_ageing *a, double mtbf_hours,
                         double ageing_rate)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    a->mtbf_hours = (mtbf_hours > 0.0) ? mtbf_hours : 10000.0;
    a->ageing_rate = ageing_rate;
    a->reliability = 1.0;
}

double gk_phys_ageing_reliability(const gk_phys_ageing *a, double hours)
{
    double effective_mtbf;
    if (a == NULL || hours < 0.0) {
        return 0.0;
    }
    /* ageing reduces the effective MTBF exponentially with machine age */
    effective_mtbf = a->mtbf_hours * exp(-a->ageing_rate * a->age_years);
    if (effective_mtbf <= 0.0) {
        return 0.0;
    }
    return exp(-hours / effective_mtbf);
}

gk_status gk_phys_ageing_advance(gk_phys_ageing *a, double years)
{
    if (a == NULL || years < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    a->age_years += years;
    a->reliability = gk_phys_ageing_reliability(a, 0.0);
    return GK_OK;
}

void gk_phys_life_init(gk_phys_life *l, double design_life_years,
                       double annual_hours)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
    l->design_life_years = (design_life_years > 0.0) ? design_life_years : 10.0;
    l->annual_hours = annual_hours;
}

gk_status gk_phys_life_advance(gk_phys_life *l, double years)
{
    if (l == NULL || years < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    l->age_years += years;
    l->hours_used += years * l->annual_hours;
    return GK_OK;
}

double gk_phys_life_remaining_years(const gk_phys_life *l)
{
    double remaining;
    if (l == NULL) {
        return 0.0;
    }
    remaining = l->design_life_years - l->age_years;
    return remaining > 0.0 ? remaining : 0.0;
}

int gk_phys_life_expired(const gk_phys_life *l)
{
    if (l == NULL) {
        return 0;
    }
    return l->age_years >= l->design_life_years;
}

void gk_phys_history_init(gk_phys_history *h)
{
    if (h == NULL) {
        return;
    }
    memset(h, 0, sizeof(*h));
}

int gk_phys_history_record(gk_phys_history *h,
                           const gk_phys_history_frame *f)
{
    if (h == NULL || f == NULL) {
        return -1;
    }
    if (h->count >= GK_PHYS_MAX_HISTORY) {
        /* shift out the oldest frame */
        memmove(&h->frames[0], &h->frames[1],
                sizeof(gk_phys_history_frame) * (GK_PHYS_MAX_HISTORY - 1));
        h->count = GK_PHYS_MAX_HISTORY - 1;
    }
    h->frames[h->count] = *f;
    h->count++;
    return h->count;
}

const gk_phys_history_frame *gk_phys_history_next(gk_phys_history *h)
{
    if (h == NULL || h->count == 0) {
        return NULL;
    }
    if (h->cursor >= h->count) {
        h->cursor = 0;
    }
    return &h->frames[h->cursor++];
}

gk_status gk_phys_history_seek(gk_phys_history *h, int index)
{
    if (h == NULL || index < 0 || index >= h->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    h->cursor = index;
    return GK_OK;
}

double gk_phys_history_total_time(const gk_phys_history *h)
{
    if (h == NULL || h->count == 0) {
        return 0.0;
    }
    return h->frames[h->count - 1].time_s - h->frames[0].time_s;
}

/* ===================================================================
 * Part B: multi-physics fields
 * =================================================================== */

const char *gk_phys_field_name(gk_phys_field_kind k)
{
    switch (k) {
    case GK_PHYS_FORCE: return "force";
    case GK_PHYS_THERMAL: return "thermal";
    case GK_PHYS_MAGNETIC: return "magnetic";
    case GK_PHYS_ELECTRIC: return "electric";
    case GK_PHYS_ACOUSTIC: return "acoustic";
    case GK_PHYS_FLOW: return "flow";
    case GK_PHYS_OPTICAL: return "optical";
    case GK_PHYS_CHEMICAL: return "chemical";
    case GK_PHYS_RADIATION: return "radiation";
    default: return "unknown";
    }
}

gk_status gk_phys_field_init(gk_phys_field *f, gk_phys_field_kind kind,
                             int rows, int cols)
{
    if (f == NULL || kind >= GK_PHYS_FIELD_COUNT) {
        return GK_ERR_INVALID_ARG;
    }
    if (rows <= 0 || cols <= 0 || rows > GK_PHYS_MAX_GRID ||
        cols > GK_PHYS_MAX_GRID) {
        return GK_ERR_OUT_OF_RANGE;
    }
    memset(f, 0, sizeof(*f));
    f->kind = kind;
    f->rows = rows;
    f->cols = cols;
    f->min_value = 0.0;
    f->max_value = 0.0;
    f->visible = 1;
    return GK_OK;
}

gk_status gk_phys_field_set(gk_phys_field *f, int r, int c, double value)
{
    if (f == NULL || r < 0 || c < 0 || r >= f->rows || c >= f->cols) {
        return GK_ERR_OUT_OF_RANGE;
    }
    f->data[r * f->cols + c] = value;
    if (value < f->min_value) {
        f->min_value = value;
    }
    if (value > f->max_value) {
        f->max_value = value;
    }
    return GK_OK;
}

double gk_phys_field_get(const gk_phys_field *f, int r, int c)
{
    if (f == NULL || r < 0 || c < 0 || r >= f->rows || c >= f->cols) {
        return 0.0;
    }
    return f->data[r * f->cols + c];
}

gk_status gk_phys_field_source(gk_phys_field *f, double cx, double cy,
                               double amplitude, double sigma)
{
    int r, c;
    if (f == NULL || sigma <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    f->min_value = 0.0;
    f->max_value = 0.0;
    for (r = 0; r < f->rows; r++) {
        for (c = 0; c < f->cols; c++) {
            double dx = (double)c - cx;
            double dy = (double)r - cy;
            double d2 = dx * dx + dy * dy;
            double v = amplitude * exp(-d2 / (2.0 * sigma * sigma));
            (void)gk_phys_field_set(f, r, c, v);
        }
    }
    return GK_OK;
}

gk_status gk_phys_field_update_stats(gk_phys_field *f)
{
    int r, c;
    if (f == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    f->min_value = 0.0;
    f->max_value = 0.0;
    for (r = 0; r < f->rows; r++) {
        for (c = 0; c < f->cols; c++) {
            double v = f->data[r * f->cols + c];
            if (v < f->min_value) {
                f->min_value = v;
            }
            if (v > f->max_value) {
                f->max_value = v;
            }
        }
    }
    return GK_OK;
}

double gk_phys_field_mean(const gk_phys_field *f)
{
    int r, c;
    double sum = 0.0;
    int n;
    if (f == NULL || f->rows <= 0 || f->cols <= 0) {
        return 0.0;
    }
    n = f->rows * f->cols;
    for (r = 0; r < f->rows; r++) {
        for (c = 0; c < f->cols; c++) {
            sum += f->data[r * f->cols + c];
        }
    }
    return sum / (double)n;
}

/* ===================================================================
 * coupling
 * =================================================================== */

void gk_phys_coupling_init(gk_phys_coupling *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    c->strength = 1.0;
}

int gk_phys_coupling_add(gk_phys_coupling *c, const gk_phys_field *f)
{
    if (c == NULL || f == NULL || c->count >= GK_PHYS_MAX_FIELDS) {
        return -1;
    }
    c->fields[c->count] = *f;
    c->count++;
    return c->count;
}

gk_status gk_phys_coupling_set(gk_phys_coupling *c, int from, int to,
                               double coefficient)
{
    if (c == NULL || from < 0 || to < 0 || from >= c->count ||
        to >= c->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    c->coupling[from][to] = coefficient;
    return GK_OK;
}

gk_status gk_phys_coupling_set_strength(gk_phys_coupling *c, double strength)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (strength < 0.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    c->strength = strength;
    return GK_OK;
}

gk_status gk_phys_coupling_show_single(gk_phys_coupling *c, int index)
{
    int i;
    if (c == NULL || index < 0 || index >= c->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    for (i = 0; i < c->count; i++) {
        c->fields[i].visible = (i == index) ? 1 : 0;
    }
    return GK_OK;
}

gk_status gk_phys_coupling_overlay_all(gk_phys_coupling *c)
{
    int i;
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < c->count; i++) {
        c->fields[i].visible = 1;
    }
    return GK_OK;
}

double gk_phys_coupling_step(gk_phys_coupling *c, int from, int to)
{
    double mean;
    if (c == NULL || from < 0 || to < 0 || from >= c->count ||
        to >= c->count) {
        return 0.0;
    }
    mean = gk_phys_field_mean(&c->fields[from]);
    {
        double transferred = mean * c->coupling[from][to] * c->strength;
        /* inject the transferred magnitude as a source in the target field */
        (void)gk_phys_field_source(&c->fields[to],
                                   (double)c->fields[to].cols / 2.0 - 0.5,
                                   (double)c->fields[to].rows / 2.0 - 0.5,
                                   transferred, 2.0);
        return transferred;
    }
}

int gk_phys_coupling_dominant(const gk_phys_coupling *c)
{
    int i;
    int best = -1;
    double best_mean = -1.0;
    if (c == NULL) {
        return -1;
    }
    for (i = 0; i < c->count; i++) {
        double m = fabs(gk_phys_field_mean(&c->fields[i]));
        if (m > best_mean) {
            best_mean = m;
            best = i;
        }
    }
    return best;
}
