#ifndef GK_SAFETY_H
#define GK_SAFETY_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_SAFETY_MAX_STEPS 32
#define GK_SAFETY_MAX_ITEMS 64
#define GK_SAFETY_NAME 64
#define GK_SAFETY_TEXT 256

/* ---- 676 safe operating procedure / 677 power-on / 678 power-off ---- */

typedef struct {
    int id;
    char text[GK_SAFETY_TEXT];
    int mandatory;
    int done;
} gk_safety_step;

typedef struct {
    gk_safety_step steps[GK_SAFETY_MAX_STEPS];
    int count;
    int current;
    char title[GK_SAFETY_NAME];
} gk_safety_procedure;

void gk_safety_procedure_init(gk_safety_procedure *p, const char *title);
int gk_safety_step_add(gk_safety_procedure *p, const char *text, int mandatory);
gk_status gk_safety_step_done(gk_safety_procedure *p, int id);
gk_status gk_safety_next(gk_safety_procedure *p);
int gk_safety_procedure_complete(const gk_safety_procedure *p);
int gk_safety_remaining(const gk_safety_procedure *p);

/* 677 power-on order / 678 power-off order share the ordered procedure. */
gk_status gk_safety_build_power_on(gk_safety_procedure *p);
gk_status gk_safety_build_power_off(gk_safety_procedure *p);

/* ---- 679-682 PPE ---- */

typedef enum {
    GK_PPE_GOGGLES = 0,     /* 680 护目镜 */
    GK_PPE_COVERALL,        /* 681 工作服 */
    GK_PPE_SHOES,           /* 682 安全鞋 */
    GK_PPE_GLOVES,
    GK_PPE_HEARING,
    GK_PPE_COUNT
} gk_ppe_kind;

const char *gk_ppe_name(gk_ppe_kind k);
const char *gk_ppe_requirement(gk_ppe_kind k);

typedef struct {
    int worn[GK_PPE_COUNT];
    int count;
} gk_ppe_state;

void gk_ppe_state_init(gk_ppe_state *s);
gk_status gk_ppe_wear(gk_ppe_state *s, gk_ppe_kind k, int worn);
int gk_ppe_compliant(const gk_ppe_state *s);
int gk_ppe_missing_count(const gk_ppe_state *s);

/* ---- 683 hazard zone / 684 rotating parts warning ---- */

typedef struct {
    double x, y, z;          /* centre */
    double radius;           /* danger radius, mm */
    int severity;            /* 1 low .. 3 high */
    char label[GK_SAFETY_NAME];
} gk_hazard_zone;

typedef struct {
    gk_hazard_zone zones[GK_SAFETY_MAX_ITEMS];
    int count;
} gk_hazard_map;

void gk_hazard_map_init(gk_hazard_map *m);
int gk_hazard_add(gk_hazard_map *m, double x, double y, double z, double radius,
                  int severity, const char *label);
/* returns the id+1 of the highest-severity zone containing the point, or 0 */
int gk_hazard_query(const gk_hazard_map *m, double x, double y, double z);
int gk_rotating_warning(double rpm, double tool_diameter);

/* ---- 685 emergency-stop drill / 686 e-stop locations ---- */

typedef struct {
    double x, y, z;
    char label[GK_SAFETY_NAME];
} gk_estop_location;

typedef struct {
    gk_estop_location spots[GK_SAFETY_MAX_ITEMS];
    int count;
} gk_estop_layout;

void gk_estop_layout_init(gk_estop_layout *l);
int gk_estop_add(gk_estop_layout *l, double x, double y, double z,
                 const char *label);
const gk_estop_location *gk_estop_nearest(const gk_estop_layout *l,
                                          double x, double y, double z);

typedef struct {
    double reaction_time;   /* seconds to press */
    double limit;
    int pressed;
    int passed;
} gk_estop_drill;

void gk_estop_drill_init(gk_estop_drill *d, double limit);
gk_status gk_estop_drill_press(gk_estop_drill *d, double reaction_time);

/* ---- 687 LOTO ---- */

typedef struct {
    int id;
    char tag[GK_SAFETY_NAME];
    char owner[GK_SAFETY_NAME];
    int locked;
} gk_loto_lock;

typedef struct {
    gk_loto_lock locks[GK_SAFETY_MAX_ITEMS];
    int count;
} gk_loto_board;

void gk_loto_board_init(gk_loto_board *b);
int gk_loto_apply(gk_loto_board *b, const char *tag, const char *owner);
gk_status gk_loto_remove(gk_loto_board *b, const char *tag, const char *owner);
int gk_loto_is_locked(const gk_loto_board *b, const char *tag);

