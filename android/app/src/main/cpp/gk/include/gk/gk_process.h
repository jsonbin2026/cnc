#ifndef GK_PROCESS_H
#define GK_PROCESS_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_PROC_MAX_STAGES 32
#define GK_PROC_NAME 64
#define GK_PROC_NOTE 128
#define GK_PROC_MAX_PARTS 64
#define GK_PROC_MAX_STEPS 64

/* Ordered manufacturing stages (622-635). */
typedef enum {
    GK_STAGE_INCOMING_INSPECTION = 0,  /* 622 来料检验 */
    GK_STAGE_BLANK_PREP,               /* 623 毛坯准备 */
    GK_STAGE_SETUP_ALIGN,              /* 624 装夹找正 */
    GK_STAGE_TOOL_SETTING,             /* 625 对刀 */
    GK_STAGE_FIRST_CUT,                /* 626 首件试切 */
    GK_STAGE_FIRST_INSPECTION,         /* 627 首件检验 */
    GK_STAGE_BATCH_MACHINING,          /* 628 批量加工 */
    GK_STAGE_ONLINE_MEASURE,           /* 629 在线测量 */
    GK_STAGE_TOOL_CHANGE,              /* 630 刀具更换 */
    GK_STAGE_CLEANING,                 /* 631 清洗 */
    GK_STAGE_DEBURRING,                /* 632 去毛刺 */
    GK_STAGE_FINAL_INSPECTION,         /* 633 终检 */
    GK_STAGE_PACKAGING,                /* 634 包装 */
    GK_STAGE_WAREHOUSING,              /* 635 入库 */
    GK_STAGE_SCRAP_DECISION,           /* 636 报废判定 */
    GK_STAGE_REWORK,                   /* 637 返工 */
    GK_STAGE_TRACEABILITY,             /* 638 追溯 */
    GK_STAGE_QR_MARKING,               /* 639 二维码标识 */
    GK_STAGE_LIFECYCLE,                /* 640 全生命周期管理 */
    GK_STAGE_COUNT
} gk_stage_kind;

const char *gk_stage_name(gk_stage_kind k);

typedef enum {
    GK_STAGE_PENDING = 0,
    GK_STAGE_ACTIVE,
    GK_STAGE_DONE,
    GK_STAGE_FAILED,
    GK_STAGE_SKIPPED
} gk_stage_status;

const char *gk_stage_status_name(gk_stage_status s);

/* ---- inspection (622, 627, 633) ---- */

typedef struct {
    double nominal;
    double tolerance;
    double measured;
} gk_inspection;

void gk_inspection_init(gk_inspection *i, double nominal, double tolerance);
int gk_inspection_pass(const gk_inspection *i);
double gk_inspection_deviation(const gk_inspection *i);

/* ---- blank / stock prep (623) ---- */

typedef struct {
    double length;
    double width;
    double height;
    double allowance;   /* machining allowance per side */
} gk_blank;

void gk_blank_init(gk_blank *b, double l, double w, double h,
                   double allowance);
double gk_blank_volume(const gk_blank *b);
int gk_blank_covers(const gk_blank *b, double fnl, double fnw, double fnh);

/* ---- setup and alignment (624) ---- */

typedef struct {
    double offset_x;
    double offset_y;
    double offset_z;
    double runout;
    int aligned;
} gk_setup;

void gk_setup_init(gk_setup *s);
gk_status gk_setup_align(gk_setup *s, double ox, double oy, double oz,
                         double runout);
int gk_setup_within_tolerance(const gk_setup *s, double runout_limit);

/* ---- tool setting (625) ---- */

typedef struct {
    int tool_id;
    double length_offset;
    double radius_offset;
    int measured;
} gk_tool_set;

void gk_tool_set_init(gk_tool_set *t, int tool_id);
gk_status gk_tool_set_probe(gk_tool_set *t, double length, double radius);

/* ---- first cut / first inspection (626, 627) ---- */

typedef struct {
    int first_cut_done;
    gk_inspection check;
    int approved;
} gk_first_article;

void gk_first_article_init(gk_first_article *fa);
gk_status gk_first_article_evaluate(gk_first_article *fa, double nominal,
                                    double tolerance, double measured);

/* ---- batch machining (628) ---- */

typedef struct {
    int target;
    int produced;
    int good;
    int scrap;
    int rework;
    int online_measure;
} gk_batch;

void gk_batch_init(gk_batch *b, int target);
gk_status gk_batch_record(gk_batch *b, int pass);
double gk_batch_yield(const gk_batch *b);
int gk_batch_remaining(const gk_batch *b);

