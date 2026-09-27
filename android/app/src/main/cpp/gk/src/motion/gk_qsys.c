#include "gk/gk_qsys.h"

#include <math.h>
#include <string.h>

static void gk__qsys_copy(char *dst, size_t cap, const char *src)
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
 * Standards (1421-1424)
 * =================================================================== */

const char *gk_qsys_standard_name(gk_qsys_standard s)
{
    switch (s) {
    case GK_QSYS_ISO9001: return "ISO9001";
    case GK_QSYS_IATF16949: return "IATF16949";
    case GK_QSYS_AS9100: return "AS9100";
    case GK_QSYS_ISO13485: return "ISO13485";
    default: return "unknown";
    }
}

int gk_qsys_standard_clauses(gk_qsys_standard s)
{
    switch (s) {
    case GK_QSYS_ISO9001: return 10;
    case GK_QSYS_IATF16949: return 12;
    case GK_QSYS_AS9100: return 15;
    case GK_QSYS_ISO13485: return 11;
    default: return 0;
    }
}

/* ===================================================================
 * Documents (1425-1429)
 * =================================================================== */

const char *gk_qsys_doc_name(gk_qsys_doc_kind k)
{
    switch (k) {
    case GK_QSYS_DOC_MANUAL: return "quality-manual";
    case GK_QSYS_DOC_PROCEDURE: return "procedure";
    case GK_QSYS_DOC_WORK_INSTR: return "work-instruction";
    case GK_QSYS_DOC_INSPECTION: return "inspection-spec";
    case GK_QSYS_DOC_RECORD: return "record-form";
    default: return "unknown";
    }
}

void gk_qsys_doc_init(gk_qsys_doc *d, const char *code, gk_qsys_doc_kind k)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    gk__qsys_copy(d->code, sizeof(d->code), code);
    d->kind = k;
    d->revision = 1;
}

gk_status gk_qsys_doc_revise(gk_qsys_doc *d)
{
    if (d == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    d->revision++;
    d->approved = 0;
    return GK_OK;
}

gk_status gk_qsys_doc_approve(gk_qsys_doc *d)
{
    if (d == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    d->approved = 1;
    return GK_OK;
}

/* ===================================================================
 * Audits (1430-1432)
 * =================================================================== */

const char *gk_qsys_audit_name(gk_qsys_audit_kind k)
{
    switch (k) {
    case GK_QSYS_AUDIT_INTERNAL: return "internal-audit";
    case GK_QSYS_AUDIT_EXTERNAL: return "external-audit";
    case GK_QSYS_AUDIT_REVIEW: return "management-review";
    default: return "unknown";
    }
}

void gk_qsys_audit_init(gk_qsys_audit *a, gk_qsys_audit_kind k)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    a->kind = k;
    a->score = 100.0;
}

gk_status gk_qsys_audit_finding(gk_qsys_audit *a, int major)
{
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    a->findings++;
    if (major) {
        a->major++;
        a->score -= 10.0;
    } else {
        a->score -= 2.0;
    }
    if (a->score < 0.0) {
        a->score = 0.0;
    }
    return GK_OK;
}

int gk_qsys_audit_passed(const gk_qsys_audit *a, double threshold)
{
    if (a == NULL) {
        return 0;
    }
    return a->score >= threshold && a->major == 0;
}

/* ===================================================================
 * Actions (1433-1435)
 * =================================================================== */

const char *gk_qsys_action_name(gk_qsys_action_kind k)
{
    switch (k) {
    case GK_QSYS_ACTION_CORRECTIVE: return "corrective";
    case GK_QSYS_ACTION_PREVENTIVE: return "preventive";
    case GK_QSYS_ACTION_IMPROVEMENT: return "improvement";
    default: return "unknown";
    }
}

void gk_qsys_action_init(gk_qsys_action *a, gk_qsys_action_kind k,
                         const char *description, int steps_total)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    a->kind = k;
    gk__qsys_copy(a->description, sizeof(a->description), description);
    a->steps_total = steps_total > 0 ? steps_total : 1;
}

gk_status gk_qsys_action_step(gk_qsys_action *a)
{
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (a->closed || a->steps_done >= a->steps_total) {
        return GK_ERR_STATE;
    }
    a->steps_done++;
    return GK_OK;
}

gk_status gk_qsys_action_close(gk_qsys_action *a)
{
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (a->steps_done < a->steps_total) {
        return GK_ERR_STATE;
    }
    a->closed = 1;
    return GK_OK;
}

int gk_qsys_action_complete(const gk_qsys_action *a)
{
    if (a == NULL) {
        return 0;
    }
    return a->closed;
}

/* ===================================================================
 * 8D / 5Why (1436-1437)
 * =================================================================== */

void gk_qsys_8d_init(gk_qsys_8d *r, int is_8d)
{
    if (r == NULL) {
        return;
    }
    memset(r, 0, sizeof(*r));
    r->is_8d = is_8d ? 1 : 0;
}

