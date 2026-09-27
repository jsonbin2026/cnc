#ifndef GK_COLLAB_H
#define GK_COLLAB_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_COLLAB_MAX_USERS 64
#define GK_COLLAB_MAX_ITEMS 128
#define GK_COLLAB_NAME 64
#define GK_COLLAB_TEXT 256

/* ---- 541 teacher client / 542 student client ---- */

typedef enum {
    GK_ROLE_TEACHER = 0,   /* 541 */
    GK_ROLE_STUDENT,       /* 542 */
    GK_ROLE_ADMIN
} gk_user_role;

typedef struct {
    int id;
    char name[GK_COLLAB_NAME];
    gk_user_role role;
    int online;
    int machine_id;
    double score;
    double progress;
} gk_user;

typedef struct {
    gk_user users[GK_COLLAB_MAX_USERS];
    int count;
    int next_id;
    int session_open;
} gk_session;

void gk_session_init(gk_session *s);
int gk_session_add_user(gk_session *s, const char *name, gk_user_role role);
gk_user *gk_session_user(gk_session *s, int id);
int gk_session_count_role(const gk_session *s, gk_user_role role);
gk_status gk_session_set_online(gk_session *s, int id, int online);

/* ---- 543 real-time monitoring ---- */

typedef struct {
    int user_id;
    double spindle;
    double feed;
    double x, y, z;
    double load;
    double time;
} gk_monitor_frame;

typedef struct {
    gk_monitor_frame frames[GK_COLLAB_MAX_USERS];
    int count;
} gk_monitor_board;

void gk_monitor_init(gk_monitor_board *m);
gk_status gk_monitor_update(gk_monitor_board *m, const gk_monitor_frame *f);
const gk_monitor_frame *gk_monitor_of(const gk_monitor_board *m, int user_id);
/* return the user with the highest load, or -1 when empty */
int gk_monitor_peak_load(const gk_monitor_board *m);

/* ---- 544 remote takeover ---- */

typedef struct {
    int controller_id;
    int target_id;
    int active;
    int actions;
} gk_takeover;

void gk_takeover_init(gk_takeover *t);
gk_status gk_takeover_start(gk_takeover *t, int controller, int target);
gk_status gk_takeover_act(gk_takeover *t);
gk_status gk_takeover_stop(gk_takeover *t);

/* ---- 545 task dispatch / 546 homework submission ---- */

typedef struct {
    int id;
    char title[GK_COLLAB_NAME];
    int assigned_to;
    double deadline;
    int status;      /* 0 open, 1 submitted, 2 graded */
    double score;
    char content[GK_COLLAB_TEXT];
} gk_assignment;

typedef struct {
    gk_assignment items[GK_COLLAB_MAX_ITEMS];
    int count;
    int next_id;
} gk_task_board;

void gk_task_board_init(gk_task_board *t);
int gk_task_dispatch(gk_task_board *t, const char *title, int user_id,
                     double deadline);
gk_status gk_task_submit(gk_task_board *t, int task_id, const char *content);
gk_status gk_task_grade(gk_task_board *t, int task_id, double score);
int gk_task_open_count(const gk_task_board *t);
int gk_task_count_for_user(const gk_task_board *t, int user_id);

/* ---- 548 leaderboard ---- */

typedef struct {
    char name[GK_COLLAB_NAME];
    double score;
} gk_rank_entry;

typedef struct {
    gk_rank_entry entries[GK_COLLAB_MAX_USERS];
    int count;
} gk_class_rank;

void gk_class_rank_init(gk_class_rank *r);
gk_status gk_class_rank_submit(gk_class_rank *r, const char *name,
                               double score);
int gk_class_rank_of(const gk_class_rank *r, const char *name);

/* ---- 549 multi-machine networking ---- */

typedef enum {
    GK_NET_STAR = 0,
    GK_NET_RING,
    GK_NET_MESH
} gk_net_topology;

typedef struct {
    int machine_id;
    int peers[GK_COLLAB_MAX_USERS];
    int peer_count;
    int linked;
} gk_machine_node;

typedef struct {
    gk_machine_node nodes[GK_COLLAB_MAX_USERS];
    int count;
    gk_net_topology topology;
    int latency_ms;
} gk_network;

void gk_network_init(gk_network *n, gk_net_topology topo);
gk_status gk_network_add_machine(gk_network *n, int machine_id);
gk_status gk_network_link(gk_network *n, int a, int b);
int gk_network_links(const gk_network *n);

/* ---- 550 danmaku chat ---- */

