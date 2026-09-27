#include "gk/gk_toolsys.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void gk__toolsys_copy(char *dst, size_t cap, const char *src)
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
 * 1331 coding / 1339 RFID
 * =================================================================== */

void gk_toolsys_id_init(gk_toolsys_id *t, const char *code, const char *name)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    gk__toolsys_copy(t->code, sizeof(t->code), code);
    gk__toolsys_copy(t->name, sizeof(t->name), name);
}

gk_status gk_toolsys_set_rfid(gk_toolsys_id *t, const char *rfid)
{
    if (t == NULL || rfid == NULL || rfid[0] == '\0') {
        return GK_ERR_INVALID_ARG;
    }
    gk__toolsys_copy(t->rfid, sizeof(t->rfid), rfid);
    return GK_OK;
}

int gk_toolsys_match_rfid(const gk_toolsys_id *t, const char *rfid)
{
    if (t == NULL || rfid == NULL || t->rfid[0] == '\0') {
        return 0;
    }
    return strcmp(t->rfid, rfid) == 0;
}

/* ===================================================================
 * 1332 presetting
 * =================================================================== */

void gk_toolsys_preset_init(gk_toolsys_preset *p)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
}

gk_status gk_toolsys_preset_set(gk_toolsys_preset *p, double len, double dia)
{
    if (p == NULL || len <= 0.0 || dia <= 0.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    p->preset_length_mm = len;
    p->preset_diameter_mm = dia;
    return GK_OK;
}

double gk_toolsys_preset_len_error(const gk_toolsys_preset *p)
{
    if (p == NULL) {
        return 0.0;
    }
    return fabs(p->measured_length_mm - p->preset_length_mm);
}

double gk_toolsys_preset_dia_error(const gk_toolsys_preset *p)
{
    if (p == NULL) {
        return 0.0;
    }
    return fabs(p->measured_diameter_mm - p->preset_diameter_mm);
}

int gk_toolsys_preset_ok(const gk_toolsys_preset *p, double tol_mm)
{
    if (p == NULL) {
        return 0;
    }
    return gk_toolsys_preset_len_error(p) <= tol_mm &&
           gk_toolsys_preset_dia_error(p) <= tol_mm;
}

/* ===================================================================
 * 1333 assembly / 1334 balance
 * =================================================================== */

void gk_toolsys_assembly_init(gk_toolsys_assembly *a)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    a->balance_grade = 2.5;
}

