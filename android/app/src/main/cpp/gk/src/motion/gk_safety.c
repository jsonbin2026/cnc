#include "gk/gk_safety.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

/* ---- procedure ---- */

void gk_safety_procedure_init(gk_safety_procedure *p, const char *title)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    gk__copy(p->title, sizeof(p->title), title);
}

int gk_safety_step_add(gk_safety_procedure *p, const char *text, int mandatory)
{
    gk_safety_step *s;
    if (p == NULL || text == NULL || p->count >= GK_SAFETY_MAX_STEPS) {
        return -1;
    }
    s = &p->steps[p->count++];
    memset(s, 0, sizeof(*s));
    s->id = p->count;
    gk__copy(s->text, sizeof(s->text), text);
    s->mandatory = mandatory ? 1 : 0;
    return s->id;
}

static gk_safety_step *gk__step(gk_safety_procedure *p, int id)
{
    int i;
    if (p == NULL) {
        return NULL;
    }
    for (i = 0; i < p->count; ++i) {
        if (p->steps[i].id == id) {
            return &p->steps[i];
        }
    }
    return NULL;
}

gk_status gk_safety_step_done(gk_safety_procedure *p, int id)
{
    gk_safety_step *s = gk__step(p, id);
    if (s == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    s->done = 1;
    return GK_OK;
}

gk_status gk_safety_next(gk_safety_procedure *p)
{
    if (p == NULL || p->count == 0) {
        return GK_ERR_STATE;
    }
    if (p->current >= p->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    p->steps[p->current].done = 1;
    p->current++;
    return GK_OK;
}

int gk_safety_procedure_complete(const gk_safety_procedure *p)
{
    int i;
    if (p == NULL || p->count == 0) {
        return 0;
    }
    for (i = 0; i < p->count; ++i) {
        if (p->steps[i].mandatory && !p->steps[i].done) {
            return 0;
        }
    }
    return 1;
}

int gk_safety_remaining(const gk_safety_procedure *p)
{
    int i;
    int n = 0;
    if (p == NULL) {
        return 0;
    }
    for (i = 0; i < p->count; ++i) {
        if (!p->steps[i].done) n++;
    }
    return n;
}

gk_status gk_safety_build_power_on(gk_safety_procedure *p)
{
    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    gk_safety_procedure_init(p, "Power-On Sequence");
    gk_safety_step_add(p, "Check machine surroundings are clear", 1);
    gk_safety_step_add(p, "Verify air and hydraulic pressure", 1);
    gk_safety_step_add(p, "Close cabinet doors and safety guards", 1);
    gk_safety_step_add(p, "Switch on main breaker", 1);
    gk_safety_step_add(p, "Release emergency stop", 1);
    gk_safety_step_add(p, "Home all axes (reference return)", 1);
    gk_safety_step_add(p, "Warm up spindle", 0);
    return GK_OK;
}

gk_status gk_safety_build_power_off(gk_safety_procedure *p)
{
    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    gk_safety_procedure_init(p, "Power-Off Sequence");
    gk_safety_step_add(p, "Stop spindle and all axes", 1);
    gk_safety_step_add(p, "Press emergency stop", 1);
    gk_safety_step_add(p, "Remove workpiece and tools", 0);
    gk_safety_step_add(p, "Clean chips and coolant", 0);
    gk_safety_step_add(p, "Switch off main breaker", 1);
    return GK_OK;
}

/* ---- PPE ---- */

const char *gk_ppe_name(gk_ppe_kind k)
{
    switch (k) {
    case GK_PPE_GOGGLES: return "safety-goggles";
    case GK_PPE_COVERALL: return "coverall";
    case GK_PPE_SHOES: return "safety-shoes";
    case GK_PPE_GLOVES: return "gloves";
    case GK_PPE_HEARING: return "hearing-protection";
    default: return "unknown";
    }
}

const char *gk_ppe_requirement(gk_ppe_kind k)
{
    switch (k) {
    case GK_PPE_GOGGLES: return "Wear to protect eyes from chips and coolant";
    case GK_PPE_COVERALL: return "Wear to avoid entanglement and skin contact";
    case GK_PPE_SHOES: return "Wear to protect feet from falling objects";
    case GK_PPE_GLOVES: return "Wear for safe part handling";
    case GK_PPE_HEARING: return "Wear where noise exceeds 85 dB";
    default: return "";
    }
}

void gk_ppe_state_init(gk_ppe_state *s)
{
    if (s != NULL) {
        memset(s, 0, sizeof(*s));
    }
}

gk_status gk_ppe_wear(gk_ppe_state *s, gk_ppe_kind k, int worn)
{
    if (s == NULL || k < 0 || k >= GK_PPE_COUNT) {
        return GK_ERR_INVALID_ARG;
    }
    s->worn[k] = worn ? 1 : 0;
    return GK_OK;
}

int gk_ppe_compliant(const gk_ppe_state *s)
{
    int i;
    if (s == NULL) {
        return 0;
    }
    for (i = 0; i < GK_PPE_COUNT; ++i) {
        if (!s->worn[i]) return 0;
    }
    return 1;
}

int gk_ppe_missing_count(const gk_ppe_state *s)
{
    int i;
    int n = 0;
    if (s == NULL) {
        return 0;
    }
    for (i = 0; i < GK_PPE_COUNT; ++i) {
        if (!s->worn[i]) n++;
    }
    return n;
}

/* ---- hazard map ---- */

void gk_hazard_map_init(gk_hazard_map *m)
{
    if (m != NULL) {
        memset(m, 0, sizeof(*m));
    }
}

int gk_hazard_add(gk_hazard_map *m, double x, double y, double z, double radius,
                  int severity, const char *label)
{
    gk_hazard_zone *zh;
    if (m == NULL || radius < 0.0 || m->count >= GK_SAFETY_MAX_ITEMS) {
        return -1;
    }
    zh = &m->zones[m->count];
    memset(zh, 0, sizeof(*zh));
    zh->x = x;
    zh->y = y;
    zh->z = z;
    zh->radius = radius;
    zh->severity = severity;
    gk__copy(zh->label, sizeof(zh->label), label);
    m->count++;
    return m->count;
}

int gk_hazard_query(const gk_hazard_map *m, double x, double y, double z)
{
    int i;
    int best = 0;
    int best_sev = 0;
    if (m == NULL) {
        return 0;
    }
    for (i = 0; i < m->count; ++i) {
        double dx = x - m->zones[i].x;
        double dy = y - m->zones[i].y;
        double dz = z - m->zones[i].z;
        if (sqrt(dx * dx + dy * dy + dz * dz) <= m->zones[i].radius) {
            if (m->zones[i].severity > best_sev) {
                best_sev = m->zones[i].severity;
                best = i + 1;
            }
        }
    }
    return best;
}

int gk_rotating_warning(double rpm, double tool_diameter)
{
    double surface_speed;
    if (rpm <= 0.0 || tool_diameter <= 0.0) {
        return 0;
    }
    surface_speed = M_PI * tool_diameter * rpm / 60000.0;   /* m/s */
    if (surface_speed >= 80.0) {
        return 3;
    }
    if (surface_speed >= 30.0) {
        return 2;
    }
    return 1;
}

/* ---- e-stop ---- */

void gk_estop_layout_init(gk_estop_layout *l)
{
    if (l != NULL) {
        memset(l, 0, sizeof(*l));
    }
}

int gk_estop_add(gk_estop_layout *l, double x, double y, double z,
                 const char *label)
{
    gk_estop_location *s;
    if (l == NULL || l->count >= GK_SAFETY_MAX_ITEMS) {
        return -1;
    }
    s = &l->spots[l->count];
    memset(s, 0, sizeof(*s));
    s->x = x;
    s->y = y;
    s->z = z;
    gk__copy(s->label, sizeof(s->label), label);
    l->count++;
    return l->count;
}

const gk_estop_location *gk_estop_nearest(const gk_estop_layout *l,
                                          double x, double y, double z)
{
    int i;
    const gk_estop_location *best = NULL;
    double best_d = 0.0;
    if (l == NULL || l->count == 0) {
        return NULL;
    }
    for (i = 0; i < l->count; ++i) {
        double dx = x - l->spots[i].x;
        double dy = y - l->spots[i].y;
        double dz = z - l->spots[i].z;
        double d = dx * dx + dy * dy + dz * dz;
        if (best == NULL || d < best_d) {
            best = &l->spots[i];
            best_d = d;
        }
    }
    return best;
}

void gk_estop_drill_init(gk_estop_drill *d, double limit)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    d->limit = limit;
}

gk_status gk_estop_drill_press(gk_estop_drill *d, double reaction_time)
{
    if (d == NULL || reaction_time < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    d->reaction_time = reaction_time;
    d->pressed = 1;
    d->passed = reaction_time <= d->limit;
    return GK_OK;
}

/* ---- LOTO ---- */

void gk_loto_board_init(gk_loto_board *b)
{
    if (b != NULL) {
        memset(b, 0, sizeof(*b));
    }
}

int gk_loto_apply(gk_loto_board *b, const char *tag, const char *owner)
{
    gk_loto_lock *l;
    if (b == NULL || tag == NULL || b->count >= GK_SAFETY_MAX_ITEMS) {
        return -1;
    }
    l = &b->locks[b->count];
    memset(l, 0, sizeof(*l));
    l->id = b->count + 1;
    gk__copy(l->tag, sizeof(l->tag), tag);
    gk__copy(l->owner, sizeof(l->owner), owner);
    l->locked = 1;
    b->count++;
    return l->id;
}

gk_status gk_loto_remove(gk_loto_board *b, const char *tag, const char *owner)
{
    int i;
    if (b == NULL || tag == NULL || owner == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < b->count; ++i) {
        if (strcmp(b->locks[i].tag, tag) == 0) {
            if (strcmp(b->locks[i].owner, owner) != 0) {
                return GK_ERR_STATE;   /* only the owner may remove */
            }
            memmove(&b->locks[i], &b->locks[i + 1],
                    (size_t)(b->count - i - 1) * sizeof(gk_loto_lock));
            b->count--;
            return GK_OK;
        }
    }
    return GK_ERR_NOT_FOUND;
}

int gk_loto_is_locked(const gk_loto_board *b, const char *tag)
{
    int i;
    if (b == NULL || tag == NULL) {
        return 0;
    }
    for (i = 0; i < b->count; ++i) {
        if (strcmp(b->locks[i].tag, tag) == 0) {
            return b->locks[i].locked;
        }
    }
    return 0;
}

/* ---- 5S ---- */

const char *gk_5s_name(gk_5s_pillar p)
{
    switch (p) {
    case GK_5S_SORT: return "sort";
    case GK_5S_SET_IN_ORDER: return "set-in-order";
    case GK_5S_SHINE: return "shine";
    case GK_5S_STANDARDIZE: return "standardize";
    case GK_5S_SUSTAIN: return "sustain";
    default: return "unknown";
    }
}

void gk_5s_board_init(gk_5s_board *b)
{
    if (b != NULL) {
        memset(b, 0, sizeof(*b));
    }
}

static gk_5s_area *gk__5s(gk_5s_board *b, const char *zone)
{
    int i;
    if (b == NULL || zone == NULL) {
        return NULL;
    }
    for (i = 0; i < b->count; ++i) {
        if (strcmp(b->areas[i].zone, zone) == 0) {
            return &b->areas[i];
        }
    }
    return NULL;
}

int gk_5s_area_add(gk_5s_board *b, const char *zone)
{
    gk_5s_area *a;
    if (b == NULL || zone == NULL || b->count >= GK_SAFETY_MAX_ITEMS) {
        return -1;
    }
    a = &b->areas[b->count];
    memset(a, 0, sizeof(*a));
    gk__copy(a->zone, sizeof(a->zone), zone);
    b->count++;
    return b->count;
}

gk_status gk_5s_set_score(gk_5s_board *b, const char *zone, gk_5s_pillar p,
                          int score)
{
    gk_5s_area *a = gk__5s(b, zone);
    if (a == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    if (p < 0 || p >= GK_5S_COUNT || score < 0 || score > 100) {
        return GK_ERR_OUT_OF_RANGE;
    }
    a->scores[p] = score;
    return GK_OK;
}

double gk_5s_area_score(const gk_5s_board *b, const char *zone)
{
    const gk_5s_board *bb = b;
    int i;
    int sum = 0;
    if (bb == NULL || zone == NULL) {
        return 0.0;
    }
    for (i = 0; i < bb->count; ++i) {
        if (strcmp(bb->areas[i].zone, zone) == 0) {
            int j;
            for (j = 0; j < GK_5S_COUNT; ++j) {
                sum += bb->areas[i].scores[j];
            }
            return (double)sum / (double)GK_5S_COUNT;
        }
    }
    return 0.0;
}

double gk_5s_overall(const gk_5s_board *b)
{
    double sum = 0.0;
    int i;
    if (b == NULL || b->count == 0) {
        return 0.0;
    }
    for (i = 0; i < b->count; ++i) {
        int j;
        for (j = 0; j < GK_5S_COUNT; ++j) {
            sum += b->areas[i].scores[j];
        }
    }
    return sum / (double)(b->count * GK_5S_COUNT);
}

/* ---- incidents ---- */

void gk_incident_library_init(gk_incident_library *l)
{
    if (l != NULL) {
        memset(l, 0, sizeof(*l));
    }
}

int gk_incident_add(gk_incident_library *l, const char *title, const char *cause,
                    const char *lesson, double severity)
{
    gk_incident_case *c;
    if (l == NULL || title == NULL || l->count >= GK_SAFETY_MAX_ITEMS) {
        return -1;
    }
    c = &l->cases[l->count];
    memset(c, 0, sizeof(*c));
    c->id = l->count + 1;
    gk__copy(c->title, sizeof(c->title), title);
    gk__copy(c->cause, sizeof(c->cause), cause);
    gk__copy(c->lesson, sizeof(c->lesson), lesson);
    c->severity = severity;
    l->count++;
    return c->id;
}

const gk_incident_case *gk_incident_find(const gk_incident_library *l,
                                         const char *title)
{
    int i;
    if (l == NULL || title == NULL) {
        return NULL;
    }
    for (i = 0; i < l->count; ++i) {
        if (strcmp(l->cases[i].title, title) == 0) {
            return &l->cases[i];
        }
    }
    return NULL;
}

int gk_incident_replay(const gk_incident_case *c, char *buf, size_t len)
{
    if (c == NULL || buf == NULL || len == 0) {
        return 0;
    }
    return snprintf(buf, len, "INCIDENT|%s|cause=%s|lesson=%s|severity=%.1f",
                    c->title, c->cause, c->lesson, c->severity);
}

/* ---- violations ---- */

void gk_violation_book_init(gk_violation_book *b)
{
    if (b != NULL) {
        memset(b, 0, sizeof(*b));
    }
}

static gk_violation_card *gk__violation(gk_violation_book *b, int user_id)
{
    int i;
    if (b == NULL) {
        return NULL;
    }
    for (i = 0; i < b->count; ++i) {
        if (b->cards[i].user_id == user_id) {
            return &b->cards[i];
        }
    }
    if (b->count >= GK_SAFETY_MAX_ITEMS) {
        return NULL;
    }
    b->cards[b->count].user_id = user_id;
    b->cards[b->count].points = 0;
    b->cards[b->count].violations = 0;
    return &b->cards[b->count++];
}

gk_status gk_violation_add(gk_violation_book *b, int user_id, int points)
{
    gk_violation_card *c;
    if (b == NULL || points < 0) {
        return GK_ERR_INVALID_ARG;
    }
    c = gk__violation(b, user_id);
    if (c == NULL) {
        return GK_ERR_OUT_OF_RANGE;
    }
    c->points += points;
    c->violations++;
    return GK_OK;
}

int gk_violation_total(const gk_violation_book *b, int user_id)
{
    int i;
    if (b == NULL) {
        return 0;
    }
    for (i = 0; i < b->count; ++i) {
        if (b->cards[i].user_id == user_id) {
            return b->cards[i].points;
        }
    }
    return 0;
}

int gk_violation_disqualified(const gk_violation_book *b, int user_id,
                              int limit)
{
    return gk_violation_total(b, user_id) >= limit;
}

/* ---- safety score / assessment ---- */

void gk_safety_score_init(gk_safety_score *s)
{
    if (s != NULL) {
        memset(s, 0, sizeof(*s));
    }
}

gk_status gk_safety_score_add(gk_safety_score *s, int correct)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    s->total += 1.0;
    if (correct) {
        s->correct += 1.0;
    }
    return GK_OK;
}

double gk_safety_score_value(const gk_safety_score *s)
{
    if (s == NULL || s->total <= 0.0) {
        return 0.0;
    }
    return s->correct / s->total;
}

int gk_safety_score_pass(const gk_safety_score *s, double threshold)
{
    return gk_safety_score_value(s) >= threshold;
}

/* ---- certificate ---- */

void gk_safety_cert_init(gk_safety_cert *c, const char *holder,
                         const char *course)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    gk__copy(c->holder, sizeof(c->holder), holder);
    gk__copy(c->course, sizeof(c->course), course);
}

gk_status gk_safety_cert_issue(gk_safety_cert *c, double score, double at,
                               double threshold)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    c->score = score;
    c->issued_at = at;
    c->valid = score >= threshold;
    return c->valid ? GK_OK : GK_ERR_STATE;
}

