#ifndef GK_TEACH_H
#define GK_TEACH_H

#include <stddef.h>
#include "gk/gk_error.h"
#include "gk/gk_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================= course / lesson management (280-287) ================= */

#define GK_COURSE_MAX_STEPS 64
#define GK_COURSE_MAX_LESSONS 32

typedef struct {
    int id;
    char title[64];
    char body[256];
    char highlight[64];   /* UI element to highlight (283) */
    char narration[256];  /* voice-over script (284) */
    char subtitle[160];   /* on-screen subtitle (285) */
    int has_animation;    /* target demo animation (286) */
} gk_lesson_step;

typedef struct {
    int id;
    char name[64];
    char json[512];       /* raw JSON lesson definition (281) */
    gk_lesson_step steps[GK_COURSE_MAX_STEPS];
    size_t step_count;
    int unlocked;         /* level unlocked (301) */
} gk_lesson;

typedef struct {
    gk_lesson lessons[GK_COURSE_MAX_LESSONS];
    size_t lesson_count;
    size_t current_lesson;
    size_t current_step;
    int completed;
} gk_course;

void gk_course_init(gk_course *c);
gk_status gk_course_add_lesson(gk_course *c, int id, const char *name,
                               const char *json);
gk_status gk_course_add_step(gk_course *c, size_t lesson_index,
                             const gk_lesson_step *step);
gk_status gk_course_goto(gk_course *c, size_t lesson_index, size_t step);
gk_status gk_course_next_step(gk_course *c);   /* guided step (282) */
gk_status gk_course_prev_step(gk_course *c);
const gk_lesson_step *gk_course_current_step(const gk_course *c);
const char *gk_course_current_highlight(const gk_course *c);

/* learning progress record (287) */
typedef struct {
    unsigned char steps_done[GK_COURSE_MAX_LESSONS][GK_COURSE_MAX_STEPS];
    size_t lessons_done;
    size_t total_steps_done;
    double total_time;
} gk_progress;

void gk_progress_init(gk_progress *p);
gk_status gk_progress_mark(gk_progress *p, size_t lesson, size_t step,
                           double time_spent);
int gk_progress_is_done(const gk_progress *p, size_t lesson, size_t step);
double gk_progress_completion(const gk_progress *p, const gk_course *c);

/* ================= built-in manuals (288-289) ================= */

typedef struct {
    int code;
    const char *name;
    const char *description;
    int is_mcode;
} gk_manual_entry;

const gk_manual_entry *gk_manual_lookup(int code, int is_mcode);
const gk_manual_entry *gk_manual_g(int code);
const gk_manual_entry *gk_manual_m(int code);
size_t gk_manual_count(int is_mcode);

/* ================= shortcuts / scoring (290-296) ================= */

typedef struct {
    char keys[32];
    char action[64];
    size_t uses;
} gk_shortcut;

typedef struct {
    gk_shortcut items[32];
    size_t count;
} gk_shortcut_set;

void gk_shortcut_set_init(gk_shortcut_set *s);
gk_status gk_shortcut_add(gk_shortcut_set *s, const char *keys,
                          const char *action);
gk_status gk_shortcut_use(gk_shortcut_set *s, const char *keys);
size_t gk_shortcut_total_uses(const gk_shortcut_set *s);

typedef struct {
    double correctness;   /* 0..1 */
    double speed;         /* 0..1 */
    double efficiency;    /* 0..1 */
    double penalties;
    double score;         /* 0..100 */
} gk_score;

void gk_score_init(gk_score *s);
/* Recompute the weighted score (291). */
double gk_score_compute(gk_score *s);

/* wrong-answer log (293) */
typedef struct {
    int question_id;
    int chosen;
    int correct;
    char topic[64];
    size_t attempts;
} gk_wrong_answer;

#define GK_TEACH_MAX_WRONG 128

typedef struct {
    gk_wrong_answer items[GK_TEACH_MAX_WRONG];
    size_t count;
} gk_wrong_log;

void gk_wrong_log_init(gk_wrong_log *l);
gk_status gk_wrong_log_add(gk_wrong_log *l, int question_id, int chosen,
                           int correct, const char *topic);
