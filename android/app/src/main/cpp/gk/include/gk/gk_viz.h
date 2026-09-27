#ifndef GK_VIZ_H
#define GK_VIZ_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_VIZ_MAX_SERIES 16
#define GK_VIZ_MAX_POINTS 512
#define GK_VIZ_GRID 16

/* ---- 479 oscilloscope ---- */

typedef struct {
    char name[32];
    double samples[GK_VIZ_MAX_POINTS];
    int count;
    int enabled;
} gk_series;

typedef struct {
    gk_series series[GK_VIZ_MAX_SERIES];
    int count;
    double timebase;    /* seconds per division */
    double scale;
} gk_scope;

void gk_scope_init(gk_scope *s);
int gk_scope_add_channel(gk_scope *s, const char *name);
gk_status gk_scope_push(gk_scope *s, int channel, double value);
double gk_scope_value(const gk_scope *s, int channel, int i);
double gk_scope_peak(const gk_scope *s, int channel);
double gk_scope_mean(const gk_scope *s, int channel);

/* ---- 480 data logger ---- */

typedef struct {
    double time;
    double values[GK_VIZ_MAX_SERIES];
    int channels;
} gk_log_sample;

typedef struct {
    gk_log_sample samples[GK_VIZ_MAX_POINTS];
    int count;
    int channels;
    double rate;      /* Hz */
} gk_logger;

void gk_logger_init(gk_logger *l, int channels, double rate);
gk_status gk_logger_record(gk_logger *l, double time, const double *values);
const gk_log_sample *gk_logger_at(const gk_logger *l, int i);
double gk_logger_span(const gk_logger *l);
/* 493/494/495 export sampled series as CSV */
int gk_logger_export(const gk_logger *l, char *buf, size_t len);

/* ---- 481 cutting force heatmap, 482 temperature field ---- */

typedef struct {
    double cells[GK_VIZ_GRID][GK_VIZ_GRID];
    int rows;
    int cols;
    double min;
    double max;
} gk_field;

void gk_field_init(gk_field *f, int rows, int cols);
gk_status gk_field_set(gk_field *f, int r, int c, double value);
double gk_field_get(const gk_field *f, int r, int c);
double gk_field_mean(const gk_field *f);
void gk_field_rescale(gk_field *f);
/* 484 error cloud: max deviation magnitude. */
double gk_field_max_deviation(const gk_field *f, double target);

/* ---- 483 velocity vector field ---- */

typedef struct {
    double u[GK_VIZ_GRID][GK_VIZ_GRID];
    double v[GK_VIZ_GRID][GK_VIZ_GRID];
    int rows;
    int cols;
} gk_vector_field;

void gk_vfield_init(gk_vector_field *f, int rows, int cols);
gk_status gk_vfield_set(gk_vector_field *f, int r, int c, double u, double v);
double gk_vfield_magnitude(const gk_vector_field *f, int r, int c);

/* ---- 485 toolpath Gantt chart ---- */

typedef struct {
    char label[32];
    double start;
    double end;
    int tool;
} gk_gantt_task;

typedef struct {
    gk_gantt_task tasks[128];
    int count;
} gk_gantt;

void gk_gantt_init(gk_gantt *g);
gk_status gk_gantt_add(gk_gantt *g, const char *label, double start,
                       double end, int tool);
double gk_gantt_total(const gk_gantt *g);

/* ---- 486 load timeline / 487 alarm timeline ---- */

typedef struct {
    double time;
    double load;
    int alarm_code;
} gk_timeline_entry;

typedef struct {
    gk_timeline_entry entries[GK_VIZ_MAX_POINTS];
    int count;
    double min_load;
    double max_load;
} gk_timeline;

void gk_timeline_init(gk_timeline *t);
gk_status gk_timeline_add(gk_timeline *t, double time, double load,
                          int alarm_code);
int gk_timeline_alarm_count(const gk_timeline *t);

/* ---- 488 process decomposition diagram ---- */

typedef struct {
    int id;
    char name[32];
    int parent;
    double duration;
} gk_process_node;

typedef struct {
    gk_process_node nodes[64];
    int count;
} gk_process_tree;

void gk_process_tree_init(gk_process_tree *p);
gk_status gk_process_add(gk_process_tree *p, int id, const char *name,
                         int parent, double duration);
double gk_process_subtree_duration(const gk_process_tree *p, int id);

/* ---- 489 depth section ---- */

typedef struct {
    double z[GK_VIZ_MAX_POINTS];
    int count;
} gk_section;

void gk_section_init(gk_section *s);
gk_status gk_section_add(gk_section *s, double z);
double gk_section_min(const gk_section *s);
double gk_section_depth_range(const gk_section *s);

/* ---- 490 dual view / 491 target vs actual ---- */

typedef struct {
    gk_series target;
    gk_series actual;
} gk_dual_view;

void gk_dual_view_init(gk_dual_view *d, const char *name);
gk_status gk_dual_view_push(gk_dual_view *d, double target, double actual);
double gk_dual_view_max_error(const gk_dual_view *d);

/* ---- 492 curve overlay / 493-495 export ---- */

int gk_viz_overlay(const gk_scope *s, char *buf, size_t len);
int gk_viz_export_chart(const gk_scope *s, char *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* GK_VIZ_H */
