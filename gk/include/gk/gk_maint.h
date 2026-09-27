#ifndef GK_MAINT_H
#define GK_MAINT_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_MAINT_MAX_TASKS 64
#define GK_MAINT_MAX_PARTS 64
#define GK_MAINT_NAME 64
#define GK_MAINT_NOTE 128

/* ---- 656 maintenance schedule / 657-660 daily-weekly-monthly-yearly ---- */

typedef enum {
    GK_MAINT_DAILY = 0,     /* 657 */
    GK_MAINT_WEEKLY,        /* 658 */
    GK_MAINT_MONTHLY,       /* 659 */
    GK_MAINT_YEARLY,        /* 660 */
    GK_MAINT_INTERVAL
} gk_maint_period;

const char *gk_maint_period_name(gk_maint_period p);
/* nominal interval in hours used to derive due dates */
double gk_maint_period_hours(gk_maint_period p);

typedef struct {
    int id;
    char name[GK_MAINT_NAME];
    gk_maint_period period;
    double interval_hours;
    double last_hours;
    int done_count;
} gk_maint_task;

typedef struct {
    gk_maint_task tasks[GK_MAINT_MAX_TASKS];
    int count;
    int next_id;
} gk_maint_schedule;

void gk_maint_schedule_init(gk_maint_schedule *s);
int gk_maint_task_add(gk_maint_schedule *s, const char *name,
                      gk_maint_period period);
gk_status gk_maint_task_done(gk_maint_schedule *s, int id, double now_hours);
/* hours remaining until the task is due; <=0 means overdue */
double gk_maint_task_due_in(const gk_maint_schedule *s, int id,
                            double now_hours);
int gk_maint_task_is_due(const gk_maint_schedule *s, int id, double now_hours);
int gk_maint_count_by_period(const gk_maint_schedule *s, gk_maint_period p);
int gk_maint_due_count(const gk_maint_schedule *s, double now_hours);

/* ---- 661 lubrication / 662 oil level / 663 auto-lube ---- */

typedef struct {
    double capacity_l;
    double level_l;
    double low_threshold;   /* fraction, e.g. 0.2 */
    double consumption_lph;
} gk_lube_system;

void gk_lube_system_init(gk_lube_system *l, double capacity, double threshold);
gk_status gk_lube_refill(gk_lube_system *l, double amount);
gk_status gk_lube_consume(gk_lube_system *l, double hours);
double gk_lube_level_fraction(const gk_lube_system *l);
int gk_lube_level_low(const gk_lube_system *l);
/* auto-lube: returns number of pump pulses required for `hours` */
int gk_lube_auto_pulse(const gk_lube_system *l, double hours,
                       double pulse_volume);

/* ---- 664 guide cleaning / 665 filter / 666 belt check ---- */

typedef struct {
    double last_clean_hours;
    double interval_hours;
    int cleaned;
} gk_guide_clean;

void gk_guide_clean_init(gk_guide_clean *g, double interval_hours);
gk_status gk_guide_clean_run(gk_guide_clean *g, double now_hours);
int gk_guide_clean_due(const gk_guide_clean *g, double now_hours);

typedef struct {
    double installed_hours;
    double life_hours;
    double pressure_drop;   /* bar, rising means clogged */
    double max_pressure_drop;
    int replaced;
} gk_filter;

void gk_filter_init(gk_filter *f, double life_hours, double max_drop);
gk_status gk_filter_replace(gk_filter *f, double now_hours);
int gk_filter_needs_change(const gk_filter *f, double now_hours);

typedef struct {
    double tension_n;
    double min_tension;
    double max_tension;
    double wear;            /* 0..1 */
    int checked;
} gk_belt;

void gk_belt_init(gk_belt *b, double min_tension, double max_tension);
gk_status gk_belt_check(gk_belt *b, double tension, double wear);
int gk_belt_ok(const gk_belt *b);

/* ---- 667 precision check / 668 laser interferometer / 669 ballbar ---- */

typedef struct {
    double nominal;
    double measured;
    double error;
    double allowed;
} gk_precision_axis;

typedef struct {
    gk_precision_axis axes[8];
    int count;
} gk_precision_check;

void gk_precision_check_init(gk_precision_check *c);
gk_status gk_precision_add(gk_precision_check *c, double nominal,
                           double measured, double allowed);
int gk_precision_all_pass(const gk_precision_check *c);
double gk_precision_max_error(const gk_precision_check *c);