typedef struct {
    int user_id;
    char text[GK_COLLAB_TEXT];
    double time;
} gk_danmaku;

typedef struct {
    gk_danmaku messages[GK_COLLAB_MAX_ITEMS];
    int count;
    int max_len;
} gk_chat;

void gk_chat_init(gk_chat *c);
gk_status gk_chat_send(gk_chat *c, int user_id, const char *text);
int gk_chat_count(const gk_chat *c);
/* rate-limit: minimum seconds between messages */
int gk_chat_allowed(const gk_chat *c, double now, double min_gap);

/* ---- 551 screen broadcast / 552 voice intercom ---- */

typedef struct {
    int broadcaster;
    int viewers[GK_COLLAB_MAX_USERS];
    int viewer_count;
    int active;
    int quality;
} gk_broadcast;

void gk_broadcast_init(gk_broadcast *b);
gk_status gk_broadcast_start(gk_broadcast *b, int broadcaster, int quality);
gk_status gk_broadcast_join(gk_broadcast *b, int viewer);
gk_status gk_broadcast_stop(gk_broadcast *b);

/* ---- 553 group collaboration ---- */

typedef struct {
    int id;
    char name[GK_COLLAB_NAME];
    int members[GK_COLLAB_MAX_USERS];
    int member_count;
} gk_group;

typedef struct {
    gk_group groups[16];
    int count;
} gk_group_set;

void gk_group_set_init(gk_group_set *g);
int gk_group_create(gk_group_set *g, const char *name);
gk_status gk_group_join(gk_group_set *g, int group_id, int user_id);
int gk_group_size(const gk_group_set *g, int group_id);

/* ---- 554 competition mode ---- */

typedef struct {
    int id;
    char name[GK_COLLAB_NAME];
    double start;
    double duration;
    int running;
    int winner;
    double best_time;
} gk_competition;

void gk_competition_init(gk_competition *c);
gk_status gk_competition_start(gk_competition *c, const char *name,
                               double start, double duration);
gk_status gk_competition_record(gk_competition *c, int user_id, double time);
int gk_competition_is_over(const gk_competition *c, double now);

/* ---- 555 class / 556 student / 557 teacher management ---- */

typedef struct {
    char name[GK_COLLAB_NAME];
    int teacher_id;
    int students[GK_COLLAB_MAX_USERS];
    int student_count;
} gk_classroom;

void gk_classroom_init(gk_classroom *c, const char *name);
gk_status gk_classroom_add_student(gk_classroom *c, int user_id);
gk_status gk_classroom_remove_student(gk_classroom *c, int user_id);
int gk_classroom_has_student(const gk_classroom *c, int user_id);
int gk_classroom_size(const gk_classroom *c);

/* ---- 558 course assignment / 559 progress tracking ---- */

typedef struct {
    char course[GK_COLLAB_NAME];
    int assigned_to;
    double progress;
    int completed;
} gk_course_assign;

typedef struct {
    gk_course_assign items[GK_COLLAB_MAX_ITEMS];
    int count;
} gk_progress_tracker;

void gk_progress_tracker_init(gk_progress_tracker *p);
gk_status gk_course_assign_add(gk_progress_tracker *p, const char *course,
                               int user_id);
gk_status gk_course_progress(gk_progress_tracker *p, const char *course,
                             int user_id, double progress);
double gk_course_avg_progress(const gk_progress_tracker *p,
                              const char *course);

/* ---- 560 home-school communication ---- */

typedef struct {
    char from[GK_COLLAB_NAME];
    char to[GK_COLLAB_NAME];
    char message[GK_COLLAB_TEXT];
    double time;
} gk_message;

typedef struct {
    gk_message messages[GK_COLLAB_MAX_ITEMS];
    int count;
} gk_message_box;

void gk_message_box_init(gk_message_box *m);
gk_status gk_message_send(gk_message_box *m, const char *from, const char *to,
                          const char *text, double time);
int gk_message_count_to(const gk_message_box *m, const char *to);

/* ---- 561 course editor ---- */

typedef struct {
    int id;
    char title[GK_COLLAB_NAME];
    char body[GK_COLLAB_TEXT];
    int order;
} gk_course_slide;

typedef struct {
    gk_course_slide slides[GK_COLLAB_MAX_ITEMS];
    int count;
} gk_course_editor;

void gk_course_editor_init(gk_course_editor *e);
int gk_course_add_slide(gk_course_editor *e, const char *title,
                        const char *body);
gk_status gk_course_move_slide(gk_course_editor *e, int id, int new_order);
const gk_course_slide *gk_course_next(const gk_course_editor *e,
                                      int current_order);

