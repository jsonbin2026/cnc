#ifndef GK_PROGFLOW_H
#define GK_PROGFLOW_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_PROGFLOW_NAME 48
#define GK_PROGFLOW_TEXT 256
#define GK_PROGFLOW_MAX 32

/* ===================================================================
 * Batch 57: real programming flow (1461-1475)
 * Prefix: gk_progflow_
 * =================================================================== */

typedef enum {
    GK_PROGFLOW_DRAWING = 0,   /* 1461 drawing analysis */
    GK_PROGFLOW_PROCESS,       /* 1462 process analysis */
    GK_PROGFLOW_MODEL,         /* 1463 modeling */
    GK_PROGFLOW_CAM,           /* 1464 programming */
    GK_PROGFLOW_SIMULATION,    /* 1465 simulation */
    GK_PROGFLOW_POST,          /* 1466 post processing */
    GK_PROGFLOW_VERIFY,        /* 1467 program verification */
    GK_PROGFLOW_FIRST_CUT,     /* 1468 first article trial cut */
    GK_PROGFLOW_OPTIMIZE,      /* 1469 program optimization */
    GK_PROGFLOW_FREEZE,        /* 1470 program freezing */
    GK_PROGFLOW_ARCHIVE,       /* 1471 archiving */
    GK_PROGFLOW_VERSION,       /* 1472 version management */
    GK_PROGFLOW_PERMISSION,    /* 1473 permission management */
    GK_PROGFLOW_BACKUP,        /* 1474 backup */
    GK_PROGFLOW_RESTORE        /* 1475 restore */
} gk_progflow_stage;

const char *gk_progflow_stage_name(gk_progflow_stage s);
int gk_progflow_stage_index(gk_progflow_stage s);

typedef struct {
    char name[GK_PROGFLOW_NAME];
    int current;      /* current stage index 0..14 */
    int revision;
    int frozen;
    int archived;
    int backed_up;
    int verified;
    int simulated;
    int permission_ok;
} gk_progflow;

void gk_progflow_init(gk_progflow *p, const char *name);
gk_status gk_progflow_advance(gk_progflow *p, gk_progflow_stage stage);
gk_status gk_progflow_set_flag(gk_progflow *p, const char *flag, int value);
int gk_progflow_flag(const gk_progflow *p, const char *flag);
double gk_progflow_progress(const gk_progflow *p);

/* simulation / verification results */
typedef struct {
    int collisions;
    int overtravels;
    double cycle_estimate_min;
} gk_progflow_sim;

void gk_progflow_sim_init(gk_progflow_sim *s);
int gk_progflow_sim_clean(const gk_progflow_sim *s);

/* first-article result */
typedef struct {
    double measured;
    double nominal;
    double tolerance;
} gk_progflow_first_article;

void gk_progflow_first_article_init(gk_progflow_first_article *f, double nominal,
                                    double tolerance, double measured);
int gk_progflow_first_article_ok(const gk_progflow_first_article *f);

/* backup / restore */
typedef struct {
    char path[GK_PROGFLOW_TEXT];
    int revision;
} gk_progflow_backup;

void gk_progflow_backup_init(gk_progflow_backup *b, const char *path,
                             int revision);
gk_status gk_progflow_restore(gk_progflow *p, const gk_progflow_backup *b);

#ifdef __cplusplus
}
#endif

#endif /* GK_PROGFLOW_H */
