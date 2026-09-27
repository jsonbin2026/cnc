#ifndef GK_WF_H
#define GK_WF_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_WF_MAX_STEPS 32
#define GK_WF_NAME 48
#define GK_WF_NOTE 256

/* ===================================================================
 * Batch 45: real operation workflows (1206-1229)
 * Prefix: gk_wf_
 * =================================================================== */

typedef struct {
    char name[GK_WF_NAME];
    int done;
} gk_wf_step;

typedef struct {
    char name[GK_WF_NAME];
    gk_wf_step steps[GK_WF_MAX_STEPS];
    int step_count;
    int current;
    int aborted;
} gk_wf_procedure;

/* 1206-1219: named procedures */
gk_status gk_wf_init(gk_wf_procedure *p, const char *name);
gk_status gk_wf_add_step(gk_wf_procedure *p, const char *step);

/* 1206 power-on / 1207 homing / 1208 tool setting / 1209 trial cut */
gk_status gk_wf_start(gk_wf_procedure *p);
gk_status gk_wf_complete_step(gk_wf_procedure *p);
gk_status gk_wf_abort(gk_wf_procedure *p);
int gk_wf_done(const gk_wf_procedure *p);
double gk_wf_progress(const gk_wf_procedure *p);
/* 1210 first-article / 1211 batch / 1212 sampling / ... templates */
gk_status gk_wf_load_template(gk_wf_procedure *p, int feature_id);

/* 1213 tool change / 1214 workpiece change / 1215 shutdown / 1216 estop */
gk_status gk_wf_estop(gk_wf_procedure *p);

/* 1217 alarm handling */
typedef struct {
    int code;
    char action[GK_WF_NOTE];
    int resolved;
} gk_wf_alarm_action;

void gk_wf_alarm_action_init(gk_wf_alarm_action *a, int code,
                             const char *action);
gk_status gk_wf_alarm_resolve(gk_wf_alarm_action *a);

/* 1218 shift handover */
gk_status gk_wf_handover(char *out, size_t out_cap, const char *from,
                         const char *to, const char *note);

/* 1219-1223 process kinds: maintenance/repair/cleaning/5S/safety */
typedef enum {
    GK_WF_PROC_MAINTENANCE = 0, /* 1219 */
    GK_WF_PROC_REPAIR,          /* 1220 */
    GK_WF_PROC_CLEANING,        /* 1221 */
    GK_WF_PROC_5S,              /* 1222 */
    GK_WF_PROC_SAFETY           /* 1223 */
} gk_wf_process_kind;

const char *gk_wf_process_name(gk_wf_process_kind k);
gk_status gk_wf_process_run(gk_wf_process_kind k, int *checks_passed,
                            int *checks_total);

/* 1224-1229 checklists: on / off / daily / weekly / monthly / yearly */
typedef enum {
    GK_WF_CHK_POWER_ON = 0, /* 1224 */
    GK_WF_CHK_POWER_OFF,    /* 1225 */
    GK_WF_CHK_DAILY,        /* 1226 */
    GK_WF_CHK_WEEKLY,       /* 1227 */
    GK_WF_CHK_MONTHLY,      /* 1228 */
    GK_WF_CHK_YEARLY        /* 1229 */
} gk_wf_checklist_kind;

const char *gk_wf_checklist_name(gk_wf_checklist_kind k);
int gk_wf_checklist_item_count(gk_wf_checklist_kind k);

typedef struct {
    gk_wf_checklist_kind kind;
    int checked[GK_WF_MAX_STEPS];
    int item_count;
} gk_wf_checklist;

gk_status gk_wf_checklist_init(gk_wf_checklist *c, gk_wf_checklist_kind kind);
gk_status gk_wf_checklist_check(gk_wf_checklist *c, int index, int value);
int gk_wf_checklist_all(const gk_wf_checklist *c);
int gk_wf_checklist_score(const gk_wf_checklist *c);

#ifdef __cplusplus
}
#endif

#endif /* GK_WF_H */
