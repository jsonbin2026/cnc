#ifndef GK_FSCEN_H
#define GK_FSCEN_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_FSCEN_NAME 48
#define GK_FSCEN_DESC 128

/* ===================================================================
 * Batch 47: real fault scenarios (1258-1290)
 * Prefix: gk_fscen_
 * =================================================================== */

typedef enum {
    GK_FSCEN_POWER_LOSS = 0, /* 1258 sudden power loss */
    GK_FSCEN_VOLTAGE_FLUX,   /* 1259 voltage fluctuation */
    GK_FSCEN_AIR_DROP,       /* 1260 air pressure drop */
    GK_FSCEN_HYD_LEAK,       /* 1261 hydraulic leak */
    GK_FSCEN_COOLANT_LEAK,   /* 1262 coolant leak */
    GK_FSCEN_LUBE_LOW,       /* 1263 lubrication shortage */
    GK_FSCEN_BELT_BREAK,     /* 1264 belt break */
    GK_FSCEN_COUPLING_LOOSE, /* 1265 coupling loose */
    GK_FSCEN_SCREW_JAM,      /* 1266 ball screw jam */
    GK_FSCEN_GUIDE_DAMAGE,   /* 1267 guide damage */
    GK_FSCEN_BEARING_DAMAGE, /* 1268 bearing damage */
    GK_FSCEN_MOTOR_OVERHEAT, /* 1269 motor overheat */
    GK_FSCEN_DRIVE_FAULT,    /* 1270 drive fault */
    GK_FSCEN_ENCODER_FAULT,  /* 1271 encoder fault */
    GK_FSCEN_LIMIT_FAULT,    /* 1272 limit switch fault */
    GK_FSCEN_ESTOP_MISTOUCH, /* 1273 estop mis-touch */
    GK_FSCEN_PROG_DELETED,   /* 1274 program deleted */
    GK_FSCEN_PARAM_CHANGED,  /* 1275 parameter mis-change */
    GK_FSCEN_TOOL_REVERSED,  /* 1276 tool reversed */
    GK_FSCEN_PART_REVERSED,  /* 1277 part reversed */
    GK_FSCEN_FIXTURE_LOOSE,  /* 1278 fixture not clamped */
    GK_FSCEN_TOOLSET_ERROR,  /* 1279 tool setting error */
    GK_FSCEN_COORD_ERROR,    /* 1280 coordinate error */
    GK_FSCEN_COMP_ERROR,     /* 1281 tool comp error */
    GK_FSCEN_PROG_ERROR,     /* 1282 program error */
    GK_FSCEN_CRASH,          /* 1283 machine crash */
    GK_FSCEN_TOOL_BREAK,     /* 1284 tool break */
    GK_FSCEN_TOOL_BURN,      /* 1285 tool burn */
    GK_FSCEN_EDGE_CHIP,      /* 1286 edge chipping */
    GK_FSCEN_PART_FLY,       /* 1287 part thrown out */
    GK_FSCEN_FIRE,           /* 1288 fire */
    GK_FSCEN_LEAKAGE,        /* 1289 electric leakage */
    GK_FSCEN_INJURY          /* 1290 personnel injury */
} gk_fscen_kind;

const char *gk_fscen_name(gk_fscen_kind k);
int gk_fscen_severity(gk_fscen_kind k);
int gk_fscen_is_safety(gk_fscen_kind k);

typedef struct {
    gk_fscen_kind kind;
    double probability;
    int detected;
    int stopped;
    char description[GK_FSCEN_DESC];
} gk_fscen_event;

/* 1258-1290 inject + react */
gk_status gk_fscen_trigger(gk_fscen_event *e, gk_fscen_kind k);
gk_status gk_fscen_detect(gk_fscen_event *e, int detected);
gk_status gk_fscen_respond(gk_fscen_event *e, int stopped);
int gk_fscen_handled(const gk_fscen_event *e);

/* recommended recovery action */
const char *gk_fscen_recovery(gk_fscen_kind k);

/* accumulate MTBF-style statistics */
typedef struct {
    double total_hours;
    int failure_count;
    double repair_hours;
} gk_fscen_stats;

void gk_fscen_stats_init(gk_fscen_stats *s);
gk_status gk_fscen_stats_add(gk_fscen_stats *s, double uptime_hours,
                             double repair_hours);
double gk_fscen_mtbf(const gk_fscen_stats *s);
double gk_fscen_mttr(const gk_fscen_stats *s);
double gk_fscen_availability(const gk_fscen_stats *s);

#ifdef __cplusplus
}
#endif

#endif /* GK_FSCEN_H */
