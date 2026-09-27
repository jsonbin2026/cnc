#ifndef GK_MSPEC_H
#define GK_MSPEC_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_MSPEC_TEXT 48

/* ===================================================================
 * Batch 58: final machine characteristics (1476-1525)
 * Prefix: gk_mspec_
 * =================================================================== */

typedef struct {
    /* 1476-1481 nameplate data */
    char model[GK_MSPEC_TEXT];      /* 1476 nameplate */
    char serial[GK_MSPEC_TEXT];     /* 1477 serial number */
    char manufacture_date[GK_MSPEC_TEXT]; /* 1478 manufacture date */
    double weight_kg;               /* 1479 weight */
    double power_kw;                /* 1480 total power */
    double length_mm;               /* 1481 dimensions */
    double width_mm;
    double height_mm;

    /* 1482-1488 travel and spindle */
    double travel_x_mm;             /* 1482 travel */
    double travel_y_mm;
    double travel_z_mm;
    double accuracy_mm;             /* 1483 accuracy */
    double repeatability_mm;        /* 1484 repeat positioning */
    double spindle_rpm_min;         /* 1485 spindle speed range */
    double spindle_rpm_max;
    double rapid_mm_min;            /* 1486 rapid feed */
    double feed_min_mm_min;         /* 1487 cutting feed range */
    double feed_max_mm_min;
    int atc_capacity;               /* 1488 tool magazine capacity */

    /* 1489-1491 max tool limits */
    double max_tool_diameter_mm;    /* 1489 */
    double max_tool_length_mm;      /* 1490 */
    double max_tool_weight_kg;      /* 1491 */

    /* 1492-1494 table */
    double table_length_mm;         /* 1492 */
    double table_width_mm;
    double table_load_kg;           /* 1493 */
    double tslot_width_mm;          /* 1494 */
    double tslot_pitch_mm;

    /* 1495-1497 spindle interface */
    char spindle_taper[GK_MSPEC_TEXT]; /* 1495 */
    double spindle_power_kw;        /* 1496 */
    double spindle_torque_nm;       /* 1497 */

    /* 1498-1501 services */
    double coolant_tank_l;          /* 1498 */
    double air_pressure_mpa;        /* 1499 */
    double voltage_v;               /* 1500 */
    double frequency_hz;            /* 1501 */

    /* 1502-1506 environment and ratings */
    double ambient_temp_min_c;      /* 1502 */
    double ambient_temp_max_c;
    double ambient_humidity_max;    /* 1503 */
    char ip_rating[GK_MSPEC_TEXT];  /* 1504 */
    double noise_db;                /* 1505 */
    double vibration_mm_s;          /* 1506 */

    /* 1507-1509 thermal */
    double thermal_balance_min;     /* 1507 */
    int has_warmup_program;         /* 1508 */
    int has_warmup_cycle;           /* 1509 */

    /* 1510-1515 acceptance / installation */
    int accuracy_inspected;         /* 1510 */
    char acceptance_standard[GK_MSPEC_TEXT]; /* 1511 */
    int installed;                  /* 1512 */
    int leveled;                    /* 1513 */
    int anchored;                   /* 1514 */
    int damped;                     /* 1515 */

    /* 1516-1525 hookups */
    int grounded;                   /* 1516 */
    int power_connected;            /* 1517 */
    int air_connected;             /* 1518 */
    int water_connected;           /* 1519 */
    int chip_conveyor_connected;    /* 1520 */
    int coolant_drain_connected;    /* 1521 */
    int smoke_exhaust_connected;    /* 1522 */
    int lighting_connected;         /* 1523 */
    int network_connected;          /* 1524 */
    int signal_tower_connected;     /* 1525 */
} gk_mspec;

void gk_mspec_init(gk_mspec *m);

/* individual setters/getters keyed by feature id (1476-1525) */
gk_status gk_mspec_set_str(gk_mspec *m, int fid, const char *value);
const char *gk_mspec_get_str(const gk_mspec *m, int fid);
gk_status gk_mspec_set_num(gk_mspec *m, int fid, double value);
double gk_mspec_get_num(const gk_mspec *m, int fid);
gk_status gk_mspec_set_flag(gk_mspec *m, int fid, int value);
int gk_mspec_get_flag(const gk_mspec *m, int fid);
const char *gk_mspec_field_name(int fid);

/* derived checks */
int gk_mspec_spindle_range_ok(const gk_mspec *m);
int gk_mspec_tool_fits(const gk_mspec *m, double dia_mm, double len_mm,
                       double weight_kg);
int gk_mspec_fully_hooked_up(const gk_mspec *m);
int gk_mspec_ready_for_use(const gk_mspec *m);

#ifdef __cplusplus
}
#endif

#endif /* GK_MSPEC_H */