int gk_safety_cert_text(const gk_safety_cert *c, char *buf, size_t len)
{
    if (c == NULL || buf == NULL || len == 0) {
        return 0;
    }
    return snprintf(buf, len, "SAFETY CERT|%s|%s|%.1f|%s", c->holder,
                    c->course, c->score, c->valid ? "valid" : "invalid");
}

/* ---- emergency / fire drill / leakage ---- */

void gk_emergency_plan_init(gk_emergency_plan *p)
{
    if (p != NULL) {
        memset(p, 0, sizeof(*p));
    }
}

int gk_emergency_add(gk_emergency_plan *p, const char *name, double limit)
{
    gk_emergency_drill *d;
    if (p == NULL || name == NULL || limit <= 0.0 ||
        p->count >= GK_SAFETY_MAX_ITEMS) {
        return -1;
    }
    d = &p->drills[p->count];
    memset(d, 0, sizeof(*d));
    gk__copy(d->name, sizeof(d->name), name);
    d->limit = limit;
    p->count++;
    return p->count;
}

gk_status gk_emergency_complete(gk_emergency_plan *p, int id,
                                double response_time)
{
    gk_emergency_drill *d;
    if (p == NULL || response_time < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    if (id < 1 || id > p->count) {
        return GK_ERR_NOT_FOUND;
    }
    d = &p->drills[id - 1];
    d->response_time = response_time;
    d->completed = 1;
    return GK_OK;
}

int gk_emergency_all_pass(const gk_emergency_plan *p)
{
    int i;
    if (p == NULL || p->count == 0) {
        return 0;
    }
    for (i = 0; i < p->count; ++i) {
        if (!p->drills[i].completed ||
            p->drills[i].response_time > p->drills[i].limit) {
            return 0;
        }
    }
    return 1;
}

void gk_leakage_init(gk_leakage_protector *l, double threshold_ma,
                     double max_trip_ms)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
    l->threshold_ma = threshold_ma;
    l->max_trip_ms = max_trip_ms;
}

gk_status gk_leakage_test(gk_leakage_protector *l, double current_ma,
                          double trip_ms)
{
    if (l == NULL || current_ma < 0.0 || trip_ms < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    l->leakage_current_ma = current_ma;
    l->trip_time_ms = trip_ms;
    if (current_ma >= l->threshold_ma) {
        l->tripped = 1;
    } else {
        l->tripped = 0;
    }
    return GK_OK;
}

int gk_leakage_pass(const gk_leakage_protector *l)
{
    if (l == NULL) {
        return 0;
    }
    if (!l->tripped) {
        return 0;
    }
    return l->trip_time_ms <= l->max_trip_ms;
}
