#ifndef GK_ALARM_H
#define GK_ALARM_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_ALARM_MAX_ACTIVE 64
#define GK_ALARM_MAX_HISTORY 256
#define GK_ALARM_CODE_LEN 16
#define GK_ALARM_TEXT_LEN 128

/* Alarm categories covering items 333-341. */
typedef enum {
    GK_ALARM_NONE = 0,
    GK_ALARM_OVERTRAVEL,        /* 333 OT overtravel */
    GK_ALARM_SERVO_OVERLOAD,    /* 334 servo overload */
    GK_ALARM_SPINDLE_OVERLOAD,  /* 335 spindle overload */
    GK_ALARM_TOOL_BREAKAGE,     /* 336 tool breakage */
    GK_ALARM_TOOL_LIFE,         /* 337 tool life */
    GK_ALARM_COOLANT_LOW,       /* 338 coolant low */
    GK_ALARM_AIR_PRESSURE_LOW,  /* 339 air pressure low */
    GK_ALARM_HYDRAULIC_LOW,     /* 340 hydraulic low */
    GK_ALARM_LUBE_LOW,          /* 341 lubrication low */
    GK_ALARM_SYNTAX,            /* 342 program syntax error */
    GK_ALARM_UNDEFINED_G,       /* 343 undefined G code */
    GK_ALARM_UNDEFINED_M,       /* 344 undefined M code */
    GK_ALARM_COORD_OVERTRAVEL,  /* 345 coordinate overtravel */
    GK_ALARM_COLLISION,         /* 346 collision */
    GK_ALARM_ESTOP,             /* 347 emergency stop */
    GK_ALARM_POWER_LOSS,        /* 348 power loss */
    GK_ALARM_CATEGORY_COUNT
} gk_alarm_category;

/* Severity for display and escalation. */
typedef enum {
    GK_SEVERITY_INFO = 0,
    GK_SEVERITY_WARNING,
    GK_SEVERITY_ERROR,
    GK_SEVERITY_FATAL
} gk_severity;

const char *gk_alarm_category_name(gk_alarm_category c);
const char *gk_severity_name(gk_severity s);

typedef struct {
    int id;
    gk_alarm_category category;
    gk_severity severity;
    int axis;                 /* 0-5, -1 if n/a */
    double value;             /* measured/triggering value */
    double limit;             /* threshold that was violated */
    double time;              /* seconds since program start */
    char code[GK_ALARM_CODE_LEN];
    char text[GK_ALARM_TEXT_LEN];
    int active;
} gk_alarm;

/* ---- alarm manual (349) ---- */

typedef struct {
    gk_alarm_category category;
    char code[GK_ALARM_CODE_LEN];
    char title[GK_ALARM_TEXT_LEN];
    char cause[256];
    char remedy[256];
} gk_alarm_manual_entry;

const gk_alarm_manual_entry *gk_alarm_manual_lookup(gk_alarm_category c);
int gk_alarm_manual_count(void);

/* ---- quick static constructors for the detection functions ---- */

/* 333 overtravel: returns 1 when pos exceeds axis positive or negative limit. */
int gk_alarm_check_overtravel(int axis, double pos, double limit_pos,
                              double limit_neg, gk_alarm *out);
/* 334/335 overload: load > rated * factor */
int gk_alarm_check_overload(gk_alarm_category cat, double load, double rated,
                            double factor, gk_alarm *out);
/* 336 breakage: sudden change in cutting force */
int gk_alarm_check_tool_breakage(double prev_force, double force,
                                 double ratio, gk_alarm *out);
/* 337 tool life: remaining life fraction below threshold */
int gk_alarm_check_tool_life(double remaining, double threshold,
                             int tool_no, gk_alarm *out);
/* 338-341 fluid/pressure level below threshold */
int gk_alarm_check_level(gk_alarm_category cat, double level,
                         double threshold, gk_alarm *out);
/* 345 coordinate overtravel given soft limits */
int gk_alarm_check_coord(double pos, double soft_min, double soft_max,
                         gk_alarm *out);
/* 347 emergency stop event */
int gk_alarm_estop(double time, gk_alarm *out);
/* 348 power loss event, returns number of recoverable alarms */
int gk_alarm_power_loss(gk_alarm *out);

/* ---- runtime manager: active list + history timeline (350-351) ---- */

typedef struct {
    gk_alarm active[GK_ALARM_MAX_ACTIVE];
    int active_count;
    gk_alarm history[GK_ALARM_MAX_HISTORY];
    int history_count;
    int next_id;
    int buzzer_on;      /* 355 */
    int lamp_on;        /* 356 */
    double lamp_phase;  /* blink phase in seconds */
} gk_alarm_manager;

