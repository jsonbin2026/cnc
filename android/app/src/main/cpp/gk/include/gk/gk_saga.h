#ifndef GK_SAGA_H
#define GK_SAGA_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_SAGA_MAX_ITEMS 128
#define GK_SAGA_NAME 64
#define GK_SAGA_TEXT 256

/* ===================================================================
 * Part A: reverse inference (769-776)
 * =================================================================== */

/* 769 finished part -> program */
typedef struct {
    double width;
    double height;
    double depth;
    int holes;
    double slot_length;
} gk_deduce_part;

typedef struct {
    char program[GK_SAGA_TEXT];
    int operations;
    double estimated_minutes;
} gk_deduce_program;

gk_status gk_deduce_program_from_part(const gk_deduce_part *part,
                                      gk_deduce_program *out);

/* 770 sound -> machine state */
typedef enum {
    GK_DEDUCE_SOUND_IDLE = 0,
    GK_DEDUCE_SOUND_CUTTING,
    GK_DEDUCE_SOUND_CHATTER,
    GK_DEDUCE_SOUND_TOOL_WEAR,
    GK_DEDUCE_SOUND_RAPID,
    GK_DEDUCE_SOUND_COUNT
} gk_deduce_sound;

const char *gk_deduce_sound_name(gk_deduce_sound s);

typedef struct {
    gk_deduce_sound state;
    double confidence;
    int suggested_rpm;
    int suggested_feed;
} gk_deduce_acoustic;

gk_status gk_deduce_state_from_sound(double dominant_hz, double amplitude_db,
                                     gk_deduce_acoustic *out);

/* 771 curve -> parameters */
typedef struct {
    double slope;
    double intercept;
    double r_squared;
} gk_deduce_line;

gk_status gk_deduce_params_from_curve(const double *x, const double *y,
                                      int n, gk_deduce_line *out);

/* 772 surface texture -> tool */
typedef struct {
    double feed_per_tooth;
    int flutes;
    double diameter;
} gk_deduce_tool;

gk_status gk_deduce_tool_from_texture(double scallop_height,
                                      double cutter_radius,
                                      gk_deduce_tool *out);

/* 773 alarm -> operator action */
typedef struct {
    char code[16];
    char action[GK_SAGA_TEXT];
    int severity;
} gk_deduce_action;

gk_status gk_deduce_action_from_alarm(const char *alarm_code,
                                      gk_deduce_action *out);

/* 774 scrap part -> process cause */
typedef enum {
    GK_DEDUCE_CAUSE_NONE = 0,
    GK_DEDUCE_CAUSE_WRONG_TOOL,
    GK_DEDUCE_CAUSE_WORN_TOOL,
    GK_DEDUCE_CAUSE_WRONG_FEED,
    GK_DEDUCE_CAUSE_LOOSE_CLAMP,
    GK_DEDUCE_CAUSE_COUNT
} gk_deduce_cause;

const char *gk_deduce_cause_name(gk_deduce_cause c);

gk_status gk_deduce_cause_from_scrap(double dimension_error,
                                     double surface_error,
                                     gk_deduce_cause *out);

/* 775 cost -> craft */
typedef struct {
    double machine_rate;
    double labor_rate;
    double material_cost;
    double setup_minutes;
    int quantity;
} gk_deduce_craft_input;

typedef struct {
    double unit_cost;
    double per_unit_minutes;
    char recommendation[GK_SAGA_TEXT];
} gk_deduce_craft;

gk_status gk_deduce_craft_from_cost(const gk_deduce_craft_input *in,
                                    double budget_per_unit,
                                    gk_deduce_craft *out);

/* 776 toolpath -> part feature */
typedef struct {
    int contour_segments;
    int pocket_regions;
    int holes;
    double bounding_volume;
} gk_deduce_feature;

gk_status gk_deduce_feature_from_toolpath(const double *toolpath_x,
                                          const double *toolpath_y, int n,
                                          gk_deduce_feature *out);

/* ===================================================================
 * Part B: narrative & gamification (777-788)
 * =================================================================== */

/* 777 mentor-apprentice story */
typedef struct {
    char name[GK_SAGA_NAME];
    int level;
    double skill;
    double reputation;
} gk_saga_actor;

void gk_saga_actor_init(gk_saga_actor *a, const char *name);
gk_status gk_saga_mentor_teach(gk_saga_actor *mentor, gk_saga_actor *apprentice,
                               double amount);

/* 778 factory order intake */
typedef struct {
    char customer[GK_SAGA_NAME];
    int quantity;
    double unit_price;
    double deadline_days;
    int difficulty;
} gk_saga_order;

typedef struct {
    gk_saga_order orders[GK_SAGA_MAX_ITEMS];
    int count;
    double cash;
} gk_saga_factory;

void gk_saga_factory_init(gk_saga_factory *f, double starting_cash);
int gk_saga_factory_intake(gk_saga_factory *f, const gk_saga_order *order);
gk_status gk_saga_factory_fulfil(gk_saga_factory *f, int order_id);

/* 779 incident review */
typedef struct {
    char title[GK_SAGA_NAME];
    char root_cause[GK_SAGA_TEXT];
    char lesson[GK_SAGA_TEXT];
    int preventable;
} gk_saga_incident;

void gk_saga_incident_init(gk_saga_incident *i, const char *title);
gk_status gk_saga_incident_review(gk_saga_incident *i, const char *root_cause,
                                  const char *lesson);

