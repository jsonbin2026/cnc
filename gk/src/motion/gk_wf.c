#include "gk/gk_wf.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static void gk__wf_copy(char *dst, size_t cap, const char *src)
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

/* ===================================================================
 * Procedure (1206-1216)
 * =================================================================== */

gk_status gk_wf_init(gk_wf_procedure *p, const char *name)
{
    if (p == NULL || name == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    memset(p, 0, sizeof(*p));
    gk__wf_copy(p->name, sizeof(p->name), name);
    return GK_OK;
}

gk_status gk_wf_add_step(gk_wf_procedure *p, const char *step)
{
    if (p == NULL || step == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (p->step_count >= GK_WF_MAX_STEPS) {
        return GK_ERR_OVERFLOW;
    }
    gk__wf_copy(p->steps[p->step_count].name,
                sizeof(p->steps[p->step_count].name), step);
    p->steps[p->step_count].done = 0;
    p->step_count++;
    return GK_OK;
}

gk_status gk_wf_start(gk_wf_procedure *p)
{
    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (p->step_count == 0) {
        return GK_ERR_STATE;
    }
    p->current = 0;
    p->aborted = 0;
    return GK_OK;
}

gk_status gk_wf_complete_step(gk_wf_procedure *p)
{
    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (p->aborted) {
        return GK_ERR_STATE;
    }
    if (p->current >= p->step_count) {
        return GK_ERR_STATE;
    }
    p->steps[p->current].done = 1;
    p->current++;
    return GK_OK;
}

gk_status gk_wf_abort(gk_wf_procedure *p)
{
    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    p->aborted = 1;
    return GK_OK;
}

int gk_wf_done(const gk_wf_procedure *p)
{
    if (p == NULL || p->aborted) {
        return 0;
    }
    return p->current >= p->step_count && p->step_count > 0;
}

double gk_wf_progress(const gk_wf_procedure *p)
{
    if (p == NULL || p->step_count == 0) {
        return 0.0;
    }
    return (double)p->current / (double)p->step_count;
}

gk_status gk_wf_load_template(gk_wf_procedure *p, int feature_id)
{
    static const struct {
        int id;
        const char *name;
        const char *steps[6];
    } tmpl[] = {
        {1206, "power-on", {"inspect", "close breaker", "boot hmi", "home axes",
                            "warm up", "ready"}},
        {1207, "homing", {"mode jog-ref", "Z home", "X home", "Y home",
                          "verify", "done"}},
        {1208, "tool-setting", {"load tool", "touch off X", "touch off Y",
                                "touch off Z", "record offsets", "verify"}},
        {1209, "trial-cut", {"load program", "single block", "dry run",
                             "first cut", "measure", "adjust"}},
        {1210, "first-article", {"machining", "dimension check", "surface check",
                                 "record", "approve", "release"}},
        {1211, "batch", {"stage material", "run loop", "monitor", "count",
                         "sample", "finish"}},
        {1212, "sampling", {"pick sample", "gauge", "compare", "trend",
                            "decide", "record"}},
        {1213, "tool-change", {"stop spindle", "retract", "unclamp",
                               "swap tool", "clamp", "measure"}},
        {1214, "workpiece-change", {"open door", "unclamp", "remove",
                                    "load new", "clamp", "close door"}},
        {1215, "shutdown", {"end program", "retract", "spindle off",
                            "coolant off", "power off", "log"}},
        {1216, "estop", {"press estop", "confirm stop", "diagnose", "reset",
                         "re-home", "resume"}},
    };
    size_t i, j;
    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < sizeof(tmpl) / sizeof(tmpl[0]); i++) {
        if (tmpl[i].id == feature_id) {
            gk_wf_init(p, tmpl[i].name);
            for (j = 0; j < 6; j++) {
                gk_wf_add_step(p, tmpl[i].steps[j]);
            }
            return GK_OK;
        }
    }
    return GK_ERR_NOT_FOUND;
}

gk_status gk_wf_estop(gk_wf_procedure *p)
{
    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    p->aborted = 1;
    p->current = 0;
    return GK_OK;
}

/* ===================================================================
 * Alarm handling (1217)
 * =================================================================== */

void gk_wf_alarm_action_init(gk_wf_alarm_action *a, int code,
                             const char *action)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    a->code = code;
    gk__wf_copy(a->action, sizeof(a->action), action);
}