/* ---- 562 3D scene editor ---- */

typedef struct {
    int id;
    char name[GK_COLLAB_NAME];
    double x, y, z;
    double rot;
    double scale;
    int visible;
} gk_scene_object;

typedef struct {
    gk_scene_object objects[GK_COLLAB_MAX_ITEMS];
    int count;
    char environment[GK_COLLAB_NAME];
} gk_scene;

void gk_scene_init(gk_scene *s);
int gk_scene_add(gk_scene *s, const char *name, double x, double y, double z);
gk_status gk_scene_transform(gk_scene *s, int id, double x, double y, double z,
                             double rot);
gk_status gk_scene_remove(gk_scene *s, int id);

/* ---- 563 quiz tool / 564 exam generation ---- */

typedef enum {
    GK_Q_SINGLE = 0,
    GK_Q_MULTI,
    GK_Q_FILL
} gk_question_kind;

typedef struct {
    int id;
    char prompt[GK_COLLAB_TEXT];
    gk_question_kind kind;
    char answer[GK_COLLAB_TEXT];
    double points;
    int difficulty;
} gk_question;

typedef struct {
    gk_question questions[GK_COLLAB_MAX_ITEMS];
    int count;
} gk_question_bank;

void gk_question_bank_init(gk_question_bank *b);
int gk_question_add(gk_question_bank *b, const char *prompt,
                    gk_question_kind kind, const char *answer, double points,
                    int difficulty);
/* assemble an exam: pick `n` questions whose total points >= min_points */
int gk_exam_generate(const gk_question_bank *b, int n, double min_points,
                     int *out_ids, int max_out);

/* ---- 565-576 content libraries ---- */

typedef struct {
    int id;
    char name[GK_COLLAB_NAME];
    char vendor[GK_COLLAB_NAME];
    double value;
    char unit[16];
} gk_lib_item;

typedef struct {
    gk_lib_item items[GK_COLLAB_MAX_ITEMS];
    int count;
    char domain[GK_COLLAB_NAME];
} gk_library;

void gk_library_init(gk_library *l, const char *domain);
int gk_library_add(gk_library *l, const char *name, const char *vendor,
                   double value, const char *unit);
const gk_lib_item *gk_library_find(const gk_library *l, const char *name);
int gk_library_count(const gk_library *l);

/* ---- 577 online update / 578 content distribution ---- */

typedef struct {
    int major, minor, patch;
} gk_version_num;

typedef struct {
    gk_version_num current;
    gk_version_num latest;
    int update_available;
    double size_mb;
    int channel;
} gk_updater;

void gk_updater_init(gk_updater *u, int major, int minor, int patch);
gk_status gk_updater_check(gk_updater *u, int major, int minor, int patch,
                           double size_mb);
int gk_updater_install(gk_updater *u);
const char *gk_updater_version_string(const gk_updater *u, char *buf,
                                      size_t len);

typedef struct {
    int peers;
    double bandwidth_mbps;
    int chunks;
    int distributed;
} gk_distribution;

void gk_distribution_init(gk_distribution *d, int peers, double bandwidth);
gk_status gk_distribution_push(gk_distribution *d, int chunks);

/* ---- 579 version management ---- */

typedef struct {
    int revision;
    char note[GK_COLLAB_NAME];
    int author_id;
} gk_content_version;

typedef struct {
    gk_content_version versions[GK_COLLAB_MAX_ITEMS];
    int count;
} gk_content_history;

void gk_content_history_init(gk_content_history *h);
int gk_content_commit(gk_content_history *h, const char *note, int author);
const gk_content_version *gk_content_version_at(const gk_content_history *h,
                                                int revision);

/* ---- 580 user creation ---- */

typedef struct {
    int author_id;
    char title[GK_COLLAB_NAME];
    char type[GK_COLLAB_NAME];
    int published;
    double rating;
    int rating_count;
} gk_ugc_item;

typedef struct {
    gk_ugc_item items[GK_COLLAB_MAX_ITEMS];
    int count;
} gk_ugc_store;

void gk_ugc_store_init(gk_ugc_store *s);
int gk_ugc_publish(gk_ugc_store *s, int author_id, const char *title,
                   const char *type);
gk_status gk_ugc_rate(gk_ugc_store *s, int id, double rating);
double gk_ugc_rating(const gk_ugc_store *s, int id);
int gk_ugc_count_by_author(const gk_ugc_store *s, int author_id);

#ifdef __cplusplus
}
#endif

#endif /* GK_COLLAB_H */