int gk_wrong_log_repeat_count(const gk_wrong_log *l);
/* Auto-grading of a set of answers (295). */
size_t gk_auto_grade(const int *answers, const int *keys, size_t n);
/* Grade transcript export (294). */
size_t gk_transcript_export(const gk_score *score,
                            const gk_wrong_log *log, char *out,
                            size_t out_size);
/* Exam mode toggle & time limit (296). */
typedef struct {
    int exam_mode;
    double limit_seconds;
    double elapsed;
} gk_exam;

void gk_exam_init(gk_exam *e);
void gk_exam_start(gk_exam *e, double limit_seconds);
int gk_exam_time_up(const gk_exam *e);

/* ================= competition / gamification (297-306) ================= */

#define GK_LEADERBOARD_MAX 32

typedef struct {
    char name[32];
    int score;
} gk_rank_entry;

typedef struct {
    gk_rank_entry entries[GK_LEADERBOARD_MAX];
    size_t count;
} gk_leaderboard;

void gk_leaderboard_init(gk_leaderboard *b);
gk_status gk_leaderboard_submit(gk_leaderboard *b, const char *name,
                                int score);
int gk_leaderboard_rank_of(const gk_leaderboard *b, const char *name);

typedef struct {
    int points;
    int level;
    int streak;
} gk_player_points;

void gk_player_points_init(gk_player_points *p);
void gk_player_points_add(gk_player_points *p, int points);
int gk_player_level_for_points(int points);

/* badges (300) */
typedef struct {
    char id[32];
    char name[64];
    char description[128];
    int earned;
    double threshold;
} gk_badge;

#define GK_TEACH_MAX_BADGES 32

typedef struct {
    gk_badge items[GK_TEACH_MAX_BADGES];
    size_t count;
} gk_badge_set;

void gk_badge_set_init(gk_badge_set *s);
gk_status gk_badge_add(gk_badge_set *s, const char *id, const char *name,
                       const char *desc, double threshold);
size_t gk_badge_evaluate(gk_badge_set *s, double value);

/* certification certificate (298) */
typedef struct {
    char holder[64];
    char course[64];
    int score;
    int year;
    int month;
    int day;
    int valid;
} gk_certificate;

void gk_certificate_build(gk_certificate *cert, const char *holder,
                          const char *course, int score, int y, int m, int d);
size_t gk_certificate_text(const gk_certificate *cert, char *out,
                           size_t out_size);

/* challenge modes (302-306) */
typedef enum {
    GK_CHALLENGE_STAGE = 0,   /* 302 level challenge */
    GK_CHALLENGE_TIMED,       /* 303 timed challenge */
    GK_CHALLENGE_PRECISION,   /* 304 precision challenge */
    GK_CHALLENGE_EFFICIENCY,  /* 305 efficiency challenge */
    GK_CHALLENGE_TEAM         /* 306 team task */
} gk_challenge_kind;

const char *gk_challenge_name(gk_challenge_kind k);

typedef struct {
    gk_challenge_kind kind;
    double target;
    double achieved;
    double time_limit;
    double elapsed;
    int members;             /* team size (306) */
    int passed;
} gk_challenge;

gk_status gk_challenge_init(gk_challenge *c, gk_challenge_kind kind,
                            double target, double time_limit);
gk_status gk_challenge_update(gk_challenge *c, double achieved, double dt);
int gk_challenge_passed(const gk_challenge *c);

/* ================= narrative / role play (307-315) ================= */

typedef struct {
    int id;
    char speaker[32];
    char text[256];
    int next_id;
    int branch_a;    /* 311 branching: choice A -> node id */
    int branch_b;
} gk_dialog_node;

#define GK_TEACH_MAX_DIALOG 64

typedef struct {
    gk_dialog_node nodes[GK_TEACH_MAX_DIALOG];
    size_t count;
    int current;
} gk_dialog;

void gk_dialog_init(gk_dialog *d);
gk_status gk_dialog_add(gk_dialog *d, int id, const char *speaker,
                        const char *text);
