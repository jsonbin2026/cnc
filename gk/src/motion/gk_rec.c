#include "gk/gk_rec.h"

#include <stdio.h>
#include <string.h>

static void gk__rec_copy(char *dst, size_t cap, const char *src)
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

const char *gk_rec_log_name(gk_rec_log_kind k)
{
    switch (k) {
    case GK_REC_LOG_MACHINING: return "machining-log";
    case GK_REC_LOG_ALARM: return "alarm-log";
    case GK_REC_LOG_OPERATION: return "operation-log";
    case GK_REC_LOG_MAINTENANCE: return "maintenance-log";
    case GK_REC_LOG_PARAM: return "parameter-log";
    case GK_REC_LOG_PROGRAM: return "program-log";
    case GK_REC_LOG_TOOL: return "tool-log";
    case GK_REC_LOG_PART: return "part-log";
    case GK_REC_LOG_ENERGY: return "energy-log";
    default: return "unknown";
    }
}

gk_status gk_rec_logbook_init(gk_rec_logbook *lb, gk_rec_log_kind kind)
{
    if (lb == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    memset(lb, 0, sizeof(*lb));
    lb->kind = kind;
    return GK_OK;
}

gk_status gk_rec_log_append(gk_rec_logbook *lb, const char *timestamp,
                            const char *message, double value)
{
    gk_rec_entry *e;
    if (lb == NULL || timestamp == NULL || message == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (lb->count >= GK_REC_MAX) {
        return GK_ERR_OVERFLOW;
    }
    e = &lb->entries[lb->count];
    gk__rec_copy(e->timestamp, sizeof(e->timestamp), timestamp);
    gk__rec_copy(e->message, sizeof(e->message), message);
    e->kind = lb->kind;
    e->value = value;
    lb->count++;
    return GK_OK;
}

int gk_rec_log_count(const gk_rec_logbook *lb)
{
    if (lb == NULL) {
        return 0;
    }
    return lb->count;
}

const gk_rec_entry *gk_rec_log_get(const gk_rec_logbook *lb, int index)
{
    if (lb == NULL || index < 0 || index >= lb->count) {
        return NULL;
    }
    return &lb->entries[index];
}

double gk_rec_log_sum(const gk_rec_logbook *lb)
{
    double s = 0.0;
    int i;
    if (lb == NULL) {
        return 0.0;
    }
    for (i = 0; i < lb->count; i++) {
        s += lb->entries[i].value;
    }
    return s;
}

double gk_rec_log_avg(const gk_rec_logbook *lb)
{
    if (lb == NULL || lb->count == 0) {
        return 0.0;
    }
    return gk_rec_log_sum(lb) / (double)lb->count;
}

/* ===================================================================
 * Production metrics (1300-1304)
 * =================================================================== */

void gk_rec_production_init(gk_rec_production *p)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
}

double gk_rec_availability(const gk_rec_production *p)
{
    if (p == NULL || p->planned_time_min <= 0.0) {
        return 0.0;
    }
    return p->run_time_min / p->planned_time_min;
}

double gk_rec_performance(const gk_rec_production *p)
{
    if (p == NULL || p->run_time_min <= 0.0) {
        return 0.0;
    }
    return (p->ideal_cycle_min * p->total_count) / p->run_time_min;
}

double gk_rec_quality(const gk_rec_production *p)
{
    if (p == NULL || p->total_count <= 0.0) {
        return 0.0;
    }
    return p->good_count / p->total_count;
}

double gk_rec_oee(const gk_rec_production *p)
{
    return gk_rec_availability(p) * gk_rec_performance(p) * gk_rec_quality(p);
}

double gk_rec_scrap_rate(const gk_rec_production *p)
{
    if (p == NULL || p->total_count <= 0.0) {
        return 0.0;
    }
    return (p->total_count - p->good_count) / p->total_count;
}

/* ===================================================================
 * Reliability (1305-1306)
 * =================================================================== */

void gk_rec_reliability_init(gk_rec_reliability *r)
{
    if (r == NULL) {
        return;
    }
    memset(r, 0, sizeof(*r));
}

double gk_rec_mtbf(const gk_rec_reliability *r)
{
    if (r == NULL || r->failures <= 0) {
        return 0.0;
    }
    return r->total_run_hours / (double)r->failures;
}

double gk_rec_mttr(const gk_rec_reliability *r)
{
    if (r == NULL || r->failures <= 0) {
        return 0.0;
    }
    return r->total_repair_hours / (double)r->failures;
}

/* ===================================================================
 * Reports (1307-1311)
 * =================================================================== */

const char *gk_rec_report_name(gk_rec_report_period p)
{
    switch (p) {
    case GK_REC_REPORT_DAILY: return "daily";
    case GK_REC_REPORT_WEEKLY: return "weekly";
    case GK_REC_REPORT_MONTHLY: return "monthly";
    case GK_REC_REPORT_YEARLY: return "yearly";
    default: return "unknown";
    }
}

void gk_rec_report_init(gk_rec_report *rpt, gk_rec_report_period p)
{
    if (rpt == NULL) {
        return;
    }
    memset(rpt, 0, sizeof(*rpt));
    rpt->period = p;
}

gk_status gk_rec_report_fill(gk_rec_report *rpt, double good, double total,
                             double run_min, double planned_min)
{
    if (rpt == NULL || good < 0.0 || total < 0.0 || run_min < 0.0 ||
        planned_min < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    rpt->good_count = good;
    rpt->total_count = total;
    rpt->run_time_min = run_min;
    rpt->planned_time_min = planned_min;
    return GK_OK;
}

double gk_rec_report_yield(const gk_rec_report *rpt)
{
    if (rpt == NULL || rpt->total_count <= 0.0) {
        return 0.0;
    }
    return rpt->good_count / rpt->total_count;
}

gk_status gk_rec_report_render(const gk_rec_report *rpt, char *out,
                               size_t out_cap)
{
    int n;
    if (rpt == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "REPORT[%s] yield=%.3f",
                 gk_rec_report_name(rpt->period),
                 gk_rec_report_yield(rpt));
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}
