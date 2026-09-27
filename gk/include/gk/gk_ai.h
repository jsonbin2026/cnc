#ifndef GK_AI_H
#define GK_AI_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_AI_MAX_CLASSES 16
#define GK_AI_MAX_FEATURES 16
#define GK_AI_MAX_SAMPLES 512

/* ---- 516 intelligent error explanation ---- */

typedef struct {
    int code;
    char explanation[256];
    char suggestion[256];
} gk_error_explanation;

gk_status gk_ai_explain_error(int code, gk_error_explanation *out);

/* ---- 517 parameter recommendation ---- */

typedef struct {
    char material[32];
    int tool_diameter_x100;
    int flutes;
    double feed;
    double speed;
    double depth;
} gk_param_reco;

gk_status gk_ai_recommend_params(const char *material, double diameter,
                                 int flutes, gk_param_reco *out);

/* ---- 518 natural language to G-code ---- */

int gk_ai_nl_to_gcode(const char *sentence, char *buf, size_t len);

/* ---- 519 voice assistant ---- */

typedef struct {
    int listening;
    char last_command[128];
    char response[128];
    int recognized;
} gk_voice_assistant;

void gk_voice_assistant_init(gk_voice_assistant *v);
gk_status gk_voice_listen(gk_voice_assistant *v, const char *transcript);

/* ---- 520 intelligent tool setting ---- */

typedef struct {
    double contact_x, contact_y, contact_z;
    double measured_offset;
    int probes;
    int success;
} gk_auto_toolset;

void gk_auto_toolset_init(gk_auto_toolset *t);
gk_status gk_auto_toolset_probe(gk_auto_toolset *t, double x, double y,
                                double z);

/* ---- 521 anomaly prediction / 522 tool breakage prediction ---- */

typedef struct {
    double history[GK_AI_MAX_SAMPLES];
    int count;
    double mean;
    double stddev;
    double threshold;
} gk_anomaly_detector;

void gk_anomaly_init(gk_anomaly_detector *d, double threshold);
gk_status gk_anomaly_observe(gk_anomaly_detector *d, double value);
double gk_anomaly_zscore(const gk_anomaly_detector *d, double value);
int gk_anomaly_is_anomaly(const gk_anomaly_detector *d, double value);
/* tool life predictor: linear regression on (time, wear). */
double gk_ai_predict_tool_life(const double *times, const double *wear,
                               int n, double wear_limit);

/* ---- 523 automatic programming ---- */

typedef struct {
    double length;
    double width;
    double depth;
    int passes;
    double feed;
} gk_auto_program;

int gk_ai_generate_program(const gk_auto_program *ap, char *buf, size_t len);

/* ---- 524 learner profile / 525 ability radar ---- */

typedef struct {
    char name[32];
    double score;
} gk_ability;

typedef struct {
    char learner[32];
    gk_ability abilities[GK_AI_MAX_FEATURES];
    int count;
    double overall;
} gk_learner_profile;

void gk_learner_profile_init(gk_learner_profile *p, const char *name);
gk_status gk_learner_add_ability(gk_learner_profile *p, const char *name,
                                 double score);
void gk_learner_update_overall(gk_learner_profile *p);
/* returns a CSV of the radar axes */
int gk_learner_radar(const gk_learner_profile *p, char *buf, size_t len);

/* ---- 526 adaptive question selection ---- */

int gk_ai_adaptive_difficulty(double ability, double target);

/* ---- 527 error pattern mining ---- */

typedef struct {
    int wrong_ids[128];
    int count;
    int pattern[8];
    int pattern_len;
} gk_error_pattern;

void gk_error_pattern_init(gk_error_pattern *p);
gk_status gk_error_pattern_add(gk_error_pattern *p, int question_id);
/* find the longest repeated subsequence of length >= 2; returns its length. */
int gk_error_pattern_mine(gk_error_pattern *p);

/* ---- 528 score prediction ---- */

double gk_ai_predict_score(const double *recent_scores, int n);

/* ---- 529 course recommendation ---- */

int gk_ai_recommend_courses(const double *ability_scores, int n,
                            int *course_ids, int max_out);

/* ---- 530 group comparison / 531 clustering ---- */

typedef struct {
    char name[32];
    double features[4];
} gk_cluster_point;

typedef struct {
    gk_cluster_point points[128];
    int count;
    int labels[128];
    int k;
} gk_clustering;

void gk_clustering_init(gk_clustering *c, int k);
gk_status gk_clustering_add(gk_clustering *c, const char *name,
                            const double *features, int nfeatures);
