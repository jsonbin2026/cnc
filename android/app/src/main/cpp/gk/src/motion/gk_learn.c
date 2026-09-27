#include "gk/gk_learn.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

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

static int gk__streq(const char *a, const char *b)
{
    return a != NULL && b != NULL && strcmp(a, b) == 0;
}

const char *gk_learn_dim_name(gk_learn_dim d)
{
    switch (d) {
    case GK_LEARN_DIM_SAFETY: return "safety";
    case GK_LEARN_DIM_SETUP: return "setup";
    case GK_LEARN_DIM_PROGRAM: return "programming";
    case GK_LEARN_DIM_OPERATION: return "operation";
    case GK_LEARN_DIM_INSPECTION: return "inspection";
    case GK_LEARN_DIM_TROUBLESHOOT: return "troubleshooting";
    default: return "unknown";
    }
}

const char *gk_learn_role_name(gk_learn_role r)
{
    switch (r) {
    case GK_LEARN_ROLE_BUTTON: return "button";
    case GK_LEARN_ROLE_SLIDER: return "slider";
    case GK_LEARN_ROLE_FIELD: return "textfield";
    case GK_LEARN_ROLE_CHART: return "image";
    default: return "generic";
    }
}

gk_status gk_learn_screenreader_desc(gk_learn_role role, const char *label,
                                     const char *value, char *out,
                                     size_t out_cap)
{
    int n;
    const char *lbl = (label != NULL) ? label : "";
    const char *val = value;
    if (out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    if (val != NULL && val[0] != '\0') {
        n = snprintf(out, out_cap, "%s, %s, %s", gk_learn_role_name(role), lbl,
                     val);
    } else {
        n = snprintf(out, out_cap, "%s, %s", gk_learn_role_name(role), lbl);
    }
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

void gk_learn_kbdnav_init(gk_learn_kbdnav *n)
{
    if (n == NULL) {
        return;
    }
    memset(n, 0, sizeof(*n));
    n->wrap = 1;
}

int gk_learn_kbdnav_add(gk_learn_kbdnav *n, const char *label)
{
    if (n == NULL || label == NULL || n->count >= GK_LEARN_MAX_ITEMS) {
        return -1;
    }
    gk__copy(n->labels[n->count], sizeof(n->labels[n->count]), label);
    n->count++;
    return n->count;
}

int gk_learn_kbdnav_next(gk_learn_kbdnav *n)
{
    if (n == NULL || n->count == 0) {
        return -1;
    }
    n->focus++;
    if (n->focus >= n->count) {
        n->focus = n->wrap ? 0 : n->count - 1;
    }
    return n->focus;
}

int gk_learn_kbdnav_prev(gk_learn_kbdnav *n)
{
    if (n == NULL || n->count == 0) {
        return -1;
    }
    n->focus--;
    if (n->focus < 0) {
        n->focus = n->wrap ? n->count - 1 : 0;
    }
    return n->focus;
}

int gk_learn_kbdnav_focus(gk_learn_kbdnav *n, const char *label)
{
    int i;
    if (n == NULL || label == NULL) {
        return -1;
    }
    for (i = 0; i < n->count; i++) {
        if (gk__streq(n->labels[i], label)) {
            n->focus = i;
            return i;
        }
    }
    return -1;
}

void gk_learn_touch_init(gk_learn_touch *t)
{
    if (t == NULL) {
        return;
    }
    t->min_target_mm = 9.0;
    t->scale = 1.0;
}

gk_status gk_learn_touch_target(gk_learn_touch *t, double width_mm,
                                double height_mm)
{
    double required;
    if (t == NULL || width_mm <= 0.0 || height_mm <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    required = (width_mm < height_mm) ? width_mm : height_mm;
    if (required >= t->min_target_mm) {
        return GK_OK;
    }
    t->scale = t->min_target_mm / required;
    return GK_OK;
}

double gk_learn_touch_size(const gk_learn_touch *t, double base_mm)
{
    if (t == NULL) {
        return base_mm;
    }
    return base_mm * t->scale;
}

void gk_learn_student_init(gk_learn_student *s, const char *name)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    gk__copy(s->name, sizeof(s->name), name);
}

gk_status gk_learn_student_set(gk_learn_student *s, gk_learn_dim d,
                               double score)
{
    if (s == NULL || d >= GK_LEARN_DIMS) {
        return GK_ERR_INVALID_ARG;
    }
    if (score < 0.0) {
        score = 0.0;
    }
    if (score > 100.0) {
        score = 100.0;
    }
    s->score[d] = score;
    return GK_OK;
}

double gk_learn_student_avg(const gk_learn_student *s)
{
    int i;
    double sum = 0.0;
    if (s == NULL) {
        return 0.0;
    }
    for (i = 0; i < GK_LEARN_DIMS; i++) {
        sum += s->score[i];
    }
    return sum / (double)GK_LEARN_DIMS;
}

double gk_learn_radar_point(const gk_learn_student *s, gk_learn_dim d)
{
    double v;
    if (s == NULL || d >= GK_LEARN_DIMS) {
        return 0.0;
    }
    v = s->score[d] / 100.0;
    if (v < 0.0) {
        v = 0.0;
    }
    if (v > 1.0) {
        v = 1.0;
    }
    return v;
}

int gk_learn_student_weakest(const gk_learn_student *s)
{
    int i;
    int best = -1;
    double best_v = 1000.0;
    if (s == NULL) {
        return -1;
    }
    for (i = 0; i < GK_LEARN_DIMS; i++) {
        if (s->score[i] < best_v) {
            best_v = s->score[i];
            best = i;
        }
    }
    return best;
}

gk_status gk_learn_recommend_course(const gk_learn_student *s, char *out,
                                    size_t out_cap)
{
    int weakest;
    int n;
    if (s == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    weakest = gk_learn_student_weakest(s);
    if (weakest < 0) {
        return GK_ERR_STATE;
    }
    n = snprintf(out, out_cap, "course:remedial-%s",
                 gk_learn_dim_name((gk_learn_dim)weakest));
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

void gk_learn_errors_init(gk_learn_errors *e)
{
    if (e == NULL) {
        return;
    }
    memset(e, 0, sizeof(*e));
}

gk_status gk_learn_errors_add(gk_learn_errors *e, const char *pattern)
{
    int i;
    if (e == NULL || pattern == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < e->count; i++) {
        if (gk__streq(e->errors[i].pattern, pattern)) {
            e->errors[i].count++;
            return GK_OK;
        }
    }
    if (e->count >= GK_LEARN_MAX_ITEMS) {
        return GK_ERR_OVERFLOW;
    }
    gk__copy(e->errors[e->count].pattern, sizeof(e->errors[e->count].pattern),
             pattern);
    e->errors[e->count].count = 1;
    e->count++;
    return GK_OK;
}

int gk_learn_errors_top(const gk_learn_errors *e)
{
    int i;
    int best = -1;
    int best_c = 0;
    if (e == NULL) {
        return -1;
    }
    for (i = 0; i < e->count; i++) {
        if (e->errors[i].count > best_c) {
            best_c = e->errors[i].count;
            best = i;
        }
    }
    return best;
}

double gk_learn_predict_grade(const double *history, int count)
{
    int i;
    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_xx = 0.0;
    double n, slope, intercept;
    if (history == NULL || count < 2) {
        return (history != NULL && count == 1) ? history[0] : 0.0;
    }
    for (i = 0; i < count; i++) {
        double x = (double)i;
        sum_x += x;
        sum_y += history[i];
        sum_xy += x * history[i];
        sum_xx += x * x;
    }
    n = (double)count;
    if (n * sum_xx - sum_x * sum_x == 0.0) {
        return history[count - 1];
    }
    slope = (n * sum_xy - sum_x * sum_y) / (n * sum_xx - sum_x * sum_x);
    intercept = (sum_y - slope * sum_x) / n;
    return slope * n + intercept;
}

int gk_learn_cohort_percentile(const double *sorted_scores, int count,
                               double score)
{
    int i;
    int below = 0;
    if (sorted_scores == NULL || count <= 0) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        if (sorted_scores[i] < score) {
            below++;
        }
    }
    return (below * 100) / count;
}

gk_status gk_learn_dashboard_build(const gk_learn_student *students, int count,
                                   double risk_threshold, gk_learn_dashboard *out)
{
    int i;
    double sum = 0.0;
    if (students == NULL || out == NULL || count <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    out->students = count;
    out->min = 1e30;
    out->max = -1e30;
    out->at_risk = 0;
    for (i = 0; i < count; i++) {
        double a = gk_learn_student_avg(&students[i]);
        sum += a;
        if (a < out->min) {
            out->min = a;
        }
        if (a > out->max) {
            out->max = a;
        }
        if (a < risk_threshold) {
            out->at_risk++;
        }
    }
    out->mean = sum / (double)count;
    return GK_OK;
}

gk_status gk_learn_analyze_trend(const double *history, int count,
                                 gk_learn_trend *out)
{
    int i;
    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_xx = 0.0;
    double n;
    if (history == NULL || out == NULL || count < 2) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < count; i++) {
        double x = (double)i;
        sum_x += x;
        sum_y += history[i];
        sum_xy += x * history[i];
        sum_xx += x * x;
    }
    n = (double)count;
    if (n * sum_xx - sum_x * sum_x == 0.0) {
        out->slope = 0.0;
    } else {
        out->slope = (n * sum_xy - sum_x * sum_y) /
                     (n * sum_xx - sum_x * sum_x);
    }
    out->improving = out->slope > 0.0;
    return GK_OK;
}

void gk_learn_abtest_init(gk_learn_abtest *a)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
}

double gk_learn_abtest_uplift(const gk_learn_abtest *a)
{
    double pc, pv;
    if (a == NULL || a->control_n == 0) {
        return 0.0;
    }
    pc = (double)a->control_pass / (double)a->control_n;
    if (a->variant_n == 0) {
        return 0.0;
    }
    pv = (double)a->variant_pass / (double)a->variant_n;
    return pv - pc;
}

int gk_learn_abtest_significant(const gk_learn_abtest *a)
{
    double pc, pv, p, se;
    if (a == NULL || a->control_n == 0 || a->variant_n == 0) {
        return 0;
    }
    pc = (double)a->control_pass / (double)a->control_n;
    pv = (double)a->variant_pass / (double)a->variant_n;
    p = (pc * a->control_n + pv * a->variant_n) /
        (double)(a->control_n + a->variant_n);
    se = sqrt(p * (1.0 - p) *
              (1.0 / a->control_n + 1.0 / a->variant_n));
    if (se <= 0.0) {
        return 0;
    }
    return fabs(pv - pc) / se > 1.96;
}

void gk_learn_eval_init(gk_learn_eval *e)
{
    if (e == NULL) {
        return;
    }
    memset(e, 0, sizeof(*e));
}

gk_status gk_learn_eval_add(gk_learn_eval *e, double weight, double score)
{
    if (e == NULL || weight < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    if (e->count >= GK_LEARN_DIMS) {
        return GK_ERR_OVERFLOW;
    }
    e->weight[e->count] = weight;
    e->score[e->count] = score;
    e->count++;
    return GK_OK;
}

double gk_learn_eval_total(const gk_learn_eval *e)
{
    int i;
    double wsum = 0.0;
    double total = 0.0;
    if (e == NULL) {
        return 0.0;
    }
    for (i = 0; i < e->count; i++) {
        total += e->weight[i] * e->score[i];
        wsum += e->weight[i];
    }
    if (wsum <= 0.0) {
        return 0.0;
    }
    return total / wsum;
}

gk_status gk_learn_bar_chart(const char *labels, const double *values,
                             int count, char *out, size_t out_cap)
{
    int i;
    int off = 0;
    if (labels == NULL || values == NULL || out == NULL || out_cap == 0 ||
        count <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    out[0] = '\0';
    for (i = 0; i < count; i++) {
        int bars = (int)(values[i] + 0.5);
        int j;
        int n;
        if (bars < 0) {
            bars = 0;
        }
        if (bars > 40) {
            bars = 40;
        }
        n = snprintf(out + off, out_cap - (size_t)off, "%c|",
                     labels[i]);
        if (n < 0 || (size_t)n >= out_cap - (size_t)off) {
            return GK_ERR_OUT_OF_RANGE;
        }
        off += n;
        for (j = 0; j < bars; j++) {
            if ((size_t)off + 1 >= out_cap) {
                return GK_ERR_OUT_OF_RANGE;
            }
            out[off++] = '#';
            out[off] = '\0';
        }
        if ((size_t)off + 1 >= out_cap) {
            return GK_ERR_OUT_OF_RANGE;
        }
        out[off++] = '\n';
        out[off] = '\0';
    }
    return GK_OK;
}

gk_status gk_learn_report(const gk_learn_student *students, int count,
                          const char *title, char *out, size_t out_cap)
{
    int i;
    int off = 0;
    if (students == NULL || out == NULL || out_cap == 0 || count <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    {
        int n = snprintf(out, out_cap, "REPORT %s\n",
                         title != NULL ? title : "");
        if (n < 0 || (size_t)n >= out_cap) {
            return GK_ERR_OUT_OF_RANGE;
        }
        off = n;
    }
    for (i = 0; i < count; i++) {
        int n = snprintf(out + off, out_cap - (size_t)off, "%s avg=%.1f\n",
                         students[i].name, gk_learn_student_avg(&students[i]));
        if (n < 0 || (size_t)n >= out_cap - (size_t)off) {
            return GK_ERR_OUT_OF_RANGE;
        }
        off += n;
    }
    return GK_OK;
}

gk_status gk_learn_export_csv(const gk_learn_student *students, int count,
                              char *out, size_t out_cap)
{
    int i, d;
    int off = 0;
    if (students == NULL || out == NULL || out_cap == 0 || count <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    {
        int n = snprintf(out, out_cap, "name");
        if (n < 0 || (size_t)n >= out_cap) {
            return GK_ERR_OUT_OF_RANGE;
        }
        off = n;
    }
    for (d = 0; d < GK_LEARN_DIMS; d++) {
        int n = snprintf(out + off, out_cap - (size_t)off, ",%s",
                         gk_learn_dim_name((gk_learn_dim)d));
        if (n < 0 || (size_t)n >= out_cap - (size_t)off) {
            return GK_ERR_OUT_OF_RANGE;
        }
        off += n;
    }
    if ((size_t)off + 1 >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    out[off++] = '\n';
    out[off] = '\0';
    for (i = 0; i < count; i++) {
        int n = snprintf(out + off, out_cap - (size_t)off, "%s",
                         students[i].name);
        if (n < 0 || (size_t)n >= out_cap - (size_t)off) {
            return GK_ERR_OUT_OF_RANGE;
        }
        off += n;
        for (d = 0; d < GK_LEARN_DIMS; d++) {
            n = snprintf(out + off, out_cap - (size_t)off, ",%.1f",
                         students[i].score[d]);
            if (n < 0 || (size_t)n >= out_cap - (size_t)off) {
                return GK_ERR_OUT_OF_RANGE;
            }
            off += n;
        }
        if ((size_t)off + 1 >= out_cap) {
            return GK_ERR_OUT_OF_RANGE;
        }
        out[off++] = '\n';
        out[off] = '\0';
    }
    return GK_OK;
}

gk_status gk_learn_redact(const char *text, const char *token,
                          const char *replacement, char *out, size_t out_cap)
{
    size_t tlen;
    int off = 0;
    const char *p;
    if (text == NULL || token == NULL || replacement == NULL || out == NULL ||
        out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    tlen = strlen(token);
    if (tlen == 0) {
        gk__copy(out, out_cap, text);
        return GK_OK;
    }
    out[0] = '\0';
    p = text;
    while (*p != '\0') {
        if (strncmp(p, token, tlen) == 0) {
            int n = snprintf(out + off, out_cap - (size_t)off, "%s",
                             replacement);
            if (n < 0 || (size_t)n >= out_cap - (size_t)off) {
                return GK_ERR_OUT_OF_RANGE;
            }
            off += n;
            p += tlen;
        } else {
            if ((size_t)off + 1 >= out_cap) {
                return GK_ERR_OUT_OF_RANGE;
            }
            out[off++] = *p++;
            out[off] = '\0';
        }
    }
    return GK_OK;
}

unsigned long gk_learn_anonymize(const char *identifier, unsigned long salt)
{
    unsigned long h = 1469598103934665603UL ^ salt;
    size_t i;
    if (identifier == NULL) {
        return 0;
    }
    for (i = 0; identifier[i] != '\0'; i++) {
        h ^= (unsigned char)identifier[i];
        h *= 1099511628211UL;
    }
    return h;
}
