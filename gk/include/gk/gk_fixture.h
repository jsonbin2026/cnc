#ifndef GK_FIXTURE_H
#define GK_FIXTURE_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_FIX_NAME 48
#define GK_FIX_MAX 64

/* ===================================================================
 * Batch 51: real fixture system (1351-1370)
 * Prefix: gk_fixture_
 * =================================================================== */

typedef enum {
    GK_FIX_VISE = 0,        /* 1351 bench vise */
    GK_FIX_PARALLEL_VISE,   /* 1352 parallel-jaw vise */
    GK_FIX_3JAW_CHUCK,      /* 1353 three-jaw chuck */
    GK_FIX_4JAW_CHUCK,      /* 1354 four-jaw chuck */
    GK_FIX_COLLET,          /* 1355 spring collet */
    GK_FIX_HYDRAULIC,       /* 1356 hydraulic fixture */
    GK_FIX_PNEUMATIC,       /* 1357 pneumatic fixture */
    GK_FIX_MAGNETIC,        /* 1358 magnetic fixture */
    GK_FIX_VACUUM,          /* 1359 vacuum chuck */
    GK_FIX_SPECIAL,         /* 1360 special fixture */
    GK_FIX_MODULAR,         /* 1361 modular fixture */
    GK_FIX_FLEXIBLE,        /* 1362 flexible fixture */
    GK_FIX_ZERO_POINT,      /* 1363 zero-point locating */
    GK_FIX_QUICK_CHANGE     /* 1364 quick-change fixture */
} gk_fixture_kind;

const char *gk_fixture_name(gk_fixture_kind k);
double gk_fixture_max_clamp_force(gk_fixture_kind k); /* N */

typedef struct {
    gk_fixture_kind kind;
    double clamp_force;
    double repeatability_mm;
    int clamped;
} gk_fixture;

gk_status gk_fixture_init(gk_fixture *f, gk_fixture_kind k);
gk_status gk_fixture_clamp(gk_fixture *f, double force);
gk_status gk_fixture_unclamp(gk_fixture *f);
int gk_fixture_secure(const gk_fixture *f, double part_weight_n);

/* 1365 design / 1366 manufacture / 1367 commissioning / 1368 maintenance */
typedef struct {
    char name[GK_FIX_NAME];
    double tolerance_mm;
    int designed;
    int manufactured;
    int commissioned;
    int maintained;
} gk_fixture_lifecycle;

void gk_fixture_lifecycle_init(gk_fixture_lifecycle *l, const char *name,
                               double tolerance_mm);
gk_status gk_fixture_design(gk_fixture_lifecycle *l);
gk_status gk_fixture_manufacture(gk_fixture_lifecycle *l);
gk_status gk_fixture_commission(gk_fixture_lifecycle *l, double measured_err);
gk_status gk_fixture_maintain(gk_fixture_lifecycle *l);
int gk_fixture_ready(const gk_fixture_lifecycle *l);

/* 1369 inventory / 1370 cost */
typedef struct {
    char id[GK_FIX_NAME];
    gk_fixture_kind kind;
    double cost;
    int quantity;
} gk_fixture_stock_item;

typedef struct {
    gk_fixture_stock_item items[GK_FIX_MAX];
    int count;
} gk_fixture_stock;

void gk_fixture_stock_init(gk_fixture_stock *s);
gk_status gk_fixture_stock_add(gk_fixture_stock *s, const char *id,
                               gk_fixture_kind k, double cost, int qty);
int gk_fixture_stock_total_qty(const gk_fixture_stock *s);
double gk_fixture_stock_total_cost(const gk_fixture_stock *s);
gk_status gk_fixture_stock_consume(gk_fixture_stock *s, const char *id,
                                   int qty);

#ifdef __cplusplus
}
#endif

#endif /* GK_FIXTURE_H */
