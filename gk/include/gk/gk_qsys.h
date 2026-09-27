#ifndef GK_QSYS_H
#define GK_QSYS_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_QSYS_NAME 48
#define GK_QSYS_TEXT 256
#define GK_QSYS_MAX 64

/* ===================================================================
 * Batch 55: real quality system (1421-1440)
 * Prefix: gk_qsys_
 * =================================================================== */

/* 1421-1424 standards */
typedef enum {
    GK_QSYS_ISO9001 = 0,   /* 1421 */
    GK_QSYS_IATF16949,     /* 1422 */
    GK_QSYS_AS9100,        /* 1423 */
    GK_QSYS_ISO13485       /* 1424 */
} gk_qsys_standard;

const char *gk_qsys_standard_name(gk_qsys_standard s);
int gk_qsys_standard_clauses(gk_qsys_standard s);

/* 1425-1429 documents */
typedef enum {
    GK_QSYS_DOC_MANUAL = 0,     /* 1425 quality manual */
    GK_QSYS_DOC_PROCEDURE,      /* 1426 procedure */
    GK_QSYS_DOC_WORK_INSTR,     /* 1427 work instruction */
    GK_QSYS_DOC_INSPECTION,     /* 1428 inspection spec */
    GK_QSYS_DOC_RECORD          /* 1429 record form */
} gk_qsys_doc_kind;

const char *gk_qsys_doc_name(gk_qsys_doc_kind k);

typedef struct {
    char code[GK_QSYS_NAME];
    gk_qsys_doc_kind kind;
    int revision;
    int approved;
} gk_qsys_doc;

void gk_qsys_doc_init(gk_qsys_doc *d, const char *code, gk_qsys_doc_kind k);
gk_status gk_qsys_doc_revise(gk_qsys_doc *d);
gk_status gk_qsys_doc_approve(gk_qsys_doc *d);

/* 1430-1432 audits / management review */
typedef enum {
    GK_QSYS_AUDIT_INTERNAL = 0, /* 1430 */
    GK_QSYS_AUDIT_EXTERNAL,     /* 1431 */
    GK_QSYS_AUDIT_REVIEW        /* 1432 */
} gk_qsys_audit_kind;

const char *gk_qsys_audit_name(gk_qsys_audit_kind k);

typedef struct {
    gk_qsys_audit_kind kind;
    int findings;
    int major;
    double score;
} gk_qsys_audit;

void gk_qsys_audit_init(gk_qsys_audit *a, gk_qsys_audit_kind k);
gk_status gk_qsys_audit_finding(gk_qsys_audit *a, int major);
int gk_qsys_audit_passed(const gk_qsys_audit *a, double threshold);

/* 1433-1435 corrective / preventive / improvement + 1436 8D + 1437 5Why */
typedef enum {
    GK_QSYS_ACTION_CORRECTIVE = 0, /* 1433 */
    GK_QSYS_ACTION_PREVENTIVE,     /* 1434 */
    GK_QSYS_ACTION_IMPROVEMENT     /* 1435 */
} gk_qsys_action_kind;

const char *gk_qsys_action_name(gk_qsys_action_kind k);

typedef struct {
    gk_qsys_action_kind kind;
    char description[GK_QSYS_TEXT];
    int steps_done;
    int steps_total;
    int closed;
} gk_qsys_action;

void gk_qsys_action_init(gk_qsys_action *a, gk_qsys_action_kind k,
                         const char *description, int steps_total);
gk_status gk_qsys_action_step(gk_qsys_action *a);
gk_status gk_qsys_action_close(gk_qsys_action *a);
int gk_qsys_action_complete(const gk_qsys_action *a);

/* 8D report: 8 disciplines */
typedef struct {
    int disciplines_done;
    int is_8d;
} gk_qsys_8d;

void gk_qsys_8d_init(gk_qsys_8d *r, int is_8d);
gk_status gk_qsys_8d_advance(gk_qsys_8d *r);
int gk_qsys_8d_complete(const gk_qsys_8d *r);

/* 5-Why analysis */
typedef struct {
    char chain[5][GK_QSYS_TEXT];
    int depth;
} gk_qsys_5why;

void gk_qsys_5why_init(gk_qsys_5why *w);
gk_status gk_qsys_5why_add(gk_qsys_5why *w, const char *why);
const char *gk_qsys_5why_root(const gk_qsys_5why *w);

/* 1438 fishbone */
typedef enum {
    GK_QSYS_BONE_MAN = 0,
    GK_QSYS_BONE_MACHINE,
    GK_QSYS_BONE_MATERIAL,
    GK_QSYS_BONE_METHOD,
    GK_QSYS_BONE_MEASUREMENT,
    GK_QSYS_BONE_ENVIRONMENT
} gk_qsys_bone;

const char *gk_qsys_bone_name(gk_qsys_bone b);

typedef struct {
    int causes[6];
} gk_qsys_fishbone;

void gk_qsys_fishbone_init(gk_qsys_fishbone *f);
gk_status gk_qsys_fishbone_add(gk_qsys_fishbone *f, gk_qsys_bone b);
int gk_qsys_fishbone_total(const gk_qsys_fishbone *f);
gk_qsys_bone gk_qsys_fishbone_main(const gk_qsys_fishbone *f);

/* 1439 FMEA RPN + 1440 SPC */
typedef struct {
    int severity;
    int occurrence;
    int detection;
} gk_qsys_fmea_item;

void gk_qsys_fmea_init(gk_qsys_fmea_item *it, int sev, int occ, int det);
int gk_qsys_rpn(const gk_qsys_fmea_item *it);
int gk_qsys_fmea_critical(const gk_qsys_fmea_item *it, int rpn_limit);

typedef struct {
    double sum;
    double sum_sq;
    int n;
    double usl;
    double lsl;
} gk_qsys_spc;

void gk_qsys_spc_init(gk_qsys_spc *s, double usl, double lsl);
gk_status gk_qsys_spc_add(gk_qsys_spc *s, double value);
double gk_qsys_spc_mean(const gk_qsys_spc *s);
double gk_qsys_spc_stddev(const gk_qsys_spc *s);
double gk_qsys_spc_cpk(const gk_qsys_spc *s);

#ifdef __cplusplus
}
#endif

#endif /* GK_QSYS_H */
