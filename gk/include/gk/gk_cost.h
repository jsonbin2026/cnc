#ifndef GK_COST_H
#define GK_COST_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_COST_MAX_ITEMS 64
#define GK_COST_NAME 64
#define GK_COST_REPORT 2048

/* ---- 641 real-time power display ---- */

typedef enum {
    GK_POWER_SPINDLE = 0,   /* 642 */
    GK_POWER_SERVO,         /* 643 */
    GK_POWER_COOLANT,       /* 644 */
    GK_POWER_LIGHTING,      /* 645 */
    GK_POWER_AUXILIARY,
    GK_POWER_COUNT
} gk_power_kind;

const char *gk_power_name(gk_power_kind k);

typedef struct {
    gk_power_kind kind;
    char name[GK_COST_NAME];
    double kw;              /* current power draw */
    double hours;           /* accumulated hours at this power */
} gk_power_load;

typedef struct {
    gk_power_load loads[GK_POWER_COUNT];
    int count;
    double tariff;          /* currency per kWh */
    double total_kwh;
} gk_power_meter;

void gk_power_meter_init(gk_power_meter *m, double tariff);
gk_status gk_power_add(gk_power_meter *m, gk_power_kind kind,
                       const char *name);
gk_status gk_power_set(gk_power_meter *m, gk_power_kind kind, double kw);
/* integrate current loads over dt seconds, accumulating kWh and load hours */
gk_status gk_power_accumulate(gk_power_meter *m, double dt_seconds);
double gk_power_total_kw(const gk_power_meter *m);
double gk_power_kind_kw(const gk_power_meter *m, gk_power_kind kind);

/* ---- 646 electricity cost ---- */

double gk_electricity_cost(const gk_power_meter *m);

/* ---- 647-650 cost components ---- */

typedef struct {
    double price_per_tool;      /* tool cost */
    double tool_life_minutes;
    double tool_usage_minutes;
} gk_tool_cost;

void gk_tool_cost_init(gk_tool_cost *t, double price, double life_minutes);
double gk_tool_cost_amount(const gk_tool_cost *t);

typedef struct {
    double price_per_kg;        /* material cost */
    double mass_kg;
    double scrap_rate;          /* 0..1 */
} gk_material_cost;

void gk_material_cost_init(gk_material_cost *m, double price_per_kg,
                           double mass_kg, double scrap_rate);
double gk_material_cost_amount(const gk_material_cost *m);

typedef struct {
    double hourly_rate;         /* labor cost */
    double hours;
    double overhead_rate;
} gk_labor_cost;

void gk_labor_cost_init(gk_labor_cost *l, double hourly_rate, double hours,
                        double overhead_rate);
double gk_labor_cost_amount(const gk_labor_cost *l);

typedef struct {
    gk_tool_cost tool;
    gk_material_cost material;
    gk_labor_cost labor;
    double electricity;
    double machine_hours;
    double machine_rate;
} gk_cost_breakdown;

void gk_cost_breakdown_init(gk_cost_breakdown *c);
double gk_cost_machine_amount(const gk_cost_breakdown *c);
double gk_cost_total(const gk_cost_breakdown *c);

/* ---- 651 carbon footprint ---- */

typedef struct {
    double grid_factor;         /* kg CO2 per kWh */
    double material_factor;     /* kg CO2 per kg */
    double tool_factor;         /* kg CO2 per tool */
} gk_carbon_factors;

void gk_carbon_factors_init(gk_carbon_factors *f, double grid, double material,
                            double tool);
double gk_carbon_footprint(const gk_carbon_factors *f,
                           const gk_cost_breakdown *c, const gk_power_meter *m);

/* ---- 652 energy-saving advice ---- */

typedef struct {
    char text[GK_COST_NAME];
    double saving_percent;
    double saving_amount;
} gk_energy_tip;

typedef struct {
    gk_energy_tip tips[GK_COST_MAX_ITEMS];
    int count;
} gk_energy_advisor;

void gk_energy_advisor_init(gk_energy_advisor *a);
/* analyse a breakdown and propose savings; returns number of tips */
int gk_energy_advise(gk_energy_advisor *a, const gk_cost_breakdown *c,
                     const gk_power_meter *m);
double gk_energy_total_saving(const gk_energy_advisor *a);

/* ---- 653 report / 654 comparison / 655 prediction ---- */

typedef struct {
    char part[GK_COST_NAME];
    gk_cost_breakdown breakdown;
} gk_cost_record;

typedef struct {
    gk_cost_record records[GK_COST_MAX_ITEMS];
    int count;
} gk_cost_book;

void gk_cost_book_init(gk_cost_book *b);
int gk_cost_book_add(gk_cost_book *b, const char *part,
                     const gk_cost_breakdown *c);
int gk_cost_report(const gk_cost_book *b, char *buf, size_t len);
/* compare two parts by name; returns negative if a is cheaper */
double gk_cost_compare(const gk_cost_book *b, const char *part_a,
                       const char *part_b);
/* linear prediction: project cost after producing n more units */
double gk_cost_predict(const gk_cost_record *r, int n_units);

#ifdef __cplusplus
}
#endif

#endif /* GK_COST_H */
