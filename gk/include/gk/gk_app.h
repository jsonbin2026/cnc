#ifndef GK_APP_H
#define GK_APP_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_APP_NAME 64
#define GK_APP_TEXT 2048
#define GK_APP_MAX_ITEMS 64

/* ===================================================================
 * Part A: surface detail & markings (971-980)
 * =================================================================== */

/* 971 casting texture */
typedef struct {
    double roughness_um;
    double metallic;
    double scale;
    unsigned int seed;
} gk_app_texture;

void gk_app_texture_init(gk_app_texture *t, double roughness_um,
                         double metallic);
/* procedural surface height for a UV coordinate, in micrometres */
double gk_app_texture_height(const gk_app_texture *t, double u, double v);

/* 972 sheet-metal seam */
typedef struct {
    double gap_mm;
    double depth_mm;
    int visible;
} gk_app_seam;

void gk_app_seam_init(gk_app_seam *s, double gap_mm, double depth_mm);
double gk_app_seam_area(const gk_app_seam *s, double length_mm);

/* 973 screw / bolt detail */
typedef enum {
    GK_APP_SCREW_SOCKET = 0,
    GK_APP_SCREW_HEX,
    GK_APP_SCREW_PHILLIPS
} gk_app_screw_head;

const char *gk_app_screw_name(gk_app_screw_head h);

typedef struct {
    gk_app_screw_head head;
    double diameter_mm;
    double length_mm;
    int count;
} gk_app_screw;

void gk_app_screw_init(gk_app_screw *s, gk_app_screw_head head,
                       double diameter_mm, double length_mm);
double gk_app_screw_total_length(const gk_app_screw *s);

/* 974 nameplate */
typedef struct {
    double width_mm;
    double height_mm;
    char material[GK_APP_NAME];
    int rivets;
} gk_app_nameplate;

void gk_app_nameplate_init(gk_app_nameplate *p, double width_mm,
                           double height_mm, const char *material);
double gk_app_nameplate_area(const gk_app_nameplate *p);

/* 975 serial number plate */
gk_status gk_app_serial_render(const char *serial, char *out, size_t out_cap);

/* 976 warning label */
typedef enum {
    GK_APP_WARN_ELECTRICAL = 0,
    GK_APP_WARN_HOT,
    GK_APP_WARN_CRUSH,
    GK_APP_WARN_ENTANGLE,
    GK_APP_WARN_LASER
} gk_app_warning;

const char *gk_app_warning_text(gk_app_warning w);
gk_status gk_app_warning_label(gk_app_warning w, char *out, size_t out_cap);

/* 977 instruction sticker */
typedef struct {
    int step;
    char text[GK_APP_TEXT];
} gk_app_sticker;

void gk_app_sticker_init(gk_app_sticker *s);
gk_status gk_app_sticker_add(gk_app_sticker *s, const char *step_text);
int gk_app_sticker_count(const gk_app_sticker *s);

/* 978 brand logo */
typedef struct {
    char brand[GK_APP_NAME];
    double width_mm;
    double height_mm;
    int monochrome;
} gk_app_logo;

void gk_app_logo_init(gk_app_logo *l, const char *brand);
gk_status gk_app_logo_render(const gk_app_logo *l, char *out, size_t out_cap);

/* 979 model designation */
gk_status gk_app_model_render(const char *series, int number, char *out,
                              size_t out_cap);

/* 980 manufacturing date */
gk_status gk_app_date_render(int year, int month, int day, char *out,
                             size_t out_cap);

/* ===================================================================
 * Part B: covers, routing, accessories (981-1000)
 * =================================================================== */

typedef enum {
    GK_APP_COVER_DUST = 0,       /* 981 */
    GK_APP_COVER_TELESCOPIC,     /* 982 */
    GK_APP_COVER_BELLOWS,        /* 983 */
    GK_APP_COVER_SPIRAL          /* 984 */
} gk_app_cover_kind;

const char *gk_app_cover_name(gk_app_cover_kind k);

typedef struct {
    gk_app_cover_kind kind;
    double stroke_mm;
    double section_pitch_mm;   /* bellows fold pitch */
    double closed_length_mm;
} gk_app_cover;

void gk_app_cover_init(gk_app_cover *c, gk_app_cover_kind kind,
                       double stroke_mm, double closed_length_mm);
/* required extended length to cover the full stroke */
double gk_app_cover_extended(const gk_app_cover *c);
/* number of bellows folds needed */
int gk_app_cover_folds(const gk_app_cover *c);

/* 985 drag chain */
typedef struct {
    double link_pitch_mm;
    double bend_radius_mm;
    double travel_mm;
} gk_app_dragchain;