/* 780 technical breakthrough */
typedef struct {
    char topic[GK_SAGA_NAME];
    double progress;      /* 0..1 */
    int attempts;
    int solved;
} gk_saga_challenge;

void gk_saga_challenge_init(gk_saga_challenge *c, const char *topic);
gk_status gk_saga_challenge_attempt(gk_saga_challenge *c, double effort);

/* 781 startup mode */
typedef struct {
    double capital;
    double revenue;
    int employees;
    int months_active;
    int bankrupt;
} gk_saga_startup;

void gk_saga_startup_init(gk_saga_startup *s, double capital);
gk_status gk_saga_startup_simulate_month(gk_saga_startup *s, double income,
                                         double expense);

/* 782 branching plot */
typedef struct {
    int id;
    char text[GK_SAGA_TEXT];
    int next_a;
    int next_b;
    int terminal;
} gk_saga_branch;

typedef struct {
    gk_saga_branch nodes[GK_SAGA_MAX_ITEMS];
    int count;
    int current;
    int decisions;
} gk_saga_plot;

void gk_saga_plot_init(gk_saga_plot *p);
int gk_saga_plot_add(gk_saga_plot *p, const char *text, int next_a,
                     int next_b, int terminal);
gk_status gk_saga_plot_start(gk_saga_plot *p, int node_id);
gk_status gk_saga_plot_choose(gk_saga_plot *p, int branch); /* 0 = a, 1 = b */
const gk_saga_branch *gk_saga_plot_current(const gk_saga_plot *p);

/* 783 NPC dialogue (reuses the gk_teach dialog graph semantics) */
typedef struct {
    int id;
    char speaker[GK_SAGA_NAME];
    char line[GK_SAGA_TEXT];
    int mood;
} gk_saga_npc_line;

typedef struct {
    gk_saga_npc_line lines[GK_SAGA_MAX_ITEMS];
    int count;
} gk_saga_npc_script;

void gk_saga_npc_script_init(gk_saga_npc_script *s);
int gk_saga_npc_add(gk_saga_npc_script *s, const char *speaker,
                    const char *line, int mood);
const char *gk_saga_npc_greet(const gk_saga_npc_script *s, int mood);

/* 784 immersion script / 785 first person */
typedef enum {
    GK_SAGA_VIEW_FIRST_PERSON = 0,
    GK_SAGA_VIEW_THIRD_PERSON
} gk_saga_view;

typedef struct {
    gk_saga_view view;
    double head_height;
    double fov_deg;
    int hands_visible;
    int voice_enabled;
} gk_saga_immersion;

void gk_saga_immersion_init(gk_saga_immersion *im);
gk_status gk_saga_immersion_set_view(gk_saga_immersion *im, gk_saga_view v);
/* immersion score 0..1 from view, hands and voice settings */
double gk_saga_immersion_score(const gk_saga_immersion *im);

/* 786 quest system */
typedef enum {
    GK_SAGA_QUEST_AVAILABLE = 0,
    GK_SAGA_QUEST_ACTIVE,
    GK_SAGA_QUEST_COMPLETE
} gk_saga_quest_state;

typedef struct {
    int id;
    char title[GK_SAGA_NAME];
    gk_saga_quest_state state;
    double progress;
    double target;
    int reward_points;
} gk_saga_quest;

typedef struct {
    gk_saga_quest quests[GK_SAGA_MAX_ITEMS];
    int count;
    int completed;
    int total_points;
} gk_saga_quest_log;

void gk_saga_quest_log_init(gk_saga_quest_log *l);
int gk_saga_quest_add(gk_saga_quest_log *l, const char *title, double target,
                      int reward);
gk_status gk_saga_quest_accept(gk_saga_quest_log *l, int id);
gk_status gk_saga_quest_progress(gk_saga_quest_log *l, int id, double delta);
gk_status gk_saga_quest_complete(gk_saga_quest_log *l, int id);

/* 787 achievement system */
typedef struct {
    char id[GK_SAGA_NAME];
    char name[GK_SAGA_NAME];
    char description[GK_SAGA_TEXT];
    int points;
    int unlocked;
} gk_saga_achievement;

typedef struct {
    gk_saga_achievement items[GK_SAGA_MAX_ITEMS];
    int count;
    int total_points;
} gk_saga_achievements;

void gk_saga_achievements_init(gk_saga_achievements *a);
int gk_saga_achievement_add(gk_saga_achievements *a, const char *id,
                            const char *name, const char *desc, int points);
gk_status gk_saga_achievement_unlock(gk_saga_achievements *a, const char *id);
int gk_saga_achievement_is_unlocked(const gk_saga_achievements *a,
                                    const char *id);

/* 788 title system */
typedef struct {
    char id[GK_SAGA_NAME];
    char title[GK_SAGA_NAME];
    double threshold;   /* points required */
    int granted;
} gk_saga_title_def;

typedef struct {
    gk_saga_title_def defs[GK_SAGA_MAX_ITEMS];
    int count;
    char current[GK_SAGA_NAME];
} gk_saga_titles;

void gk_saga_titles_init(gk_saga_titles *t);
int gk_saga_title_add(gk_saga_titles *t, const char *id, const char *title,
                      double threshold);
/* grant the highest title whose threshold is met by `points` */
const char *gk_saga_title_evaluate(gk_saga_titles *t, double points);

#ifdef __cplusplus
}
#endif

#endif /* GK_SAGA_H */
