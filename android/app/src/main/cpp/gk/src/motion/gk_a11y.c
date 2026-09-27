#include "gk/gk_a11y.h"

#include <math.h>
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

const char *gk_a11y_colorblind_name(gk_a11y_colorblind m)
{
    switch (m) {
    case GK_A11Y_CB_NONE: return "none";
    case GK_A11Y_CB_DEUTERANOPIA: return "deuteranopia";
    case GK_A11Y_CB_PROTANOPIA: return "protanopia";
    case GK_A11Y_CB_TRITANOPIA: return "tritanopia";
    default: return "unknown";
    }
}

void gk_a11y_colorblind_map(gk_a11y_colorblind m, unsigned char *r,
                            unsigned char *g, unsigned char *b)
{
    double dr, dg, db;
    if (r == NULL || g == NULL || b == NULL) {
        return;
    }
    dr = (double)*r;
    dg = (double)*g;
    db = (double)*b;
    switch (m) {
    case GK_A11Y_CB_NONE:
        return;
    case GK_A11Y_CB_DEUTERANOPIA:
        /* collapse the green channel toward a blue/yellow axis */
        {
            double nl = 0.625 * dr + 0.375 * dg;
            double nb = 0.70 * db + 0.30 * dg;
            dr = 0.625 * dr + 0.375 * dg;
            dg = nl;
            db = nb;
        }
        break;
    case GK_A11Y_CB_PROTANOPIA:
        dr = 0.567 * dr + 0.433 * dg;
        dg = 0.558 * dg + 0.442 * dr;
        break;
    case GK_A11Y_CB_TRITANOPIA:
        dr = 0.95 * dr + 0.05 * dg;
        db = 0.433 * dg + 0.567 * db;
        break;
    default:
        return;
    }
    if (dr < 0.0) {
        dr = 0.0;
    }
    if (dr > 255.0) {
        dr = 255.0;
    }
    if (dg < 0.0) {
        dg = 0.0;
    }
    if (dg > 255.0) {
        dg = 255.0;
    }
    if (db < 0.0) {
        db = 0.0;
    }
    if (db > 255.0) {
        db = 255.0;
    }
    *r = (unsigned char)(dr + 0.5);
    *g = (unsigned char)(dg + 0.5);
    *b = (unsigned char)(db + 0.5);
}

void gk_a11y_contrast_init(gk_a11y_contrast *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    c->contrast = 100;
}

gk_status gk_a11y_contrast_set(gk_a11y_contrast *c, int contrast_percent)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (contrast_percent < 100 || contrast_percent > 300) {
        return GK_ERR_OUT_OF_RANGE;
    }
    c->contrast = contrast_percent;
    return GK_OK;
}

double gk_a11y_contrast_luminance(const gk_a11y_contrast *c, double input)
{
    double factor;
    double v;
    if (c == NULL) {
        return input;
    }
    factor = (double)c->contrast / 100.0;
    v = (input - 0.5) * factor + 0.5;
    if (c->invert) {
        v = 1.0 - v;
    }
    if (v < 0.0) {
        v = 0.0;
    }
    if (v > 1.0) {
        v = 1.0;
    }
    return v;
}

void gk_a11y_font_init(gk_a11y_font *f)
{
    if (f == NULL) {
        return;
    }
    f->scale = 1.0;
    f->min_size = 8;
    f->max_size = 72;
}

