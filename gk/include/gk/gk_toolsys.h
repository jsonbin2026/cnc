#ifndef GK_TOOLSYS_H
#define GK_TOOLSYS_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_TOOLSYS_MAX 128
#define GK_TOOLSYS_CODE 32
#define GK_TOOLSYS_NAME 48
#define GK_TOOLSYS_ID 64

/* ===================================================================
 * Batch 50: real tool system (1331-1350)
 * Prefix: gk_toolsys_
 * =================================================================== */

/* 1331 coding / 1339 RFID identification */
typedef struct {
    char code[GK_TOOLSYS_CODE];
    char rfid[GK_TOOLSYS_CODE];
    char name[GK_TOOLSYS_NAME];
    double diameter_mm;
    double length_mm;
} gk_toolsys_id;

void gk_toolsys_id_init(gk_toolsys_id *t, const char *code, const char *name);
gk_status gk_toolsys_set_rfid(gk_toolsys_id *t, const char *rfid);
int gk_toolsys_match_rfid(const gk_toolsys_id *t, const char *rfid);

/* 1332 presetting */
typedef struct {
    double preset_length_mm;
    double preset_diameter_mm;
    double measured_length_mm;
    double measured_diameter_mm;
} gk_toolsys_preset;

void gk_toolsys_preset_init(gk_toolsys_preset *p);
gk_status gk_toolsys_preset_set(gk_toolsys_preset *p, double len, double dia);
double gk_toolsys_preset_len_error(const gk_toolsys_preset *p);
double gk_toolsys_preset_dia_error(const gk_toolsys_preset *p);
int gk_toolsys_preset_ok(const gk_toolsys_preset *p, double tol_mm);

/* 1333 assembly / 1334 dynamic balance */
typedef struct {
    int components;
    double balance_grade;
} gk_toolsys_assembly;

void gk_toolsys_assembly_init(gk_toolsys_assembly *a);
gk_status gk_toolsys_assembly_add(gk_toolsys_assembly *a,
                                  const char *component);
int gk_toolsys_balance_ok(const gk_toolsys_assembly *a);

/* 1335 runout / 1337 wear / 1338 breakage detection */
typedef struct {
    double runout_um;
    double wear_mm;
    int broken;
} gk_toolsys_condition;

void gk_toolsys_condition_init(gk_toolsys_condition *c);
gk_status gk_toolsys_set_runout(gk_toolsys_condition *c, double runout_um);
int gk_toolsys_runout_ok(const gk_toolsys_condition *c, double limit_um);
int gk_toolsys_worn(const gk_toolsys_condition *c, double limit_mm);
gk_status gk_toolsys_mark_broken(gk_toolsys_condition *c);

/* 1336 life management */
typedef struct {
    double rated_min;
    double used_min;
} gk_toolsys_life;

void gk_toolsys_life_init(gk_toolsys_life *l, double rated_min);
gk_status gk_toolsys_life_use(gk_toolsys_life *l, double minutes);
double gk_toolsys_life_remaining(const gk_toolsys_life *l);
int gk_toolsys_life_expired(const gk_toolsys_life *l);

/* 1340-1346 tool crib management / logistics */
typedef enum {
    GK_TOOLSYS_STATUS_IN_STOCK = 0, /* 1346 */
    GK_TOOLSYS_STATUS_IN_USE,       /* 1341 delivery */
    GK_TOOLSYS_STATUS_RECOVERED,    /* 1342 recovery */
    GK_TOOLSYS_STATUS_REGRIND,      /* 1343 regrinding */
    GK_TOOLSYS_STATUS_COATED        /* 1344 coating */
} gk_toolsys_status;

const char *gk_toolsys_status_name(gk_toolsys_status s);

typedef struct {
    char id[GK_TOOLSYS_ID];
    gk_toolsys_status status;
    double cost;
    double purchase_price;
    char supplier[GK_TOOLSYS_NAME];
} gk_toolsys_item;

typedef struct {
    gk_toolsys_item items[GK_TOOLSYS_MAX];
    int count;
} gk_toolsys_crib;

void gk_toolsys_crib_init(gk_toolsys_crib *c);
gk_status gk_toolsys_crib_add(gk_toolsys_crib *c, const char *id,
                              const char *supplier, double purchase_price);
int gk_toolsys_crib_count(const gk_toolsys_crib *c);
gk_toolsys_item *gk_toolsys_crib_find(gk_toolsys_crib *c, const char *id);
gk_status gk_toolsys_crib_set_status(gk_toolsys_crib *c, const char *id,
                                     gk_toolsys_status s);
int gk_toolsys_crib_count_status(const gk_toolsys_crib *c,
                                 gk_toolsys_status s);
double gk_toolsys_crib_total_value(const gk_toolsys_crib *c); /* 1345 */

/* 1347 procurement / 1348 supplier */
typedef struct {
    char id[GK_TOOLSYS_ID];
    int quantity;
    double unit_price;
    double lead_days;
    char supplier[GK_TOOLSYS_NAME];
} gk_toolsys_order;

void gk_toolsys_order_init(gk_toolsys_order *o, const char *id, int qty,
                           double unit_price, double lead_days,
                           const char *supplier);
double gk_toolsys_order_total(const gk_toolsys_order *o);

/* 1349 trial / 1350 optimization */
typedef struct {
    double tool_life_min;
    double surface_finish_ra;
    double cost_per_part;
} gk_toolsys_trial;

void gk_toolsys_trial_init(gk_toolsys_trial *t);
double gk_toolsys_trial_score(const gk_toolsys_trial *t);

#ifdef __cplusplus
}
#endif

#endif /* GK_TOOLSYS_H */
