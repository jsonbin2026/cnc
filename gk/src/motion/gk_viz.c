#include "gk/gk_viz.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static void gk__copy(char *dst, size_t len, const char *src)
{
    size_t i;
    if (dst == NULL || len == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    for (i = 0; i + 1 < len && src[i] != '\0'; ++i) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

static int gk__appendf(char *buf, size_t len, int off, const char *fmt, ...)
{
    char tmp[256];
    va_list ap;
    size_t l;
    va_start(ap, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    l = strlen(tmp);
    if ((size_t)off + l >= len) {
        l = len > (size_t)off ? len - (size_t)off - 1 : 0;
    }
    memcpy(buf + off, tmp, l);
    off += (int)l;
    buf[off] = '\0';
    return off;
}

/* ---- oscilloscope ---- */

void gk_scope_init(gk_scope *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->timebase = 0.1;
    s->scale = 1.0;
}

int gk_scope_add_channel(gk_scope *s, const char *name)
{
    gk_series *ser;
    if (s == NULL || s->count >= GK_VIZ_MAX_SERIES) {
        return -1;
    }
    ser = &s->series[s->count];
    memset(ser, 0, sizeof(*ser));
    gk__copy(ser->name, sizeof(ser->name), name);
    ser->enabled = 1;
    s->count++;
    return s->count - 1;
}

gk_status gk_scope_push(gk_scope *s, int channel, double value)
{
    gk_series *ser;
    if (s == NULL || channel < 0 || channel >= s->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    ser = &s->series[channel];
    if (ser->count >= GK_VIZ_MAX_POINTS) {
        memmove(&ser->samples[0], &ser->samples[1],
                sizeof(ser->samples[0]) * (GK_VIZ_MAX_POINTS - 1));
        ser->count = GK_VIZ_MAX_POINTS - 1;
    }
    ser->samples[ser->count++] = value;
    return GK_OK;
}

double gk_scope_value(const gk_scope *s, int channel, int i)
{
    if (s == NULL || channel < 0 || channel >= s->count) {
        return 0.0;
    }
    if (i < 0 || i >= s->series[channel].count) {
        return 0.0;
    }
    return s->series[channel].samples[i];
}

double gk_scope_peak(const gk_scope *s, int channel)
{
    double peak = 0.0;
    int i;
    if (s == NULL || channel < 0 || channel >= s->count) {
        return 0.0;
    }
    for (i = 0; i < s->series[channel].count; ++i) {
        double v = fabs(s->series[channel].samples[i]);
        if (v > peak) peak = v;
    }
    return peak;
}

double gk_scope_mean(const gk_scope *s, int channel)
{
    double sum = 0.0;
    int i;
    if (s == NULL || channel < 0 || channel >= s->count) {
        return 0.0;
    }
    if (s->series[channel].count == 0) {
        return 0.0;
    }
    for (i = 0; i < s->series[channel].count; ++i) {
        sum += s->series[channel].samples[i];
    }
    return sum / s->series[channel].count;
}

/* ---- data logger ---- */

void gk_logger_init(gk_logger *l, int channels, double rate)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
    l->channels = channels > GK_VIZ_MAX_SERIES ? GK_VIZ_MAX_SERIES : channels;
    l->rate = rate;
}

gk_status gk_logger_record(gk_logger *l, double time, const double *values)
{
    gk_log_sample *s;
    int i;
    if (l == NULL || values == NULL || l->count >= GK_VIZ_MAX_POINTS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    s = &l->samples[l->count++];
    s->time = time;
    s->channels = l->channels;
    for (i = 0; i < l->channels; ++i) {
        s->values[i] = values[i];
    }
    return GK_OK;
}

const gk_log_sample *gk_logger_at(const gk_logger *l, int i)
{
    if (l == NULL || i < 0 || i >= l->count) {
        return NULL;
    }
    return &l->samples[i];
}

double gk_logger_span(const gk_logger *l)
{
    if (l == NULL || l->count < 2) {
        return 0.0;
    }
    return l->samples[l->count - 1].time - l->samples[0].time;
}

int gk_logger_export(const gk_logger *l, char *buf, size_t len)
{
    int off = 0;
    int i, c;
    if (l == NULL || buf == NULL) {
        return 0;
    }
    buf[0] = '\0';
    off = gk__appendf(buf, len, off, "time");
    for (c = 0; c < l->channels; ++c) {
        off = gk__appendf(buf, len, off, ",ch%d", c);
    }
    off = gk__appendf(buf, len, off, "\n");
    for (i = 0; i < l->count; ++i) {
        off = gk__appendf(buf, len, off, "%.3f", l->samples[i].time);
        for (c = 0; c < l->channels; ++c) {
            off = gk__appendf(buf, len, off, ",%.3f",
                              l->samples[i].values[c]);
        }
        off = gk__appendf(buf, len, off, "\n");
    }
    return off;
}

/* ---- fields ---- */

void gk_field_init(gk_field *f, int rows, int cols)
{
    if (f == NULL) {
        return;
    }
    memset(f, 0, sizeof(*f));
    f->rows = rows > GK_VIZ_GRID ? GK_VIZ_GRID : rows;
    f->cols = cols > GK_VIZ_GRID ? GK_VIZ_GRID : cols;
}

gk_status gk_field_set(gk_field *f, int r, int c, double value)
{
    if (f == NULL || r < 0 || r >= f->rows || c < 0 || c >= f->cols) {
        return GK_ERR_OUT_OF_RANGE;
    }
    f->cells[r][c] = value;
    return GK_OK;
}

double gk_field_get(const gk_field *f, int r, int c)
{
    if (f == NULL || r < 0 || r >= f->rows || c < 0 || c >= f->cols) {
        return 0.0;
    }
    return f->cells[r][c];
}

double gk_field_mean(const gk_field *f)
{
    double sum = 0.0;
    int r, c;
    if (f == NULL || f->rows <= 0 || f->cols <= 0) {
        return 0.0;
    }
    for (r = 0; r < f->rows; ++r) {
        for (c = 0; c < f->cols; ++c) {
            sum += f->cells[r][c];
        }
    }
    return sum / (f->rows * f->cols);
}

void gk_field_rescale(gk_field *f)
{
    int r, c;
    if (f == NULL) {
        return;
    }
    f->min = 0.0;
    f->max = 0.0;
    for (r = 0; r < f->rows; ++r) {
        for (c = 0; c < f->cols; ++c) {
            double v = f->cells[r][c];
            if (r == 0 && c == 0) {
                f->min = f->max = v;
            } else {
                if (v < f->min) f->min = v;
                if (v > f->max) f->max = v;
            }
        }
    }
}

double gk_field_max_deviation(const gk_field *f, double target)
{
    double best = 0.0;
    int r, c;
    if (f == NULL) {
        return 0.0;
    }
    for (r = 0; r < f->rows; ++r) {
        for (c = 0; c < f->cols; ++c) {
            double d = fabs(f->cells[r][c] - target);
            if (d > best) best = d;
        }
    }
    return best;
}

/* ---- vector field ---- */

void gk_vfield_init(gk_vector_field *f, int rows, int cols)
{
    if (f == NULL) {
        return;
    }
    memset(f, 0, sizeof(*f));
    f->rows = rows > GK_VIZ_GRID ? GK_VIZ_GRID : rows;
    f->cols = cols > GK_VIZ_GRID ? GK_VIZ_GRID : cols;
}

gk_status gk_vfield_set(gk_vector_field *f, int r, int c, double u, double v)
{
    if (f == NULL || r < 0 || r >= f->rows || c < 0 || c >= f->cols) {
        return GK_ERR_OUT_OF_RANGE;
    }
    f->u[r][c] = u;
    f->v[r][c] = v;
    return GK_OK;
}

double gk_vfield_magnitude(const gk_vector_field *f, int r, int c)
{
    if (f == NULL || r < 0 || r >= f->rows || c < 0 || c >= f->cols) {
        return 0.0;
    }
    return sqrt(f->u[r][c] * f->u[r][c] + f->v[r][c] * f->v[r][c]);
}

/* ---- gantt ---- */

void gk_gantt_init(gk_gantt *g)
{
    if (g != NULL) {
        memset(g, 0, sizeof(*g));
    }
}

gk_status gk_gantt_add(gk_gantt *g, const char *label, double start,
                       double end, int tool)
{
    gk_gantt_task *t;
    if (g == NULL || g->count >= 128 || end < start) {
        return GK_ERR_INVALID_ARG;
    }
    t = &g->tasks[g->count++];
    memset(t, 0, sizeof(*t));
    gk__copy(t->label, sizeof(t->label), label);
    t->start = start;
    t->end = end;
    t->tool = tool;
    return GK_OK;
}

double gk_gantt_total(const gk_gantt *g)
{
    double lo = 0.0, hi = 0.0;
    int i;
    if (g == NULL || g->count == 0) {
        return 0.0;
    }
    lo = g->tasks[0].start;
    hi = g->tasks[0].end;
    for (i = 1; i < g->count; ++i) {
        if (g->tasks[i].start < lo) lo = g->tasks[i].start;
        if (g->tasks[i].end > hi) hi = g->tasks[i].end;
    }
    return hi - lo;
}

/* ---- timeline ---- */

void gk_timeline_init(gk_timeline *t)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
}

gk_status gk_timeline_add(gk_timeline *t, double time, double load,
                          int alarm_code)
{
    gk_timeline_entry *e;
    if (t == NULL || t->count >= GK_VIZ_MAX_POINTS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    e = &t->entries[t->count++];
    e->time = time;
    e->load = load;
    e->alarm_code = alarm_code;
    if (t->count == 1) {
        t->min_load = t->max_load = load;
    } else {
        if (load < t->min_load) t->min_load = load;
        if (load > t->max_load) t->max_load = load;
    }
    return GK_OK;
}

int gk_timeline_alarm_count(const gk_timeline *t)
{
    int i;
    int n = 0;
    if (t == NULL) {
        return 0;
    }
    for (i = 0; i < t->count; ++i) {
        if (t->entries[i].alarm_code != 0) n++;
    }
    return n;
}

/* ---- process tree ---- */

void gk_process_tree_init(gk_process_tree *p)
{
    if (p != NULL) {
        memset(p, 0, sizeof(*p));
    }
}

gk_status gk_process_add(gk_process_tree *p, int id, const char *name,
                         int parent, double duration)
{
    gk_process_node *n;
    if (p == NULL || p->count >= 64) {
        return GK_ERR_OUT_OF_RANGE;
    }
    n = &p->nodes[p->count++];
    memset(n, 0, sizeof(*n));
    n->id = id;
    gk__copy(n->name, sizeof(n->name), name);
    n->parent = parent;
    n->duration = duration;
    return GK_OK;
}

double gk_process_subtree_duration(const gk_process_tree *p, int id)
{
    double total = 0.0;
    int i;
    const gk_process_node *self = NULL;
    if (p == NULL) {
        return 0.0;
    }
    for (i = 0; i < p->count; ++i) {
        if (p->nodes[i].id == id) {
            self = &p->nodes[i];
            break;
        }
    }
    if (self == NULL) {
        return 0.0;
    }
    total = self->duration;
    for (i = 0; i < p->count; ++i) {
        if (p->nodes[i].parent == id) {
            total += gk_process_subtree_duration(p, p->nodes[i].id);
        }
    }
    return total;
}

/* ---- section ---- */

void gk_section_init(gk_section *s)
{
    if (s != NULL) {
        memset(s, 0, sizeof(*s));
    }
}

gk_status gk_section_add(gk_section *s, double z)
{
    if (s == NULL || s->count >= GK_VIZ_MAX_POINTS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    s->z[s->count++] = z;
    return GK_OK;
}

double gk_section_min(const gk_section *s)
{
    double m = 0.0;
    int i;
    if (s == NULL || s->count == 0) {
        return 0.0;
    }
    m = s->z[0];
    for (i = 1; i < s->count; ++i) {
        if (s->z[i] < m) m = s->z[i];
    }
    return m;
}

double gk_section_depth_range(const gk_section *s)
{
    double lo, hi;
    int i;
    if (s == NULL || s->count == 0) {
        return 0.0;
    }
    lo = hi = s->z[0];
    for (i = 1; i < s->count; ++i) {
        if (s->z[i] < lo) lo = s->z[i];
        if (s->z[i] > hi) hi = s->z[i];
    }
    return hi - lo;
}

/* ---- dual view ---- */

void gk_dual_view_init(gk_dual_view *d, const char *name)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    gk__copy(d->target.name, sizeof(d->target.name), name);
    gk__copy(d->actual.name, sizeof(d->actual.name), name);
}

gk_status gk_dual_view_push(gk_dual_view *d, double target, double actual)
{
    if (d == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (d->target.count >= GK_VIZ_MAX_POINTS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    d->target.samples[d->target.count++] = target;
    d->actual.samples[d->actual.count++] = actual;
    return GK_OK;
}

double gk_dual_view_max_error(const gk_dual_view *d)
{
    double best = 0.0;
    int i;
    if (d == NULL) {
        return 0.0;
    }
    for (i = 0; i < d->target.count && i < d->actual.count; ++i) {
        double e = fabs(d->target.samples[i] - d->actual.samples[i]);
        if (e > best) best = e;
    }
    return best;
}

/* ---- overlay / export ---- */

int gk_viz_overlay(const gk_scope *s, char *buf, size_t len)
{
    int off = 0;
    int c;
    int max = 0;
    if (s == NULL || buf == NULL) {
        return 0;
    }
    buf[0] = '\0';
    off = gk__appendf(buf, len, off, "OVERLAY\n");
    for (c = 0; c < s->count; ++c) {
        if (s->series[c].count > max) {
            max = s->series[c].count;
        }
    }
    for (c = 0; c < max; ++c) {
        int ch;
        for (ch = 0; ch < s->count; ++ch) {
            off = gk__appendf(buf, len, off, "%s%.3f", ch > 0 ? "," : "",
                              ch < s->count && c < s->series[ch].count
                                  ? s->series[ch].samples[c]
                                  : 0.0);
        }
        off = gk__appendf(buf, len, off, "\n");
    }
    return off;
}

int gk_viz_export_chart(const gk_scope *s, char *buf, size_t len)
{
    int off = 0;
    int c;
    if (s == NULL || buf == NULL) {
        return 0;
    }
    buf[0] = '\0';
    off = gk__appendf(buf, len, off, "CHART|channels=%d\n", s->count);
    for (c = 0; c < s->count; ++c) {
        off = gk__appendf(buf, len, off, "%s|peak=%.3f|mean=%.3f|n=%d\n",
                          s->series[c].name, gk_scope_peak(s, c),
                          gk_scope_mean(s, c), s->series[c].count);
    }
    return off;
}