void gk_alarm_manager_init(gk_alarm_manager *m);
/* Raise an alarm; assigns id, appends to history, activates buzzer+blink. */
gk_status gk_alarm_raise(gk_alarm_manager *m, const gk_alarm *a);
/* Acknowledge/clear an active alarm by id. */
gk_status gk_alarm_clear(gk_alarm_manager *m, int id);
void gk_alarm_clear_all(gk_alarm_manager *m);
int gk_alarm_active_count(const gk_alarm_manager *m);
gk_severity gk_alarm_max_severity(const gk_alarm_manager *m);
const gk_alarm *gk_alarm_first_active(const gk_alarm_manager *m);
const gk_alarm *gk_alarm_history_at(const gk_alarm_manager *m, int i);
/* Query the most recent alarm of a category from history (351). */
const gk_alarm *gk_alarm_find_category(const gk_alarm_manager *m,
                                       gk_alarm_category c);

/* 350 timeline: returns elapsed time span between first and last history entry */
double gk_alarm_timeline_span(const gk_alarm_manager *m);
/* ordered history index by time, monotonically increasing; returns NULL when
 * index out of range. */
const gk_alarm *gk_alarm_timeline_at(const gk_alarm_manager *m, int i);

/* 355 buzzer, 356 lamp */
void gk_alarm_update_indicators(gk_alarm_manager *m, double dt);
int gk_alarm_buzzer_on(const gk_alarm_manager *m);
int gk_alarm_lamp_on(const gk_alarm_manager *m);

/* 357 alarm code generation: returns "E" + category-based number. */
void gk_alarm_make_code(gk_alarm_category c, char *buf, size_t len);

/* ---- 342-344 program correctness checks ---- */
gk_status gk_alarm_check_syntax(const char *line, gk_alarm *out);
gk_status gk_alarm_check_gcode(int g, gk_alarm *out);
gk_status gk_alarm_check_mcode(int m, gk_alarm *out);

/* ---- 352-354 fault tree / simulation / injection ---- */

#define GK_FAULT_MAX_NODES 32

typedef struct {
    int id;
    char name[64];
    int parent;             /* -1 for root */
    double probability;     /* prior probability of this basic event */
} gk_fault_node;

typedef struct {
    gk_fault_node nodes[GK_FAULT_MAX_NODES];
    int count;
} gk_fault_tree;

gk_status gk_fault_tree_add(gk_fault_tree *t, int id, const char *name,
                            int parent, double probability);
/* Probability that the root event occurs (independent OR over root causes).
 * Uses 1 - prod(1 - p_i) over direct children of `node_id`. */
double gk_fault_tree_probability(const gk_fault_tree *t, int node_id);
/* Count reachable nodes beneath the given root. */
int gk_fault_tree_subtree_count(const gk_fault_tree *t, int node_id);

typedef enum {
    GK_FAULT_INJ_NONE = 0,
    GK_FAULT_INJ_SENSOR,      /* sensor failure */
    GK_FAULT_INJ_ACTUATOR,    /* actuator failure */
    GK_FAULT_INJ_CONTROLLER,  /* controller failure */
    GK_FAULT_INJ_POWER,       /* power failure */
    GK_FAULT_INJ_COUNT
} gk_fault_injection_kind;

const char *gk_fault_injection_name(gk_fault_injection_kind k);

typedef struct {
    gk_fault_injection_kind kind;
    gk_alarm_category category;   /* alarm it triggers */
    double start_time;
    double duration;              /* 0 = permanent */
    int active;
} gk_fault_injection;

typedef struct {
    gk_fault_injection injections[GK_FAULT_MAX_NODES];
    int count;
    int enabled;
} gk_fault_sim;

void gk_fault_sim_init(gk_fault_sim *s);
gk_status gk_fault_sim_add(gk_fault_sim *s, gk_fault_injection_kind kind,
                           double start_time, double duration);
/* Advance the simulation; any injection whose window is active produces an
 * alarm via `out`; returns the number of alarms emitted this step. */
int gk_fault_sim_step(gk_fault_sim *s, double now, gk_alarm *out, int max_out);

/* ---- 358 diagnosis suggestion ---- */

typedef struct {
    char summary[128];
    char action[256];
} gk_diagnosis;

gk_status gk_alarm_diagnose(gk_alarm_category c, gk_diagnosis *out);

#ifdef __cplusplus
}
#endif

#endif /* GK_ALARM_H */
