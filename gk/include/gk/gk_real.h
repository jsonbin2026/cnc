#ifndef GK_REAL_H
#define GK_REAL_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_REAL_MAX_SAMPLES 4096
#define GK_REAL_MAX_EVENTS 64
#define GK_REAL_NAME 64

/* ---- deterministic PRNG (xorshift64*) ---- */

typedef struct {
    unsigned long long state;
} gk_rng;

void gk_rng_seed(gk_rng *r, unsigned long long seed);
unsigned long long gk_rng_next(gk_rng *r);
/* uniform in [lo, hi] */
double gk_rng_uniform(gk_rng *r, double lo, double hi);
/* standard normal via Box-Muller, cached second value */
double gk_rng_normal(gk_rng *r, double mean, double stddev);
int gk_rng_chance(gk_rng *r, double probability);

/* ---- 701 blank size random / 702 hardness random / 703 tool batch ---- */

typedef struct {
    double nominal;
    double sigma;
    double min_limit;
    double max_limit;
} gk_variation;

void gk_variation_init(gk_variation *v, double nominal, double sigma,
                       double min_limit, double max_limit);
double gk_variation_sample(const gk_variation *v, gk_rng *r);

typedef struct {
    gk_variation blank_length;
    gk_variation blank_width;
    gk_variation blank_height;
    gk_variation hardness;
} gk_blank_variation;

void gk_blank_variation_init(gk_blank_variation *b);

/* tool batch: offset & wear factor per tool */
typedef struct {
    double diameter_offset;
    double runout;
    double wear_factor;
    int batch_id;
} gk_tool_batch;

void gk_tool_batch_init(gk_tool_batch *t, int batch_id, gk_rng *r);

/* ---- 704-708 environmental fluctuations ---- */

typedef struct {
    double nominal;
    double amplitude;
    double period;
    double phase;
} gk_fluctuation;

void gk_fluctuation_init(gk_fluctuation *f, double nominal, double amplitude,
                         double period, double phase);
double gk_fluctuation_at(const gk_fluctuation *f, double time);

typedef struct {
    gk_fluctuation grid_voltage;   /* 704 */
    gk_fluctuation air_pressure;   /* 705 */
    gk_fluctuation hydraulic;     /* 706 */
    gk_fluctuation temperature;   /* 707 */
    gk_fluctuation humidity;      /* 708 */
} gk_environment;

void gk_environment_init(gk_environment *e);
double gk_environment_sample(const gk_environment *e, double time,
                             int kind_index);
int gk_environment_kind_count(void);

/* ---- 709 human variance / 710 clamping force ---- */

typedef struct {
    double skill;          /* 0..1 */
    double fatigue;        /* 0..1 */
    double reaction_sigma; /* seconds */
} gk_operator_model;

void gk_operator_model_init(gk_operator_model *o, double skill);
double gk_operator_setting_error(const gk_operator_model *o, gk_rng *r);
double gk_operator_reaction_time(const gk_operator_model *o, gk_rng *r);

typedef struct {
    double nominal_force;
    double sigma;
    double max_force;
} gk_clamp_force;

void gk_clamp_force_init(gk_clamp_force *c, double nominal, double sigma,
                         double max_force);
double gk_clamp_force_sample(const gk_clamp_force *c, gk_rng *r);
int gk_clamp_force_ok(const gk_clamp_force *c, double force);

/* ---- 711 random faults / 712 random events ---- */

typedef enum {
    GK_REAL_FAULT_NONE = 0,
    GK_REAL_FAULT_TOOL_BREAK,
    GK_REAL_FAULT_POWER_LOSS,
    GK_REAL_FAULT_SPINDLE_STALL,
    GK_REAL_FAULT_COOLANT_LOSS,
    GK_REAL_FAULT_CHIP_JAM,
    GK_REAL_FAULT_COUNT
} gk_real_fault;

const char *gk_real_fault_name(gk_real_fault f);

typedef struct {
    gk_real_fault fault;
    double probability_per_hour;
} gk_fault_rule;

typedef struct {
    gk_fault_rule rules[GK_REAL_FAULT_COUNT];
    int count;
} gk_fault_model;

void gk_fault_model_init(gk_fault_model *m, double base_rate);
gk_status gk_fault_model_set(gk_fault_model *m, gk_real_fault f, double rate);
gk_real_fault gk_fault_model_sample(const gk_fault_model *m, gk_rng *r,
                                    double hours);

