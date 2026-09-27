#include "gk/gk_cost.h"

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

const char *gk_power_name(gk_power_kind k)
{
    switch (k) {
    case GK_POWER_SPINDLE: return "spindle";
    case GK_POWER_SERVO: return "servo";
    case GK_POWER_COOLANT: return "coolant";
    case GK_POWER_LIGHTING: return "lighting";
    case GK_POWER_AUXILIARY: return "auxiliary";
    default: return "unknown";
    }
}

/* ---- power meter ---- */

void gk_power_meter_init(gk_power_meter *m, double tariff)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    m->tariff = tariff;
}

static gk_power_load *gk__find(gk_power_meter *m, gk_power_kind kind)
{
    int i;
    if (m == NULL) {
        return NULL;
    }
    for (i = 0; i < m->count; ++i) {
        if (m->loads[i].kind == kind) {
            return &m->loads[i];
        }
    }
    return NULL;
}

gk_status gk_power_add(gk_power_meter *m, gk_power_kind kind, const char *name)
{
    gk_power_load *l;
    if (m == NULL || kind < 0 || kind >= GK_POWER_COUNT) {
        return GK_ERR_INVALID_ARG;
    }
    if (m->count >= GK_POWER_COUNT) {
        return GK_ERR_OUT_OF_RANGE;
    }
    l = &m->loads[m->count++];
    memset(l, 0, sizeof(*l));
    l->kind = kind;
    gk__copy(l->name, sizeof(l->name),
             name != NULL ? name : gk_power_name(kind));
    return GK_OK;
}

gk_status gk_power_set(gk_power_meter *m, gk_power_kind kind, double kw)
{
    gk_power_load *l = gk__find(m, kind);
    if (l == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    if (kw < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    l->kw = kw;
    return GK_OK;
}

gk_status gk_power_accumulate(gk_power_meter *m, double dt_seconds)
{
    int i;
    if (m == NULL || dt_seconds < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < m->count; ++i) {
        double hours = dt_seconds / 3600.0;
        m->loads[i].hours += hours;
        m->total_kwh += m->loads[i].kw * hours;
    }
    return GK_OK;
}

double gk_power_total_kw(const gk_power_meter *m)
{
    double sum = 0.0;
    int i;
    if (m == NULL) {
        return 0.0;
    }
    for (i = 0; i < m->count; ++i) {
        sum += m->loads[i].kw;
    }
    return sum;
}

double gk_power_kind_kw(const gk_power_meter *m, gk_power_kind kind)
{
    const gk_power_meter *mm = m;
    int i;
    if (mm == NULL) {
        return 0.0;
    }
    for (i = 0; i < mm->count; ++i) {
        if (mm->loads[i].kind == kind) {
            return mm->loads[i].kw;
        }
    }
    return 0.0;
}

/* ---- electricity cost ---- */

double gk_electricity_cost(const gk_power_meter *m)
{
    if (m == NULL) {
        return 0.0;
    }
    return m->total_kwh * m->tariff;
}

/* ---- tool cost ---- */

void gk_tool_cost_init(gk_tool_cost *t, double price, double life_minutes)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->price_per_tool = price;
    t->tool_life_minutes = life_minutes;
}

double gk_tool_cost_amount(const gk_tool_cost *t)
{
    if (t == NULL || t->tool_life_minutes <= 0.0) {
        return 0.0;
    }
    return t->price_per_tool * t->tool_usage_minutes / t->tool_life_minutes;
}

/* ---- material cost ---- */

void gk_material_cost_init(gk_material_cost *m, double price_per_kg,
                           double mass_kg, double scrap_rate)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    m->price_per_kg = price_per_kg;
    m->mass_kg = mass_kg;
    m->scrap_rate = scrap_rate;
}

double gk_material_cost_amount(const gk_material_cost *m)
{
    if (m == NULL) {
        return 0.0;
    }
    return m->price_per_kg * m->mass_kg * (1.0 + m->scrap_rate);
}

/* ---- labor cost ---- */

void gk_labor_cost_init(gk_labor_cost *l, double hourly_rate, double hours,
                        double overhead_rate)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
    l->hourly_rate = hourly_rate;
    l->hours = hours;
    l->overhead_rate = overhead_rate;
}

double gk_labor_cost_amount(const gk_labor_cost *l)
{
    if (l == NULL) {
        return 0.0;
    }
    return (l->hourly_rate + l->overhead_rate) * l->hours;
}

/* ---- breakdown ---- */

void gk_cost_breakdown_init(gk_cost_breakdown *c)
{
    if (c) {
        memset(c, 0, sizeof(*c));
    }
}

double gk_cost_machine_amount(const gk_cost_breakdown *c)
{
    if (c == NULL) {
        return 0.0;
    }
    return c->machine_rate * c->machine_hours;
}

double gk_cost_total(const gk_cost_breakdown *c)
{
    if (c == NULL) {
        return 0.0;
    }
    return gk_tool_cost_amount(&c->tool) +
           gk_material_cost_amount(&c->material) +
           gk_labor_cost_amount(&c->labor) +
           gk_cost_machine_amount(c) +
           c->electricity;
}

/* ---- carbon ---- */

void gk_carbon_factors_init(gk_carbon_factors *f, double grid, double material,
                            double tool)
{
    if (f == NULL) {
        return;
    }
    memset(f, 0, sizeof(*f));
    f->grid_factor = grid;
    f->material_factor = material;
    f->tool_factor = tool;
}