gk_status gk_wf_alarm_resolve(gk_wf_alarm_action *a)
{
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (a->action[0] == '\0') {
        return GK_ERR_STATE;
    }
    a->resolved = 1;
    return GK_OK;
}

/* ===================================================================
 * Shift handover (1218)
 * =================================================================== */

gk_status gk_wf_handover(char *out, size_t out_cap, const char *from,
                         const char *to, const char *note)
{
    int n;
    if (out == NULL || out_cap == 0 || from == NULL || to == NULL ||
        note == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "%s->%s: %s", from, to, note);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

/* ===================================================================
 * Processes (1219-1223)
 * =================================================================== */

const char *gk_wf_process_name(gk_wf_process_kind k)
{
    switch (k) {
    case GK_WF_PROC_MAINTENANCE: return "maintenance";
    case GK_WF_PROC_REPAIR: return "repair";
    case GK_WF_PROC_CLEANING: return "cleaning";
    case GK_WF_PROC_5S: return "5S";
    case GK_WF_PROC_SAFETY: return "safety-check";
    default: return "unknown";
    }
}

gk_status gk_wf_process_run(gk_wf_process_kind k, int *checks_passed,
                            int *checks_total)
{
    static const int totals[] = {8, 10, 6, 5, 12};
    int t;
    if (checks_passed == NULL || checks_total == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if ((int)k < 0 || (int)k > 4) {
        return GK_ERR_OUT_OF_RANGE;
    }
    t = totals[(int)k];
    *checks_total = t;
    *checks_passed = t;
    return GK_OK;
}

/* ===================================================================
 * Checklists (1224-1229)
 * =================================================================== */

const char *gk_wf_checklist_name(gk_wf_checklist_kind k)
{
    switch (k) {
    case GK_WF_CHK_POWER_ON: return "power-on-check";
    case GK_WF_CHK_POWER_OFF: return "power-off-check";
    case GK_WF_CHK_DAILY: return "daily";
    case GK_WF_CHK_WEEKLY: return "weekly";
    case GK_WF_CHK_MONTHLY: return "monthly";
    case GK_WF_CHK_YEARLY: return "yearly";
    default: return "unknown";
    }
}

int gk_wf_checklist_item_count(gk_wf_checklist_kind k)
{
    static const int counts[] = {8, 6, 10, 14, 20, 30};
    if ((int)k < 0 || (int)k > 5) {
        return 0;
    }
    return counts[(int)k];
}

gk_status gk_wf_checklist_init(gk_wf_checklist *c, gk_wf_checklist_kind kind)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    memset(c, 0, sizeof(*c));
    c->kind = kind;
    c->item_count = gk_wf_checklist_item_count(kind);
    if (c->item_count > GK_WF_MAX_STEPS) {
        c->item_count = GK_WF_MAX_STEPS;
    }
    return GK_OK;
}

gk_status gk_wf_checklist_check(gk_wf_checklist *c, int index, int value)
{
    if (c == NULL || index < 0 || index >= c->item_count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    c->checked[index] = value ? 1 : 0;
    return GK_OK;
}

int gk_wf_checklist_all(const gk_wf_checklist *c)
{
    int i;
    if (c == NULL || c->item_count == 0) {
        return 0;
    }
    for (i = 0; i < c->item_count; i++) {
        if (!c->checked[i]) {
            return 0;
        }
    }
    return 1;
}

int gk_wf_checklist_score(const gk_wf_checklist *c)
{
    int i, n = 0;
    if (c == NULL || c->item_count == 0) {
        return 0;
    }
    for (i = 0; i < c->item_count; i++) {
        if (c->checked[i]) {
            n++;
        }
    }
    return (int)(100.0 * (double)n / (double)c->item_count + 0.5);
}
