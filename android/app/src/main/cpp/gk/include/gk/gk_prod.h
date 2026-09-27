#ifndef GK_PROD_H
#define GK_PROD_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_PROD_NAME 64
#define GK_PROD_TEXT 2048
#define GK_PROD_MAX_ITEMS 64

/* ===================================================================
 * Part A: shop-floor integration (951-960)
 * =================================================================== */

/* 951 multi-machine coordination */
typedef struct {
    char name[GK_PROD_NAME];
    double available_h;   /* load capacity in hours */
    double assigned_h;
    int jobs;
} gk_prod_machine;

typedef struct {
    gk_prod_machine machines[GK_PROD_MAX_ITEMS];
    int count;
} gk_prod_cell;

void gk_prod_cell_init(gk_prod_cell *c);
int gk_prod_cell_add(gk_prod_cell *c, const char *name, double available_h);
/* assign a job of the given hours to the least-loaded machine */
int gk_prod_cell_assign(gk_prod_cell *c, double hours);
double gk_prod_cell_utilization(const gk_prod_cell *c, int index);

/* 952 flexible production line */
typedef struct {
    char name[GK_PROD_NAME];
    double cycle_time_s;
    int capable;      /* can perform the current operation */
} gk_prod_station;

typedef struct {
    gk_prod_station stations[GK_PROD_MAX_ITEMS];
    int count;
} gk_prod_line;

void gk_prod_line_init(gk_prod_line *l);
int gk_prod_line_add(gk_prod_line *l, const char *name, double cycle_time_s);
/* bottleneck station index (largest cycle time among capable stations) */
int gk_prod_line_bottleneck(const gk_prod_line *l);
double gk_prod_line_takt(const gk_prod_line *l);

/* 953 AGV loading */
typedef struct {
    double position_m;
    double speed_mps;
    double battery_pct;
    double charge_rate_pct_per_s;
} gk_prod_agv;

void gk_prod_agv_init(gk_prod_agv *a, double speed_mps);
double gk_prod_agv_travel_time(const gk_prod_agv *a, double target_m);
gk_status gk_prod_agv_move(gk_prod_agv *a, double target_m);
gk_status gk_prod_agv_charge(gk_prod_agv *a, double seconds);

/* 954 robot loading */
typedef struct {
    double approach_s;
    double grip_s;
    double transfer_s;
    double release_s;
} gk_prod_robot;

void gk_prod_robot_init(gk_prod_robot *r);
double gk_prod_robot_cycle_time(const gk_prod_robot *r, double distance_m,
                                double speed_mps);

/* 955 vision positioning */
gk_status gk_prod_vision_find(const double *xs, const double *ys, int count,
                              double *out_cx, double *out_cy);
/* offset of a detected centroid from a nominal target */
double gk_prod_vision_offset(double detected, double nominal);

/* 956 RFID traceability */
typedef struct {
    char tag_id[GK_PROD_NAME];
    char payload[GK_PROD_NAME];
    int valid;
} gk_prod_rfid;

void gk_prod_rfid_init(gk_prod_rfid *t, const char *tag_id);
gk_status gk_prod_rfid_write(gk_prod_rfid *t, const char *payload);
gk_status gk_prod_rfid_read(const gk_prod_rfid *t, char *out, size_t out_cap);

/* 957 MES integration */
typedef enum {
    GK_PROD_MES_PLANNED = 0,
    GK_PROD_MES_RELEASED,
    GK_PROD_MES_RUNNING,
    GK_PROD_MES_DONE
} gk_prod_mes_state;

const char *gk_prod_mes_state_name(gk_prod_mes_state s);

typedef struct {
    char order[GK_PROD_NAME];
    gk_prod_mes_state state;
    int good;
    int scrap;
} gk_prod_mes;

void gk_prod_mes_init(gk_prod_mes *m, const char *order);
gk_status gk_prod_mes_advance(gk_prod_mes *m);
gk_status gk_prod_mes_report(gk_prod_mes *m, int good, int scrap);

