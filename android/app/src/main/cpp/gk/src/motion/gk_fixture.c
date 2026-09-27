#include "gk/gk_fixture.h"

#include <math.h>
#include <string.h>

static void gk__fixture_copy(char *dst, size_t cap, const char *src)
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

const char *gk_fixture_name(gk_fixture_kind k)
{
    switch (k) {
    case GK_FIX_VISE: return "bench-vise";
    case GK_FIX_PARALLEL_VISE: return "parallel-vise";
    case GK_FIX_3JAW_CHUCK: return "3-jaw-chuck";
    case GK_FIX_4JAW_CHUCK: return "4-jaw-chuck";
    case GK_FIX_COLLET: return "spring-collet";
    case GK_FIX_HYDRAULIC: return "hydraulic";
    case GK_FIX_PNEUMATIC: return "pneumatic";
    case GK_FIX_MAGNETIC: return "magnetic";
    case GK_FIX_VACUUM: return "vacuum-chuck";
    case GK_FIX_SPECIAL: return "special";
    case GK_FIX_MODULAR: return "modular";
    case GK_FIX_FLEXIBLE: return "flexible";
    case GK_FIX_ZERO_POINT: return "zero-point";
    case GK_FIX_QUICK_CHANGE: return "quick-change";
    default: return "unknown";
    }
}

double gk_fixture_max_clamp_force(gk_fixture_kind k)
{
    switch (k) {
    case GK_FIX_VISE: return 20000.0;
    case GK_FIX_PARALLEL_VISE: return 30000.0;
    case GK_FIX_3JAW_CHUCK: return 40000.0;
    case GK_FIX_4JAW_CHUCK: return 35000.0;
    case GK_FIX_COLLET: return 15000.0;
    case GK_FIX_HYDRAULIC: return 60000.0;
    case GK_FIX_PNEUMATIC: return 25000.0;
    case GK_FIX_MAGNETIC: return 12000.0;
    case GK_FIX_VACUUM: return 8000.0;
    case GK_FIX_SPECIAL: return 50000.0;
    case GK_FIX_MODULAR: return 18000.0;
    case GK_FIX_FLEXIBLE: return 22000.0;
    case GK_FIX_ZERO_POINT: return 45000.0;
    case GK_FIX_QUICK_CHANGE: return 28000.0;
    default: return 0.0;
    }
}

gk_status gk_fixture_init(gk_fixture *f, gk_fixture_kind k)
{
    if (f == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    memset(f, 0, sizeof(*f));
    f->kind = k;
    f->repeatability_mm = 0.01;
    return GK_OK;
}

gk_status gk_fixture_clamp(gk_fixture *f, double force)
{
    double maxf;
    if (f == NULL || force <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    maxf = gk_fixture_max_clamp_force(f->kind);
    if (maxf <= 0.0) {
        return GK_ERR_UNSUPPORTED;
    }
    if (force > maxf) {
        return GK_ERR_OUT_OF_RANGE;
    }
    f->clamp_force = force;
    f->clamped = 1;
    return GK_OK;
}

gk_status gk_fixture_unclamp(gk_fixture *f)
{
    if (f == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    f->clamped = 0;
    f->clamp_force = 0.0;
    return GK_OK;
}

int gk_fixture_secure(const gk_fixture *f, double part_weight_n)
{
    if (f == NULL || !f->clamped) {
        return 0;
    }
    /* require safety factor 3 against part weight */
    return f->clamp_force >= 3.0 * part_weight_n;
}

/* ===================================================================
 * Lifecycle (1365-1368)
 * =================================================================== */

void gk_fixture_lifecycle_init(gk_fixture_lifecycle *l, const char *name,
                               double tolerance_mm)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
    gk__fixture_copy(l->name, sizeof(l->name), name);
    l->tolerance_mm = tolerance_mm;
}

gk_status gk_fixture_design(gk_fixture_lifecycle *l)
{
    if (l == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (l->tolerance_mm <= 0.0) {
        return GK_ERR_STATE;
    }
    l->designed = 1;
    return GK_OK;
}

gk_status gk_fixture_manufacture(gk_fixture_lifecycle *l)
{
    if (l == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!l->designed) {
        return GK_ERR_STATE;
    }
    l->manufactured = 1;
    return GK_OK;
}

gk_status gk_fixture_commission(gk_fixture_lifecycle *l, double measured_err)
{
    if (l == NULL || measured_err < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    if (!l->manufactured) {
        return GK_ERR_STATE;
    }
    if (measured_err > l->tolerance_mm) {
        return GK_ERR_OUT_OF_RANGE;
    }
    l->commissioned = 1;
    return GK_OK;
}

gk_status gk_fixture_maintain(gk_fixture_lifecycle *l)
{
    if (l == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!l->commissioned) {
        return GK_ERR_STATE;
    }
    l->maintained = 1;
    return GK_OK;
}

int gk_fixture_ready(const gk_fixture_lifecycle *l)
{
    if (l == NULL) {
        return 0;
    }
    return l->designed && l->manufactured && l->commissioned;
}

/* ===================================================================
 * Inventory (1369-1370)
 * =================================================================== */

void gk_fixture_stock_init(gk_fixture_stock *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
}

gk_status gk_fixture_stock_add(gk_fixture_stock *s, const char *id,
                               gk_fixture_kind k, double cost, int qty)
{
    int i;
    if (s == NULL || id == NULL || cost < 0.0 || qty < 0) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < s->count; i++) {
        if (strcmp(s->items[i].id, id) == 0) {
            s->items[i].quantity += qty;
            return GK_OK;
        }
    }
    if (s->count >= GK_FIX_MAX) {
        return GK_ERR_OVERFLOW;
    }
    gk__fixture_copy(s->items[s->count].id, sizeof(s->items[s->count].id), id);
    s->items[s->count].kind = k;
    s->items[s->count].cost = cost;
    s->items[s->count].quantity = qty;
    s->count++;
    return GK_OK;
}

int gk_fixture_stock_total_qty(const gk_fixture_stock *s)
{
    int i, n = 0;
    if (s == NULL) {
        return 0;
    }
    for (i = 0; i < s->count; i++) {
        n += s->items[i].quantity;
    }
    return n;
}

double gk_fixture_stock_total_cost(const gk_fixture_stock *s)
{
    double v = 0.0;
    int i;
    if (s == NULL) {
        return 0.0;
    }
    for (i = 0; i < s->count; i++) {
        v += s->items[i].cost * (double)s->items[i].quantity;
    }
    return v;
}

gk_status gk_fixture_stock_consume(gk_fixture_stock *s, const char *id,
                                   int qty)
{
    int i;
    if (s == NULL || id == NULL || qty < 0) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < s->count; i++) {
        if (strcmp(s->items[i].id, id) == 0) {
            if (s->items[i].quantity < qty) {
                return GK_ERR_OUT_OF_RANGE;
            }
            s->items[i].quantity -= qty;
            return GK_OK;
        }
    }
    return GK_ERR_NOT_FOUND;
}
