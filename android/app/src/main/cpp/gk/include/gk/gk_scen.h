#ifndef GK_SCEN_H
#define GK_SCEN_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_SCEN_NAME 48

/* ===================================================================
 * Batch 46: real machining scenarios (1230-1257)
 * Prefix: gk_scen_
 * =================================================================== */

typedef enum {
    GK_SCEN_AL_HS = 0,    /* 1230 aluminium high speed */
    GK_SCEN_STEEL_HEAVY,  /* 1231 steel heavy cutting */
    GK_SCEN_STAINLESS,    /* 1232 stainless */
    GK_SCEN_TITANIUM,     /* 1233 titanium */
    GK_SCEN_SUPERALLOY,   /* 1234 high-temp alloy */
    GK_SCEN_HARDENED,     /* 1235 hardened steel */
    GK_SCEN_CAST_IRON,    /* 1236 cast iron */
    GK_SCEN_COPPER,       /* 1237 copper electrode */
    GK_SCEN_GRAPHITE,     /* 1238 graphite */
    GK_SCEN_COMPOSITE,    /* 1239 composite */
    GK_SCEN_PLASTIC,      /* 1240 plastic */
    GK_SCEN_CERAMIC,      /* 1241 ceramic */
    GK_SCEN_THIN_WALL,    /* 1242 thin wall */
    GK_SCEN_DEEP_CAVITY,  /* 1243 deep cavity */
    GK_SCEN_DEEP_HOLE,    /* 1244 deep hole */
    GK_SCEN_MICRO_HOLE,   /* 1245 micro hole */
    GK_SCEN_MIRROR,       /* 1246 mirror finish */
    GK_SCEN_HIGH_PREC,    /* 1247 high precision */
    GK_SCEN_HSM,          /* 1248 high speed machining */
    GK_SCEN_HARD_CUT,     /* 1249 hard cutting */
    GK_SCEN_DRY,          /* 1250 dry cutting */
    GK_SCEN_WET,          /* 1251 wet cutting */
    GK_SCEN_MQL,          /* 1252 minimum quantity lubrication */
    GK_SCEN_CRYO,         /* 1253 cryogenic cooling */
    GK_SCEN_HP_COOLANT,   /* 1254 high pressure cooling */
    GK_SCEN_THROUGH_COOL, /* 1255 through-spindle cooling */
    GK_SCEN_ULTRASONIC,   /* 1256 ultrasonic vibration */
    GK_SCEN_LASER         /* 1257 laser assisted */
} gk_scen_kind;

const char *gk_scen_name(gk_scen_kind k);

typedef struct {
    double surface_speed;   /* m/min */
    double feed_per_tooth;  /* mm */
    double depth_of_cut;    /* mm */
    double coolant_flow;    /* L/min */
    int teeth;
    int is_wet;
} gk_scen_params;

/* 1239/1240/1241 etc. */
gk_status gk_scen_recommend(gk_scen_kind k, gk_scen_params *out);

/* feed rate = n * fz * z ; n = vc*1000/(pi*D) */
double gk_scen_rpm(const gk_scen_params *p, double diameter_mm);
double gk_scen_feed_rate(const gk_scen_params *p, double diameter_mm);
double gk_scen_mrr(const gk_scen_params *p, double diameter_mm, double ae,
                   double ap);

/* 1249-1257 cooling strategies */
gk_status gk_scen_set_coolant(gk_scen_params *p, gk_scen_kind mode,
                              double flow);
int gk_scen_needs_coolant(gk_scen_kind k);

/* scenario difficulty / feasibility rating */
int gk_scen_difficulty(gk_scen_kind k);
double gk_scen_expected_ra(gk_scen_kind k);

#ifdef __cplusplus
}
#endif

#endif /* GK_SCEN_H */
