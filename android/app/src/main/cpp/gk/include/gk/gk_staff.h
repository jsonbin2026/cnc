#ifndef GK_STAFF_H
#define GK_STAFF_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_STAFF_NAME 48
#define GK_STAFF_SKILL_MAX 16
#define GK_STAFF_MAX 128

/* ===================================================================
 * Batch 53: real personnel management (1386-1400)
 * Prefix: gk_staff_
 * =================================================================== */

typedef enum {
    GK_STAFF_OPERATOR = 0,  /* 1386 */
    GK_STAFF_PROGRAMMER,    /* 1387 */
    GK_STAFF_PROCESS_ENG,   /* 1388 */
    GK_STAFF_INSPECTOR,     /* 1389 */
    GK_STAFF_MAINTAINER,    /* 1390 */
    GK_STAFF_TEAM_LEADER,   /* 1391 */
    GK_STAFF_WORKSHOP_HEAD, /* 1392 */
    GK_STAFF_PROD_MANAGER   /* 1393 */
} gk_staff_role;

const char *gk_staff_role_name(gk_staff_role r);
int gk_staff_role_level(gk_staff_role r);

/* 1394 permission management */
typedef struct {
    int can_edit_program;
    int can_set_param;
    int can_start_machine;
    int can_manage_users;
} gk_staff_perms;

void gk_staff_perms_for_role(gk_staff_role r, gk_staff_perms *out);
int gk_staff_can(const gk_staff_perms *p, const char *action);

typedef struct {
    char name[GK_STAFF_NAME];
    gk_staff_role role;
    gk_staff_perms perms;
    int skills[GK_STAFF_SKILL_MAX];
    int skill_count;
    int present;
} gk_staff_member;

void gk_staff_init(gk_staff_member *m, const char *name, gk_staff_role r);
gk_status gk_staff_add_skill(gk_staff_member *m, int skill_level);
int gk_staff_skill_max(const gk_staff_member *m);

/* 1395 shift scheduling */
typedef struct {
    char member[GK_STAFF_MAX][GK_STAFF_NAME];
    int count;
    int shift_index;
} gk_staff_shift;

void gk_staff_shift_init(gk_staff_shift *s, int shift_index);
gk_status gk_staff_shift_assign(gk_staff_shift *s, const char *name);
int gk_staff_shift_size(const gk_staff_shift *s);

/* 1396 attendance */
typedef struct {
    int present_days;
    int absent_days;
    double overtime_hours;
} gk_staff_attendance;

void gk_staff_attendance_init(gk_staff_attendance *a);
gk_status gk_staff_attendance_mark(gk_staff_attendance *a, int present);
double gk_staff_attendance_rate(const gk_staff_attendance *a);

/* 1397/1420 performance */
typedef struct {
    double quality_score;   /* 0..1 */
    double efficiency;      /* 0..1 */
    double attendance;      /* 0..1 */
} gk_staff_performance;

void gk_staff_performance_init(gk_staff_performance *p);
double gk_staff_performance_score(const gk_staff_performance *p);

/* 1398 training */
typedef struct {
    int required_hours;
    int completed_hours;
} gk_staff_training;

void gk_staff_training_init(gk_staff_training *t, int required_hours);
gk_status gk_staff_training_complete(gk_staff_training *t, int hours);
int gk_staff_training_done(const gk_staff_training *t);

/* 1399 skill matrix */
typedef struct {
    int matrix[GK_STAFF_MAX][GK_STAFF_SKILL_MAX];
    int rows;
    int cols;
} gk_staff_matrix;

void gk_staff_matrix_init(gk_staff_matrix *m, int rows, int cols);
gk_status gk_staff_matrix_set(gk_staff_matrix *m, int row, int col, int level);
int gk_staff_matrix_get(const gk_staff_matrix *m, int row, int col);
int gk_staff_matrix_row_sum(const gk_staff_matrix *m, int row);
int gk_staff_matrix_best_for(const gk_staff_matrix *m, int col);

/* 1400 dispatching */
gk_status gk_staff_dispatch(const int *skills, int n, const int *required,
                            int *assignment);

#ifdef __cplusplus
}
#endif

#endif /* GK_STAFF_H */
