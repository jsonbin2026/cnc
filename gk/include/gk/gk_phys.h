#ifndef GK_PHYS_H
#define GK_PHYS_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_PHYS_MAX_GRID 64
#define GK_PHYS_MAX_FIELDS 8
#define GK_PHYS_MAX_HISTORY 256
#define GK_PHYS_NAME 32

/* ===================================================================
 * Part A: time-scale drift & ageing (801-805)
 * =================================================================== */

/* 801 daily drift: linear drift plus small daily noise */
typedef struct {
    double drift_per_day;
    double noise_sigma;
    double reference;
    double elapsed_days;
} gk_phys_daily_drift;

void gk_phys_daily_init(gk_phys_daily_drift *d, double drift_per_day,
                        double noise_sigma);
double gk_phys_daily_advance(gk_phys_daily_drift *d, double days,
                             double noise);

/* 802 monthly tool wear */
typedef struct {
    double wear_per_part;      /* VB growth per part */
    double parts_per_month;
    double vb_um;
    double vb_limit_um;
    int months;
} gk_phys_monthly_wear;

void gk_phys_monthly_wear_init(gk_phys_monthly_wear *w,
                               double wear_per_part, double vb_limit_um);
gk_status gk_phys_monthly_wear_run(gk_phys_monthly_wear *w, int months);
int gk_phys_monthly_wear_expired(const gk_phys_monthly_wear *w);

/* 803 yearly machine ageing: reliability decays exponentially */
typedef struct {
    double mtbf_hours;         /* initial mean time between failures */
    double ageing_rate;        /* per year */
    double age_years;
    double reliability;        /* 0..1 at the current age */
} gk_phys_ageing;

void gk_phys_ageing_init(gk_phys_ageing *a, double mtbf_hours,
                         double ageing_rate);
double gk_phys_ageing_reliability(const gk_phys_ageing *a, double hours);
gk_status gk_phys_ageing_advance(gk_phys_ageing *a, double years);

/* 804 ten-year life: cumulative production and remaining life */
typedef struct {
    double design_life_years;
    double annual_hours;
    double age_years;
    double hours_used;
} gk_phys_life;

void gk_phys_life_init(gk_phys_life *l, double design_life_years,
                       double annual_hours);
gk_status gk_phys_life_advance(gk_phys_life *l, double years);
double gk_phys_life_remaining_years(const gk_phys_life *l);
int gk_phys_life_expired(const gk_phys_life *l);

/* 805 machining history replay */
typedef struct {
    double time_s;
    int tool_id;
    double x;
    double y;
    double z;
    double load;
} gk_phys_history_frame;

typedef struct {
    gk_phys_history_frame frames[GK_PHYS_MAX_HISTORY];
    int count;
    int cursor;
} gk_phys_history;

void gk_phys_history_init(gk_phys_history *h);
int gk_phys_history_record(gk_phys_history *h, const gk_phys_history_frame *f);
const gk_phys_history_frame *gk_phys_history_next(gk_phys_history *h);
gk_status gk_phys_history_seek(gk_phys_history *h, int index);
double gk_phys_history_total_time(const gk_phys_history *h);

/* ===================================================================
 * Part B: multi-physics fields (806-819)
 * =================================================================== */

typedef enum {
    GK_PHYS_FORCE = 0,      /* 806 force field */
    GK_PHYS_THERMAL,        /* 807 thermal field */
    GK_PHYS_MAGNETIC,       /* 808 magnetic field */
    GK_PHYS_ELECTRIC,       /* 809 electric field */
    GK_PHYS_ACOUSTIC,       /* 810 acoustic field */
    GK_PHYS_FLOW,           /* 811 CFD flow field */
    GK_PHYS_OPTICAL,        /* 812 optical field */
    GK_PHYS_CHEMICAL,       /* 813 chemical field */
    GK_PHYS_RADIATION,      /* 814 radiation field */
    GK_PHYS_FIELD_COUNT
} gk_phys_field_kind;

const char *gk_phys_field_name(gk_phys_field_kind k);

typedef struct {
    gk_phys_field_kind kind;
    int rows;
    int cols;
    double data[GK_PHYS_MAX_GRID * GK_PHYS_MAX_GRID];
    double min_value;
    double max_value;
    int visible;
} gk_phys_field;

gk_status gk_phys_field_init(gk_phys_field *f, gk_phys_field_kind kind,
                             int rows, int cols);
gk_status gk_phys_field_set(gk_phys_field *f, int r, int c, double value);
double gk_phys_field_get(const gk_phys_field *f, int r, int c);
/* fill the field with a Gaussian source of given amplitude and sigma */
gk_status gk_phys_field_source(gk_phys_field *f, double cx, double cy,
                               double amplitude, double sigma);
gk_status gk_phys_field_update_stats(gk_phys_field *f);
double gk_phys_field_mean(const gk_phys_field *f);

/* 815-819 multi-field coupling */
typedef struct {
    gk_phys_field fields[GK_PHYS_MAX_FIELDS];
    int count;
    double coupling[GK_PHYS_MAX_FIELDS][GK_PHYS_MAX_FIELDS];
    double strength;   /* global coupling strength multiplier */
} gk_phys_coupling;

void gk_phys_coupling_init(gk_phys_coupling *c);
int gk_phys_coupling_add(gk_phys_coupling *c, const gk_phys_field *f);
gk_status gk_phys_coupling_set(gk_phys_coupling *c, int from, int to,
                               double coefficient);
gk_status gk_phys_coupling_set_strength(gk_phys_coupling *c, double strength);
/* 816: show a single field (hide the rest) */
gk_status gk_phys_coupling_show_single(gk_phys_coupling *c, int index);
/* 817: overlay all fields */
gk_status gk_phys_coupling_overlay_all(gk_phys_coupling *c);
/* 815: propagate one coupling step, returning the coupled source magnitude */
double gk_phys_coupling_step(gk_phys_coupling *c, int from, int to);
/* 819: dominant field index by mean magnitude, or -1 */
int gk_phys_coupling_dominant(const gk_phys_coupling *c);

#ifdef __cplusplus
}
#endif

#endif /* GK_PHYS_H */
