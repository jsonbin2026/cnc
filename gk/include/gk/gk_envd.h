#ifndef GK_ENVD_H
#define GK_ENVD_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_ENVD_NAME 64
#define GK_ENVD_TEXT 256

/* ===================================================================
 * Batch 44: real environment details (1176-1205)
 * Prefix: gk_envd_
 * =================================================================== */

/* 1176 workshop background */
typedef struct {
    double length_m;
    double width_m;
    double height_m;
    double ambient_c;
} gk_envd_shop;

void gk_envd_shop_init(gk_envd_shop *s, double length_m, double width_m,
                       double height_m);
double gk_envd_shop_volume(const gk_envd_shop *s);
double gk_envd_shop_floor_area(const gk_envd_shop *s);

/* 1177-1187 ambient sound sources */
typedef enum {
    GK_ENVD_SND_MACHINE = 0,   /* 1177 */
    GK_ENVD_SND_FORKLIFT,      /* 1178 */
    GK_ENVD_SND_CRANE,         /* 1179 */
    GK_ENVD_SND_PEOPLE,        /* 1180 */
    GK_ENVD_SND_BROADCAST,     /* 1181 */
    GK_ENVD_SND_VENTILATION,   /* 1182 */
    GK_ENVD_SND_AIRCON,        /* 1183 */
    GK_ENVD_SND_TRANSFORMER,   /* 1184 */
    GK_ENVD_SND_CABINET_FAN,   /* 1185 */
    GK_ENVD_SND_COOLING_TOWER, /* 1186 */
    GK_ENVD_SND_COMPRESSOR     /* 1187 */
} gk_envd_sound_kind;

const char *gk_envd_sound_name(gk_envd_sound_kind k);
double gk_envd_sound_base_db(gk_envd_sound_kind k);

typedef struct {
    double distance_m;
    double reference_m;
    double base_db;
    gk_envd_sound_kind kind;
    int enabled;
} gk_envd_sound;

void gk_envd_sound_init(gk_envd_sound *s, gk_envd_sound_kind kind,
                        double distance_m);
double gk_envd_sound_level(const gk_envd_sound *s);

/* 1188-1190 floor conditions */
typedef enum {
    GK_ENVD_FLOOR_CLEAN = 0,
    GK_ENVD_FLOOR_OIL,     /* 1188 */
    GK_ENVD_FLOOR_CHIPS,   /* 1189 */
    GK_ENVD_FLOOR_WATER    /* 1190 */
} gk_envd_floor_state;

const char *gk_envd_floor_name(gk_envd_floor_state s);

typedef struct {
    gk_envd_floor_state state;
    double coverage;
    double friction;
} gk_envd_floor;

void gk_envd_floor_init(gk_envd_floor *f);
gk_status gk_envd_floor_soil(gk_envd_floor *f, gk_envd_floor_state state,
                             double coverage);
int gk_envd_floor_slip_risk(const gk_envd_floor *f, double threshold);

/* 1191 wall sticker */
gk_status gk_envd_wall_sticker(const char *text, char *out, size_t out_cap);

/* 1192 safety aisle / 1193 fire equipment */
typedef struct {
    double width_mm;
    int marked;
    int blocked;
} gk_envd_aisle;

void gk_envd_aisle_init(gk_envd_aisle *a, double width_mm);
int gk_envd_aisle_ok(const gk_envd_aisle *a);
gk_status gk_envd_aisle_block(gk_envd_aisle *a, int blocked);

typedef struct {
    char kind[GK_ENVD_NAME];
    int count;
    int inspected;
} gk_envd_fire_equipment;

void gk_envd_fire_init(gk_envd_fire_equipment *e, const char *kind, int count);
gk_status gk_envd_fire_inspect(gk_envd_fire_equipment *e, int passed);

/* 1194-1198 storage / waste */
typedef enum {
    GK_ENVD_STORE_TOOLBOX = 0,  /* 1194 */
    GK_ENVD_STORE_MATERIAL,     /* 1195 */
    GK_ENVD_STORE_FINISHED,     /* 1196 */
    GK_ENVD_STORE_WASTE,        /* 1197 */
    GK_ENVD_STORE_CLEANING      /* 1198 */
} gk_envd_store_kind;

const char *gk_envd_store_name(gk_envd_store_kind k);

typedef struct {
    gk_envd_store_kind kind;
    int capacity;
    int used;
} gk_envd_store;

void gk_envd_store_init(gk_envd_store *s, gk_envd_store_kind kind,
                        int capacity);
int gk_envd_store_count(const gk_envd_store *s);
gk_status gk_envd_store_put(gk_envd_store *s, int items);
gk_status gk_envd_store_take(gk_envd_store *s, int items);
int gk_envd_store_full(const gk_envd_store *s);

/* 1199-1200 lighting */
typedef struct {
    int lamp_count;
    double watts_each;
    double level;
    int failed;
} gk_envd_lighting;

void gk_envd_lighting_init(gk_envd_lighting *l, int lamp_count,
                           double watts_each);
double gk_envd_lighting_power(const gk_envd_lighting *l);
gk_status gk_envd_lighting_set_level(gk_envd_lighting *l, double level);

typedef struct {
    double runtime_min;
    int on;
} gk_envd_emergency_light;

void gk_envd_emergency_light_init(gk_envd_emergency_light *e);
gk_status gk_envd_emergency_light_test(gk_envd_emergency_light *e,
                                       double duration_min);
int gk_envd_emergency_light_ok(const gk_envd_emergency_light *e);

/* 1201-1202 window / view */
typedef struct {
    double width_mm;
    double height_mm;
    double transparency;
} gk_envd_window;

void gk_envd_window_init(gk_envd_window *w, double width_mm, double height_mm);
double gk_envd_window_area(const gk_envd_window *w);
gk_status gk_envd_window_view(const char *outside, char *out, size_t out_cap);

/* 1203 shop clock */
typedef struct {
    int hour;
    int minute;
    int second;
} gk_envd_clock;

void gk_envd_clock_init(gk_envd_clock *c);
gk_status gk_envd_clock_set(gk_envd_clock *c, int hour, int minute,
                            int second);
gk_status gk_envd_clock_render(const gk_envd_clock *c, char *out,
                               size_t out_cap);

/* 1204 production board */
typedef struct {
    int planned;
    int completed;
    int target;
} gk_envd_board;

void gk_envd_board_init(gk_envd_board *b, int target);
gk_status gk_envd_board_update(gk_envd_board *b, int planned, int completed);
double gk_envd_board_achievement(const gk_envd_board *b);

/* 1205 shift handover record */
typedef struct {
    char outgoing[GK_ENVD_NAME];
    char incoming[GK_ENVD_NAME];
    char note[GK_ENVD_TEXT];
    int signed_off;
} gk_envd_handover;

void gk_envd_handover_init(gk_envd_handover *h);
gk_status gk_envd_handover_set(gk_envd_handover *h, const char *outgoing,
                               const char *incoming, const char *note);
gk_status gk_envd_handover_sign(gk_envd_handover *h);

#ifdef __cplusplus
}
#endif

#endif /* GK_ENVD_H */