gk_status gk_qsys_8d_advance(gk_qsys_8d *r)
{
    if (r == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (r->disciplines_done >= 8) {
        return GK_ERR_STATE;
    }
    r->disciplines_done++;
    return GK_OK;
}

int gk_qsys_8d_complete(const gk_qsys_8d *r)
{
    if (r == NULL) {
        return 0;
    }
    return r->disciplines_done >= 8;
}

void gk_qsys_5why_init(gk_qsys_5why *w)
{
    if (w == NULL) {
        return;
    }
    memset(w, 0, sizeof(*w));
}

gk_status gk_qsys_5why_add(gk_qsys_5why *w, const char *why)
{
    if (w == NULL || why == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (w->depth >= 5) {
        return GK_ERR_OVERFLOW;
    }
    gk__qsys_copy(w->chain[w->depth], GK_QSYS_TEXT, why);
    w->depth++;
    return GK_OK;
}

const char *gk_qsys_5why_root(const gk_qsys_5why *w)
{
    if (w == NULL || w->depth == 0) {
        return "";
    }
    return w->chain[w->depth - 1];
}

/* ===================================================================
 * Fishbone (1438)
 * =================================================================== */

const char *gk_qsys_bone_name(gk_qsys_bone b)
{
    switch (b) {
    case GK_QSYS_BONE_MAN: return "man";
    case GK_QSYS_BONE_MACHINE: return "machine";
    case GK_QSYS_BONE_MATERIAL: return "material";
    case GK_QSYS_BONE_METHOD: return "method";
    case GK_QSYS_BONE_MEASUREMENT: return "measurement";
    case GK_QSYS_BONE_ENVIRONMENT: return "environment";
    default: return "unknown";
    }
}

void gk_qsys_fishbone_init(gk_qsys_fishbone *f)
{
    if (f == NULL) {
        return;
    }
    memset(f, 0, sizeof(*f));
}

gk_status gk_qsys_fishbone_add(gk_qsys_fishbone *f, gk_qsys_bone b)
{
    if (f == NULL || (int)b < 0 || (int)b > 5) {
        return GK_ERR_OUT_OF_RANGE;
    }
    f->causes[(int)b]++;
    return GK_OK;
}

int gk_qsys_fishbone_total(const gk_qsys_fishbone *f)
{
    int i, s = 0;
    if (f == NULL) {
        return 0;
    }
    for (i = 0; i < 6; i++) {
        s += f->causes[i];
    }
    return s;
}

gk_qsys_bone gk_qsys_fishbone_main(const gk_qsys_fishbone *f)
{
    int i, best = 0;
    if (f == NULL) {
        return GK_QSYS_BONE_MAN;
    }
    for (i = 1; i < 6; i++) {
        if (f->causes[i] > f->causes[best]) {
            best = i;
        }
    }
    return (gk_qsys_bone)best;
}

/* ===================================================================
 * FMEA (1439)
 * =================================================================== */

void gk_qsys_fmea_init(gk_qsys_fmea_item *it, int sev, int occ, int det)
{
    if (it == NULL) {
        return;
    }
    it->severity = sev;
    it->occurrence = occ;
    it->detection = det;
}

int gk_qsys_rpn(const gk_qsys_fmea_item *it)
{
    if (it == NULL) {
        return 0;
    }
    return it->severity * it->occurrence * it->detection;
}

int gk_qsys_fmea_critical(const gk_qsys_fmea_item *it, int rpn_limit)
{
    if (it == NULL) {
        return 0;
    }
    return gk_qsys_rpn(it) >= rpn_limit || it->severity >= 9;
}

/* ===================================================================
 * SPC (1440)
 * =================================================================== */

void gk_qsys_spc_init(gk_qsys_spc *s, double usl, double lsl)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->usl = usl;
    s->lsl = lsl;
}

gk_status gk_qsys_spc_add(gk_qsys_spc *s, double value)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    s->sum += value;
    s->sum_sq += value * value;
    s->n++;
    return GK_OK;
}

double gk_qsys_spc_mean(const gk_qsys_spc *s)
{
    if (s == NULL || s->n == 0) {
        return 0.0;
    }
    return s->sum / (double)s->n;
}

double gk_qsys_spc_stddev(const gk_qsys_spc *s)
{
    double mean, var;
    if (s == NULL || s->n < 2) {
        return 0.0;
    }
    mean = gk_qsys_spc_mean(s);
    var = (s->sum_sq - (double)s->n * mean * mean) / (double)(s->n - 1);
    if (var < 0.0) {
        var = 0.0;
    }
    return sqrt(var);
}

double gk_qsys_spc_cpk(const gk_qsys_spc *s)
{
    double mean, sd, cpu, cpl;
    if (s == NULL) {
        return 0.0;
    }
    sd = gk_qsys_spc_stddev(s);
    if (sd <= 0.0) {
        return 0.0;
    }
    mean = gk_qsys_spc_mean(s);
    cpu = (s->usl - mean) / (3.0 * sd);
    cpl = (mean - s->lsl) / (3.0 * sd);
    return cpu < cpl ? cpu : cpl;
}
