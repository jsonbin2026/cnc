#include "gk/gk_procd.h"

#include <math.h>
#include <string.h>

static void gk__procd_copy(char *dst, size_t cap, const char *src)
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

/* ===================================================================
 * Route (1441-1443)
 * =================================================================== */

void gk_procd_route_init(gk_procd_route *r, const char *part)
{
    if (r == NULL) {
        return;
    }
    memset(r, 0, sizeof(*r));
    gk__procd_copy(r->part, sizeof(r->part), part);
}

gk_status gk_procd_route_add(gk_procd_route *r, const char *name, int operation,
                             int step)
{
    if (r == NULL || name == NULL || operation < 0 || step < 0) {
        return GK_ERR_INVALID_ARG;
    }
    if (r->count >= GK_PROCD_MAX) {
        return GK_ERR_OVERFLOW;
    }
    gk__procd_copy(r->ops[r->count].name, sizeof(r->ops[r->count].name), name);
    r->ops[r->count].operation = operation;
    r->ops[r->count].step = step;
    r->count++;
    return GK_OK;
}

int gk_procd_route_count(const gk_procd_route *r)
{
    if (r == NULL) {
        return 0;
    }
    return r->count;
}

int gk_procd_route_operations(const gk_procd_route *r)
{
    int i, maxop = 0;
    if (r == NULL) {
        return 0;
    }
    for (i = 0; i < r->count; i++) {
        if (r->ops[i].operation > maxop) {
            maxop = r->ops[i].operation;
        }
    }
    return maxop;
}

/* ===================================================================
 * Allowance / cutting (1444-1445)
 * =================================================================== */

void gk_procd_allowance_init(gk_procd_allowance *a, double stock_mm,
                             double finish_mm)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    a->stock_mm = stock_mm;
    a->finish_mm = finish_mm;
}

double gk_procd_allowance_rough(const gk_procd_allowance *a)
{
    if (a == NULL) {
        return 0.0;
    }
    return a->stock_mm - a->finish_mm - a->semi_mm;
}

int gk_procd_allowance_valid(const gk_procd_allowance *a)
{
    if (a == NULL) {
        return 0;
    }
    return a->stock_mm >= a->finish_mm + a->semi_mm && a->finish_mm >= 0.0;
}

void gk_procd_cutting_init(gk_procd_cutting *c, double speed, double feed,
                           double depth)
{
    if (c == NULL) {
        return;
    }
    c->speed = speed;
    c->feed = feed;
    c->depth = depth;
}

double gk_procd_cutting_time(const gk_procd_cutting *c, double length_mm)
{
    if (c == NULL || c->feed <= 0.0) {
        return 0.0;
    }
    /* minutes = length / feed (mm/min) */
    return length_mm / c->feed;
}

/* ===================================================================
 * Selection (1446-1449)
 * =================================================================== */

const char *gk_procd_selection_name(gk_procd_selection s)
{
    switch (s) {
    case GK_PROCD_SEL_TOOL: return "tool";
    case GK_PROCD_SEL_FIXTURE: return "fixture";
    case GK_PROCD_SEL_GAUGE: return "gauge";
    case GK_PROCD_SEL_MACHINE: return "machine";
    default: return "unknown";
    }
}

void gk_procd_choice_init(gk_procd_choice *c, gk_procd_selection k,
                          const char *pick, const char *reason)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    c->kind = k;
    gk__procd_copy(c->pick, sizeof(c->pick), pick);
    gk__procd_copy(c->reason, sizeof(c->reason), reason);
}

int gk_procd_choice_ok(const gk_procd_choice *c)
{
    if (c == NULL) {
        return 0;
    }
    return c->pick[0] != '\0';
}

/* ===================================================================
 * Quotas (1450-1451)
 * =================================================================== */

void gk_procd_time_quota_init(gk_procd_time_quota *q, double setup_min,
                              double cycle_min, int quantity)
{
    if (q == NULL) {
        return;
    }
    memset(q, 0, sizeof(*q));
    q->setup_min = setup_min;
    q->cycle_min = cycle_min;
    q->quantity = quantity;
}