/* ---- tool change (630) ---- */

typedef struct {
    int tool_id;
    double life_used;
    double life_limit;
    int needs_change;
} gk_proc_tool;

void gk_proc_tool_init(gk_proc_tool *t, int tool_id, double life_limit);
gk_status gk_proc_tool_use(gk_proc_tool *t, double amount);
int gk_proc_tool_expired(const gk_proc_tool *t);

/* ---- cleaning / deburring / packaging (631, 632, 634) ---- */

typedef struct {
    int cleaned;
    int deburred;
    double edge_radius;
    double cleanliness_mg;
} gk_finish;

void gk_finish_init(gk_finish *f);
gk_status gk_finish_clean(gk_finish *f, double cleanliness_mg);
gk_status gk_finish_deburr(gk_finish *f, double edge_radius);
int gk_finish_ok(const gk_finish *f, double max_cleanliness, double min_radius);

typedef struct {
    char package_id[GK_PROC_NAME];
    int sealed;
    int labeled;
    double weight_kg;
} gk_package;

void gk_package_init(gk_package *p);
gk_status gk_package_seal(gk_package *p, const char *id, double weight);

/* ---- warehousing / scrap / rework (635, 636, 637) ---- */

typedef enum {
    GK_DISPOSITION_PASS = 0,
    GK_DISPOSITION_REWORK,
    GK_DISPOSITION_SCRAP
} gk_disposition;

const char *gk_disposition_name(gk_disposition d);

typedef struct {
    char location[GK_PROC_NAME];
    int quantity;
    double total_weight;
} gk_warehouse;

void gk_warehouse_init(gk_warehouse *w);
gk_status gk_warehouse_store(gk_warehouse *w, const char *location, int qty,
                             double weight);
/* decide disposition from measured error, tolerance and rework capability. */
gk_disposition gk_disposition_decide(double error, double tolerance,
                                     double rework_margin);
int gk_rework_schedule(int defective_qty, double rework_time,
                       double *out_total_time);

/* ---- traceability / QR / lifecycle (638, 639, 640) ---- */

typedef struct {
    char serial[GK_PROC_NAME];
    char part[GK_PROC_NAME];
    char batch[GK_PROC_NAME];
    double timestamp;
    char operator_name[GK_PROC_NAME];
} gk_trace_record;

typedef struct {
    gk_trace_record records[GK_PROC_MAX_PARTS];
    int count;
} gk_trace_log;

void gk_trace_log_init(gk_trace_log *l);
int gk_trace_add(gk_trace_log *l, const char *serial, const char *part,
                 const char *batch, double timestamp, const char *op);
const gk_trace_record *gk_trace_find(const gk_trace_log *l,
                                     const char *serial);
int gk_trace_count_batch(const gk_trace_log *l, const char *batch);

/* QR payload encoding: "GK|part|batch|serial|ts" with a simple checksum. */
int gk_qr_encode(const gk_trace_record *r, char *buf, size_t len);
int gk_qr_checksum(const char *payload);
gk_status gk_qr_decode(const char *payload, gk_trace_record *out);

typedef struct {
    char serial[GK_PROC_NAME];
    int total_uses;
    int remaining_uses;
    int inspected;
    int retired;
    double accumulated_hours;
} gk_lifecycle;

void gk_lifecycle_init(gk_lifecycle *l, const char *serial, int total_uses);
gk_status gk_lifecycle_consume(gk_lifecycle *l, double hours);
int gk_lifecycle_should_retire(const gk_lifecycle *l);

/* ---- process pipeline ---- */

typedef struct {
    gk_stage_kind stage;
    gk_stage_status status;
    char note[GK_PROC_NOTE];
    double duration;
} gk_process_step;

typedef struct {
    gk_process_step steps[GK_PROC_MAX_STEPS];
    int count;
    int current;
} gk_proc_pipeline;

void gk_proc_pipeline_init(gk_proc_pipeline *p);
gk_status gk_proc_pipeline_add(gk_proc_pipeline *p, gk_stage_kind stage);
gk_status gk_proc_pipeline_advance(gk_proc_pipeline *p);
gk_status gk_proc_pipeline_fail(gk_proc_pipeline *p, const char *note);
gk_status gk_proc_pipeline_complete(gk_proc_pipeline *p);
int gk_proc_pipeline_is_complete(const gk_proc_pipeline *p);
int gk_proc_pipeline_done_count(const gk_proc_pipeline *p);
const gk_process_step *gk_proc_pipeline_current(const gk_proc_pipeline *p);

#ifdef __cplusplus
}
#endif

#endif /* GK_PROCESS_H */
