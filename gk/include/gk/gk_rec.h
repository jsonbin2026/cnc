#ifndef GK_REC_H
#define GK_REC_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_REC_TEXT 256
#define GK_REC_NAME 64
#define GK_REC_MAX 256

/* ===================================================================
 * Batch 48: real data records (1291-1311)
 * Prefix: gk_rec_
 * =================================================================== */

/* 1291-1299 logs */
typedef enum {
    GK_REC_LOG_MACHINING = 0, /* 1291 */
    GK_REC_LOG_ALARM,         /* 1292 */
    GK_REC_LOG_OPERATION,     /* 1293 */
    GK_REC_LOG_MAINTENANCE,   /* 1294 */
    GK_REC_LOG_PARAM,         /* 1295 */
    GK_REC_LOG_PROGRAM,       /* 1296 */
    GK_REC_LOG_TOOL,          /* 1297 */
    GK_REC_LOG_PART,          /* 1298 */
    GK_REC_LOG_ENERGY         /* 1299 */
} gk_rec_log_kind;

const char *gk_rec_log_name(gk_rec_log_kind k);

typedef struct {
    char timestamp[GK_REC_NAME];
    gk_rec_log_kind kind;
    char message[GK_REC_TEXT];
    double value;
} gk_rec_entry;

typedef struct {
    gk_rec_entry entries[GK_REC_MAX];
    int count;
    gk_rec_log_kind kind;
} gk_rec_logbook;

gk_status gk_rec_logbook_init(gk_rec_logbook *lb, gk_rec_log_kind kind);
gk_status gk_rec_log_append(gk_rec_logbook *lb, const char *timestamp,
                            const char *message, double value);
int gk_rec_log_count(const gk_rec_logbook *lb);
const gk_rec_entry *gk_rec_log_get(const gk_rec_logbook *lb, int index);
double gk_rec_log_sum(const gk_rec_logbook *lb);
double gk_rec_log_avg(const gk_rec_logbook *lb);

/* 1300 efficiency / 1301 OEE / 1302 availability / 1303 yield / 1304 fault */
typedef struct {
    double run_time_min;
    double planned_time_min;
    double good_count;
    double total_count;
    double ideal_cycle_min;
} gk_rec_production;

void gk_rec_production_init(gk_rec_production *p);
double gk_rec_availability(const gk_rec_production *p); /* 1302 */
double gk_rec_performance(const gk_rec_production *p);  /* 1300 */
double gk_rec_quality(const gk_rec_production *p);      /* 1303 */
double gk_rec_oee(const gk_rec_production *p);          /* 1301 */
double gk_rec_scrap_rate(const gk_rec_production *p);   /* 1304 */

/* 1305 MTBF / 1306 MTTR */
typedef struct {
    double total_run_hours;
    double total_repair_hours;
    int failures;
} gk_rec_reliability;

void gk_rec_reliability_init(gk_rec_reliability *r);
double gk_rec_mtbf(const gk_rec_reliability *r); /* 1305 */
double gk_rec_mttr(const gk_rec_reliability *r); /* 1306 */

/* 1307-1311 report period */
typedef enum {
    GK_REC_REPORT_DAILY = 0, /* 1308 */
    GK_REC_REPORT_WEEKLY,    /* 1309 */
    GK_REC_REPORT_MONTHLY,   /* 1310 */
    GK_REC_REPORT_YEARLY     /* 1311 */
} gk_rec_report_period;

const char *gk_rec_report_name(gk_rec_report_period p);

typedef struct {
    gk_rec_report_period period;
    double good_count;
    double total_count;
    double run_time_min;
    double planned_time_min;
} gk_rec_report;

void gk_rec_report_init(gk_rec_report *rpt, gk_rec_report_period p);
gk_status gk_rec_report_fill(gk_rec_report *rpt, double good, double total,
                             double run_min, double planned_min);
double gk_rec_report_yield(const gk_rec_report *rpt);
gk_status gk_rec_report_render(const gk_rec_report *rpt, char *out,
                               size_t out_cap);

#ifdef __cplusplus
}
#endif

#endif /* GK_REC_H */
