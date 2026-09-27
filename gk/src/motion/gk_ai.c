#include "gk/gk_ai.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static void gk__copy(char *dst, size_t len, const char *src)
{
    size_t i;
    if (dst == NULL || len == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    for (i = 0; i + 1 < len && src[i] != '\0'; ++i) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

/* ---- 516 error explanation ---- */

gk_status gk_ai_explain_error(int code, gk_error_explanation *out)
{
    if (out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    memset(out, 0, sizeof(*out));
    out->code = code;
    switch (code) {
    case 41:
        gk__copy(out->explanation, sizeof(out->explanation),
                 "Axis exceeded a travel limit.");
        gk__copy(out->suggestion, sizeof(out->suggestion),
                 "Release the limit switch by jogging inward, then re-home.");
        break;
    case 51:
        gk__copy(out->explanation, sizeof(out->explanation),
                 "Servo drew too much current.");
        gk__copy(out->suggestion, sizeof(out->suggestion),
                 "Reduce feed rate or inspect for mechanical binding.");
        break;
    case 61:
        gk__copy(out->explanation, sizeof(out->explanation),
                 "Spindle load exceeded its rating.");
        gk__copy(out->suggestion, sizeof(out->suggestion),
                 "Reduce depth of cut and verify the tool is sharp.");
        break;
    case 141:
        gk__copy(out->explanation, sizeof(out->explanation),
                 "The program used a G code the control does not support.");
        gk__copy(out->suggestion, sizeof(out->suggestion),
                 "Check the controller manual and use a supported G code.");
        break;
    default:
        gk__copy(out->explanation, sizeof(out->explanation),
                 "Unknown alarm; refer to the alarm manual.");
        gk__copy(out->suggestion, sizeof(out->suggestion),
                 "Look up the alarm code in the built-in manual.");
        break;
    }
    return GK_OK;
}

/* ---- 517 parameter recommendation ---- */

gk_status gk_ai_recommend_params(const char *material, double diameter,
                                 int flutes, gk_param_reco *out)
{
    double vc;
    if (material == NULL || out == NULL || diameter <= 0.0 || flutes <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    if (strcmp(material, "aluminum") == 0) {
        vc = 300.0;
    } else if (strcmp(material, "steel") == 0) {
        vc = 120.0;
    } else if (strcmp(material, "stainless") == 0) {
        vc = 90.0;
    } else {
        vc = 150.0;
    }
    memset(out, 0, sizeof(*out));
    gk__copy(out->material, sizeof(out->material), material);
    out->tool_diameter_x100 = (int)(diameter * 100.0 + 0.5);
    out->flutes = flutes;
    out->speed = vc * 1000.0 / (3.14159265358979 * diameter);
    out->feed = out->speed / 60.0 * 0.05 * flutes;
    out->depth = diameter * 0.5;
    return GK_OK;
}

/* ---- 518 NL to G-code ---- */

int gk_ai_nl_to_gcode(const char *sentence, char *buf, size_t len)
{
    if (sentence == NULL || buf == NULL) {
        return 0;
    }
    if (strstr(sentence, "rapid") != NULL ||
        strstr(sentence, "快速") != NULL) {
        return snprintf(buf, len, "G00 X%d Y%d\n", 0, 0);
    }
    if (strstr(sentence, "drill") != NULL || strstr(sentence, "钻") != NULL) {
        return snprintf(buf, len, "G81 Z-5.0 R2.0 F100\n");
    }
    if (strstr(sentence, "circle") != NULL || strstr(sentence, "圆") != NULL) {
        return snprintf(buf, len, "G02 X10 Y10 I5 J0 F200\n");
    }
    if (strstr(sentence, "line") != NULL || strstr(sentence, "直线") != NULL) {
        return snprintf(buf, len, "G01 X10 Y10 F200\n");
    }
    return snprintf(buf, len, "G01 X0 Y0 F100\n");
}

/* ---- 519 voice assistant ---- */

void gk_voice_assistant_init(gk_voice_assistant *v)
{
    if (v != NULL) {
        memset(v, 0, sizeof(*v));
    }
}

gk_status gk_voice_listen(gk_voice_assistant *v, const char *transcript)
{
    if (v == NULL || transcript == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    gk__copy(v->last_command, sizeof(v->last_command), transcript);
    v->listening = 1;
    if (strstr(transcript, "start") != NULL ||
        strstr(transcript, "开始") != NULL) {
        gk__copy(v->response, sizeof(v->response), "Cycle start");
        v->recognized = 1;
    } else if (strstr(transcript, "stop") != NULL ||
               strstr(transcript, "停止") != NULL) {
        gk__copy(v->response, sizeof(v->response), "Cycle stop");
        v->recognized = 1;
    } else if (strstr(transcript, "home") != NULL ||
               strstr(transcript, "回零") != NULL) {
        gk__copy(v->response, sizeof(v->response), "Return to home");
        v->recognized = 1;
    } else {
        gk__copy(v->response, sizeof(v->response), "Command not recognized");
        v->recognized = 0;
    }
    return GK_OK;
}

/* ---- 520 intelligent tool setting ---- */

void gk_auto_toolset_init(gk_auto_toolset *t)
{
    if (t != NULL) {
        memset(t, 0, sizeof(*t));
    }
}

gk_status gk_auto_toolset_probe(gk_auto_toolset *t, double x, double y,
                                double z)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    t->contact_x = x;
    t->contact_y = y;
    t->contact_z = z;
    t->measured_offset = z;
    t->probes++;
    t->success = 1;
    return GK_OK;
}

/* ---- 521/522 anomaly + tool life ---- */

void gk_anomaly_init(gk_anomaly_detector *d, double threshold)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    d->threshold = threshold > 0.0 ? threshold : 3.0;
}

gk_status gk_anomaly_observe(gk_anomaly_detector *d, double value)
{
    double old_mean;
    if (d == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (d->count >= GK_AI_MAX_SAMPLES) {
        return GK_ERR_OUT_OF_RANGE;
    }
    old_mean = d->mean;
    d->history[d->count++] = value;
    d->mean = old_mean + (value - old_mean) / d->count;
    if (d->count > 1) {
        double m2 = 0.0;
        int i;
        for (i = 0; i < d->count; ++i) {
            double dv = d->history[i] - d->mean;
            m2 += dv * dv;
        }
        d->stddev = sqrt(m2 / d->count);
    }
    return GK_OK;
}

double gk_anomaly_zscore(const gk_anomaly_detector *d, double value)
{
    if (d == NULL || d->stddev <= 0.0) {
        return 0.0;
    }
    return (value - d->mean) / d->stddev;
}

int gk_anomaly_is_anomaly(const gk_anomaly_detector *d, double value)
{
    if (d == NULL || d->stddev <= 0.0) {
        return 0;
    }
    return fabs(gk_anomaly_zscore(d, value)) > d->threshold;
}

double gk_ai_predict_tool_life(const double *times, const double *wear,
                               int n, double wear_limit)
{
    double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
    double slope, intercept;
    int i;
    if (times == NULL || wear == NULL || n < 2 || wear_limit <= 0.0) {
        return 0.0;
    }
    for (i = 0; i < n; ++i) {
        sx += times[i];
        sy += wear[i];
        sxx += times[i] * times[i];
        sxy += times[i] * wear[i];
    }
    {
        double denom = n * sxx - sx * sx;
        if (denom == 0.0) {
            return 0.0;
        }
        slope = (n * sxy - sx * sy) / denom;
        intercept = (sy - slope * sx) / n;
    }
    if (slope <= 0.0) {
        return 0.0;
    }
    return (wear_limit - intercept) / slope;
}

/* ---- 523 automatic programming ---- */

int gk_ai_generate_program(const gk_auto_program *ap, char *buf, size_t len)
{
    int off = 0;
    int p;
    if (ap == NULL || buf == NULL) {
        return 0;
    }
    off += snprintf(buf + off, len - (size_t)off, "G21 G90\n");
    for (p = 0; p < ap->passes && off < (int)len; ++p) {
        double z = -ap->depth * (p + 1) / ap->passes;
        off += snprintf(buf + off, len - (size_t)off,
                        "G01 Z%.3f F%.0f\n", z, ap->feed);
        off += snprintf(buf + off, len - (size_t)off,
                        "G01 X%.3f Y0\nG01 X%.3f Y%.3f\n"
                        "G01 X0 Y%.3f\nG01 X0 Y0\n",
                        ap->length, ap->length, ap->width, ap->width);
    }
    off += snprintf(buf + off, len - (size_t)off, "M30\n");
    return off;
}

/* ---- 524/525 learner profile ---- */

void gk_learner_profile_init(gk_learner_profile *p, const char *name)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    gk__copy(p->learner, sizeof(p->learner), name);
}

gk_status gk_learner_add_ability(gk_learner_profile *p, const char *name,
                                 double score)
{
    gk_ability *a;
    if (p == NULL || name == NULL || p->count >= GK_AI_MAX_FEATURES) {
        return GK_ERR_INVALID_ARG;
    }
    a = &p->abilities[p->count++];
    memset(a, 0, sizeof(*a));
    gk__copy(a->name, sizeof(a->name), name);
    if (score < 0.0) score = 0.0;
    if (score > 1.0) score = 1.0;
    a->score = score;
    gk_learner_update_overall(p);
    return GK_OK;
}

void gk_learner_update_overall(gk_learner_profile *p)
{
    double sum = 0.0;
    int i;
    if (p == NULL || p->count == 0) {
        return;
    }
    for (i = 0; i < p->count; ++i) {
        sum += p->abilities[i].score;
    }
    p->overall = sum / p->count;
}

int gk_learner_radar(const gk_learner_profile *p, char *buf, size_t len)
{
    int off = 0;
    int i;
    if (p == NULL || buf == NULL) {
        return 0;
    }
    buf[0] = '\0';
    for (i = 0; i < p->count; ++i) {
        off += snprintf(buf + off, len - (size_t)off, "%s%.0f",
                        i > 0 ? "," : "", p->abilities[i].score * 100.0);
    }
    return off;
}

/* ---- 526 adaptive difficulty ---- */

int gk_ai_adaptive_difficulty(double ability, double target)
{
    if (ability < 0.0) ability = 0.0;
    if (ability > 1.0) ability = 1.0;
    if (target <= 0.0) target = 0.75;
    /* keep difficulty near the learner's ability, biased by target */
    return (int)(ability * 4.0 + target) + 1;
}

/* ---- 527 error pattern mining ---- */

void gk_error_pattern_init(gk_error_pattern *p)
{
    if (p != NULL) {
        memset(p, 0, sizeof(*p));
    }
}

gk_status gk_error_pattern_add(gk_error_pattern *p, int question_id)
{
    if (p == NULL || p->count >= 128) {
        return GK_ERR_OUT_OF_RANGE;
    }
    p->wrong_ids[p->count++] = question_id;
    return GK_OK;
}

int gk_error_pattern_mine(gk_error_pattern *p)
{
    int best = 0;
    int i, j, k;
    if (p == NULL) {
        return 0;
    }
    for (i = 0; i < p->count; ++i) {
        for (j = i + 1; j < p->count; ++j) {
            int len = 0;
            while (i + len < p->count && j + len < p->count &&
                   p->wrong_ids[i + len] == p->wrong_ids[j + len]) {
                len++;
            }
            if (len > best) {
                best = len;
                if (len > p->pattern_len) {
                    for (k = 0; k < len && k < 8; ++k) {
                        p->pattern[k] = p->wrong_ids[i + k];
                    }
                    p->pattern_len = len > 8 ? 8 : len;
                }
            }
        }
    }
    return best;
}

/* ---- 528 score prediction ---- */

double gk_ai_predict_score(const double *recent_scores, int n)
{
    double sum = 0.0;
    int i;
    if (recent_scores == NULL || n <= 0) {
        return 0.0;
    }
    /* weighted moving average, recent scores weigh more */
    for (i = 0; i < n; ++i) {
        sum += recent_scores[i] * (double)(i + 1);
    }
    return sum / (double)(n * (n + 1) / 2);
}

/* ---- 529 course recommendation ---- */

int gk_ai_recommend_courses(const double *ability_scores, int n,
                            int *course_ids, int max_out)
{
    int count = 0;
    int i;
    if (ability_scores == NULL || course_ids == NULL || max_out <= 0) {
        return 0;
    }
    for (i = 0; i < n && count < max_out; ++i) {
        if (ability_scores[i] < 0.7) {
            course_ids[count++] = i;
        }
    }
    if (count == 0 && n > 0 && max_out > 0) {
        course_ids[count++] = 0;
    }
    return count;
}

/* ---- 530/531 clustering ---- */

void gk_clustering_init(gk_clustering *c, int k)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    c->k = k > 0 && k < GK_AI_MAX_CLASSES ? k : 2;
}

gk_status gk_clustering_add(gk_clustering *c, const char *name,
                            const double *features, int nfeatures)
{
    gk_cluster_point *pt;
    int i;
    if (c == NULL || name == NULL || features == NULL ||
        c->count >= 128 || nfeatures <= 0 || nfeatures > 4) {
        return GK_ERR_INVALID_ARG;
    }
    pt = &c->points[c->count++];
    memset(pt, 0, sizeof(*pt));
    gk__copy(pt->name, sizeof(pt->name), name);
    for (i = 0; i < nfeatures; ++i) {
        pt->features[i] = features[i];
    }
    return GK_OK;
}

/* k-means with deterministic initial centroids spread over the first axis. */
int gk_clustering_run(gk_clustering *c)
{
    double centers[GK_AI_MAX_CLASSES][4];
    int iter;
    int i, k;
    if (c == NULL || c->count == 0) {
        return 0;
    }
    for (k = 0; k < c->k; ++k) {
        int idx = k * c->count / c->k;
        int f;
        for (f = 0; f < 4; ++f) {
            centers[k][f] = c->points[idx].features[f];
        }
    }
    /* ensure initial centroids are distinct when the data allows it */
    for (k = 1; k < c->k; ++k) {
        int prev;
        int dup = 0;
        for (prev = 0; prev < k && !dup; ++prev) {
            int f, same = 1;
            for (f = 0; f < 4; ++f) {
                if (centers[prev][f] != centers[k][f]) {
                    same = 0;
                    break;
                }
            }
            if (same) {
                dup = 1;
            }
        }
        if (dup) {
            int i;
            for (i = 0; i < c->count; ++i) {
                int f, same_to_k = 1;
                for (f = 0; f < 4; ++f) {
                    if (centers[k][f] != c->points[i].features[f]) {
                        same_to_k = 0;
                        break;
                    }
                }
                if (!same_to_k) {
                    for (f = 0; f < 4; ++f) {
                        centers[k][f] = c->points[i].features[f];
                    }
                    break;
                }
            }
        }
    }
    for (iter = 0; iter < 20; ++iter) {
        int changed = 0;
        for (i = 0; i < c->count; ++i) {
            int best = 0;
            double bestd = 1e30;
            for (k = 0; k < c->k; ++k) {
                double d = 0.0;
                int f;
                for (f = 0; f < 4; ++f) {
                    double dv = c->points[i].features[f] - centers[k][f];
                    d += dv * dv;
                }
                if (d < bestd) {
                    bestd = d;
                    best = k;
                }
            }
            if (c->labels[i] != best) {
                c->labels[i] = best;
                changed = 1;
            }
        }
        for (k = 0; k < c->k; ++k) {
            double sum[4] = {0, 0, 0, 0};
            int cnt = 0;
            int f;
            for (i = 0; i < c->count; ++i) {
                if (c->labels[i] == k) {
                    for (f = 0; f < 4; ++f) {
                        sum[f] += c->points[i].features[f];
                    }
                    cnt++;
                }
            }
            if (cnt > 0) {
                for (f = 0; f < 4; ++f) {
                    centers[k][f] = sum[f] / cnt;
                }
            }
        }
        if (!changed) {
            break;
        }
    }
    return c->k;
}

int gk_clustering_size(const gk_clustering *c, int label)
{
    int i;
    int n = 0;
    if (c == NULL) {
        return 0;
    }
    for (i = 0; i < c->count; ++i) {
        if (c->labels[i] == label) n++;
    }
    return n;
}

double gk_ai_group_average(const double *scores, int n)
{
    double sum = 0.0;
    int i;
    if (scores == NULL || n <= 0) {
        return 0.0;
    }
    for (i = 0; i < n; ++i) {
        sum += scores[i];
    }
    return sum / n;
}

/* ---- 532 teaching effect ---- */

gk_status gk_ai_teaching_effect(const double *pre, const double *post, int n,
                                gk_teaching_effect *out)
{
    int i;
    if (pre == NULL || post == NULL || out == NULL || n <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    memset(out, 0, sizeof(*out));
    out->n = n;
    for (i = 0; i < n; ++i) {
        out->pre_mean += pre[i];
        out->post_mean += post[i];
    }
    out->pre_mean /= n;
    out->post_mean /= n;
    return GK_OK;
}

double gk_ai_effect_size(const gk_teaching_effect *e)
{
    if (e == NULL) {
        return 0.0;
    }
    return e->post_mean - e->pre_mean;
}

/* ---- 533 knowledge graph ---- */

void gk_kg_init(gk_knowledge_graph *g)
{
    if (g != NULL) {
        memset(g, 0, sizeof(*g));
    }
}

gk_status gk_kg_add_node(gk_knowledge_graph *g, int id, const char *name,
                         const char *topic)
{
    gk_kg_node *n;
    if (g == NULL || name == NULL || g->node_count >= GK_KG_MAX_NODES) {
        return GK_ERR_OUT_OF_RANGE;
    }
    n = &g->nodes[g->node_count++];
    memset(n, 0, sizeof(*n));
    n->id = id;
    gk__copy(n->name, sizeof(n->name), name);
    gk__copy(n->topic, sizeof(n->topic), topic);
    return GK_OK;
}

gk_status gk_kg_add_edge(gk_knowledge_graph *g, int from, int to,
                         double weight)
{
    gk_kg_edge *e;
    if (g == NULL || g->edge_count >= 128) {
        return GK_ERR_OUT_OF_RANGE;
    }
    e = &g->edges[g->edge_count++];
    e->from = from;
    e->to = to;
    e->weight = weight;
    return GK_OK;
}

int gk_kg_neighbors(const gk_knowledge_graph *g, int id, int *out, int max_out)
{
    int i;
    int n = 0;
    if (g == NULL || out == NULL) {
        return 0;
    }
    for (i = 0; i < g->edge_count && n < max_out; ++i) {
        if (g->edges[i].from == id) {
            out[n++] = g->edges[i].to;
        }
    }
    return n;
}

int gk_kg_path_length(const gk_knowledge_graph *g, int from, int to)
{
    int visited[GK_KG_MAX_NODES];
    int dist[GK_KG_MAX_NODES];
    int queue[GK_KG_MAX_NODES];
    int head = 0, tail = 0;
    int i;
    if (g == NULL) {
        return -1;
    }
    for (i = 0; i < GK_KG_MAX_NODES; ++i) {
        visited[i] = 0;
        dist[i] = -1;
    }
    if (from == to) {
        return 0;
    }
    queue[tail++] = from;
    visited[from] = 1;
    dist[from] = 0;
    while (head < tail) {
        int cur = queue[head++];
        for (i = 0; i < g->edge_count; ++i) {
            if (g->edges[i].from == cur && !visited[g->edges[i].to]) {
                int nx = g->edges[i].to;
                visited[nx] = 1;
                dist[nx] = dist[cur] + 1;
                if (nx == to) {
                    return dist[nx];
                }
                if (tail < GK_KG_MAX_NODES) {
                    queue[tail++] = nx;
                }
            }
        }
    }
    return -1;
}

/* ---- 534 Q&A ---- */

void gk_qa_init(gk_qa_base *q)
{
    if (q != NULL) {
        memset(q, 0, sizeof(*q));
    }
}

gk_status gk_qa_add(gk_qa_base *q, const char *question, const char *answer,
                    const char *keywords)
{
    gk_qa_entry *e;
    if (q == NULL || question == NULL || q->count >= 64) {
        return GK_ERR_INVALID_ARG;
    }
    e = &q->entries[q->count++];
    memset(e, 0, sizeof(*e));
    gk__copy(e->question, sizeof(e->question), question);
    gk__copy(e->answer, sizeof(e->answer), answer);
    gk__copy(e->keywords, sizeof(e->keywords), keywords);
    return GK_OK;
}

const char *gk_qa_ask(const gk_qa_base *q, const char *query)
{
    int i;
    int best = -1;
    int best_score = 0;
    if (q == NULL || query == NULL) {
        return NULL;
    }
    for (i = 0; i < q->count; ++i) {
        /* score = number of keywords matching the query */
        char kw[128];
        char *tok;
        int score = 0;
        gk__copy(kw, sizeof(kw), q->entries[i].keywords);
        tok = strtok(kw, " ,");
        while (tok != NULL) {
            if (strstr(query, tok) != NULL) {
                score++;
            }
            tok = strtok(NULL, " ,");
        }
        if (score > best_score ||
            (score == best_score && score > 0 && best < 0)) {
            best_score = score;
            best = i;
        }
    }
    if (best < 0) {
        return NULL;
    }
    return q->entries[best].answer;
}

/* ---- 535 image recognition ---- */

void gk_image_init(gk_image *im, int w, int h)
{
    if (im == NULL) {
        return;
    }
    memset(im, 0, sizeof(*im));
    im->width = w > 64 ? 64 : w;
    im->height = h > 64 ? 64 : h;
}

gk_status gk_image_set(gk_image *im, int x, int y, unsigned char v)
{
    if (im == NULL || x < 0 || x >= im->width || y < 0 || y >= im->height) {
        return GK_ERR_OUT_OF_RANGE;
    }
    im->pixels[y * 64 + x] = v;
    return GK_OK;
}

double gk_image_density(const gk_image *im)
{
    long sum = 0;
    int i, n;
    if (im == NULL || im->width <= 0 || im->height <= 0) {
        return 0.0;
    }
    n = im->width * im->height;
    for (i = 0; i < n; ++i) {
        sum += im->pixels[i];
    }
    return (double)sum / (255.0 * n);
}

int gk_image_classify(const gk_image *im)
{
    double d = gk_image_density(im);
    if (d < 0.15) return 0;
    if (d < 0.4) return 1;
    if (d < 0.75) return 2;
    return 3;
}

/* ---- 536 visual Q&A ---- */

int gk_vqa_answer(const gk_image *im, const char *question, char *buf,
                  size_t len)
{
    double d;
    if (im == NULL || buf == NULL) {
        return 0;
    }
    d = gk_image_density(im);
    if (question == NULL) {
        return snprintf(buf, len, "density=%.2f", d);
    }
    if (strstr(question, "bright") != NULL ||
        strstr(question, "亮") != NULL) {
        return snprintf(buf, len, d > 0.5 ? "yes" : "no");
    }
    if (strstr(question, "color") != NULL ||
        strstr(question, "what") != NULL) {
        return snprintf(buf, len, "class=%d", gk_image_classify(im));
    }
    return snprintf(buf, len, "density=%.2f", d);
}

/* ---- 537 voice dialogue ---- */

void gk_voice_dialog_init(gk_voice_dialog *d)
{
    if (d != NULL) {
        memset(d, 0, sizeof(*d));
    }
}

gk_status gk_voice_dialog_say(gk_voice_dialog *d, const char *utterance,
                              char *response, size_t len)
{
    if (d == NULL || utterance == NULL || response == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    d->turns++;
    gk__copy(d->context, sizeof(d->context), utterance);
    if (strstr(utterance, "hello") != NULL ||
        strstr(utterance, "你好") != NULL) {
        snprintf(response, len, "Hello, how can I help with your CNC?");
    } else if (strstr(utterance, "alarm") != NULL ||
               strstr(utterance, "报警") != NULL) {
        snprintf(response, len, "Which alarm code do you see?");
    } else {
        snprintf(response, len, "I heard: %s", utterance);
    }
    return GK_OK;
}

/* ---- 538 gesture ---- */

const char *gk_gesture_name(gk_gesture g)
{
    switch (g) {
    case GK_GESTURE_SWIPE_LEFT: return "swipe-left";
    case GK_GESTURE_SWIPE_RIGHT: return "swipe-right";
    case GK_GESTURE_ROTATE: return "rotate";
    case GK_GESTURE_PINCH: return "pinch";
    case GK_GESTURE_TAP: return "tap";
    default: return "none";
    }
}

gk_gesture gk_gesture_classify(double x0, double y0, double x1, double y1,
                               double scale_delta)
{
    double dx = x1 - x0;
    double dy = y1 - y0;
    if (fabs(scale_delta) > 0.2) {
        return GK_GESTURE_PINCH;
    }
    if (fabs(dx) < 5.0 && fabs(dy) < 5.0) {
        return GK_GESTURE_TAP;
    }
    if (fabs(dx) > fabs(dy)) {
        return dx < 0.0 ? GK_GESTURE_SWIPE_LEFT : GK_GESTURE_SWIPE_RIGHT;
    }
    return GK_GESTURE_ROTATE;
}

/* ---- 539 expression ---- */

const char *gk_face_expression_name(gk_face_expression e)
{
    switch (e) {
    case GK_FACE_HAPPY: return "happy";
    case GK_FACE_CONFUSED: return "confused";
    case GK_FACE_FRUSTRATED: return "frustrated";
    default: return "neutral";
    }
}

gk_face_expression gk_face_classify(double valence, double arousal)
{
    if (valence > 0.3) {
        return GK_FACE_HAPPY;
    }
    if (valence < -0.3 && arousal > 0.3) {
        return GK_FACE_FRUSTRATED;
    }
    if (valence < -0.3) {
        return GK_FACE_CONFUSED;
    }
    return GK_FACE_NEUTRAL;
}

/* ---- 540 multimodal diagnosis ---- */

gk_status gk_multimodal_diagnose(const gk_multimodal_input *in,
                                 gk_multimodal_result *out)
{
    int total;
    int max;
    if (in == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    memset(out, 0, sizeof(*out));
    total = in->error_score + in->voice_score + in->face_score +
            in->gesture_score;
    max = 4;
    out->confidence = (double)total / (double)max;
    if (total >= 3) {
        gk__copy(out->diagnosis, sizeof(out->diagnosis),
                 "Learner is struggling; provide immediate help.");
    } else if (total == 2) {
        gk__copy(out->diagnosis, sizeof(out->diagnosis),
                 "Learner shows mild confusion; offer a hint.");
    } else {
        gk__copy(out->diagnosis, sizeof(out->diagnosis),
                 "Learner is on track; continue.");
    }
    return GK_OK;
}
