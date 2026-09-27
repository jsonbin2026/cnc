#include "gk/gk_process.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void gk__copy(char *dst, size_t len, const char *src)
{
    size_t i;
    if (dst == NULL || len == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    for (i = 0; i + 1 < len && src[i] != '\0'; ++i) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

/* ---- stage names ---- */

const char *gk_stage_name(gk_stage_kind k)
{
    switch (k) {
    case GK_STAGE_INCOMING_INSPECTION: return "incoming-inspection";
    case GK_STAGE_BLANK_PREP: return "blank-preparation";
    case GK_STAGE_SETUP_ALIGN: return "setup-and-alignment";
    case GK_STAGE_TOOL_SETTING: return "tool-setting";
    case GK_STAGE_FIRST_CUT: return "first-article-cut";
    case GK_STAGE_FIRST_INSPECTION: return "first-article-inspection";
    case GK_STAGE_BATCH_MACHINING: return "batch-machining";
    case GK_STAGE_ONLINE_MEASURE: return "online-measurement";
    case GK_STAGE_TOOL_CHANGE: return "tool-change";
    case GK_STAGE_CLEANING: return "cleaning";
    case GK_STAGE_DEBURRING: return "deburring";
    case GK_STAGE_FINAL_INSPECTION: return "final-inspection";
    case GK_STAGE_PACKAGING: return "packaging";
    case GK_STAGE_WAREHOUSING: return "warehousing";
    case GK_STAGE_SCRAP_DECISION: return "scrap-decision";
    case GK_STAGE_REWORK: return "rework";
    case GK_STAGE_TRACEABILITY: return "traceability";
    case GK_STAGE_QR_MARKING: return "qr-marking";
    case GK_STAGE_LIFECYCLE: return "lifecycle-management";
    default: return "unknown";
    }
}

const char *gk_stage_status_name(gk_stage_status s)
{
    switch (s) {
    case GK_STAGE_PENDING: return "pending";
    case GK_STAGE_ACTIVE: return "active";
    case GK_STAGE_DONE: return "done";
    case GK_STAGE_FAILED: return "failed";
    case GK_STAGE_SKIPPED: return "skipped";
    default: return "unknown";
    }
}

/* ---- inspection ---- */

void gk_inspection_init(gk_inspection *i, double nominal, double tolerance)
{
    if (i == NULL) {
        return;
    }
    memset(i, 0, sizeof(*i));
    i->nominal = nominal;
    i->tolerance = tolerance;
    i->measured = nominal;
}

int gk_inspection_pass(const gk_inspection *i)
{
    if (i == NULL) {
        return 0;
    }
    return fabs(i->measured - i->nominal) <= i->tolerance;
}

double gk_inspection_deviation(const gk_inspection *i)
{
    if (i == NULL) {
        return 0.0;
    }
    return i->measured - i->nominal;
}

/* ---- blank ---- */

void gk_blank_init(gk_blank *b, double l, double w, double h, double allowance)
{
    if (b == NULL) {
        return;
    }
    memset(b, 0, sizeof(*b));
    b->length = l;
    b->width = w;
    b->height = h;
    b->allowance = allowance;
}

double gk_blank_volume(const gk_blank *b)
{
    if (b == NULL) {
        return 0.0;
    }
    return b->length * b->width * b->height;
}

int gk_blank_covers(const gk_blank *b, double fnl, double fnw, double fnh)
{
    if (b == NULL) {
        return 0;
    }
    return b->length >= fnl + 2.0 * b->allowance &&
           b->width >= fnw + 2.0 * b->allowance &&
           b->height >= fnh + 2.0 * b->allowance;
}

/* ---- setup ---- */

void gk_setup_init(gk_setup *s)
{
    if (s != NULL) {
        memset(s, 0, sizeof(*s));
    }
}

gk_status gk_setup_align(gk_setup *s, double ox, double oy, double oz,
                         double runout)
{
    if (s == NULL || runout < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    s->offset_x = ox;
    s->offset_y = oy;
    s->offset_z = oz;
    s->runout = runout;
    s->aligned = 1;
    return GK_OK;
}

int gk_setup_within_tolerance(const gk_setup *s, double runout_limit)
{
    if (s == NULL || !s->aligned) {
        return 0;
    }
    return s->runout <= runout_limit;
}

/* ---- tool setting ---- */

void gk_tool_set_init(gk_tool_set *t, int tool_id)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->tool_id = tool_id;
}

gk_status gk_tool_set_probe(gk_tool_set *t, double length, double radius)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    t->length_offset = length;
    t->radius_offset = radius;
    t->measured = 1;
    return GK_OK;
}

/* ---- first article ---- */

void gk_first_article_init(gk_first_article *fa)
{
    if (fa == NULL) {
        return;
    }
    memset(fa, 0, sizeof(*fa));
    gk_inspection_init(&fa->check, 0.0, 0.0);
}

gk_status gk_first_article_evaluate(gk_first_article *fa, double nominal,
                                    double tolerance, double measured)
{
    if (fa == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!fa->first_cut_done) {
        return GK_ERR_STATE;
    }
    fa->check.nominal = nominal;
    fa->check.tolerance = tolerance;
    fa->check.measured = measured;
    fa->approved = gk_inspection_pass(&fa->check);
    return GK_OK;
}

/* ---- batch ---- */

void gk_batch_init(gk_batch *b, int target)
{
    if (b == NULL) {
        return;
    }
    memset(b, 0, sizeof(*b));
    b->target = target;
}

gk_status gk_batch_record(gk_batch *b, int pass)
{
    if (b == NULL || b->target <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    if (b->produced >= b->target) {
        return GK_ERR_OUT_OF_RANGE;
    }
    b->produced++;
    if (pass) {
        b->good++;
    } else {
        b->scrap++;
    }
    return GK_OK;
}

double gk_batch_yield(const gk_batch *b)
{
    if (b == NULL || b->produced == 0) {
        return 0.0;
    }
    return (double)b->good / (double)b->produced;
}

int gk_batch_remaining(const gk_batch *b)
{
    if (b == NULL) {
        return 0;
    }
    return b->target - b->produced;
}

/* ---- tool life ---- */

void gk_proc_tool_init(gk_proc_tool *t, int tool_id, double life_limit)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->tool_id = tool_id;
    t->life_limit = life_limit;
}

gk_status gk_proc_tool_use(gk_proc_tool *t, double amount)
{
    if (t == NULL || amount < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    t->life_used += amount;
    t->needs_change = gk_proc_tool_expired(t);
    return GK_OK;
}

int gk_proc_tool_expired(const gk_proc_tool *t)
{
    if (t == NULL || t->life_limit <= 0.0) {
        return 0;
    }
    return t->life_used >= t->life_limit;
}

/* ---- finishing ---- */

void gk_finish_init(gk_finish *f)
{
    if (f != NULL) {
        memset(f, 0, sizeof(*f));
    }
}

gk_status gk_finish_clean(gk_finish *f, double cleanliness_mg)
{
    if (f == NULL || cleanliness_mg < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    f->cleanliness_mg = cleanliness_mg;
    f->cleaned = 1;
    return GK_OK;
}

gk_status gk_finish_deburr(gk_finish *f, double edge_radius)
{
    if (f == NULL || edge_radius < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    f->edge_radius = edge_radius;
    f->deburred = 1;
    return GK_OK;
}

int gk_finish_ok(const gk_finish *f, double max_cleanliness, double min_radius)
{
    if (f == NULL) {
        return 0;
    }
    return f->cleaned && f->deburred &&
           f->cleanliness_mg <= max_cleanliness &&
           f->edge_radius >= min_radius;
}

/* ---- packaging ---- */

void gk_package_init(gk_package *p)
{
    if (p != NULL) {
        memset(p, 0, sizeof(*p));
    }
}

gk_status gk_package_seal(gk_package *p, const char *id, double weight)
{
    if (p == NULL || id == NULL || weight < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    gk__copy(p->package_id, sizeof(p->package_id), id);
    p->weight_kg = weight;
    p->sealed = 1;
    p->labeled = 1;
    return GK_OK;
}

/* ---- disposition ---- */

const char *gk_disposition_name(gk_disposition d)
{
    switch (d) {
    case GK_DISPOSITION_PASS: return "pass";
    case GK_DISPOSITION_REWORK: return "rework";
    case GK_DISPOSITION_SCRAP: return "scrap";
    default: return "unknown";
    }
}

gk_disposition gk_disposition_decide(double error, double tolerance,
                                     double rework_margin)
{
    double a = fabs(error);
    if (a <= tolerance) {
        return GK_DISPOSITION_PASS;
    }
    if (rework_margin > 0.0 && a <= tolerance + rework_margin) {
        return GK_DISPOSITION_REWORK;
    }
    return GK_DISPOSITION_SCRAP;
}

int gk_rework_schedule(int defective_qty, double rework_time,
                       double *out_total_time)
{
    if (defective_qty < 0 || rework_time < 0.0) {
        return 0;
    }
    if (out_total_time != NULL) {
        *out_total_time = (double)defective_qty * rework_time;
    }
    return defective_qty;
}

/* ---- warehouse ---- */

void gk_warehouse_init(gk_warehouse *w)
{
    if (w != NULL) {
        memset(w, 0, sizeof(*w));
    }
}

gk_status gk_warehouse_store(gk_warehouse *w, const char *location, int qty,
                             double weight)
{
    if (w == NULL || location == NULL || qty < 0 || weight < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    gk__copy(w->location, sizeof(w->location), location);
    w->quantity = qty;
    w->total_weight = weight;
    return GK_OK;
}

/* ---- traceability ---- */

void gk_trace_log_init(gk_trace_log *l)
{
    if (l != NULL) {
        memset(l, 0, sizeof(*l));
    }
}

int gk_trace_add(gk_trace_log *l, const char *serial, const char *part,
                 const char *batch, double timestamp, const char *op)
{
    gk_trace_record *r;
    if (l == NULL || serial == NULL || l->count >= GK_PROC_MAX_PARTS) {
        return -1;
    }
    r = &l->records[l->count];
    memset(r, 0, sizeof(*r));
    gk__copy(r->serial, sizeof(r->serial), serial);
    gk__copy(r->part, sizeof(r->part), part);
    gk__copy(r->batch, sizeof(r->batch), batch);
    gk__copy(r->operator_name, sizeof(r->operator_name), op);
    r->timestamp = timestamp;
    l->count++;
    return l->count;
}

const gk_trace_record *gk_trace_find(const gk_trace_log *l,
                                     const char *serial)
{
    int i;
    if (l == NULL || serial == NULL) {
        return NULL;
    }
    for (i = 0; i < l->count; ++i) {
        if (strcmp(l->records[i].serial, serial) == 0) {
            return &l->records[i];
        }
    }
    return NULL;
}

int gk_trace_count_batch(const gk_trace_log *l, const char *batch)
{
    int i;
    int n = 0;
    if (l == NULL || batch == NULL) {
        return 0;
    }
    for (i = 0; i < l->count; ++i) {
        if (strcmp(l->records[i].batch, batch) == 0) n++;
    }
    return n;
}

/* ---- QR ---- */

int gk_qr_checksum(const char *payload)
{
    int sum = 0;
    size_t i;
    if (payload == NULL) {
        return 0;
    }
    for (i = 0; payload[i] != '\0'; ++i) {
        sum = (sum * 31 + (unsigned char)payload[i]) % 100000;
    }
    return sum;
}

int gk_qr_encode(const gk_trace_record *r, char *buf, size_t len)
{
    char body[GK_PROC_NOTE * 2];
    int csum;
    int n;
    if (r == NULL || buf == NULL || len == 0) {
        return 0;
    }
    n = snprintf(body, sizeof(body), "GK|%s|%s|%s|%.1f",
                 r->part, r->batch, r->serial, r->timestamp);
    if (n < 0) {
        return 0;
    }
    csum = gk_qr_checksum(body);
    return snprintf(buf, len, "%s|%05d", body, csum);
}

gk_status gk_qr_decode(const char *payload, gk_trace_record *out)
{
    char work[GK_PROC_NOTE * 2];
    char *p;
    char *fields[6];
    int nf = 0;
    char *last;
    if (payload == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    gk__copy(work, sizeof(work), payload);
    last = strrchr(work, '|');
    if (last == NULL) {
        return GK_ERR_PARSE;
    }
    /* verify checksum */
    {
        char body[GK_PROC_NOTE * 2];
        int given;
        size_t blen = (size_t)(last - work);
        memcpy(body, work, blen);
        body[blen] = '\0';
        given = atoi(last + 1);
        if (given != gk_qr_checksum(body)) {
            return GK_ERR_PARSE;
        }
    }
    *last = '\0';
    p = strtok(work, "|");
    while (p != NULL && nf < 5) {
        fields[nf++] = p;
        p = strtok(NULL, "|");
    }
    if (nf < 5 || strcmp(fields[0], "GK") != 0) {
        return GK_ERR_PARSE;
    }
    memset(out, 0, sizeof(*out));
    gk__copy(out->part, sizeof(out->part), fields[1]);
    gk__copy(out->batch, sizeof(out->batch), fields[2]);
    gk__copy(out->serial, sizeof(out->serial), fields[3]);
    out->timestamp = atof(fields[4]);
    return GK_OK;
}

/* ---- lifecycle ---- */

void gk_lifecycle_init(gk_lifecycle *l, const char *serial, int total_uses)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
    gk__copy(l->serial, sizeof(l->serial), serial);
    l->total_uses = total_uses;
    l->remaining_uses = total_uses;
}

gk_status gk_lifecycle_consume(gk_lifecycle *l, double hours)
{
    if (l == NULL || hours < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    if (l->remaining_uses <= 0) {
        return GK_ERR_STATE;
    }
    l->remaining_uses--;
    l->accumulated_hours += hours;
    return GK_OK;
}

int gk_lifecycle_should_retire(const gk_lifecycle *l)
{
    if (l == NULL) {
        return 1;
    }
    return l->remaining_uses <= 0;
}

/* ---- process pipeline ---- */

void gk_proc_pipeline_init(gk_proc_pipeline *p)
{
    if (p != NULL) {
        memset(p, 0, sizeof(*p));
    }
}

gk_status gk_proc_pipeline_add(gk_proc_pipeline *p, gk_stage_kind stage)
{
    gk_process_step *s;
    if (p == NULL || stage < 0 || stage >= GK_STAGE_COUNT) {
        return GK_ERR_INVALID_ARG;
    }
    if (p->count >= GK_PROC_MAX_STEPS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    s = &p->steps[p->count];
    memset(s, 0, sizeof(*s));
    s->stage = stage;
    s->status = GK_STAGE_PENDING;
    p->count++;
    return GK_OK;
}

gk_status gk_proc_pipeline_advance(gk_proc_pipeline *p)
{
    if (p == NULL || p->count == 0) {
        return GK_ERR_STATE;
    }
    if (p->current >= p->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    p->steps[p->current].status = GK_STAGE_DONE;
    p->current++;
    return GK_OK;
}

gk_status gk_proc_pipeline_fail(gk_proc_pipeline *p, const char *note)
{
    if (p == NULL || p->current >= p->count) {
        return GK_ERR_STATE;
    }
    p->steps[p->current].status = GK_STAGE_FAILED;
    if (note != NULL) {
        gk__copy(p->steps[p->current].note,
                 sizeof(p->steps[p->current].note), note);
    }
    return GK_OK;
}

gk_status gk_proc_pipeline_complete(gk_proc_pipeline *p)
{
    int i;
    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < p->count; ++i) {
        if (p->steps[i].status == GK_STAGE_PENDING ||
            p->steps[i].status == GK_STAGE_ACTIVE) {
            p->steps[i].status = GK_STAGE_DONE;
        }
    }
    p->current = p->count;
    return GK_OK;
}

int gk_proc_pipeline_is_complete(const gk_proc_pipeline *p)
{
    if (p == NULL || p->count == 0) {
        return 0;
    }
    return p->current >= p->count;
}

int gk_proc_pipeline_done_count(const gk_proc_pipeline *p)
{
    int i;
    int n = 0;
    if (p == NULL) {
        return 0;
    }
    for (i = 0; i < p->count; ++i) {
        if (p->steps[i].status == GK_STAGE_DONE) n++;
    }
    return n;
}

const gk_process_step *gk_proc_pipeline_current(const gk_proc_pipeline *p)
{
    if (p == NULL || p->current >= p->count) {
        return NULL;
    }
    return &p->steps[p->current];
}
