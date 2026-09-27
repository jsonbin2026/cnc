#ifndef GK_PMGMT_H
#define GK_PMGMT_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_PMGMT_ID 32
#define GK_PMGMT_NAME 48
#define GK_PMGMT_MAX 128

/* ===================================================================
 * Batch 54: real production management (1401-1420)
 * Prefix: gk_pmgmt_
 * =================================================================== */

/* 1401 orders */
typedef enum {
    GK_PMGMT_ORDER_NEW = 0,
    GK_PMGMT_ORDER_SCHEDULED,
    GK_PMGMT_ORDER_RUNNING,
    GK_PMGMT_ORDER_DONE,
    GK_PMGMT_ORDER_LATE
} gk_pmgmt_order_state;

const char *gk_pmgmt_order_state_name(gk_pmgmt_order_state s);

typedef struct {
    char id[GK_PMGMT_ID];
    char part[GK_PMGMT_NAME];
    int quantity;
    double due_day;
    gk_pmgmt_order_state state;
} gk_pmgmt_order;

void gk_pmgmt_order_init(gk_pmgmt_order *o, const char *id, const char *part,
                         int qty, double due_day);
gk_status gk_pmgmt_order_schedule(gk_pmgmt_order *o);
gk_status gk_pmgmt_order_start(gk_pmgmt_order *o);
gk_status gk_pmgmt_order_finish(gk_pmgmt_order *o);
int gk_pmgmt_order_late(const gk_pmgmt_order *o, double today);

/* 1402-1405 scheduling / dispatching / progress / delivery */
typedef struct {
    char order[GK_PMGMT_MAX][GK_PMGMT_ID];
    double start[GK_PMGMT_MAX];
    double finish[GK_PMGMT_MAX];
    int count;
} gk_pmgmt_schedule;

void gk_pmgmt_schedule_init(gk_pmgmt_schedule *s);
gk_status gk_pmgmt_schedule_add(gk_pmgmt_schedule *s, const char *order,
                                double start, double finish);
double gk_pmgmt_schedule_makespan(const gk_pmgmt_schedule *s);
int gk_pmgmt_schedule_overlaps(const gk_pmgmt_schedule *s);

typedef struct {
    int total;
    int completed;
} gk_pmgmt_progress;

void gk_pmgmt_progress_init(gk_pmgmt_progress *p, int total);
gk_status gk_pmgmt_progress_set(gk_pmgmt_progress *p, int completed);
double gk_pmgmt_progress_ratio(const gk_pmgmt_progress *p);

typedef struct {
    double promised_day;
    double actual_day;
} gk_pmgmt_delivery;

void gk_pmgmt_delivery_init(gk_pmgmt_delivery *d, double promised, double actual);
int gk_pmgmt_delivery_ontime(const gk_pmgmt_delivery *d);

/* 1406 quality / 1407 cost / 1408 equipment / 1409 tooling / 1410 material /
 * 1411 inventory / 1412 purchase / 1413 supplier / 1414 customer /
 * 1415 after-sales / 1416 reports / 1417 kanban / 1418 exception /
 * 1419 meetings / 1420 performance */
typedef enum {
    GK_PMGMT_MODULE_QUALITY = 0,   /* 1406 */
    GK_PMGMT_MODULE_COST,          /* 1407 */
    GK_PMGMT_MODULE_EQUIPMENT,     /* 1408 */
    GK_PMGMT_MODULE_TOOLING,       /* 1409 */
    GK_PMGMT_MODULE_MATERIAL,      /* 1410 */
    GK_PMGMT_MODULE_INVENTORY,     /* 1411 */
    GK_PMGMT_MODULE_PURCHASE,      /* 1412 */
    GK_PMGMT_MODULE_SUPPLIER,      /* 1413 */
    GK_PMGMT_MODULE_CUSTOMER,      /* 1414 */
    GK_PMGMT_MODULE_AFTERSALES,    /* 1415 */
    GK_PMGMT_MODULE_REPORT,        /* 1416 */
    GK_PMGMT_MODULE_KANBAN,        /* 1417 */
    GK_PMGMT_MODULE_EXCEPTION,     /* 1418 */
    GK_PMGMT_MODULE_MEETING,       /* 1419 */
    GK_PMGMT_MODULE_PERFORMANCE    /* 1420 */
} gk_pmgmt_module;

const char *gk_pmgmt_module_name(gk_pmgmt_module m);

typedef struct {
    gk_pmgmt_module module;
    char records[GK_PMGMT_MAX];
    int entry_count;
} gk_pmgmt_registry;

void gk_pmgmt_registry_init(gk_pmgmt_registry *r, gk_pmgmt_module m);
gk_status gk_pmgmt_registry_add(gk_pmgmt_registry *r, int n);
int gk_pmgmt_registry_count(const gk_pmgmt_registry *r);

/* exception handling (1418) */
typedef struct {
    char code[GK_PMGMT_ID];
    double severity;
    int resolved;
} gk_pmgmt_exception;

void gk_pmgmt_exception_init(gk_pmgmt_exception *e, const char *code,
                             double severity);
gk_status gk_pmgmt_exception_resolve(gk_pmgmt_exception *e);

/* meeting (1419) */
typedef struct {
    char topic[GK_PMGMT_NAME];
    double minutes;
    int attendees;
    int decisions;
} gk_pmgmt_meeting;

void gk_pmgmt_meeting_init(gk_pmgmt_meeting *m, const char *topic,
                           int attendees);
gk_status gk_pmgmt_meeting_add_decision(gk_pmgmt_meeting *m, double minutes);

#ifdef __cplusplus
}
#endif

#endif /* GK_PMGMT_H */
