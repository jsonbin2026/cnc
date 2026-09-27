#include "gk/gk_pmgmt.h"

#include <string.h>

static void gk__pmgmt_copy(char *dst, size_t cap, const char *src)
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

const char *gk_pmgmt_order_state_name(gk_pmgmt_order_state s)
{
    switch (s) {
    case GK_PMGMT_ORDER_NEW: return "new";
    case GK_PMGMT_ORDER_SCHEDULED: return "scheduled";
    case GK_PMGMT_ORDER_RUNNING: return "running";
    case GK_PMGMT_ORDER_DONE: return "done";
    case GK_PMGMT_ORDER_LATE: return "late";
    default: return "unknown";
    }
}

void gk_pmgmt_order_init(gk_pmgmt_order *o, const char *id, const char *part,
                         int qty, double due_day)
{
    if (o == NULL) {
        return;
    }
    memset(o, 0, sizeof(*o));
    gk__pmgmt_copy(o->id, sizeof(o->id), id);
    gk__pmgmt_copy(o->part, sizeof(o->part), part);
    o->quantity = qty;
    o->due_day = due_day;
    o->state = GK_PMGMT_ORDER_NEW;
}

gk_status gk_pmgmt_order_schedule(gk_pmgmt_order *o)
{
    if (o == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (o->state != GK_PMGMT_ORDER_NEW) {
        return GK_ERR_STATE;
    }
    o->state = GK_PMGMT_ORDER_SCHEDULED;
    return GK_OK;
}

gk_status gk_pmgmt_order_start(gk_pmgmt_order *o)
{
    if (o == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (o->state != GK_PMGMT_ORDER_SCHEDULED) {
        return GK_ERR_STATE;
    }
    o->state = GK_PMGMT_ORDER_RUNNING;
    return GK_OK;
}

gk_status gk_pmgmt_order_finish(gk_pmgmt_order *o)
{
    if (o == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (o->state != GK_PMGMT_ORDER_RUNNING) {
        return GK_ERR_STATE;
    }
    o->state = GK_PMGMT_ORDER_DONE;
    return GK_OK;
}

int gk_pmgmt_order_late(const gk_pmgmt_order *o, double today)
{
    if (o == NULL) {
        return 0;
    }
    if (o->state == GK_PMGMT_ORDER_DONE) {
        return 0;
    }
    return today > o->due_day;
}

/* ===================================================================
 * Schedule (1402-1405)
 * =================================================================== */

void gk_pmgmt_schedule_init(gk_pmgmt_schedule *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
}

gk_status gk_pmgmt_schedule_add(gk_pmgmt_schedule *s, const char *order,
                                double start, double finish)
{
    int i;
    if (s == NULL || order == NULL || start < 0.0 || finish < start) {
        return GK_ERR_INVALID_ARG;
    }
    if (s->count >= GK_PMGMT_MAX) {
        return GK_ERR_OVERFLOW;
    }
    /* ensure no overlap with existing jobs */
    for (i = 0; i < s->count; i++) {
        if (!(finish <= s->start[i] || start >= s->finish[i])) {
            return GK_ERR_ALREADY_EXISTS;
        }
    }
    gk__pmgmt_copy(s->order[s->count], GK_PMGMT_ID, order);
    s->start[s->count] = start;
    s->finish[s->count] = finish;
    s->count++;
    return GK_OK;
}

double gk_pmgmt_schedule_makespan(const gk_pmgmt_schedule *s)
{
    int i;
    double end = 0.0;
    if (s == NULL) {
        return 0.0;
    }
    for (i = 0; i < s->count; i++) {
        if (s->finish[i] > end) {
            end = s->finish[i];
        }
    }
    return end;
}

int gk_pmgmt_schedule_overlaps(const gk_pmgmt_schedule *s)
{
    int i, j;
    if (s == NULL) {
        return 0;
    }
    for (i = 0; i < s->count; i++) {
        for (j = i + 1; j < s->count; j++) {
            if (!(s->finish[i] <= s->start[j] ||
                  s->start[i] >= s->finish[j])) {
                return 1;
            }
        }
    }
    return 0;
}

/* ===================================================================
 * Progress / delivery (1404-1405)
 * =================================================================== */

void gk_pmgmt_progress_init(gk_pmgmt_progress *p, int total)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->total = total;
}

gk_status gk_pmgmt_progress_set(gk_pmgmt_progress *p, int completed)
{
    if (p == NULL || completed < 0 || completed > p->total) {
        return GK_ERR_OUT_OF_RANGE;
    }
    p->completed = completed;
    return GK_OK;
}

double gk_pmgmt_progress_ratio(const gk_pmgmt_progress *p)
{
    if (p == NULL || p->total <= 0) {
        return 0.0;
    }
    return (double)p->completed / (double)p->total;
}

void gk_pmgmt_delivery_init(gk_pmgmt_delivery *d, double promised, double actual)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    d->promised_day = promised;
    d->actual_day = actual;
}

int gk_pmgmt_delivery_ontime(const gk_pmgmt_delivery *d)
{
    if (d == NULL) {
        return 0;
    }
    return d->actual_day <= d->promised_day;
}

/* ===================================================================
 * Modules (1406-1417, 1420)
 * =================================================================== */

const char *gk_pmgmt_module_name(gk_pmgmt_module m)
{
    switch (m) {
    case GK_PMGMT_MODULE_QUALITY: return "quality";
    case GK_PMGMT_MODULE_COST: return "cost";
    case GK_PMGMT_MODULE_EQUIPMENT: return "equipment";
    case GK_PMGMT_MODULE_TOOLING: return "tooling";
    case GK_PMGMT_MODULE_MATERIAL: return "material";
    case GK_PMGMT_MODULE_INVENTORY: return "inventory";
    case GK_PMGMT_MODULE_PURCHASE: return "purchase";
    case GK_PMGMT_MODULE_SUPPLIER: return "supplier";
    case GK_PMGMT_MODULE_CUSTOMER: return "customer";
    case GK_PMGMT_MODULE_AFTERSALES: return "after-sales";
    case GK_PMGMT_MODULE_REPORT: return "report";
    case GK_PMGMT_MODULE_KANBAN: return "kanban";
    case GK_PMGMT_MODULE_EXCEPTION: return "exception";
    case GK_PMGMT_MODULE_MEETING: return "meeting";
    case GK_PMGMT_MODULE_PERFORMANCE: return "performance";
    default: return "unknown";
    }
}

void gk_pmgmt_registry_init(gk_pmgmt_registry *r, gk_pmgmt_module m)
{
    if (r == NULL) {
        return;
    }
    memset(r, 0, sizeof(*r));
    r->module = m;
}

gk_status gk_pmgmt_registry_add(gk_pmgmt_registry *r, int n)
{
    if (r == NULL || n < 0) {
        return GK_ERR_INVALID_ARG;
    }
    if (r->entry_count + n > GK_PMGMT_MAX) {
        return GK_ERR_OUT_OF_RANGE;
    }
    r->entry_count += n;
    return GK_OK;
}

int gk_pmgmt_registry_count(const gk_pmgmt_registry *r)
{
    if (r == NULL) {
        return 0;
    }
    return r->entry_count;
}

/* ===================================================================
 * Exception (1418)
 * =================================================================== */

void gk_pmgmt_exception_init(gk_pmgmt_exception *e, const char *code,
                             double severity)
{
    if (e == NULL) {
        return;
    }
    memset(e, 0, sizeof(*e));
    gk__pmgmt_copy(e->code, sizeof(e->code), code);
    e->severity = severity;
}

gk_status gk_pmgmt_exception_resolve(gk_pmgmt_exception *e)
{
    if (e == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    e->resolved = 1;
    return GK_OK;
}

/* ===================================================================
 * Meeting (1419)
 * =================================================================== */

void gk_pmgmt_meeting_init(gk_pmgmt_meeting *m, const char *topic,
                           int attendees)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    gk__pmgmt_copy(m->topic, sizeof(m->topic), topic);
    m->attendees = attendees;
}

gk_status gk_pmgmt_meeting_add_decision(gk_pmgmt_meeting *m, double minutes)
{
    if (m == NULL || minutes < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    m->minutes += minutes;
    m->decisions++;
    return GK_OK;
}