void gk_app_dragchain_init(gk_app_dragchain *d, double link_pitch_mm,
                           double bend_radius_mm);
int gk_app_dragchain_links(const gk_app_dragchain *d, double travel_mm);
double gk_app_dragchain_length(const gk_app_dragchain *d, double travel_mm);

/* 986-988 routing (cable / air / oil) */
typedef enum {
    GK_APP_ROUTE_CABLE = 0,   /* 986 */
    GK_APP_ROUTE_AIR,         /* 987 */
    GK_APP_ROUTE_OIL          /* 988 */
} gk_app_route_kind;

const char *gk_app_route_name(gk_app_route_kind k);

typedef struct {
    gk_app_route_kind kind;
    double diameter_mm;
    double bend_radius_mm;
    double length_mm;
    int segments;
} gk_app_route;

void gk_app_route_init(gk_app_route *r, gk_app_route_kind kind,
                       double diameter_mm, double length_mm);
/* bend allowance added to the routed length */
double gk_app_route_total(const gk_app_route *r);

/* 989 hydraulic power unit */
typedef struct {
    double pressure_mpa;
    double flow_lpm;
    double tank_l;
    int running;
} gk_app_hydraulic;

void gk_app_hydraulic_init(gk_app_hydraulic *h, double pressure_mpa,
                           double flow_lpm, double tank_l);
double gk_app_hydraulic_power_kw(const gk_app_hydraulic *h);

/* 990 pneumatic FRL unit */
typedef struct {
    double pressure_mpa;
    double filter_um;
    double lubricator_ml;
    int regulator_ok;
} gk_app_pneumatic;

void gk_app_pneumatic_init(gk_app_pneumatic *p, double pressure_mpa);
int gk_app_pneumatic_ok(const gk_app_pneumatic *p);

/* 991 oil chiller */
typedef struct {
    double capacity_kw;
    double setpoint_c;
    double actual_c;
} gk_app_chiller;

void gk_app_chiller_init(gk_app_chiller *c, double capacity_kw,
                         double setpoint_c);
double gk_app_chiller_error(const gk_app_chiller *c);

/* 992-994 electrical cabinet */
typedef struct {
    double width_mm;
    double height_mm;
    double fan_cfm;
    double vent_area_cm2;
    int door_open;
    int fan_on;
} gk_app_cabinet;

void gk_app_cabinet_init(gk_app_cabinet *c, double width_mm, double height_mm);
gk_status gk_app_cabinet_door(gk_app_cabinet *c, int open);
int gk_app_cabinet_fan_needed(const gk_app_cabinet *c, double internal_temp_c,
                              double ambient_temp_c);
double gk_app_cabinet_vent_ratio(const gk_app_cabinet *c);

/* 995-998 cooling */
typedef enum {
    GK_APP_COOL_SPINDLE = 0,   /* 995 */
    GK_APP_COOL_TOOL,          /* 996 */
    GK_APP_COOL_NOZZLE,        /* 997 */
    GK_APP_COOL_INTERNAL       /* 998 */
} gk_app_cool_kind;

const char *gk_app_cool_name(gk_app_cool_kind k);

typedef struct {
    gk_app_cool_kind kind;
    double diameter_mm;
    double pressure_mpa;
    double flow_lpm;
    double angle_deg;      /* universal nozzle orientation */
} gk_app_cool;

void gk_app_cool_init(gk_app_cool *c, gk_app_cool_kind kind, double diameter_mm);
double gk_app_cool_velocity(const gk_app_cool *c);
gk_status gk_app_cool_aim(gk_app_cool *c, double angle_deg);

/* 999 work light */
typedef struct {
    double brightness_lm;
    double colour_k;
    int on;
} gk_app_light;

void gk_app_light_init(gk_app_light *l, double brightness_lm, double colour_k);
double gk_app_light_illuminance(const gk_app_light *l, double distance_m);

/* 1000 tri-colour stack light */
typedef enum {
    GK_APP_STACK_RED = 0,
    GK_APP_STACK_AMBER,
    GK_APP_STACK_GREEN,
    GK_APP_STACK_OFF
} gk_app_stack_state;

const char *gk_app_stack_name(gk_app_stack_state s);

typedef struct {
    gk_app_stack_state state;
    int buzzer;
} gk_app_stack;

void gk_app_stack_init(gk_app_stack *s);
gk_status gk_app_stack_set(gk_app_stack *s, gk_app_stack_state state);
/* map a machine alarm code to a stack-light state */
gk_app_stack_state gk_app_stack_from_alarm(int severity);

#ifdef __cplusplus
}
#endif

#endif /* GK_APP_H */
