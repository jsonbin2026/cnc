#ifndef GK_MEASURE_H
#define GK_MEASURE_H

#include <stddef.h>
#include "gk/gk_error.h"
#include "gk/gk_math.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_MEASURE_MAX_SAMPLES 512

typedef enum {
    GK_GAUGE_CALIPER = 0,      /* 260 virtual caliper */
    GK_GAUGE_MICROMETER,       /* 261 virtual micrometer */
    GK_GAUGE_HEIGHT,           /* 262 virtual height gauge */
    GK_GAUGE_BORE_MICROMETER,  /* 263 virtual bore micrometer */
    GK_GAUGE_CMM,              /* 264 virtual CMM */
    GK_GAUGE_ROUGHNESS,        /* 265 virtual roughness tester */
    GK_GAUGE_PROFILOMETER,     /* 266 virtual profilometer */
    GK_GAUGE_ROUNDNESS,        /* 267 virtual roundness tester */
    GK_GAUGE_VISION,           /* 268 virtual vision measuring machine */
    GK_GAUGE_LASER_SCAN,       /* 269 virtual laser scanner */
    GK_GAUGE_ONLINE_PROBE,     /* 270 online touch probe */
    GK_GAUGE_COUNT
} gk_gauge_kind;

const char *gk_gauge_name(gk_gauge_kind k);

typedef struct {
    gk_gauge_kind kind;
    double resolution;    /* mm */
    double accuracy;      /* +/- mm */
    double range_min;
    double range_max;
    int points;           /* number of sampled points (profilometer etc.) */
} gk_gauge;

gk_status gk_gauge_init(gk_gauge *g, gk_gauge_kind kind);
int gk_gauge_is_in_range(const gk_gauge *g, double reading);
/* Simulate a reading with deterministic rounding to the gauge resolution. */
double gk_gauge_read(const gk_gauge *g, double true_value);

/* ---- measurement result / recording ---- */

typedef struct {
    double nominal;
    double true_value;
    double measured;
    double error;
    double tolerance;
    int out_of_tolerance;
    char label[64];
} gk_measure_result;

/* Evaluate a measurement against a nominal (spec) value.
 * `nominal` is the drawing target, `actual` is the true physical value.
 * The recorded `measured` is the gauge-quantized actual value and `error`
 * is measured - nominal. */
gk_measure_result gk_measure_eval(const gk_gauge *g, double nominal,
                                  double actual, double tolerance,
                                  const char *label);
int gk_measure_out_of_tolerance(const gk_measure_result *r);
int gk_measure_is_conforming(const gk_measure_result *r);

/* ---- auto measurement cycle (271) ---- */

typedef enum {
    GK_CYCLE_IDLE = 0,
    GK_CYCLE_APPROACH,
    GK_CYCLE_PROBE,
    GK_CYCLE_RETRACT,
    GK_CYCLE_RECORD,
    GK_CYCLE_DONE
} gk_measure_cycle_state;

typedef struct {
    gk_measure_cycle_state state;
    double clearance;
    double depth;
    double feed;
    int index;
    int total;
    double elapsed;
} gk_measure_cycle;

void gk_measure_cycle_init(gk_measure_cycle *c, double clearance,
                           double depth, double feed, int total);
gk_measure_cycle_state gk_measure_cycle_step(gk_measure_cycle *c, double dt,
                                             double *out_reading);

/* ---- GD&T tolerance analysis (272) ---- */

typedef struct {
    double nominal;
    double upper_tol;
    double lower_tol;
    int is_positional;    /* true if tolerance is a diameter zone */
} gk_gdt_tolerance;

typedef struct {
    double measured;
    int within;
    double deviation;
    double allowed;
} gk_gdt_result;

gk_gdt_result gk_gdt_check(const gk_gdt_tolerance *t, double measured);
/* Worst-case stack-up of independent tolerances. */
double gk_gdt_worst_case(const double *tol_plus, const double *tol_minus,
                         size_t n);
/* Statistical (RSS) stack-up. */
double gk_gdt_rss(const double *tols, size_t n);

/* ---- SPC / capability ---- */

typedef struct {
    double samples[GK_MEASURE_MAX_SAMPLES];
    size_t count;
    double target;
} gk_sample_set;

void gk_sample_set_init(gk_sample_set *s, double target);
gk_status gk_sample_add(gk_sample_set *s, double value);

double gk_sample_mean(const gk_sample_set *s);
double gk_sample_stddev(const gk_sample_set *s, int sampled);
double gk_sample_min(const gk_sample_set *s);
double gk_sample_max(const gk_sample_set *s);
double gk_sample_range(const gk_sample_set *s);

/* Control limits: mean +/- 3 sigma (SPC chart, item 273). */
void gk_spc_limits(const gk_sample_set *s, double *ucl, double *lcl,
                   double *center);
int gk_spc_out_of_control(const gk_sample_set *s, size_t index);
int gk_spc_out_of_control_count(const gk_sample_set *s);

/* Process capability indices. spec limits are absolute values. */
gk_status gk_cpk(const gk_sample_set *s, double usl, double lsl, double *cpk);
gk_status gk_cp(const gk_sample_set *s, double usl, double lsl, double *cp);
gk_status gk_ppk(const gk_sample_set *s, double usl, double lsl, double *ppk);

/* ---- reporting (276-279) ---- */

typedef struct {
    char content[1024];
    size_t length;
    size_t passed;
    size_t failed;
} gk_report;

void gk_report_init(gk_report *r);
gk_status gk_report_add_line(gk_report *r, const gk_measure_result *result,
                             const char *unit);
const char *gk_report_text(const gk_report *r);

/* CSV export of recorded samples. Returns bytes written. */
size_t gk_samples_to_csv(const gk_sample_set *s, char *out, size_t out_size);

/* ---- alarm (277) ---- */

typedef struct {
    int active;
    double nominal;
    double actual;
    double overage;     /* mm beyond tolerance */
    char label[64];
} gk_tolerance_alarm;

void gk_tolerance_alarm_init(gk_tolerance_alarm *a);
gk_status gk_tolerance_alarm_check(gk_tolerance_alarm *a,
                                   const gk_measure_result *r);

#ifdef __cplusplus
}
#endif

#endif
