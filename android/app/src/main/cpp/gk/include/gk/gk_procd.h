#ifndef GK_PROCD_H
#define GK_PROCD_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_PROCD_NAME 48
#define GK_PROCD_TEXT 128
#define GK_PROCD_MAX 64

/* ===================================================================
 * Batch 56: real process design (1441-1460)
 * Prefix: gk_procd_
 * =================================================================== */

/* 1441 route / 1442 operation / 1443 step */
typedef struct {
    char name[GK_PROCD_NAME];
    int operation;
    int step;
} gk_procd_operation;

typedef struct {
    char part[GK_PROCD_NAME];
    gk_procd_operation ops[GK_PROCD_MAX];
    int count;
} gk_procd_route;

void gk_procd_route_init(gk_procd_route *r, const char *part);
gk_status gk_procd_route_add(gk_procd_route *r, const char *name, int operation,
                             int step);
int gk_procd_route_count(const gk_procd_route *r);
int gk_procd_route_operations(const gk_procd_route *r);

/* 1444 machining allowance / 1445 cutting params */
typedef struct {
    double stock_mm;    /* raw allowance */
    double finish_mm;   /* final allowance */
    double semi_mm;
} gk_procd_allowance;

void gk_procd_allowance_init(gk_procd_allowance *a, double stock_mm,
                             double finish_mm);
double gk_procd_allowance_rough(const gk_procd_allowance *a);
int gk_procd_allowance_valid(const gk_procd_allowance *a);

typedef struct {
    double speed;
    double feed;
    double depth;
} gk_procd_cutting;

void gk_procd_cutting_init(gk_procd_cutting *c, double speed, double feed,
                           double depth);
double gk_procd_cutting_time(const gk_procd_cutting *c, double length_mm);

/* 1446-1449 selection: tool / fixture / gauge / machine */
typedef enum {
    GK_PROCD_SEL_TOOL = 0,   /* 1446 */
    GK_PROCD_SEL_FIXTURE,    /* 1447 */
    GK_PROCD_SEL_GAUGE,      /* 1448 */
    GK_PROCD_SEL_MACHINE     /* 1449 */
} gk_procd_selection;

const char *gk_procd_selection_name(gk_procd_selection s);

typedef struct {
    gk_procd_selection kind;
    char pick[GK_PROCD_NAME];
    char reason[GK_PROCD_TEXT];
} gk_procd_choice;

void gk_procd_choice_init(gk_procd_choice *c, gk_procd_selection k,
                          const char *pick, const char *reason);
int gk_procd_choice_ok(const gk_procd_choice *c);

/* 1450-1451 quotas: man-hour / material */
typedef struct {
    double setup_min;
    double cycle_min;
    int quantity;
} gk_procd_time_quota;

void gk_procd_time_quota_init(gk_procd_time_quota *q, double setup_min,
                              double cycle_min, int quantity);
double gk_procd_time_quota_total(const gk_procd_time_quota *q);
double gk_procd_time_quota_per_part(const gk_procd_time_quota *q);

typedef struct {
    double finished_mass_kg;
    double scrap_rate;
} gk_procd_material_quota;

void gk_procd_material_quota_init(gk_procd_material_quota *q,
                                  double finished_mass_kg, double scrap_rate);
double gk_procd_material_quota_required(const gk_procd_material_quota *q,
                                        int quantity);

/* 1452-1460 process card / review / validation / optimization / freezing /
 * change / version / knowledge base / expert system */
typedef enum {
    GK_PROCD_STAGE_CARD = 0,      /* 1452 */
    GK_PROCD_STAGE_REVIEW,        /* 1453 */
    GK_PROCD_STAGE_VALIDATE,      /* 1454 */
    GK_PROCD_STAGE_OPTIMIZE,      /* 1455 */
    GK_PROCD_STAGE_FREEZE,        /* 1456 */
    GK_PROCD_STAGE_CHANGE,        /* 1457 */
    GK_PROCD_STAGE_VERSION,       /* 1458 */
    GK_PROCD_STAGE_KNOWLEDGE,     /* 1459 */
    GK_PROCD_STAGE_EXPERT         /* 1460 */
} gk_procd_stage;

const char *gk_procd_stage_name(gk_procd_stage s);

typedef struct {
    char name[GK_PROCD_NAME];
    gk_procd_stage stage;
    int revision;
    int closed;
    char note[GK_PROCD_TEXT];
} gk_procd_plan;

void gk_procd_plan_init(gk_procd_plan *p, const char *name);
gk_status gk_procd_plan_advance(gk_procd_plan *p, gk_procd_stage stage);
gk_status gk_procd_plan_revise(gk_procd_plan *p, const char *note);
gk_status gk_procd_plan_close(gk_procd_plan *p);
int gk_procd_plan_frozen(const gk_procd_plan *p);

/* 1459 knowledge base / 1460 expert system */
typedef struct {
    char rule[GK_PROCD_MAX][GK_PROCD_TEXT];
    int count;
} gk_procd_knowledge;

void gk_procd_knowledge_init(gk_procd_knowledge *k);
gk_status gk_procd_knowledge_add(gk_procd_knowledge *k, const char *rule);
int gk_procd_knowledge_count(const gk_procd_knowledge *k);
const char *gk_procd_expert_recommend(const gk_procd_knowledge *k,
                                      const char *topic);

#ifdef __cplusplus
}
#endif

#endif /* GK_PROCD_H */