gk_status gk_a11y_font_set_scale(gk_a11y_font *f, double scale)
{
    if (f == NULL || scale <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    if (scale > 4.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    f->scale = scale;
    return GK_OK;
}

int gk_a11y_font_size(const gk_a11y_font *f, int base_size)
{
    int size;
    if (f == NULL) {
        return base_size;
    }
    size = (int)((double)base_size * f->scale + 0.5);
    if (size < f->min_size) {
        size = f->min_size;
    }
    if (size > f->max_size) {
        size = f->max_size;
    }
    return size;
}

void gk_a11y_voice_init(gk_a11y_voice *v)
{
    if (v == NULL) {
        return;
    }
    memset(v, 0, sizeof(*v));
}

gk_status gk_a11y_voice_add(gk_a11y_voice *v, const char *phrase,
                            const char *action)
{
    if (v == NULL || phrase == NULL || action == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (v->count >= GK_A11Y_MAX_ITEMS) {
        return GK_ERR_OVERFLOW;
    }
    gk__copy(v->cmds[v->count].phrase, sizeof(v->cmds[v->count].phrase),
             phrase);
    gk__copy(v->cmds[v->count].action, sizeof(v->cmds[v->count].action),
             action);
    v->count++;
    return GK_OK;
}

const char *gk_a11y_voice_match(const gk_a11y_voice *v, const char *spoken)
{
    int i;
    if (v == NULL || spoken == NULL) {
        return NULL;
    }
    for (i = 0; i < v->count; i++) {
        if (strcmp(v->cmds[i].phrase, spoken) == 0) {
            return v->cmds[i].action;
        }
    }
    return NULL;
}

void gk_a11y_onehand_init(gk_a11y_onehand *o)
{
    if (o == NULL) {
        return;
    }
    memset(o, 0, sizeof(*o));
    o->grip_center_x = 0.5;
    o->grip_center_y = 0.5;
}

gk_status gk_a11y_onehand_enable(gk_a11y_onehand *o, int left_handed)
{
    if (o == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    o->left_handed = left_handed ? 1 : 0;
    o->mirrored = o->left_handed;
    o->grip_center_x = o->left_handed ? 0.25 : 0.75;
    return GK_OK;
}

gk_status gk_a11y_onehand_remap(const gk_a11y_onehand *o, double x, double y,
                                double *out_x, double *out_y)
{
    double rx, ry;
    if (o == NULL || out_x == NULL || out_y == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    rx = x;
    ry = y;
    if (o->mirrored) {
        rx = 1.0 - rx;
    }
    /* compress toward the grip centre so the control stays reachable */
    rx = o->grip_center_x + (rx - 0.5) * 0.8;
    ry = o->grip_center_y + (ry - 0.5) * 0.8;
    if (rx < 0.0) {
        rx = 0.0;
    }
    if (rx > 1.0) {
        rx = 1.0;
    }
    if (ry < 0.0) {
        ry = 0.0;
    }
    if (ry > 1.0) {
        ry = 1.0;
    }
    *out_x = rx;
    *out_y = ry;
    return GK_OK;
}

void gk_a11y_subtitles_init(gk_a11y_subtitles *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->enabled = 1;
}

gk_status gk_a11y_subtitles_add(gk_a11y_subtitles *s, const char *text,
                                double start_s, double end_s)
{
    if (s == NULL || text == NULL || end_s <= start_s) {
        return GK_ERR_INVALID_ARG;
    }
    if (s->count >= GK_A11Y_MAX_ITEMS) {
        return GK_ERR_OVERFLOW;
    }
    gk__copy(s->lines[s->count].text, sizeof(s->lines[s->count].text), text);
    s->lines[s->count].start_s = start_s;
    s->lines[s->count].end_s = end_s;
    s->count++;
    return GK_OK;
}

const char *gk_a11y_subtitles_at(const gk_a11y_subtitles *s, double t)
{
    int i;
    if (s == NULL || !s->enabled) {
        return NULL;
    }
    for (i = 0; i < s->count; i++) {
        if (t >= s->lines[i].start_s && t < s->lines[i].end_s) {
            return s->lines[i].text;
        }
    }
    return NULL;
}

void gk_a11y_slowmo_init(gk_a11y_slowmo *m)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    m->time_scale = 1.0;
}

gk_status gk_a11y_slowmo_set(gk_a11y_slowmo *m, double scale)
{
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (scale <= 0.0 || scale > 1.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    m->time_scale = scale;
    m->enabled = scale < 1.0;
    return GK_OK;
}

double gk_a11y_slowmo_apply(const gk_a11y_slowmo *m, double delta_s)
{
    if (m == NULL) {
        return delta_s;
    }
    return delta_s * m->time_scale;
}