double gk_carbon_footprint(const gk_carbon_factors *f,
                           const gk_cost_breakdown *c, const gk_power_meter *m)
{
    double kwh = 0.0;
    double tools = 0.0;
    if (f == NULL) {
        return 0.0;
    }
    if (m != NULL) {
        kwh = m->total_kwh;
    }
    if (c != NULL && c->tool.tool_life_minutes > 0.0) {
        tools = c->tool.tool_usage_minutes / c->tool.tool_life_minutes;
    }
    return f->grid_factor * kwh +
           f->material_factor * (c != NULL ? c->material.mass_kg *
                                             (1.0 + c->material.scrap_rate)
                                           : 0.0) +
           f->tool_factor * tools;
}

/* ---- energy advisor ---- */

void gk_energy_advisor_init(gk_energy_advisor *a)
{
    if (a != NULL) {
        memset(a, 0, sizeof(*a));
    }
}

static void gk__tip(gk_energy_advisor *a, const char *text, double percent,
                    double amount)
{
    gk_energy_tip *t;
    if (a == NULL || a->count >= GK_COST_MAX_ITEMS) {
        return;
    }
    t = &a->tips[a->count++];
    gk__copy(t->text, sizeof(t->text), text);
    t->saving_percent = percent;
    t->saving_amount = amount;
}

int gk_energy_advise(gk_energy_advisor *a, const gk_cost_breakdown *c,
                     const gk_power_meter *m)
{
    double total_kw;
    if (a == NULL || m == NULL) {
        return 0;
    }
    gk_energy_advisor_init(a);
    total_kw = gk_power_total_kw(m);
    if (total_kw <= 0.0) {
        return 0;
    }
    if (gk_power_kind_kw(m, GK_POWER_COOLANT) > 0.2 * total_kw) {
        gk__tip(a, "Reduce coolant pump speed when idle",
                0.15, gk_electricity_cost(m) * 0.15);
    }
    if (gk_power_kind_kw(m, GK_POWER_LIGHTING) > 0.1 * total_kw) {
        gk__tip(a, "Switch to LED lighting",
                0.08, gk_electricity_cost(m) * 0.08);
    }
    if (gk_power_kind_kw(m, GK_POWER_AUXILIARY) > 0.2 * total_kw) {
        gk__tip(a, "Power down auxiliary systems between cycles",
                0.10, gk_electricity_cost(m) * 0.10);
    }
    if (c != NULL && c->machine_hours > 8.0) {
        gk__tip(a, "Schedule long runs during off-peak tariff",
                0.05, gk_electricity_cost(m) * 0.05);
    }
    if (a->count == 0) {
        gk__tip(a, "Energy usage already efficient", 0.0, 0.0);
    }
    return a->count;
}

double gk_energy_total_saving(const gk_energy_advisor *a)
{
    double sum = 0.0;
    int i;
    if (a == NULL) {
        return 0.0;
    }
    for (i = 0; i < a->count; ++i) {
        sum += a->tips[i].saving_amount;
    }
    return sum;
}

/* ---- cost book ---- */

void gk_cost_book_init(gk_cost_book *b)
{
    if (b != NULL) {
        memset(b, 0, sizeof(*b));
    }
}

int gk_cost_book_add(gk_cost_book *b, const char *part,
                     const gk_cost_breakdown *c)
{
    gk_cost_record *r;
    if (b == NULL || part == NULL || c == NULL ||
        b->count >= GK_COST_MAX_ITEMS) {
        return -1;
    }
    r = &b->records[b->count];
    gk__copy(r->part, sizeof(r->part), part);
    r->breakdown = *c;
    b->count++;
    return b->count;
}

int gk_cost_report(const gk_cost_book *b, char *buf, size_t len)
{
    int i;
    int n = 0;
    if (b == NULL || buf == NULL || len == 0) {
        return 0;
    }
    n += snprintf(buf + n, len - (size_t)n, "COST REPORT|items=%d\n",
                  b->count);
    for (i = 0; i < b->count && (size_t)n < len; ++i) {
        n += snprintf(buf + n, len - (size_t)n, "%s|tool=%.2f|material=%.2f|"
                      "labor=%.2f|machine=%.2f|electricity=%.2f|total=%.2f\n",
                      b->records[i].part,
                      gk_tool_cost_amount(&b->records[i].breakdown.tool),
                      gk_material_cost_amount(&b->records[i].breakdown.material),
                      gk_labor_cost_amount(&b->records[i].breakdown.labor),
                      gk_cost_machine_amount(&b->records[i].breakdown),
                      b->records[i].breakdown.electricity,
                      gk_cost_total(&b->records[i].breakdown));
    }
    return n;
}

static const gk_cost_record *gk__record(const gk_cost_book *b,
                                        const char *part)
{
    int i;
    if (b == NULL || part == NULL) {
        return NULL;
    }
    for (i = 0; i < b->count; ++i) {
        if (strcmp(b->records[i].part, part) == 0) {
            return &b->records[i];
        }
    }
    return NULL;
}

double gk_cost_compare(const gk_cost_book *b, const char *part_a,
                       const char *part_b)
{
    const gk_cost_record *ra = gk__record(b, part_a);
    const gk_cost_record *rb = gk__record(b, part_b);
    if (ra == NULL || rb == NULL) {
        return 0.0;
    }
    return gk_cost_total(&ra->breakdown) - gk_cost_total(&rb->breakdown);
}

double gk_cost_predict(const gk_cost_record *r, int n_units)
{
    double unit_cost;
    if (r == NULL || n_units < 0) {
        return 0.0;
    }
    unit_cost = gk_cost_total(&r->breakdown);
    return unit_cost * (double)n_units;
}