typedef struct {
    int id;
    char name[GK_REAL_NAME];
    double probability_per_hour;
    double impact;
} gk_random_event;

typedef struct {
    gk_random_event events[GK_REAL_MAX_EVENTS];
    int count;
} gk_event_model;

void gk_event_model_init(gk_event_model *m);
int gk_event_model_add(gk_event_model *m, const char *name, double rate,
                       double impact);
int gk_event_model_sample(const gk_event_model *m, gk_rng *r, double hours,
                          int *out_ids, int max_out);

/* ---- 713 measurement noise / 714 reading error ---- */

typedef struct {
    double sigma;
    double bias;
    double resolution;
} gk_measure_noise;

void gk_measure_noise_init(gk_measure_noise *m, double sigma, double bias,
                           double resolution);
double gk_measure_noise_apply(const gk_measure_noise *m, double value,
                              gk_rng *r);

/* ---- 715 Monte-Carlo / 716 statistics ---- */

typedef double (*gk_real_model_fn)(void *ctx, gk_rng *r);

typedef struct {
    double mean;
    double stddev;
    double minimum;
    double maximum;
    double p05;
    double p50;
    double p95;
    int samples;
} gk_stats;

void gk_stats_init(gk_stats *s);
gk_status gk_stats_add(gk_stats *s, double value);
gk_status gk_stats_finalize(gk_stats *s);
/* run a Monte-Carlo simulation; returns number of samples taken */
int gk_monte_carlo(gk_real_model_fn fn, void *ctx, gk_rng *r, int samples,
                   double *out_values, int max_out, gk_stats *out_stats);

/* ---- 717-730 disruption scenarios ---- */

typedef enum {
    GK_DR_POWER_RESUME = 0,     /* 717 断电续切 */
    GK_DR_TOOL_CHANGE_BROKEN,   /* 718 断刀更换 */
    GK_DR_CRASH_RECOVERY,       /* 719 撞机善后 */
    GK_DR_BATCH_SCRAP,          /* 720 批量废品 */
    GK_DR_CUSTOMER_COMPLAINT,   /* 721 客户投诉 */
    GK_DR_DEADLINE_SQUEEZE,     /* 722 交期压缩 */
    GK_DR_MACHINE_BREAKDOWN,    /* 723 设备故障 */
    GK_DR_MATERIAL_REJECT,      /* 724 原料不合格 */
    GK_DR_STAFF_SHORTAGE,       /* 725 人手不足 */
    GK_DR_OVERNIGHT_RUSH,       /* 726 连夜赶工 */
    GK_DR_FATIGUE_OPERATION,    /* 727 疲劳操作 */
    GK_DR_SAFETY_ACCIDENT,      /* 728 安全事故 */
    GK_DR_FIRE,                 /* 729 火灾 */
    GK_DR_ELECTRICAL_LEAK,      /* 730 漏电 */
    GK_DR_COUNT
} gk_disruption_kind;

const char *gk_disruption_name(gk_disruption_kind k);

typedef struct {
    gk_disruption_kind kind;
    double severity;        /* 0..1 */
    double downtime_hours;
    double cost_impact;
    int handled;
} gk_disruption;

typedef struct {
    gk_disruption events[GK_REAL_MAX_EVENTS];
    int count;
} gk_disruption_log;

void gk_disruption_log_init(gk_disruption_log *l);
gk_status gk_disruption_add(gk_disruption_log *l, gk_disruption_kind kind,
                            double severity);
gk_status gk_disruption_handle(gk_disruption_log *l, int index);
double gk_disruption_total_cost(const gk_disruption_log *l);
double gk_disruption_total_downtime(const gk_disruption_log *l);

/* 717 power-loss resume: compute safe re-entry move from last position */
int gk_power_resume(gk_disruption_log *l, const double *last_pos,
                    const double *safe_pos, int axes);

/* ---- lifecycle integration helper ---- */

typedef struct {
    gk_rng rng;
    gk_environment environment;
    gk_operator_model operator_model;
    gk_fault_model faults;
    gk_measure_noise noise;
    gk_disruption_log disruptions;
} gk_realism;

void gk_realism_init(gk_realism *r, unsigned long long seed);
/* advance the simulation by dt and fire any random faults/events */
int gk_realism_step(gk_realism *r, double time, double dt);

#ifdef __cplusplus
}
#endif

#endif /* GK_REAL_H */
