#ifndef GK_LEARN_H
#define GK_LEARN_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_LEARN_NAME 64
#define GK_LEARN_TEXT 2048
#define GK_LEARN_DIMS 6
#define GK_LEARN_MAX_ITEMS 128

typedef enum {
    GK_LEARN_DIM_SAFETY = 0,
    GK_LEARN_DIM_SETUP,
    GK_LEARN_DIM_PROGRAM,
    GK_LEARN_DIM_OPERATION,
    GK_LEARN_DIM_INSPECTION,
    GK_LEARN_DIM_TROUBLESHOOT
} gk_learn_dim;

const char *gk_learn_dim_name(gk_learn_dim d);

/* ===================================================================
 * Part A: accessibility extras (901-903)
 * =================================================================== */

/* 901 screen reader description for a widget */
typedef enum {
    GK_LEARN_ROLE_BUTTON = 0,
    GK_LEARN_ROLE_SLIDER,
    GK_LEARN_ROLE_FIELD,
    GK_LEARN_ROLE_CHART
} gk_learn_role;

const char *gk_learn_role_name(gk_learn_role r);
gk_status gk_learn_screenreader_desc(gk_learn_role role, const char *label,
                                     const char *value, char *out,
                                     size_t out_cap);

/* 902 keyboard navigation */
typedef struct {
    char labels[GK_LEARN_MAX_ITEMS][GK_LEARN_NAME];
    int count;
    int focus;
    int wrap;
} gk_learn_kbdnav;

void gk_learn_kbdnav_init(gk_learn_kbdnav *n);
int gk_learn_kbdnav_add(gk_learn_kbdnav *n, const char *label);
int gk_learn_kbdnav_next(gk_learn_kbdnav *n);
int gk_learn_kbdnav_prev(gk_learn_kbdnav *n);
int gk_learn_kbdnav_focus(gk_learn_kbdnav *n, const char *label);

/* 903 touch optimisation */
typedef struct {
    double min_target_mm;
    double scale;
} gk_learn_touch;

void gk_learn_touch_init(gk_learn_touch *t);
gk_status gk_learn_touch_target(gk_learn_touch *t, double width_mm,
                                double height_mm);
double gk_learn_touch_size(const gk_learn_touch *t, double base_mm);

/* ===================================================================
 * Part B: learning analytics (904-918)
 * =================================================================== */

/* 904 student profile / 905 ability radar */
typedef struct {
    char name[GK_LEARN_NAME];
    double score[GK_LEARN_DIMS];   /* 0..100 */
    double study_hours;
    int exercises_done;
} gk_learn_student;

void gk_learn_student_init(gk_learn_student *s, const char *name);
gk_status gk_learn_student_set(gk_learn_student *s, gk_learn_dim d,
                               double score);
double gk_learn_student_avg(const gk_learn_student *s);
/* normalised radar point in [0,1] for a dimension */
double gk_learn_radar_point(const gk_learn_student *s, gk_learn_dim d);
/* index of the weakest dimension */
int gk_learn_student_weakest(const gk_learn_student *s);

/* 908 course recommendation for the weakest dimension */
gk_status gk_learn_recommend_course(const gk_learn_student *s, char *out,
                                    size_t out_cap);

/* 906 error pattern mining */
typedef struct {
    char pattern[GK_LEARN_NAME];
    int count;
} gk_learn_error;

typedef struct {
    gk_learn_error errors[GK_LEARN_MAX_ITEMS];
    int count;
} gk_learn_errors;

void gk_learn_errors_init(gk_learn_errors *e);
gk_status gk_learn_errors_add(gk_learn_errors *e, const char *pattern);
int gk_learn_errors_top(const gk_learn_errors *e);

/* 907 grade prediction: least-squares linear extrapolation */
double gk_learn_predict_grade(const double *history, int count);

/* 909 cohort comparison */
int gk_learn_cohort_percentile(const double *sorted_scores, int count,
                               double score);

/* 910 teacher dashboard */
typedef struct {
    int students;
    double mean;
    double min;
    double max;
    int at_risk;    /* below threshold */
} gk_learn_dashboard;

gk_status gk_learn_dashboard_build(const gk_learn_student *students, int count,
                                   double risk_threshold,
                                   gk_learn_dashboard *out);

/* 911 learning analysis trend */
typedef struct {
    double slope;
    int improving;
} gk_learn_trend;

gk_status gk_learn_analyze_trend(const double *history, int count,
                                 gk_learn_trend *out);

/* 912 A/B test */
typedef struct {
    int control_n;
    int control_pass;
    int variant_n;
    int variant_pass;
} gk_learn_abtest;

void gk_learn_abtest_init(gk_learn_abtest *a);
double gk_learn_abtest_uplift(const gk_learn_abtest *a);
/* crude significance indicator: |z| > 1.96 */
int gk_learn_abtest_significant(const gk_learn_abtest *a);

/* 913 teaching evaluation: weighted criteria */
typedef struct {
    double weight[GK_LEARN_DIMS];
    double score[GK_LEARN_DIMS];
    int count;
} gk_learn_eval;

void gk_learn_eval_init(gk_learn_eval *e);
gk_status gk_learn_eval_add(gk_learn_eval *e, double weight, double score);
double gk_learn_eval_total(const gk_learn_eval *e);

/* 914 data visualisation */
gk_status gk_learn_bar_chart(const char *labels, const double *values,
                             int count, char *out, size_t out_cap);

/* 915 report generation */
gk_status gk_learn_report(const gk_learn_student *students, int count,
                          const char *title, char *out, size_t out_cap);

/* 916 data export (CSV) */
gk_status gk_learn_export_csv(const gk_learn_student *students, int count,
                              char *out, size_t out_cap);

/* 917 privacy protection: redact a token everywhere it appears */
gk_status gk_learn_redact(const char *text, const char *token,
                          const char *replacement, char *out, size_t out_cap);

/* 918 anonymisation: deterministic pseudonym */
unsigned long gk_learn_anonymize(const char *identifier, unsigned long salt);

#ifdef __cplusplus
}
#endif

#endif /* GK_LEARN_H */
