#include "gk/gk_teach.h"

#include <string.h>
#include <math.h>
#include <stdio.h>

/* ================= course ================= */

void gk_course_init(gk_course *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
}

gk_status gk_course_add_lesson(gk_course *c, int id, const char *name,
                               const char *json)
{
    gk_lesson *l;
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (c->lesson_count >= GK_COURSE_MAX_LESSONS) {
        return GK_ERR_OVERFLOW;
    }
    l = &c->lessons[c->lesson_count];
    memset(l, 0, sizeof(*l));
    l->id = id;
    if (name != NULL) {
        size_t i;
        for (i = 0; i < sizeof(l->name) - 1 && name[i] != '\0'; ++i) {
            l->name[i] = name[i];
        }
    }
    if (json != NULL) {
        size_t i;
        for (i = 0; i < sizeof(l->json) - 1 && json[i] != '\0'; ++i) {
            l->json[i] = json[i];
        }
    }
    c->lesson_count += 1;
    return GK_OK;
}

gk_status gk_course_add_step(gk_course *c, size_t lesson_index,
                             const gk_lesson_step *step)
{
    gk_lesson *l;
    if (c == NULL || step == NULL || lesson_index >= c->lesson_count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    l = &c->lessons[lesson_index];
    if (l->step_count >= GK_COURSE_MAX_STEPS) {
        return GK_ERR_OVERFLOW;
    }
    l->steps[l->step_count] = *step;
    l->step_count += 1;
    return GK_OK;
}

gk_status gk_course_goto(gk_course *c, size_t lesson_index, size_t step)
{
    if (c == NULL || lesson_index >= c->lesson_count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    if (step >= c->lessons[lesson_index].step_count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    c->current_lesson = lesson_index;
    c->current_step = step;
    return GK_OK;
}

gk_status gk_course_next_step(gk_course *c)
{
    if (c == NULL || c->lesson_count == 0) {
        return GK_ERR_STATE;
    }
    if (c->current_step + 1 < c->lessons[c->current_lesson].step_count) {
        c->current_step += 1;
        return GK_OK;
    }
    if (c->current_lesson + 1 < c->lesson_count) {
        c->current_lesson += 1;
        c->current_step = 0;
        return GK_OK;
    }
    c->completed = 1;
    return GK_ERR_OUT_OF_RANGE;
}

gk_status gk_course_prev_step(gk_course *c)
{
    if (c == NULL || c->lesson_count == 0) {
        return GK_ERR_STATE;
    }
    if (c->current_step > 0) {
        c->current_step -= 1;
        return GK_OK;
    }
    if (c->current_lesson > 0) {
        c->current_lesson -= 1;
        c->current_step = c->lessons[c->current_lesson].step_count - 1;
        return GK_OK;
    }
    return GK_ERR_OUT_OF_RANGE;
}

const gk_lesson_step *gk_course_current_step(const gk_course *c)
{
    if (c == NULL || c->lesson_count == 0) {
        return NULL;
    }
    if (c->current_step >= c->lessons[c->current_lesson].step_count) {
        return NULL;
    }
    return &c->lessons[c->current_lesson].steps[c->current_step];
}

const char *gk_course_current_highlight(const gk_course *c)
{
    const gk_lesson_step *s = gk_course_current_step(c);
    return s != NULL ? s->highlight : "";
}

/* progress */

void gk_progress_init(gk_progress *p)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
}

gk_status gk_progress_mark(gk_progress *p, size_t lesson, size_t step,
                           double time_spent)
{
    if (p == NULL || lesson >= GK_COURSE_MAX_LESSONS ||
        step >= GK_COURSE_MAX_STEPS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    if (!p->steps_done[lesson][step]) {
        p->steps_done[lesson][step] = 1;
        p->total_steps_done += 1;
    }
    p->total_time += time_spent;
    return GK_OK;
}

int gk_progress_is_done(const gk_progress *p, size_t lesson, size_t step)
{
    if (p == NULL || lesson >= GK_COURSE_MAX_LESSONS ||
        step >= GK_COURSE_MAX_STEPS) {
        return 0;
    }
    return p->steps_done[lesson][step] ? 1 : 0;
}

double gk_progress_completion(const gk_progress *p, const gk_course *c)
{
    size_t total = 0;
    size_t done = 0;
    size_t i, j;
    if (p == NULL || c == NULL) {
        return 0.0;
    }
    for (i = 0; i < c->lesson_count; ++i) {
        for (j = 0; j < c->lessons[i].step_count; ++j) {
            total += 1;
            if (p->steps_done[i][j]) {
                done += 1;
            }
        }
    }
    if (total == 0) {
        return 0.0;
    }
    return (double)done / (double)total;
}

/* ================= manuals ================= */

static const gk_manual_entry g_gmanual[] = {
    {0, "G00", "Rapid positioning", 0},
    {1, "G01", "Linear interpolation", 0},
    {2, "G02", "Circular interpolation CW", 0},
    {3, "G03", "Circular interpolation CCW", 0},
    {4, "G04", "Dwell", 0},
    {17, "G17", "XY plane selection", 0},
    {21, "G21", "Metric units", 0},
    {28, "G28", "Return to reference point", 0},
    {40, "G40", "Cancel cutter compensation", 0},
    {41, "G41", "Cutter compensation left", 0},
    {42, "G42", "Cutter compensation right", 0},
    {43, "G43", "Tool length compensation +", 0},
    {80, "G80", "Cancel canned cycle", 0},
    {81, "G81", "Drilling canned cycle", 0},
    {83, "G83", "Peck drilling cycle", 0},
    {84, "G84", "Tapping cycle", 0},
    {90, "G90", "Absolute programming", 0},
    {91, "G91", "Incremental programming", 0}
};

static const gk_manual_entry g_mmanual[] = {
    {0, "M00", "Program stop", 1},
    {1, "M01", "Optional stop", 1},
    {2, "M02", "Program end", 1},
    {3, "M03", "Spindle forward", 1},
    {4, "M04", "Spindle reverse", 1},
    {5, "M05", "Spindle stop", 1},
    {6, "M06", "Automatic tool change", 1},
    {8, "M08", "Coolant on", 1},
    {9, "M09", "Coolant off", 1},
    {30, "M30", "Program end and reset", 1},
    {98, "M98", "Subprogram call", 1},
    {99, "M99", "Subprogram return", 1}
};

const gk_manual_entry *gk_manual_lookup(int code, int is_mcode)
{
    const gk_manual_entry *set = is_mcode ? g_mmanual : g_gmanual;
    size_t n = is_mcode
                   ? sizeof(g_mmanual) / sizeof(g_mmanual[0])
                   : sizeof(g_gmanual) / sizeof(g_gmanual[0]);
    size_t i;
    for (i = 0; i < n; ++i) {
        if (set[i].code == code) {
            return &set[i];
        }
    }
    return NULL;
}

const gk_manual_entry *gk_manual_g(int code)
{
    return gk_manual_lookup(code, 0);
}

const gk_manual_entry *gk_manual_m(int code)
{
    return gk_manual_lookup(code, 1);
}

size_t gk_manual_count(int is_mcode)
{
    return is_mcode ? sizeof(g_mmanual) / sizeof(g_mmanual[0])
                    : sizeof(g_gmanual) / sizeof(g_gmanual[0]);
}

/* ================= shortcuts ================= */

void gk_shortcut_set_init(gk_shortcut_set *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
}

gk_status gk_shortcut_add(gk_shortcut_set *s, const char *keys,
                          const char *action)
{
    gk_shortcut *item;
    if (s == NULL || keys == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (s->count >= 32) {
        return GK_ERR_OVERFLOW;
    }
    item = &s->items[s->count++];
    memset(item, 0, sizeof(*item));
    {
        size_t i;
        for (i = 0; i < sizeof(item->keys) - 1 && keys[i] != '\0'; ++i) {
            item->keys[i] = keys[i];
        }
        if (action != NULL) {
            for (i = 0; i < sizeof(item->action) - 1 && action[i] != '\0';
                 ++i) {
                item->action[i] = action[i];
            }
        }
    }
    return GK_OK;
}

gk_status gk_shortcut_use(gk_shortcut_set *s, const char *keys)
{
    size_t i;
    if (s == NULL || keys == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < s->count; ++i) {
        if (strcmp(s->items[i].keys, keys) == 0) {
            s->items[i].uses += 1;
            return GK_OK;
        }
    }
    return GK_ERR_NOT_FOUND;
}

size_t gk_shortcut_total_uses(const gk_shortcut_set *s)
{
    size_t i, n = 0;
    if (s == NULL) {
        return 0;
    }
    for (i = 0; i < s->count; ++i) {
        n += s->items[i].uses;
    }
    return n;
}

/* ================= score ================= */

void gk_score_init(gk_score *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
}

double gk_score_compute(gk_score *s)
{
    double raw;
    if (s == NULL) {
        return 0.0;
    }
    raw = s->correctness * 0.5 + s->speed * 0.2 + s->efficiency * 0.3;
    s->score = raw * 100.0 - s->penalties;
    if (s->score < 0.0) {
        s->score = 0.0;
    }
    if (s->score > 100.0) {
        s->score = 100.0;
    }
    return s->score;
}

void gk_wrong_log_init(gk_wrong_log *l)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
}

gk_status gk_wrong_log_add(gk_wrong_log *l, int question_id, int chosen,
                           int correct, const char *topic)
{
    size_t i;
    if (l == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    /* merge repeated wrong answers to the same question */
    for (i = 0; i < l->count; ++i) {
        if (l->items[i].question_id == question_id) {
            l->items[i].attempts += 1;
            l->items[i].chosen = chosen;
            return GK_OK;
        }
    }
    if (l->count >= GK_TEACH_MAX_WRONG) {
        return GK_ERR_OVERFLOW;
    }
    l->items[l->count].question_id = question_id;
    l->items[l->count].chosen = chosen;
    l->items[l->count].correct = correct;
    l->items[l->count].attempts = 1;
    if (topic != NULL) {
        size_t k;
        for (k = 0; k < sizeof(l->items[l->count].topic) - 1 &&
                    topic[k] != '\0';
             ++k) {
            l->items[l->count].topic[k] = topic[k];
        }
    }
    l->count += 1;
    return GK_OK;
}

int gk_wrong_log_repeat_count(const gk_wrong_log *l)
{
    size_t i;
    int n = 0;
    if (l == NULL) {
        return 0;
    }
    for (i = 0; i < l->count; ++i) {
        if (l->items[i].attempts > 1) {
            n += 1;
        }
    }
    return n;
}

size_t gk_auto_grade(const int *answers, const int *keys, size_t n)
{
    size_t i;
    size_t correct = 0;
    if (answers == NULL || keys == NULL) {
        return 0;
    }
    for (i = 0; i < n; ++i) {
        if (answers[i] == keys[i]) {
            correct += 1;
        }
    }
    return correct;
}

size_t gk_transcript_export(const gk_score *score,
                            const gk_wrong_log *log, char *out,
                            size_t out_size)
{
    size_t written = 0;
    size_t i;
    if (out == NULL || out_size == 0) {
        return 0;
    }
    if (score != NULL) {
        written += (size_t)snprintf(out + written, out_size - written,
                                    "score=%.1f correctness=%.2f\n",
                                    score->score, score->correctness);
    }
    if (log != NULL) {
        for (i = 0; i < log->count && written + 1 < out_size; ++i) {
            written += (size_t)snprintf(
                out + written, out_size - written,
                "wrong q%d chose %d (correct %d) topic=%s attempts=%zu\n",
                log->items[i].question_id, log->items[i].chosen,
                log->items[i].correct, log->items[i].topic,
                log->items[i].attempts);
        }
    }
    return written;
}

void gk_exam_init(gk_exam *e)
{
    if (e == NULL) {
        return;
    }
    memset(e, 0, sizeof(*e));
}

void gk_exam_start(gk_exam *e, double limit_seconds)
{
    if (e == NULL) {
        return;
    }
    e->exam_mode = 1;
    e->limit_seconds = limit_seconds;
    e->elapsed = 0.0;
}

int gk_exam_time_up(const gk_exam *e)
{
    if (e == NULL || e->limit_seconds <= 0.0) {
        return 0;
    }
    return e->elapsed >= e->limit_seconds ? 1 : 0;
}

/* ================= leaderboard / gamification ================= */

void gk_leaderboard_init(gk_leaderboard *b)
{
    if (b == NULL) {
        return;
    }
    memset(b, 0, sizeof(*b));
}

gk_status gk_leaderboard_submit(gk_leaderboard *b, const char *name,
                                int score)
{
    size_t i;
    gk_rank_entry e;
    if (b == NULL || name == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    /* insert keeping descending order */
    if (b->count < GK_LEADERBOARD_MAX) {
        b->count += 1;
    } else if (score <= b->entries[b->count - 1].score) {
        return GK_ERR_OVERFLOW;
    }
    memset(&e, 0, sizeof(e));
    {
        size_t k;
        for (k = 0; k < sizeof(e.name) - 1 && name[k] != '\0'; ++k) {
            e.name[k] = name[k];
        }
    }
    e.score = score;
    i = b->count - 1;
    while (i > 0 && b->entries[i - 1].score < score) {
        b->entries[i] = b->entries[i - 1];
        i -= 1;
    }
    b->entries[i] = e;
    return GK_OK;
}

int gk_leaderboard_rank_of(const gk_leaderboard *b, const char *name)
{
    size_t i;
    if (b == NULL || name == NULL) {
        return -1;
    }
    for (i = 0; i < b->count; ++i) {
        if (strcmp(b->entries[i].name, name) == 0) {
            return (int)i + 1;
        }
    }
    return -1;
}

void gk_player_points_init(gk_player_points *p)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->level = 1;
}

void gk_player_points_add(gk_player_points *p, int points)
{
    if (p == NULL) {
        return;
    }
    p->points += points;
    if (p->points < 0) {
        p->points = 0;
    }
    if (points > 0) {
        p->streak += 1;
    } else if (points < 0) {
        p->streak = 0;
    }
    p->level = gk_player_level_for_points(p->points);
}

int gk_player_level_for_points(int points)
{
    if (points < 0) {
        return 1;
    }
    return points / 100 + 1;
}

void gk_badge_set_init(gk_badge_set *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
}

gk_status gk_badge_add(gk_badge_set *s, const char *id, const char *name,
                       const char *desc, double threshold)
{
    gk_badge *b;
    size_t i;
    if (s == NULL || id == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (s->count >= GK_TEACH_MAX_BADGES) {
        return GK_ERR_OVERFLOW;
    }
    b = &s->items[s->count++];
    memset(b, 0, sizeof(*b));
    for (i = 0; i < sizeof(b->id) - 1 && id[i] != '\0'; ++i) {
        b->id[i] = id[i];
    }
    if (name != NULL) {
        for (i = 0; i < sizeof(b->name) - 1 && name[i] != '\0'; ++i) {
            b->name[i] = name[i];
        }
    }
    if (desc != NULL) {
        for (i = 0; i < sizeof(b->description) - 1 && desc[i] != '\0'; ++i) {
            b->description[i] = desc[i];
        }
    }
    b->threshold = threshold;
    return GK_OK;
}

size_t gk_badge_evaluate(gk_badge_set *s, double value)
{
    size_t i, earned = 0;
    if (s == NULL) {
        return 0;
    }
    for (i = 0; i < s->count; ++i) {
        if (!s->items[i].earned && value >= s->items[i].threshold) {
            s->items[i].earned = 1;
        }
        if (s->items[i].earned) {
            earned += 1;
        }
    }
    return earned;
}

void gk_certificate_build(gk_certificate *cert, const char *holder,
                          const char *course, int score, int y, int m, int d)
{
    if (cert == NULL) {
        return;
    }
    memset(cert, 0, sizeof(*cert));
    if (holder != NULL) {
        size_t i;
        for (i = 0; i < sizeof(cert->holder) - 1 && holder[i] != '\0'; ++i) {
            cert->holder[i] = holder[i];
        }
    }
    if (course != NULL) {
        size_t i;
        for (i = 0; i < sizeof(cert->course) - 1 && course[i] != '\0'; ++i) {
            cert->course[i] = course[i];
        }
    }
    cert->score = score;
    cert->year = y;
    cert->month = m;
    cert->day = d;
    cert->valid = (score >= 60) ? 1 : 0;
}

size_t gk_certificate_text(const gk_certificate *cert, char *out,
                           size_t out_size)
{
    if (cert == NULL || out == NULL || out_size == 0) {
        return 0;
    }
    return (size_t)snprintf(out, out_size,
                            "Certificate: %s completed '%s' with %d (%04d-%02d-%02d) %s",
                            cert->holder, cert->course, cert->score,
                            cert->year, cert->month, cert->day,
                            cert->valid ? "VALID" : "INVALID");
}

/* ================= challenge ================= */

const char *gk_challenge_name(gk_challenge_kind k)
{
    switch (k) {
    case GK_CHALLENGE_STAGE:
        return "stage";
    case GK_CHALLENGE_TIMED:
        return "timed";
    case GK_CHALLENGE_PRECISION:
        return "precision";
    case GK_CHALLENGE_EFFICIENCY:
        return "efficiency";
    case GK_CHALLENGE_TEAM:
        return "team";
    default:
        return "unknown";
    }
}

gk_status gk_challenge_init(gk_challenge *c, gk_challenge_kind kind,
                            double target, double time_limit)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    memset(c, 0, sizeof(*c));
    c->kind = kind;
    c->target = target;
    c->time_limit = time_limit;
    c->members = 1;
    return GK_OK;
}

gk_status gk_challenge_update(gk_challenge *c, double achieved, double dt)
{
    if (c == NULL || dt < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    c->elapsed += dt;
    c->achieved = achieved;
    switch (c->kind) {
    case GK_CHALLENGE_TIMED:
        c->passed = (c->achieved >= c->target &&
                     (c->time_limit <= 0.0 || c->elapsed <= c->time_limit))
                        ? 1
                        : 0;
        break;
    case GK_CHALLENGE_PRECISION:
        /* achieved is an error magnitude: smaller is better */
        c->passed = (c->achieved <= c->target) ? 1 : 0;
        break;
    default:
        c->passed = (c->achieved >= c->target) ? 1 : 0;
        break;
    }
    return GK_OK;
}

int gk_challenge_passed(const gk_challenge *c)
{
    return c == NULL ? 0 : c->passed;
}

/* ================= dialog ================= */

void gk_dialog_init(gk_dialog *d)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    d->current = -1;
}

gk_status gk_dialog_add(gk_dialog *d, int id, const char *speaker,
                        const char *text)
{
    gk_dialog_node *n;
    if (d == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (d->count >= GK_TEACH_MAX_DIALOG) {
        return GK_ERR_OVERFLOW;
    }
    n = &d->nodes[d->count++];
    memset(n, 0, sizeof(*n));
    n->id = id;
    n->next_id = -1;
    n->branch_a = -1;
    n->branch_b = -1;
    if (speaker != NULL) {
        size_t i;
        for (i = 0; i < sizeof(n->speaker) - 1 && speaker[i] != '\0'; ++i) {
            n->speaker[i] = speaker[i];
        }
    }
    if (text != NULL) {
        size_t i;
        for (i = 0; i < sizeof(n->text) - 1 && text[i] != '\0'; ++i) {
            n->text[i] = text[i];
        }
    }
    if (d->current < 0) {
        d->current = 0;
    }
    return GK_OK;
}

static int dialog_find(const gk_dialog *d, int id)
{
    size_t i;
    for (i = 0; i < d->count; ++i) {
        if (d->nodes[i].id == id) {
            return (int)i;
        }
    }
    return -1;
}

const gk_dialog_node *gk_dialog_current(const gk_dialog *d)
{
    if (d == NULL || d->current < 0 || (size_t)d->current >= d->count) {
        return NULL;
    }
    return &d->nodes[d->current];
}

gk_status gk_dialog_advance(gk_dialog *d)
{
    const gk_dialog_node *n;
    int idx;
    if (d == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    n = gk_dialog_current(d);
    if (n == NULL) {
        return GK_ERR_STATE;
    }
    if (n->next_id < 0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    idx = dialog_find(d, n->next_id);
    if (idx < 0) {
        return GK_ERR_NOT_FOUND;
    }
    d->current = idx;
    return GK_OK;
}

gk_status gk_dialog_choose(gk_dialog *d, int branch)
{
    const gk_dialog_node *n;
    int target;
    int idx;
    if (d == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    n = gk_dialog_current(d);
    if (n == NULL) {
        return GK_ERR_STATE;
    }
    target = (branch == 0) ? n->branch_a : n->branch_b;
    if (target < 0) {
        return GK_ERR_NOT_FOUND;
    }
    idx = dialog_find(d, target);
    if (idx < 0) {
        return GK_ERR_NOT_FOUND;
    }
    d->current = idx;
    return GK_OK;
}

/* ================= training modes ================= */

const char *gk_train_mode_name(gk_train_mode m)
{
    switch (m) {
    case GK_TRAIN_FORWARD:
        return "forward";
    case GK_TRAIN_REVERSE:
        return "reverse";
    case GK_TRAIN_BLIND:
        return "blind";
    default:
        return "unknown";
    }
}

void gk_train_session_init(gk_train_session *s, gk_train_mode mode)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->mode = mode;
    if (mode == GK_TRAIN_BLIND) {
        s->ui_hidden = 1;
        s->program_hidden = 1;
    } else if (mode == GK_TRAIN_REVERSE) {
        s->program_hidden = 1;
    }
}

void gk_train_session_error(gk_train_session *s)
{
    if (s == NULL) {
        return;
    }
    s->errors += 1;
}

/* ================= replay ================= */

void gk_replay_init(gk_replay *r)
{
    if (r == NULL) {
        return;
    }
    memset(r, 0, sizeof(*r));
}

gk_status gk_replay_push(gk_replay *r, double timestamp, const double axis[6],
                         int event)
{
    gk_replay_frame *f;
    if (r == NULL || axis == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (r->count >= GK_TEACH_MAX_FRAMES) {
        return GK_ERR_OVERFLOW;
    }
    f = &r->frames[r->count++];
    f->tick = r->count;
    f->timestamp = timestamp;
    memcpy(f->axis, axis, 6 * sizeof(double));
    f->event = event;
    return GK_OK;
}

gk_status gk_replay_start_replay(gk_replay *r)
{
    if (r == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    r->live = 0;
    r->recording = 0;
    r->cursor = 0;
    return GK_OK;
}

const gk_replay_frame *gk_replay_next(gk_replay *r)
{
    if (r == NULL || r->cursor >= r->count) {
        return NULL;
    }
    return &r->frames[r->cursor++];
}

int gk_replay_at_end(const gk_replay *r)
{
    return (r == NULL || r->cursor >= r->count) ? 1 : 0;
}

/* ================= cognitive science ================= */

void gk_heatmap_init(gk_heatmap *h)
{
    if (h == NULL) {
        return;
    }
    memset(h, 0, sizeof(*h));
}

static gk_heat_region *heat_find(gk_heatmap *h, const char *region)
{
    size_t i;
    if (region == NULL) {
        return NULL;
    }
    for (i = 0; i < h->count; ++i) {
        if (strcmp(h->regions[i].region, region) == 0) {
            return &h->regions[i];
        }
    }
    return NULL;
}

gk_status gk_heatmap_add_region(gk_heatmap *h, const char *region)
{
    gk_heat_region *r;
    size_t i;
    if (h == NULL || region == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (heat_find(h, region) != NULL) {
        return GK_ERR_ALREADY_EXISTS;
    }
    if (h->count >= GK_COG_MAX_REGIONS) {
        return GK_ERR_OVERFLOW;
    }
    r = &h->regions[h->count++];
    memset(r, 0, sizeof(*r));
    for (i = 0; i < sizeof(r->region) - 1 && region[i] != '\0'; ++i) {
        r->region[i] = region[i];
    }
    return GK_OK;
}

gk_status gk_heatmap_observe(gk_heatmap *h, const char *region, double dt)
{
    gk_heat_region *r;
    if (h == NULL || dt < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    r = heat_find(h, region);
    if (r == NULL) {
        gk_status st = gk_heatmap_add_region(h, region);
        if (st != GK_OK) {
            return st;
        }
        r = heat_find(h, region);
    }
    r->dwell += dt;
    return GK_OK;
}

double gk_heatmap_weight(const gk_heatmap *h, const char *region)
{
    double total = 0.0;
    size_t i;
    if (h == NULL || region == NULL) {
        return 0.0;
    }
    for (i = 0; i < h->count; ++i) {
        total += h->regions[i].dwell;
    }
    if (total <= 0.0) {
        return 0.0;
    }
    for (i = 0; i < h->count; ++i) {
        if (strcmp(h->regions[i].region, region) == 0) {
            return h->regions[i].dwell / total;
        }
    }
    return 0.0;
}

const char *gk_heatmap_hottest(const gk_heatmap *h)
{
    size_t i;
    size_t best = 0;
    if (h == NULL || h->count == 0) {
        return "";
    }
    for (i = 1; i < h->count; ++i) {
        if (h->regions[i].dwell > h->regions[best].dwell) {
            best = i;
        }
    }
    return h->regions[best].region;
}

double gk_gaze_velocity(const gk_gaze *a, const gk_gaze *b)
{
    double dx, dy, dt;
    if (a == NULL || b == NULL) {
        return 0.0;
    }
    dt = b->t - a->t;
    if (dt <= 0.0) {
        return 0.0;
    }
    dx = b->x - a->x;
    dy = b->y - a->y;
    return sqrt(dx * dx + dy * dy) / dt;
}

int gk_gaze_is_fixation(const gk_gaze *a, const gk_gaze *b, double vel_thresh)
{
    return gk_gaze_velocity(a, b) < vel_thresh ? 1 : 0;
}

double gk_cognitive_load(double task_demand, double working_capacity)
{
    if (working_capacity <= 0.0) {
        return 1.0;
    }
    return task_demand / working_capacity;
}

const char *gk_cognitive_load_level(double load)
{
    if (load < 0.4) {
        return "low";
    }
    if (load < 0.8) {
        return "optimal";
    }
    if (load < 1.2) {
        return "high";
    }
    return "overload";
}

double gk_memory_retention(double stability_days, double elapsed_days)
{
    if (stability_days <= 0.0) {
        return 0.0;
    }
    return exp(-elapsed_days / stability_days);
}

double gk_next_review_interval(double previous_interval, int quality)
{
    double factor;
    double base = previous_interval > 0.0 ? previous_interval : 1.0;
    switch (quality) {
    case 0:
        factor = 0.5;
        break;
    case 1:
        factor = 0.8;
        break;
    case 2:
        factor = 1.2;
        break;
    case 3:
        factor = 1.8;
        break;
    default:
        factor = 2.5;
        break;
    }
    return base * factor;
}

size_t gk_micro_session_count(size_t total_items, size_t per_session)
{
    if (per_session == 0) {
        return 0;
    }
    return (total_items + per_session - 1) / per_session;
}

int gk_flow_detect(double skill, double challenge)
{
    double ratio;
    if (skill <= 0.0) {
        return 0;
    }
    ratio = challenge / skill;
    return (ratio >= 0.8 && ratio <= 1.2) ? 1 : 0;
}

double gk_metacog_calibration(const gk_metacog *m)
{
    if (m == NULL) {
        return 0.0;
    }
    return 1.0 - fabs(m->confidence - m->accuracy);
}

const char *gk_error_attribution(double skill, double luck)
{
    if (skill > luck) {
        return "internal";
    }
    if (luck > skill) {
        return "external";
    }
    return "mixed";
}

double gk_transfer_gain(double baseline, double novel)
{
    if (baseline <= 0.0) {
        return 0.0;
    }
    return (novel - baseline) / baseline;
}

double gk_reaction_mean(const double *times, size_t n)
{
    double sum = 0.0;
    size_t i;
    if (times == NULL || n == 0) {
        return 0.0;
    }
    for (i = 0; i < n; ++i) {
        sum += times[i];
    }
    return sum / (double)n;
}

double gk_reaction_stddev(const double *times, size_t n)
{
    double mean;
    double sum = 0.0;
    size_t i;
    if (times == NULL || n < 2) {
        return 0.0;
    }
    mean = gk_reaction_mean(times, n);
    for (i = 0; i < n; ++i) {
        double d = times[i] - mean;
        sum += d * d;
    }
    return sqrt(sum / (double)(n - 1));
}

double gk_error_rate(const int *results, size_t n)
{
    size_t i;
    size_t errors = 0;
    if (results == NULL || n == 0) {
        return 0.0;
    }
    for (i = 0; i < n; ++i) {
        if (!results[i]) {
            errors += 1;
        }
    }
    return (double)errors / (double)n;
}

double gk_engagement_index(double active_time, double idle_time,
                           double error_rate)
{
    double total = active_time + idle_time;
    double active_ratio;
    if (total <= 0.0) {
        return 0.0;
    }
    active_ratio = active_time / total;
    {
        double v = active_ratio * (1.0 - error_rate);
        if (v < 0.0) {
            v = 0.0;
        }
        if (v > 1.0) {
            v = 1.0;
        }
        return v;
    }
}