int gk_clustering_run(gk_clustering *c);
int gk_clustering_size(const gk_clustering *c, int label);
double gk_ai_group_average(const double *scores, int n);

/* ---- 532 teaching effect analysis ---- */

typedef struct {
    double pre_mean;
    double post_mean;
    int n;
} gk_teaching_effect;

gk_status gk_ai_teaching_effect(const double *pre, const double *post, int n,
                                gk_teaching_effect *out);
double gk_ai_effect_size(const gk_teaching_effect *e);

/* ---- 533 knowledge graph ---- */

#define GK_KG_MAX_NODES 64
typedef struct {
    int id;
    char name[48];
    char topic[32];
} gk_kg_node;

typedef struct {
    int from;
    int to;
    double weight;
} gk_kg_edge;

typedef struct {
    gk_kg_node nodes[GK_KG_MAX_NODES];
    int node_count;
    gk_kg_edge edges[128];
    int edge_count;
} gk_knowledge_graph;

void gk_kg_init(gk_knowledge_graph *g);
gk_status gk_kg_add_node(gk_knowledge_graph *g, int id, const char *name,
                         const char *topic);
gk_status gk_kg_add_edge(gk_knowledge_graph *g, int from, int to,
                         double weight);
/* returns the number of neighbors written to out */
int gk_kg_neighbors(const gk_knowledge_graph *g, int id, int *out, int max_out);
/* shortest path length via BFS; -1 if unreachable */
int gk_kg_path_length(const gk_knowledge_graph *g, int from, int to);

/* ---- 534 intelligent Q&A ---- */

typedef struct {
    char question[128];
    char answer[256];
    char keywords[128];
} gk_qa_entry;

typedef struct {
    gk_qa_entry entries[64];
    int count;
} gk_qa_base;

void gk_qa_init(gk_qa_base *q);
gk_status gk_qa_add(gk_qa_base *q, const char *question, const char *answer,
                    const char *keywords);
const char *gk_qa_ask(const gk_qa_base *q, const char *query);

/* ---- 535 image recognition ---- */

typedef struct {
    int width, height;
    unsigned char pixels[64 * 64];
} gk_image;

void gk_image_init(gk_image *im, int w, int h);
gk_status gk_image_set(gk_image *im, int x, int y, unsigned char v);
/* simple recognition: returns a class id based on density histograms. */
int gk_image_classify(const gk_image *im);
double gk_image_density(const gk_image *im);

/* ---- 536 visual Q&A ---- */

typedef struct {
    char question[128];
} gk_vqa_query;

int gk_vqa_answer(const gk_image *im, const char *question, char *buf,
                  size_t len);

/* ---- 537 voice dialogue ---- */

typedef struct {
    int turns;
    char context[128];
} gk_voice_dialog;

void gk_voice_dialog_init(gk_voice_dialog *d);
gk_status gk_voice_dialog_say(gk_voice_dialog *d, const char *utterance,
                              char *response, size_t len);

/* ---- 538 gesture recognition ---- */

typedef enum {
    GK_GESTURE_NONE = 0,
    GK_GESTURE_SWIPE_LEFT,
    GK_GESTURE_SWIPE_RIGHT,
    GK_GESTURE_ROTATE,
    GK_GESTURE_PINCH,
    GK_GESTURE_TAP,
    GK_GESTURE_COUNT
} gk_gesture;

const char *gk_gesture_name(gk_gesture g);
/* classify from a start and end position plus scale change. */
gk_gesture gk_gesture_classify(double x0, double y0, double x1, double y1,
                               double scale_delta);

/* ---- 539 expression recognition ---- */

typedef enum {
    GK_FACE_NEUTRAL = 0,
    GK_FACE_HAPPY,
    GK_FACE_CONFUSED,
    GK_FACE_FRUSTRATED,
    GK_FACE_COUNT
} gk_face_expression;

const char *gk_face_expression_name(gk_face_expression e);
gk_face_expression gk_face_classify(double valence, double arousal);

/* ---- 540 multimodal diagnosis ---- */

typedef struct {
    int error_score;
    int voice_score;
    int face_score;
    int gesture_score;
} gk_multimodal_input;

typedef struct {
    char diagnosis[128];
    double confidence;
} gk_multimodal_result;

gk_status gk_multimodal_diagnose(const gk_multimodal_input *in,
                                 gk_multimodal_result *out);

#ifdef __cplusplus
}
#endif

#endif /* GK_AI_H */