/* ---- 688-693 5S management ---- */

typedef enum {
    GK_5S_SORT = 0,         /* 689 整理 */
    GK_5S_SET_IN_ORDER,     /* 690 整顿 */
    GK_5S_SHINE,            /* 691 清扫 */
    GK_5S_STANDARDIZE,      /* 692 清洁 */
    GK_5S_SUSTAIN,          /* 693 素养 */
    GK_5S_COUNT
} gk_5s_pillar;

const char *gk_5s_name(gk_5s_pillar p);

typedef struct {
    char zone[GK_SAFETY_NAME];
    int scores[GK_5S_COUNT];   /* 0..100 each */
} gk_5s_area;

typedef struct {
    gk_5s_area areas[GK_SAFETY_MAX_ITEMS];
    int count;
} gk_5s_board;

void gk_5s_board_init(gk_5s_board *b);
int gk_5s_area_add(gk_5s_board *b, const char *zone);
gk_status gk_5s_set_score(gk_5s_board *b, const char *zone, gk_5s_pillar p,
                          int score);
double gk_5s_area_score(const gk_5s_board *b, const char *zone);
double gk_5s_overall(const gk_5s_board *b);

/* ---- 694 incident replay / 695 violation points / 696 safety assessment ---- */

typedef struct {
    int id;
    char title[GK_SAFETY_NAME];
    char cause[GK_SAFETY_TEXT];
    char lesson[GK_SAFETY_TEXT];
    double severity;
} gk_incident_case;

typedef struct {
    gk_incident_case cases[GK_SAFETY_MAX_ITEMS];
    int count;
} gk_incident_library;

void gk_incident_library_init(gk_incident_library *l);
int gk_incident_add(gk_incident_library *l, const char *title, const char *cause,
                    const char *lesson, double severity);
const gk_incident_case *gk_incident_find(const gk_incident_library *l,
                                         const char *title);
/* replay returns the ordered lesson text for the case */
int gk_incident_replay(const gk_incident_case *c, char *buf, size_t len);

typedef struct {
    int user_id;
    int points;             /* violation points */
    int violations;
} gk_violation_card;

typedef struct {
    gk_violation_card cards[GK_SAFETY_MAX_ITEMS];
    int count;
} gk_violation_book;

void gk_violation_book_init(gk_violation_book *b);
gk_status gk_violation_add(gk_violation_book *b, int user_id, int points);
int gk_violation_total(const gk_violation_book *b, int user_id);
int gk_violation_disqualified(const gk_violation_book *b, int user_id,
                              int limit);

typedef struct {
    double correct;
    double total;
} gk_safety_score;

void gk_safety_score_init(gk_safety_score *s);
gk_status gk_safety_score_add(gk_safety_score *s, int correct);
double gk_safety_score_value(const gk_safety_score *s);
int gk_safety_score_pass(const gk_safety_score *s, double threshold);

/* ---- 697 safety certificate ---- */

typedef struct {
    char holder[GK_SAFETY_NAME];
    char course[GK_SAFETY_NAME];
    double score;
    double issued_at;
    int valid;
} gk_safety_cert;

void gk_safety_cert_init(gk_safety_cert *c, const char *holder,
                         const char *course);
gk_status gk_safety_cert_issue(gk_safety_cert *c, double score, double at,
                               double threshold);
int gk_safety_cert_text(const gk_safety_cert *c, char *buf, size_t len);

/* ---- 698 emergency response / 699 fire drill / 700 earth leakage ---- */

typedef struct {
    char name[GK_SAFETY_NAME];
    double response_time;
    double limit;
    int completed;
} gk_emergency_drill;

typedef struct {
    gk_emergency_drill drills[GK_SAFETY_MAX_ITEMS];
    int count;
} gk_emergency_plan;

void gk_emergency_plan_init(gk_emergency_plan *p);
int gk_emergency_add(gk_emergency_plan *p, const char *name, double limit);
gk_status gk_emergency_complete(gk_emergency_plan *p, int id,
                                double response_time);
int gk_emergency_all_pass(const gk_emergency_plan *p);

typedef struct {
    double leakage_current_ma;
    double threshold_ma;
    double trip_time_ms;
    double max_trip_ms;
    int tripped;
} gk_leakage_protector;

void gk_leakage_init(gk_leakage_protector *l, double threshold_ma,
                     double max_trip_ms);
gk_status gk_leakage_test(gk_leakage_protector *l, double current_ma,
                          double trip_ms);
int gk_leakage_pass(const gk_leakage_protector *l);

#ifdef __cplusplus
}
#endif

#endif /* GK_SAFETY_H */