double gk_procd_time_quota_total(const gk_procd_time_quota *q)
{
    if (q == NULL) {
        return 0.0;
    }
    return q->setup_min + q->cycle_min * (double)q->quantity;
}

double gk_procd_time_quota_per_part(const gk_procd_time_quota *q)
{
    if (q == NULL || q->quantity <= 0) {
        return 0.0;
    }
    return gk_procd_time_quota_total(q) / (double)q->quantity;
}

void gk_procd_material_quota_init(gk_procd_material_quota *q,
                                  double finished_mass_kg, double scrap_rate)
{
    if (q == NULL) {
        return;
    }
    memset(q, 0, sizeof(*q));
    q->finished_mass_kg = finished_mass_kg;
    q->scrap_rate = scrap_rate;
}

double gk_procd_material_quota_required(const gk_procd_material_quota *q,
                                        int quantity)
{
    if (q == NULL || q->scrap_rate >= 1.0) {
        return 0.0;
    }
    return q->finished_mass_kg * (double)quantity / (1.0 - q->scrap_rate);
}

/* ===================================================================
 * Plan stages (1452-1458)
 * =================================================================== */

const char *gk_procd_stage_name(gk_procd_stage s)
{
    switch (s) {
    case GK_PROCD_STAGE_CARD: return "process-card";
    case GK_PROCD_STAGE_REVIEW: return "review";
    case GK_PROCD_STAGE_VALIDATE: return "validate";
    case GK_PROCD_STAGE_OPTIMIZE: return "optimize";
    case GK_PROCD_STAGE_FREEZE: return "freeze";
    case GK_PROCD_STAGE_CHANGE: return "change";
    case GK_PROCD_STAGE_VERSION: return "version";
    case GK_PROCD_STAGE_KNOWLEDGE: return "knowledge-base";
    case GK_PROCD_STAGE_EXPERT: return "expert-system";
    default: return "unknown";
    }
}

void gk_procd_plan_init(gk_procd_plan *p, const char *name)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    gk__procd_copy(p->name, sizeof(p->name), name);
    p->stage = GK_PROCD_STAGE_CARD;
    p->revision = 1;
}

gk_status gk_procd_plan_advance(gk_procd_plan *p, gk_procd_stage stage)
{
    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (p->closed) {
        return GK_ERR_STATE;
    }
    if ((int)stage <= (int)p->stage) {
        return GK_ERR_STATE;
    }
    p->stage = stage;
    return GK_OK;
}

gk_status gk_procd_plan_revise(gk_procd_plan *p, const char *note)
{
    if (p == NULL || note == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    p->revision++;
    gk__procd_copy(p->note, sizeof(p->note), note);
    return GK_OK;
}

gk_status gk_procd_plan_close(gk_procd_plan *p)
{
    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    p->closed = 1;
    return GK_OK;
}

int gk_procd_plan_frozen(const gk_procd_plan *p)
{
    if (p == NULL) {
        return 0;
    }
    return p->closed || (int)p->stage >= (int)GK_PROCD_STAGE_FREEZE;
}

/* ===================================================================
 * Knowledge / expert (1459-1460)
 * =================================================================== */

void gk_procd_knowledge_init(gk_procd_knowledge *k)
{
    if (k == NULL) {
        return;
    }
    memset(k, 0, sizeof(*k));
}

gk_status gk_procd_knowledge_add(gk_procd_knowledge *k, const char *rule)
{
    if (k == NULL || rule == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (k->count >= GK_PROCD_MAX) {
        return GK_ERR_OVERFLOW;
    }
    gk__procd_copy(k->rule[k->count], GK_PROCD_TEXT, rule);
    k->count++;
    return GK_OK;
}

int gk_procd_knowledge_count(const gk_procd_knowledge *k)
{
    if (k == NULL) {
        return 0;
    }
    return k->count;
}

const char *gk_procd_expert_recommend(const gk_procd_knowledge *k,
                                      const char *topic)
{
    int i;
    if (k == NULL || topic == NULL) {
        return NULL;
    }
    for (i = 0; i < k->count; i++) {
        if (strstr(k->rule[i], topic) != NULL) {
            return k->rule[i];
        }
    }
    return NULL;
}