gk_status gk_toolsys_assembly_add(gk_toolsys_assembly *a,
                                  const char *component)
{
    if (a == NULL || component == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (a->components >= GK_TOOLSYS_MAX) {
        return GK_ERR_OVERFLOW;
    }
    a->components++;
    return GK_OK;
}

int gk_toolsys_balance_ok(const gk_toolsys_assembly *a)
{
    if (a == NULL) {
        return 0;
    }
    return a->balance_grade <= 2.5;
}

/* ===================================================================
 * 1335/1337/1338 condition
 * =================================================================== */

void gk_toolsys_condition_init(gk_toolsys_condition *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
}

gk_status gk_toolsys_set_runout(gk_toolsys_condition *c, double runout_um)
{
    if (c == NULL || runout_um < 0.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    c->runout_um = runout_um;
    return GK_OK;
}

int gk_toolsys_runout_ok(const gk_toolsys_condition *c, double limit_um)
{
    if (c == NULL) {
        return 0;
    }
    return c->runout_um <= limit_um;
}

int gk_toolsys_worn(const gk_toolsys_condition *c, double limit_mm)
{
    if (c == NULL) {
        return 0;
    }
    return c->wear_mm >= limit_mm;
}

gk_status gk_toolsys_mark_broken(gk_toolsys_condition *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    c->broken = 1;
    return GK_OK;
}

/* ===================================================================
 * 1336 life
 * =================================================================== */

void gk_toolsys_life_init(gk_toolsys_life *l, double rated_min)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
    l->rated_min = rated_min;
}

gk_status gk_toolsys_life_use(gk_toolsys_life *l, double minutes)
{
    if (l == NULL || minutes < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    l->used_min += minutes;
    return GK_OK;
}

double gk_toolsys_life_remaining(const gk_toolsys_life *l)
{
    double r;
    if (l == NULL) {
        return 0.0;
    }
    r = l->rated_min - l->used_min;
    return r > 0.0 ? r : 0.0;
}

int gk_toolsys_life_expired(const gk_toolsys_life *l)
{
    if (l == NULL) {
        return 0;
    }
    return l->used_min >= l->rated_min;
}

/* ===================================================================
 * 1340-1346 crib
 * =================================================================== */

const char *gk_toolsys_status_name(gk_toolsys_status s)
{
    switch (s) {
    case GK_TOOLSYS_STATUS_IN_STOCK: return "in-stock";
    case GK_TOOLSYS_STATUS_IN_USE: return "in-use";
    case GK_TOOLSYS_STATUS_RECOVERED: return "recovered";
    case GK_TOOLSYS_STATUS_REGRIND: return "regrind";
    case GK_TOOLSYS_STATUS_COATED: return "coated";
    default: return "unknown";
    }
}

void gk_toolsys_crib_init(gk_toolsys_crib *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
}

gk_status gk_toolsys_crib_add(gk_toolsys_crib *c, const char *id,
                              const char *supplier, double purchase_price)
{
    gk_toolsys_item *it;
    if (c == NULL || id == NULL || supplier == NULL || purchase_price < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    if (c->count >= GK_TOOLSYS_MAX) {
        return GK_ERR_OVERFLOW;
    }
    if (gk_toolsys_crib_find(c, id) != NULL) {
        return GK_ERR_ALREADY_EXISTS;
    }
    it = &c->items[c->count];
    memset(it, 0, sizeof(*it));
    gk__toolsys_copy(it->id, sizeof(it->id), id);
    gk__toolsys_copy(it->supplier, sizeof(it->supplier), supplier);
    it->purchase_price = purchase_price;
    it->cost = purchase_price;
    it->status = GK_TOOLSYS_STATUS_IN_STOCK;
    c->count++;
    return GK_OK;
}

int gk_toolsys_crib_count(const gk_toolsys_crib *c)
{
    if (c == NULL) {
        return 0;
    }
    return c->count;
}

gk_toolsys_item *gk_toolsys_crib_find(gk_toolsys_crib *c, const char *id)
{
    int i;
    if (c == NULL || id == NULL) {
        return NULL;
    }
    for (i = 0; i < c->count; i++) {
        if (strcmp(c->items[i].id, id) == 0) {
            return &c->items[i];
        }
    }
    return NULL;
}

gk_status gk_toolsys_crib_set_status(gk_toolsys_crib *c, const char *id,
                                     gk_toolsys_status s)
{
    gk_toolsys_item *it = gk_toolsys_crib_find(c, id);
    if (it == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    it->status = s;
    return GK_OK;
}

int gk_toolsys_crib_count_status(const gk_toolsys_crib *c,
                                 gk_toolsys_status s)
{
    int i, n = 0;
    if (c == NULL) {
        return 0;
    }
    for (i = 0; i < c->count; i++) {
        if (c->items[i].status == s) {
            n++;
        }
    }
    return n;
}

double gk_toolsys_crib_total_value(const gk_toolsys_crib *c)
{
    double v = 0.0;
    int i;
    if (c == NULL) {
        return 0.0;
    }
    for (i = 0; i < c->count; i++) {
        v += c->items[i].cost;
    }
    return v;
}

/* ===================================================================
 * 1347/1348 order
 * =================================================================== */

void gk_toolsys_order_init(gk_toolsys_order *o, const char *id, int qty,
                           double unit_price, double lead_days,
                           const char *supplier)
{
    if (o == NULL) {
        return;
    }
    memset(o, 0, sizeof(*o));
    gk__toolsys_copy(o->id, sizeof(o->id), id);
    gk__toolsys_copy(o->supplier, sizeof(o->supplier), supplier);
    o->quantity = qty;
    o->unit_price = unit_price;
    o->lead_days = lead_days;
}

double gk_toolsys_order_total(const gk_toolsys_order *o)
{
    if (o == NULL) {
        return 0.0;
    }
    return (double)o->quantity * o->unit_price;
}

/* ===================================================================
 * 1349/1350 trial
 * =================================================================== */

void gk_toolsys_trial_init(gk_toolsys_trial *t)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
}

double gk_toolsys_trial_score(const gk_toolsys_trial *t)
{
    /* higher life and lower Ra/cost are better */
    if (t == NULL) {
        return 0.0;
    }
    return t->tool_life_min / ((t->surface_finish_ra + 0.1) *
                               (t->cost_per_part + 0.01));
}