const gk_dialog_node *gk_dialog_current(const gk_dialog *d);
gk_status gk_dialog_advance(gk_dialog *d);            /* NPC dialog (312) */
gk_status gk_dialog_choose(gk_dialog *d, int branch); /* branching (311) */

typedef enum {
    GK_TRAIN_FORWARD = 0,   /* 313 reverse teaching */
    GK_TRAIN_REVERSE,       /* 314 reverse training */
    GK_TRAIN_BLIND          /* 315 blind operation */
} gk_train_mode;

const char *gk_train_mode_name(gk_train_mode m);
typedef struct {
    gk_train_mode mode;
    int ui_hidden;
    int program_hidden;
    int errors;
} gk_train_session;

void gk_train_session_init(gk_train_session *s, gk_train_mode mode);
void gk_train_session_error(gk_train_session *s);

/* ================= live / record / replay (316-318) ================= */

typedef struct {
    size_t tick;
    double timestamp;
    double axis[6];
    int event;
} gk_replay_frame;

#define GK_TEACH_MAX_FRAMES 256

typedef struct {
    gk_replay_frame frames[GK_TEACH_MAX_FRAMES];
    size_t count;
    size_t cursor;
    int live;
    int recording;
} gk_replay;

void gk_replay_init(gk_replay *r);
gk_status gk_replay_push(gk_replay *r, double timestamp, const double axis[6],
                         int event);
gk_status gk_replay_start_replay(gk_replay *r);
const gk_replay_frame *gk_replay_next(gk_replay *r);
int gk_replay_at_end(const gk_replay *r);

/* ================= cognitive science (319-332) ================= */

/* Attention heat map (319): accumulate gaze dwell per UI region. */
#define GK_COG_MAX_REGIONS 32

typedef struct {
    char region[32];
    double dwell;
    double weight;    /* 0..1 normalized share */
} gk_heat_region;

typedef struct {
    gk_heat_region regions[GK_COG_MAX_REGIONS];
    size_t count;
} gk_heatmap;

void gk_heatmap_init(gk_heatmap *h);
gk_status gk_heatmap_add_region(gk_heatmap *h, const char *region);
gk_status gk_heatmap_observe(gk_heatmap *h, const char *region, double dt);
double gk_heatmap_weight(const gk_heatmap *h, const char *region);
const char *gk_heatmap_hottest(const gk_heatmap *h);

/* Eye tracking (320): fixation/saccade classification. */
typedef struct {
    double x;
    double y;
    double t;
} gk_gaze;

int gk_gaze_is_fixation(const gk_gaze *a, const gk_gaze *b, double vel_thresh);
double gk_gaze_velocity(const gk_gaze *a, const gk_gaze *b);

/* Cognitive load (321) based on task demand vs capacity. */
double gk_cognitive_load(double task_demand, double working_capacity);
const char *gk_cognitive_load_level(double load);

/* Forgetting / memory curve (322) and Ebbinghaus review (323). */
double gk_memory_retention(double stability_days, double elapsed_days);
/* Spaced repetition: next review interval (324). */
double gk_next_review_interval(double previous_interval, int quality);
/* Micro-learning: split content into sessions (325). */
size_t gk_micro_session_count(size_t total_items, size_t per_session);

/* Flow detection (326). */
int gk_flow_detect(double skill, double challenge);

/* Metacognition / error attribution / transfer (327-329). */
typedef struct {
    double confidence;
    double accuracy;
} gk_metacog;

double gk_metacog_calibration(const gk_metacog *m);
const char *gk_error_attribution(double skill, double luck);
double gk_transfer_gain(double baseline, double novel);

/* Reaction time / error-rate statistics (330-331). */
double gk_reaction_mean(const double *times, size_t n);
double gk_reaction_stddev(const double *times, size_t n);
double gk_error_rate(const int *results, size_t n);   /* results: 1 ok, 0 err */

/* Learning behavior analysis (332): engagement index 0..1. */
double gk_engagement_index(double active_time, double idle_time,
                           double error_rate);

#ifdef __cplusplus
}
#endif

#endif