typedef struct {
    double position[64];
    double deviation[64];
    int count;
    double wavelength;      /* nm */
} gk_interferometer;

void gk_interferometer_init(gk_interferometer *i);
gk_status gk_interferometer_sample(gk_interferometer *i, double position,
                                   double deviation);
double gk_interferometer_linear_error(const gk_interferometer *i);
/* complete ballbar circularity result */
typedef struct {
    double radius;
    double radial_error;
    double circularity;
    double backlash;
} gk_ballbar;

void gk_ballbar_init(gk_ballbar *b, double radius);
gk_status gk_ballbar_run(gk_ballbar *b, double radial_error, double backlash);
int gk_ballbar_pass(const gk_ballbar *b, double tolerance);

/* ---- 670 maintenance fault tree ---- */

typedef struct {
    int id;
    int parent;
    char name[GK_MAINT_NAME];
    double probability;     /* failure probability of a leaf */
    int is_leaf;
} gk_maint_fault_node;

typedef struct {
    gk_maint_fault_node nodes[GK_MAINT_MAX_PARTS];
    int count;
} gk_maint_fault_tree;

void gk_maint_fault_tree_init(gk_maint_fault_tree *t);
gk_status gk_maint_fault_node_add(gk_maint_fault_tree *t, int id, int parent,
                                  const char *name, double prob, int is_leaf);
/* top-event probability: leaves combine with OR (1 - prod(1-p)) */
double gk_maint_fault_probability(const gk_maint_fault_tree *t, int node_id);

/* ---- 671 spare parts / 672 work order / 673 repair log / 674 reminder --- */

typedef struct {
    int id;
    char name[GK_MAINT_NAME];
    int stock;
    int min_stock;
    double unit_price;
} gk_spare_part;

typedef struct {
    gk_spare_part parts[GK_MAINT_MAX_PARTS];
    int count;
} gk_spare_store;

void gk_spare_store_init(gk_spare_store *s);
int gk_spare_add(gk_spare_store *s, const char *name, int stock, int min_stock,
                 double price);
gk_status gk_spare_consume(gk_spare_store *s, int id, int qty);
gk_status gk_spare_restock(gk_spare_store *s, int id, int qty);
int gk_spare_below_min(const gk_spare_store *s);
double gk_spare_value(const gk_spare_store *s);

typedef enum {
    GK_WO_OPEN = 0,
    GK_WO_IN_PROGRESS,
    GK_WO_DONE
} gk_work_order_status;

const char *gk_work_order_status_name(gk_work_order_status s);

typedef struct {
    int id;
    char description[GK_MAINT_NOTE];
    int priority;
    gk_work_order_status status;
    int assignee;
    double hours_spent;
} gk_work_order;

typedef struct {
    gk_work_order orders[GK_MAINT_MAX_TASKS];
    int count;
    int next_id;
} gk_work_order_book;

void gk_work_order_book_init(gk_work_order_book *b);
int gk_work_order_create(gk_work_order_book *b, const char *desc, int priority);
gk_status gk_work_order_assign(gk_work_order_book *b, int id, int assignee);
gk_status gk_work_order_log(gk_work_order_book *b, int id, double hours);
gk_status gk_work_order_complete(gk_work_order_book *b, int id);
int gk_work_order_open_count(const gk_work_order_book *b);

typedef struct {
    int work_order_id;
    char note[GK_MAINT_NOTE];
    double hours;
    double cost;
} gk_repair_log;

typedef struct {
    gk_repair_log entries[GK_MAINT_MAX_TASKS];
    int count;
    double total_cost;
} gk_repair_history;

void gk_repair_history_init(gk_repair_history *h);
gk_status gk_repair_record_add(gk_repair_history *h, int work_order_id,
                               const char *note, double hours, double cost);
double gk_repair_total_hours(const gk_repair_history *h);

/* 674 reminder: next due timestamp for a task */
double gk_maint_next_due(const gk_maint_task *t, double now_hours);

/* ---- 675 life prediction ---- */

typedef struct {
    double design_hours;
    double used_hours;
    double degradation;     /* measured wear rate multiplier */
} gk_component_life;

void gk_component_life_init(gk_component_life *l, double design_hours);
/* remaining useful life in hours given current usage and degradation */
double gk_component_rul(const gk_component_life *l);
double gk_component_health(const gk_component_life *l);

#ifdef __cplusplus
}
#endif

#endif /* GK_MAINT_H */