/* 958 ERP order interface */
typedef struct {
    char order[GK_PROD_NAME];
    int quantity;
    int produced;
    long due_day;
} gk_prod_erp;

void gk_prod_erp_init(gk_prod_erp *e, const char *order, int quantity,
                      long due_day);
double gk_prod_erp_progress(const gk_prod_erp *e);
gk_status gk_prod_erp_receive(gk_prod_erp *e, int qty);

/* 959 WMS warehouse */
typedef struct {
    char sku[GK_PROD_NAME];
    int on_hand;
    int reserved;
} gk_prod_wms;

void gk_prod_wms_init(gk_prod_wms *w, const char *sku, int on_hand);
gk_status gk_prod_wms_reserve(gk_prod_wms *w, int qty);
gk_status gk_prod_wms_pick(gk_prod_wms *w, int qty);
int gk_prod_wms_available(const gk_prod_wms *w);

/* 960 automated storage / retrieval system */
typedef struct {
    int bins;
    int stored;
    double aisle_time_s;
} gk_prod_asrs;

void gk_prod_asrs_init(gk_prod_asrs *a, int bins);
gk_status gk_prod_asrs_store(gk_prod_asrs *a);
gk_status gk_prod_asrs_retrieve(gk_prod_asrs *a);
double gk_prod_asrs_cycle_time(const gk_prod_asrs *a);

/* ===================================================================
 * Part B: optimisation (961-970)
 * =================================================================== */

/* 961 intelligent scheduling */
typedef struct {
    char job[GK_PROD_NAME];
    int priority;
    long due_day;
    double duration_h;
} gk_prod_job;

/* order the job indices by priority (desc) then due (asc); writes order[] */
gk_status gk_prod_schedule(const gk_prod_job *jobs, int count, int *order);

/* 962 process/operation sequencing with precedence */
typedef struct {
    char op[GK_PROD_NAME];
    int predecessor;   /* index or -1 */
} gk_prod_operation;

gk_status gk_prod_optimize_process(const gk_prod_operation *ops, int count,
                                   int *order);

/* 963 path optimisation: nearest-neighbour tour over 2D points */
gk_status gk_prod_optimize_path(const double *xs, const double *ys, int count,
                                int *order, double *out_length);

/* generic bounded 1-D optimiser */
typedef double (*gk_prod_objective)(double x, void *ctx);

typedef enum {
    GK_PROD_MINIMISE = 0,
    GK_PROD_MAXIMISE
} gk_prod_goal;

typedef struct {
    double x;
    double value;
    int evaluations;
} gk_prod_result;

gk_status gk_prod_optimize(double lo, double hi, int steps,
                           gk_prod_objective f, void *ctx, gk_prod_goal goal,
                           gk_prod_result *out);

/* 964-970 domain wrappers around the generic optimiser */
gk_status gk_prod_optimize_params(gk_prod_objective f, void *ctx, double lo,
                                  double hi, int steps, gk_prod_result *out);
gk_status gk_prod_optimize_tech(gk_prod_objective f, void *ctx, double lo,
                                double hi, int steps, gk_prod_result *out);
gk_status gk_prod_optimize_cost(gk_prod_objective f, void *ctx, double lo,
                                double hi, int steps, gk_prod_result *out);
gk_status gk_prod_optimize_energy(gk_prod_objective f, void *ctx, double lo,
                                  double hi, int steps, gk_prod_result *out);
gk_status gk_prod_optimize_quality(gk_prod_objective f, void *ctx, double lo,
                                   double hi, int steps, gk_prod_result *out);
gk_status gk_prod_optimize_efficiency(gk_prod_objective f, void *ctx,
                                      double lo, double hi, int steps,
                                      gk_prod_result *out);
gk_status gk_prod_optimize_comprehensive(gk_prod_objective f, void *ctx,
                                         double lo, double hi, int steps,
                                         gk_prod_result *out);

#ifdef __cplusplus
}
#endif

#endif /* GK_PROD_H */
