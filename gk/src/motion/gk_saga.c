#include "gk/gk_saga.h"

#include <math.h>
#include <string.h>

static void gk_saga_copy(char *dst, const char *src, size_t cap)
{
    size_t n;
    if (dst == NULL || cap == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    n = strlen(src);
    if (n >= cap) {
        n = cap - 1;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static void gk_saga_cat(char *dst, const char *src, size_t cap)
{
    size_t d;
    if (dst == NULL || src == NULL || cap == 0) {
        return;
    }
    d = strlen(dst);
    if (d >= cap) {
        return;
    }
    gk_saga_copy(dst + d, src, cap - d);
}

static int gk_saga_streq(const char *a, const char *b)
{
    if (a == NULL || b == NULL) {
        return a == b;
    }
    return strcmp(a, b) == 0;
}

static double clamp01(double v)
{
    if (v < 0.0) {
        return 0.0;
    }
    if (v > 1.0) {
        return 1.0;
    }
    return v;
}

/* ===================================================================
 * 769 finished part -> program
 * =================================================================== */

gk_status gk_deduce_program_from_part(const gk_deduce_part *part,
                                      gk_deduce_program *out)
{
    if (part == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (part->width <= 0.0 || part->height <= 0.0 || part->depth <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    out->operations = 2; /* face + contour */
    out->operations += part->holes * 1;
    out->operations += part->slot_length > 0.0 ? 1 : 0;
    out->program[0] = '\0';
    gk_saga_copy(out->program, "G21 G90 G54\nG0 Z5\n", sizeof(out->program));
    gk_saga_cat(out->program, "T1 M6\nS2000 M3\nG0 X0 Y0\n", sizeof(out->program));
    gk_saga_cat(out->program, "G1 Z-2 F100\nG1 X100 Y0 F300\n", sizeof(out->program));
    gk_saga_cat(out->program, "G1 X100 Y80\nG1 X0 Y80\nG1 X0 Y0\n", sizeof(out->program));
    if (part->holes > 0) {
        gk_saga_cat(out->program, "\n( drilling )\nG81 Z-5 R2\n", sizeof(out->program));
    }
    gk_saga_cat(out->program, "\nM30\n", sizeof(out->program));
    out->estimated_minutes = part->depth * 0.5 + part->holes * 0.2 +
                             part->width * 0.01;
    return GK_OK;
}

/* ===================================================================
 * 770 sound -> state
 * =================================================================== */

const char *gk_deduce_sound_name(gk_deduce_sound s)
{
    switch (s) {
    case GK_DEDUCE_SOUND_IDLE: return "idle";
    case GK_DEDUCE_SOUND_CUTTING: return "cutting";
    case GK_DEDUCE_SOUND_CHATTER: return "chatter";
    case GK_DEDUCE_SOUND_TOOL_WEAR: return "tool-wear";
    case GK_DEDUCE_SOUND_RAPID: return "rapid";
    default: return "unknown";
    }
}

gk_status gk_deduce_state_from_sound(double dominant_hz, double amplitude_db,
                                     gk_deduce_acoustic *out)
{
    if (out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (dominant_hz < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    out->confidence = 0.5;
    out->suggested_rpm = 2000;
    out->suggested_feed = 300;
    if (amplitude_db < 20.0 && dominant_hz < 100.0) {
        out->state = GK_DEDUCE_SOUND_IDLE;
        out->confidence = 0.9;
        out->suggested_rpm = 0;
        out->suggested_feed = 0;
    } else if (dominant_hz < 500.0) {
        out->state = GK_DEDUCE_SOUND_CUTTING;
        out->confidence = 0.8;
    } else if (dominant_hz > 3000.0) {
        out->state = GK_DEDUCE_SOUND_CHATTER;
        out->confidence = 0.7;
        out->suggested_rpm = 1600;
    } else if (amplitude_db > 85.0) {
        out->state = GK_DEDUCE_SOUND_TOOL_WEAR;
        out->confidence = 0.6;
        out->suggested_feed = 200;
    } else {
        out->state = GK_DEDUCE_SOUND_RAPID;
        out->confidence = 0.5;
    }
    return GK_OK;
}

/* ===================================================================
 * 771 curve -> parameters (least squares)
 * =================================================================== */

gk_status gk_deduce_params_from_curve(const double *x, const double *y,
                                      int n, gk_deduce_line *out)
{
    int i;
    double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0, syy = 0.0;
    double denom;
    if (x == NULL || y == NULL || out == NULL || n < 2) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < n; i++) {
        sx += x[i];
        sy += y[i];
        sxx += x[i] * x[i];
        sxy += x[i] * y[i];
        syy += y[i] * y[i];
    }
    denom = n * sxx - sx * sx;
    if (fabs(denom) < 1e-12) {
        return GK_ERR_OUT_OF_RANGE; /* vertical / degenerate line */
    }
    out->slope = (n * sxy - sx * sy) / denom;
    out->intercept = (sy - out->slope * sx) / n;
    {
        double num = n * sxy - sx * sy;
        double den1 = sqrt(n * sxx - sx * sx);
        double den2 = sqrt(n * syy - sy * sy);
        if (den1 > 1e-12 && den2 > 1e-12) {
            double r = num / (den1 * den2);
            out->r_squared = r * r;
        } else {
            out->r_squared = 0.0;
        }
    }
    return GK_OK;
}

/* ===================================================================
 * 772 texture -> tool
 * =================================================================== */

gk_status gk_deduce_tool_from_texture(double scallop_height,
                                      double cutter_radius,
                                      gk_deduce_tool *out)
{
    if (out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (scallop_height <= 0.0 || cutter_radius <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    /* cusp height h = f^2 / (8 R)  =>  f = sqrt(8 R h) for a ball-nose */
    out->feed_per_tooth = sqrt(8.0 * cutter_radius * scallop_height);
    out->diameter = cutter_radius * 2.0;
    out->flutes = 2;
    return GK_OK;
}

/* ===================================================================
 * 773 alarm -> action
 * =================================================================== */

gk_status gk_deduce_action_from_alarm(const char *alarm_code,
                                      gk_deduce_action *out)
{
    if (alarm_code == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    gk_saga_copy(out->code, alarm_code, sizeof(out->code));
    if (gk_saga_streq(alarm_code, "E041")) {
        gk_saga_copy(out->action, "Reduce spindle load; check tool wear",
                     sizeof(out->action));
        out->severity = 3;
    } else if (gk_saga_streq(alarm_code, "E001")) {
        gk_saga_copy(out->action, "Press reset and re-reference axes",
                     sizeof(out->action));
        out->severity = 2;
    } else if (gk_saga_streq(alarm_code, "E100")) {
        gk_saga_copy(out->action, "Check coolant level and flow switch",
                     sizeof(out->action));
        out->severity = 1;
    } else {
        gk_saga_copy(out->action, "Refer to the machine manual",
                     sizeof(out->action));
        out->severity = 1;
    }
    return GK_OK;
}

/* ===================================================================
 * 774 scrap -> cause
 * =================================================================== */

const char *gk_deduce_cause_name(gk_deduce_cause c)
{
    switch (c) {
    case GK_DEDUCE_CAUSE_NONE: return "none";
    case GK_DEDUCE_CAUSE_WRONG_TOOL: return "wrong-tool";
    case GK_DEDUCE_CAUSE_WORN_TOOL: return "worn-tool";
    case GK_DEDUCE_CAUSE_WRONG_FEED: return "wrong-feed";
    case GK_DEDUCE_CAUSE_LOOSE_CLAMP: return "loose-clamp";
    default: return "unknown";
    }
}

gk_status gk_deduce_cause_from_scrap(double dimension_error,
                                     double surface_error,
                                     gk_deduce_cause *out)
{
    if (out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (fabs(dimension_error) < 1e-9 && fabs(surface_error) < 1e-9) {
        *out = GK_DEDUCE_CAUSE_NONE;
    } else if (dimension_error > 0.05 && surface_error < 0.05) {
        *out = GK_DEDUCE_CAUSE_WRONG_TOOL;
    } else if (dimension_error > 0.05 && surface_error > 0.05) {
        *out = GK_DEDUCE_CAUSE_WORN_TOOL;
    } else if (fabs(dimension_error) < 0.05 && surface_error > 0.1) {
        *out = GK_DEDUCE_CAUSE_WRONG_FEED;
    } else {
        *out = GK_DEDUCE_CAUSE_LOOSE_CLAMP;
    }
    return GK_OK;
}

/* ===================================================================
 * 775 cost -> craft
 * =================================================================== */

gk_status gk_deduce_craft_from_cost(const gk_deduce_craft_input *in,
                                    double budget_per_unit,
                                    gk_deduce_craft *out)
{
    double cycle;
    if (in == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (in->quantity <= 0 || in->machine_rate < 0.0 || in->labor_rate < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    cycle = 2.0 + in->setup_minutes / in->quantity;
    out->per_unit_minutes = cycle;
    out->unit_cost = in->material_cost +
                     cycle / 60.0 * (in->machine_rate + in->labor_rate);
    out->recommendation[0] = '\0';
    if (budget_per_unit > 0.0 && out->unit_cost > budget_per_unit) {
        gk_saga_copy(out->recommendation,
                     "Reduce cycle time or negotiate a higher price",
                     sizeof(out->recommendation));
    } else {
        gk_saga_copy(out->recommendation, "Cost is within budget",
                     sizeof(out->recommendation));
    }
    return GK_OK;
}

/* ===================================================================
 * 776 toolpath -> feature
 * =================================================================== */

gk_status gk_deduce_feature_from_toolpath(const double *toolpath_x,
                                          const double *toolpath_y, int n,
                                          gk_deduce_feature *out)
{
    int i;
    double minx, maxx, miny, maxy;
    double path_len = 0.0;
    if (toolpath_x == NULL || toolpath_y == NULL || out == NULL || n <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    out->contour_segments = n;
    out->pocket_regions = 0;
    out->holes = 0;
    minx = maxx = toolpath_x[0];
    miny = maxy = toolpath_y[0];
    for (i = 1; i < n; i++) {
        double dx = toolpath_x[i] - toolpath_x[i - 1];
        double dy = toolpath_y[i] - toolpath_y[i - 1];
        path_len += sqrt(dx * dx + dy * dy);
        if (toolpath_x[i] < minx) minx = toolpath_x[i];
        if (toolpath_x[i] > maxx) maxx = toolpath_x[i];
        if (toolpath_y[i] < miny) miny = toolpath_y[i];
        if (toolpath_y[i] > maxy) maxy = toolpath_y[i];
    }
    out->bounding_volume = (maxx - minx) * (maxy - miny);
    /* closed loops with small perimeter indicate holes */
    {
        double span = (maxx - minx) + (maxy - miny);
        if (span > 0.0 && path_len > span * 4.0) {
            out->pocket_regions = 1;
        }
        if (path_len < span * 0.5 + 1e-9) {
            out->holes = 1;
        }
    }
    return GK_OK;
}

/* ===================================================================
 * Part B: narrative & gamification
 * =================================================================== */

/* 777 mentor / apprentice */
void gk_saga_actor_init(gk_saga_actor *a, const char *name)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    gk_saga_copy(a->name, name, sizeof(a->name));
    a->level = 1;
    a->skill = 0.1;
    a->reputation = 0.0;
}

gk_status gk_saga_mentor_teach(gk_saga_actor *mentor,
                               gk_saga_actor *apprentice, double amount)
{
    if (mentor == NULL || apprentice == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (amount <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    /* transfer skill limited by the mentor's own skill and a 0.2 gap cap */
    {
        double gain = amount * (mentor->skill - apprentice->skill);
        if (gain <= 0.0) {
            return GK_ERR_STATE; /* apprentice already at mentor level */
        }
        if (gain > 0.2) {
            gain = 0.2;
        }
        apprentice->skill += gain;
    }
    apprentice->level = 1 + (int)(apprentice->skill * 10.0);
    apprentice->reputation += amount * 0.5;
    return GK_OK;
}

/* 778 factory order intake */
void gk_saga_factory_init(gk_saga_factory *f, double starting_cash)
{
    if (f == NULL) {
        return;
    }
    memset(f, 0, sizeof(*f));
    f->cash = starting_cash;
}

int gk_saga_factory_intake(gk_saga_factory *f, const gk_saga_order *order)
{
    gk_saga_order *slot;
    if (f == NULL || order == NULL) {
        return -1;
    }
    if (order->quantity <= 0 || f->count >= GK_SAGA_MAX_ITEMS) {
        return -1;
    }
    slot = &f->orders[f->count];
    *slot = *order;
    f->count++;
    return f->count; /* order id is 1-based */
}

gk_status gk_saga_factory_fulfil(gk_saga_factory *f, int order_id)
{
    gk_saga_order *o;
    if (f == NULL || order_id <= 0 || order_id > f->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    o = &f->orders[order_id - 1];
    if (o->quantity <= 0) {
        return GK_ERR_STATE; /* removed / already fulfilled */
    }
    if (o->deadline_days <= 0.0) {
        /* missed deadline: penalty of half the revenue */
        f->cash += o->unit_price * o->quantity * 0.5;
    } else {
        f->cash += o->unit_price * o->quantity;
    }
    o->quantity = 0;
    return GK_OK;
}

/* 779 incident review */
void gk_saga_incident_init(gk_saga_incident *i, const char *title)
{
    if (i == NULL) {
        return;
    }
    memset(i, 0, sizeof(*i));
    gk_saga_copy(i->title, title, sizeof(i->title));
}

gk_status gk_saga_incident_review(gk_saga_incident *i, const char *root_cause,
                                  const char *lesson)
{
    if (i == NULL || root_cause == NULL || lesson == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    gk_saga_copy(i->root_cause, root_cause, sizeof(i->root_cause));
    gk_saga_copy(i->lesson, lesson, sizeof(i->lesson));
    i->preventable = 1;
    return GK_OK;
}

/* 780 technical breakthrough */
void gk_saga_challenge_init(gk_saga_challenge *c, const char *topic)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    gk_saga_copy(c->topic, topic, sizeof(c->topic));
}

gk_status gk_saga_challenge_attempt(gk_saga_challenge *c, double effort)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (c->solved) {
        return GK_ERR_STATE;
    }
    if (effort <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    c->attempts++;
    /* diminishing returns: each attempt adds effort/(1+attempts) */
    c->progress = clamp01(c->progress + effort / (double)(1 + c->attempts));
    if (c->progress >= 1.0) {
        c->solved = 1;
    }
    return GK_OK;
}

/* 781 startup mode */
void gk_saga_startup_init(gk_saga_startup *s, double capital)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->capital = capital;
}

gk_status gk_saga_startup_simulate_month(gk_saga_startup *s, double income,
                                         double expense)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (s->bankrupt) {
        return GK_ERR_STATE;
    }
    if (income < 0.0 || expense < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    s->months_active++;
    s->revenue += income;
    s->capital += income - expense;
    /* each profitable month can fund another employee */
    if (income > expense) {
        s->employees += 1;
    }
    if (s->capital <= 0.0) {
        s->capital = 0.0;
        s->bankrupt = 1;
    }
    return GK_OK;
}

/* 782 branching plot */
void gk_saga_plot_init(gk_saga_plot *p)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->current = -1;
}

int gk_saga_plot_add(gk_saga_plot *p, const char *text, int next_a, int next_b,
                     int terminal)
{
    gk_saga_branch *b;
    if (p == NULL || p->count >= GK_SAGA_MAX_ITEMS) {
        return -1;
    }
    b = &p->nodes[p->count];
    memset(b, 0, sizeof(*b));
    b->id = p->count + 1;
    gk_saga_copy(b->text, text, sizeof(b->text));
    b->next_a = next_a;
    b->next_b = next_b;
    b->terminal = terminal;
    p->count++;
    return b->id;
}

gk_status gk_saga_plot_start(gk_saga_plot *p, int node_id)
{
    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (node_id < 1 || node_id > p->count) {
        return GK_ERR_NOT_FOUND;
    }
    p->current = node_id;
    p->decisions = 0;
    return GK_OK;
}

gk_status gk_saga_plot_choose(gk_saga_plot *p, int branch)
{
    const gk_saga_branch *cur;
    int next;
    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    cur = gk_saga_plot_current(p);
    if (cur == NULL) {
        return GK_ERR_STATE;
    }
    if (cur->terminal) {
        return GK_ERR_STATE;
    }
    next = (branch == 0) ? cur->next_a : cur->next_b;
    if (next < 1 || next > p->count) {
        return GK_ERR_NOT_FOUND;
    }
    p->current = next;
    p->decisions++;
    return GK_OK;
}

const gk_saga_branch *gk_saga_plot_current(const gk_saga_plot *p)
{
    if (p == NULL || p->current < 1 || p->current > p->count) {
        return NULL;
    }
    return &p->nodes[p->current - 1];
}

/* 783 NPC dialogue */
void gk_saga_npc_script_init(gk_saga_npc_script *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
}

int gk_saga_npc_add(gk_saga_npc_script *s, const char *speaker,
                    const char *line, int mood)
{
    gk_saga_npc_line *l;
    if (s == NULL || line == NULL || s->count >= GK_SAGA_MAX_ITEMS) {
        return -1;
    }
    l = &s->lines[s->count];
    memset(l, 0, sizeof(*l));
    l->id = s->count + 1;
    gk_saga_copy(l->speaker, speaker, sizeof(l->speaker));
    gk_saga_copy(l->line, line, sizeof(l->line));
    l->mood = mood;
    s->count++;
    return l->id;
}

const char *gk_saga_npc_greet(const gk_saga_npc_script *s, int mood)
{
    int i;
    if (s == NULL) {
        return NULL;
    }
    for (i = 0; i < s->count; i++) {
        if (s->lines[i].mood == mood) {
            return s->lines[i].line;
        }
    }
    return s->count > 0 ? s->lines[0].line : NULL;
}

/* 784-785 immersion */
void gk_saga_immersion_init(gk_saga_immersion *im)
{
    if (im == NULL) {
        return;
    }
    memset(im, 0, sizeof(*im));
    im->view = GK_SAGA_VIEW_FIRST_PERSON;
    im->head_height = 1.7;
    im->fov_deg = 90.0;
    im->hands_visible = 1;
    im->voice_enabled = 0;
}

gk_status gk_saga_immersion_set_view(gk_saga_immersion *im, gk_saga_view v)
{
    if (im == NULL || v > GK_SAGA_VIEW_THIRD_PERSON) {
        return GK_ERR_INVALID_ARG;
    }
    im->view = v;
    return GK_OK;
}

double gk_saga_immersion_score(const gk_saga_immersion *im)
{
    double score = 0.0;
    if (im == NULL) {
        return 0.0;
    }
    if (im->view == GK_SAGA_VIEW_FIRST_PERSON) {
        score += 0.5;
    } else {
        score += 0.2;
    }
    if (im->hands_visible) {
        score += 0.2;
    }
    if (im->voice_enabled) {
        score += 0.2;
    }
    if (im->fov_deg >= 90.0 && im->fov_deg <= 120.0) {
        score += 0.1;
    }
    return clamp01(score);
}

/* 786 quest system */
void gk_saga_quest_log_init(gk_saga_quest_log *l)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
}

int gk_saga_quest_add(gk_saga_quest_log *l, const char *title, double target,
                      int reward)
{
    gk_saga_quest *q;
    if (l == NULL || title == NULL || l->count >= GK_SAGA_MAX_ITEMS) {
        return -1;
    }
    if (target <= 0.0 || reward < 0) {
        return -1;
    }
    q = &l->quests[l->count];
    memset(q, 0, sizeof(*q));
    q->id = l->count + 1;
    gk_saga_copy(q->title, title, sizeof(q->title));
    q->state = GK_SAGA_QUEST_AVAILABLE;
    q->progress = 0.0;
    q->target = target;
    q->reward_points = reward;
    l->count++;
    return q->id;
}

gk_status gk_saga_quest_accept(gk_saga_quest_log *l, int id)
{
    if (l == NULL || id < 1 || id > l->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    if (l->quests[id - 1].state != GK_SAGA_QUEST_AVAILABLE) {
        return GK_ERR_STATE;
    }
    l->quests[id - 1].state = GK_SAGA_QUEST_ACTIVE;
    return GK_OK;
}

gk_status gk_saga_quest_progress(gk_saga_quest_log *l, int id, double delta)
{
    gk_saga_quest *q;
    if (l == NULL || id < 1 || id > l->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    q = &l->quests[id - 1];
    if (q->state != GK_SAGA_QUEST_ACTIVE) {
        return GK_ERR_STATE;
    }
    if (delta <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    q->progress += delta;
    if (q->progress >= q->target) {
        q->progress = q->target;
        q->state = GK_SAGA_QUEST_COMPLETE;
        l->completed++;
        l->total_points += q->reward_points;
    }
    return GK_OK;
}

gk_status gk_saga_quest_complete(gk_saga_quest_log *l, int id)
{
    gk_saga_quest *q;
    if (l == NULL || id < 1 || id > l->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    q = &l->quests[id - 1];
    if (q->state == GK_SAGA_QUEST_COMPLETE) {
        return GK_ERR_STATE;
    }
    q->progress = q->target;
    q->state = GK_SAGA_QUEST_COMPLETE;
    l->completed++;
    l->total_points += q->reward_points;
    return GK_OK;
}

/* 787 achievements */
void gk_saga_achievements_init(gk_saga_achievements *a)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
}

int gk_saga_achievement_add(gk_saga_achievements *a, const char *id,
                            const char *name, const char *desc, int points)
{
    gk_saga_achievement *ac;
    if (a == NULL || id == NULL || name == NULL ||
        a->count >= GK_SAGA_MAX_ITEMS) {
        return -1;
    }
    ac = &a->items[a->count];
    memset(ac, 0, sizeof(*ac));
    gk_saga_copy(ac->id, id, sizeof(ac->id));
    gk_saga_copy(ac->name, name, sizeof(ac->name));
    gk_saga_copy(ac->description, desc, sizeof(ac->description));
    ac->points = points;
    ac->unlocked = 0;
    a->count++;
    return a->count;
}

gk_status gk_saga_achievement_unlock(gk_saga_achievements *a, const char *id)
{
    int i;
    if (a == NULL || id == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < a->count; i++) {
        if (gk_saga_streq(a->items[i].id, id)) {
            if (a->items[i].unlocked) {
                return GK_ERR_STATE;
            }
            a->items[i].unlocked = 1;
            a->total_points += a->items[i].points;
            return GK_OK;
        }
    }
    return GK_ERR_NOT_FOUND;
}

int gk_saga_achievement_is_unlocked(const gk_saga_achievements *a,
                                    const char *id)
{
    int i;
    if (a == NULL || id == NULL) {
        return 0;
    }
    for (i = 0; i < a->count; i++) {
        if (gk_saga_streq(a->items[i].id, id)) {
            return a->items[i].unlocked;
        }
    }
    return 0;
}

/* 788 titles */
void gk_saga_titles_init(gk_saga_titles *t)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
}

int gk_saga_title_add(gk_saga_titles *t, const char *id, const char *title,
                      double threshold)
{
    gk_saga_title_def *d;
    if (t == NULL || id == NULL || title == NULL ||
        t->count >= GK_SAGA_MAX_ITEMS) {
        return -1;
    }
    d = &t->defs[t->count];
    memset(d, 0, sizeof(*d));
    gk_saga_copy(d->id, id, sizeof(d->id));
    gk_saga_copy(d->title, title, sizeof(d->title));
    d->threshold = threshold;
    d->granted = 0;
    t->count++;
    return t->count;
}

const char *gk_saga_title_evaluate(gk_saga_titles *t, double points)
{
    int i;
    int best = -1;
    double best_thresh = -1.0;
    if (t == NULL) {
        return NULL;
    }
    for (i = 0; i < t->count; i++) {
        if (points >= t->defs[i].threshold &&
            t->defs[i].threshold > best_thresh) {
            best_thresh = t->defs[i].threshold;
            best = i;
        }
    }
    if (best < 0) {
        return NULL;
    }
    t->defs[best].granted = 1;
    gk_saga_copy(t->current, t->defs[best].title, sizeof(t->current));
    return t->current;
}
