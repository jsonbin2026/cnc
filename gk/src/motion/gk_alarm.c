#include "gk/gk_alarm.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

const gk_alarm_manual_entry *gk_alarm_manual_lookup(gk_alarm_category c);

static void gk__copy(char *dst, size_t len, const char *src)
{
    size_t i;
    if (dst == NULL || len == 0) {
        return;
    }
    for (i = 0; i + 1 < len && src[i] != '\0'; ++i) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

const char *gk_alarm_category_name(gk_alarm_category c)
{
    switch (c) {
    case GK_ALARM_NONE: return "none";
    case GK_ALARM_OVERTRAVEL: return "overtravel";
    case GK_ALARM_SERVO_OVERLOAD: return "servo-overload";
    case GK_ALARM_SPINDLE_OVERLOAD: return "spindle-overload";
    case GK_ALARM_TOOL_BREAKAGE: return "tool-breakage";
    case GK_ALARM_TOOL_LIFE: return "tool-life";
    case GK_ALARM_COOLANT_LOW: return "coolant-low";
    case GK_ALARM_AIR_PRESSURE_LOW: return "air-pressure-low";
    case GK_ALARM_HYDRAULIC_LOW: return "hydraulic-low";
    case GK_ALARM_LUBE_LOW: return "lube-low";
    case GK_ALARM_SYNTAX: return "syntax";
    case GK_ALARM_UNDEFINED_G: return "undefined-g";
    case GK_ALARM_UNDEFINED_M: return "undefined-m";
    case GK_ALARM_COORD_OVERTRAVEL: return "coord-overtravel";
    case GK_ALARM_COLLISION: return "collision";
    case GK_ALARM_ESTOP: return "estop";
    case GK_ALARM_POWER_LOSS: return "power-loss";
    default: return "unknown";
    }
}

const char *gk_severity_name(gk_severity s)
{
    switch (s) {
    case GK_SEVERITY_INFO: return "info";
    case GK_SEVERITY_WARNING: return "warning";
    case GK_SEVERITY_ERROR: return "error";
    case GK_SEVERITY_FATAL: return "fatal";
    default: return "unknown";
    }
}

static gk_severity gk__severity_for(gk_alarm_category c)
{
    switch (c) {
    case GK_ALARM_OVERTRAVEL:
    case GK_ALARM_COORD_OVERTRAVEL:
    case GK_ALARM_COLLISION:
    case GK_ALARM_ESTOP:
    case GK_ALARM_POWER_LOSS:
        return GK_SEVERITY_FATAL;
    case GK_ALARM_SERVO_OVERLOAD:
    case GK_ALARM_SPINDLE_OVERLOAD:
    case GK_ALARM_TOOL_BREAKAGE:
    case GK_ALARM_SYNTAX:
    case GK_ALARM_UNDEFINED_G:
    case GK_ALARM_UNDEFINED_M:
        return GK_SEVERITY_ERROR;
    default:
        return GK_SEVERITY_WARNING;
    }
}

void gk_alarm_make_code(gk_alarm_category c, char *buf, size_t len)
{
    const gk_alarm_manual_entry *e;
    if (buf == NULL || len == 0) {
        return;
    }
    e = gk_alarm_manual_lookup(c);
    if (e != NULL) {
        gk__copy(buf, len, e->code);
        return;
    }
    snprintf(buf, len, "E%03d", (int)c * 10 + 1);
}

static void gk__fill(gk_alarm *out, gk_alarm_category cat, int axis,
                     double value, double limit, const char *text)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->category = cat;
    out->severity = gk__severity_for(cat);
    out->axis = axis;
    out->value = value;
    out->limit = limit;
    out->active = 1;
    gk_alarm_make_code(cat, out->code, sizeof(out->code));
    snprintf(out->text, sizeof(out->text), "%s%s%s",
             gk_alarm_category_name(cat),
             text != NULL ? ": " : "",
             text != NULL ? text : "");
}

int gk_alarm_check_overtravel(int axis, double pos, double limit_pos,
                              double limit_neg, gk_alarm *out)
{
    if (pos > limit_pos) {
        gk__fill(out, GK_ALARM_OVERTRAVEL, axis, pos, limit_pos,
                 "positive limit");
        return 1;
    }
    if (pos < limit_neg) {
        gk__fill(out, GK_ALARM_OVERTRAVEL, axis, pos, limit_neg,
                 "negative limit");
        return 1;
    }
    return 0;
}

int gk_alarm_check_overload(gk_alarm_category cat, double load, double rated,
                            double factor, gk_alarm *out)
{
    double limit = rated * factor;
    if (limit != 0.0 && load > limit) {
        gk__fill(out, cat, -1, load, limit, "overload");
        return 1;
    }
    return 0;
}

int gk_alarm_check_tool_breakage(double prev_force, double force,
                                 double ratio, gk_alarm *out)
{
    if (prev_force > 0.0 && force < prev_force * ratio) {
        gk__fill(out, GK_ALARM_TOOL_BREAKAGE, -1, force, prev_force * ratio,
                 "force drop");
        return 1;
    }
    if (prev_force > 0.0 && force > prev_force / ratio) {
        gk__fill(out, GK_ALARM_TOOL_BREAKAGE, -1, force, prev_force / ratio,
                 "force spike");
        return 1;
    }
    return 0;
}

int gk_alarm_check_tool_life(double remaining, double threshold,
                             int tool_no, gk_alarm *out)
{
    if (remaining < threshold) {
        char buf[32];
        gk__fill(out, GK_ALARM_TOOL_LIFE, -1, remaining, threshold, NULL);
        snprintf(buf, sizeof(buf), "T%02d", tool_no);
        snprintf(out->text, sizeof(out->text), "tool-life: %s remaining %.3f",
                 buf, remaining);
        return 1;
    }
    return 0;
}

int gk_alarm_check_level(gk_alarm_category cat, double level,
                         double threshold, gk_alarm *out)
{
    if (level < threshold) {
        gk__fill(out, cat, -1, level, threshold, "level below threshold");
        return 1;
    }
    return 0;
}

int gk_alarm_check_coord(double pos, double soft_min, double soft_max,
                         gk_alarm *out)
{
    if (pos > soft_max || pos < soft_min) {
        double limit = pos > soft_max ? soft_max : soft_min;
        gk__fill(out, GK_ALARM_COORD_OVERTRAVEL, -1, pos, limit,
                 "soft limit");
        return 1;
    }
    return 0;
}

int gk_alarm_estop(double time, gk_alarm *out)
{
    gk__fill(out, GK_ALARM_ESTOP, -1, time, 0.0, "emergency stop pressed");
    if (out != NULL) {
        out->time = time;
    }
    return 1;
}

int gk_alarm_power_loss(gk_alarm *out)
{
    gk__fill(out, GK_ALARM_POWER_LOSS, -1, 0.0, 0.0,
             "power restored, position unverified");
    return 1;
}

/* ---------------- manual ---------------- */

static const gk_alarm_manual_entry g_manual[] = {
    { GK_ALARM_OVERTRAVEL, "E041", "Overtravel (OT)",
      "Axis moved beyond a hardware/soft limit.",
      "Jog the axis back inside the limits, then reset and re-home." },
    { GK_ALARM_SERVO_OVERLOAD, "E051", "Servo overload",
      "Servo current exceeded its rated load for too long.",
      "Reduce feed/acceleration or check for mechanical binding." },
    { GK_ALARM_SPINDLE_OVERLOAD, "E061", "Spindle overload",
      "Spindle load exceeded rated power.",
      "Reduce depth of cut or spindle speed; verify tool sharpness." },
    { GK_ALARM_TOOL_BREAKAGE, "E071", "Tool breakage",
      "Sudden loss of load indicates a broken tool.",
      "Replace the tool and inspect the workpiece for damage." },
    { GK_ALARM_TOOL_LIFE, "E081", "Tool life expired",
      "Tool reached its usable life limit.",
      "Change the tool or reset its life counter." },
    { GK_ALARM_COOLANT_LOW, "E091", "Coolant low",
      "Coolant tank level below minimum.",
      "Refill the coolant tank." },
    { GK_ALARM_AIR_PRESSURE_LOW, "E101", "Air pressure low",
      "Pneumatic supply pressure below minimum.",
      "Check the air compressor and supply line." },
    { GK_ALARM_HYDRAULIC_LOW, "E111", "Hydraulic pressure low",
      "Hydraulic supply pressure below minimum.",
      "Check the hydraulic pump and relief valve." },
    { GK_ALARM_LUBE_LOW, "E121", "Lubrication low",
      "Way/ball-screw lubrication level low.",
      "Refill the lubrication reservoir." },
    { GK_ALARM_SYNTAX, "E131", "Syntax error",
      "The program contains malformed syntax.",
      "Review the flagged line and correct the syntax." },
    { GK_ALARM_UNDEFINED_G, "E141", "Undefined G code",
      "A G code not supported by this control was used.",
      "Verify the G code against the controller manual." },
    { GK_ALARM_UNDEFINED_M, "E151", "Undefined M code",
      "An M code not supported by this control was used.",
      "Verify the M code against the controller manual." },
    { GK_ALARM_COORD_OVERTRAVEL, "E161", "Coordinate overtravel",
      "Programmed position lies outside the soft limits.",
      "Adjust the program or the work coordinate offset." },
    { GK_ALARM_COLLISION, "E171", "Collision",
      "A collision was detected between machine components.",
      "Stop the machine, inspect the tool and fixture, then re-zero." },
    { GK_ALARM_ESTOP, "E181", "Emergency stop",
      "The emergency stop was pressed.",
      "Investigate the cause, release E-stop and reset." },
    { GK_ALARM_POWER_LOSS, "E191", "Power loss recovery",
      "The machine lost power and must recover its position.",
      "Reference the axes before resuming machining." },
};

int gk_alarm_manual_count(void)
{
    return (int)(sizeof(g_manual) / sizeof(g_manual[0]));
}

const gk_alarm_manual_entry *gk_alarm_manual_lookup(gk_alarm_category c)
{
    int i;
    for (i = 0; i < gk_alarm_manual_count(); ++i) {
        if (g_manual[i].category == c) {
            return &g_manual[i];
        }
    }
    return NULL;
}

/* ---------------- manager ---------------- */

void gk_alarm_manager_init(gk_alarm_manager *m)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    m->next_id = 1;
}

gk_status gk_alarm_raise(gk_alarm_manager *m, const gk_alarm *a)
{
    if (m == NULL || a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    gk_alarm copy = *a;
    copy.id = m->next_id++;
    copy.active = 1;
    if (copy.code[0] == '\0') {
        gk_alarm_make_code(copy.category, copy.code, sizeof(copy.code));
    }
    if (m->active_count < GK_ALARM_MAX_ACTIVE) {
        m->active[m->active_count++] = copy;
    }
    if (m->history_count < GK_ALARM_MAX_HISTORY) {
        m->history[m->history_count++] = copy;
    }
    m->buzzer_on = 1;
    m->lamp_on = 1;
    return GK_OK;
}

gk_status gk_alarm_clear(gk_alarm_manager *m, int id)
{
    int i;
    int found = 0;
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < m->active_count; ++i) {
        if (m->active[i].id == id) {
            memmove(&m->active[i], &m->active[i + 1],
                    (size_t)(m->active_count - i - 1) * sizeof(m->active[0]));
            m->active_count--;
            found = 1;
            break;
        }
    }
    if (!found) {
        return GK_ERR_NOT_FOUND;
    }
    if (m->active_count == 0) {
        m->buzzer_on = 0;
        m->lamp_on = 0;
    }
    return GK_OK;
}

void gk_alarm_clear_all(gk_alarm_manager *m)
{
    if (m == NULL) {
        return;
    }
    m->active_count = 0;
    m->buzzer_on = 0;
    m->lamp_on = 0;
}

int gk_alarm_active_count(const gk_alarm_manager *m)
{
    return m != NULL ? m->active_count : 0;
}

gk_severity gk_alarm_max_severity(const gk_alarm_manager *m)
{
    gk_severity best = GK_SEVERITY_INFO;
    int i;
    if (m == NULL || m->active_count == 0) {
        return GK_SEVERITY_INFO;
    }
    for (i = 0; i < m->active_count; ++i) {
        if (m->active[i].severity > best) {
            best = m->active[i].severity;
        }
    }
    return best;
}

const gk_alarm *gk_alarm_first_active(const gk_alarm_manager *m)
{
    if (m == NULL || m->active_count == 0) {
        return NULL;
    }
    return &m->active[0];
}

const gk_alarm *gk_alarm_history_at(const gk_alarm_manager *m, int i)
{
    if (m == NULL || i < 0 || i >= m->history_count) {
        return NULL;
    }
    return &m->history[i];
}

const gk_alarm *gk_alarm_find_category(const gk_alarm_manager *m,
                                       gk_alarm_category c)
{
    int i;
    if (m == NULL) {
        return NULL;
    }
    for (i = m->history_count - 1; i >= 0; --i) {
        if (m->history[i].category == c) {
            return &m->history[i];
        }
    }
    return NULL;
}

double gk_alarm_timeline_span(const gk_alarm_manager *m)
{
    double lo, hi;
    int i;
    if (m == NULL || m->history_count == 0) {
        return 0.0;
    }
    lo = hi = m->history[0].time;
    for (i = 1; i < m->history_count; ++i) {
        if (m->history[i].time < lo) lo = m->history[i].time;
        if (m->history[i].time > hi) hi = m->history[i].time;
    }
    return hi - lo;
}

const gk_alarm *gk_alarm_timeline_at(const gk_alarm_manager *m, int i)
{
    return gk_alarm_history_at(m, i);
}

void gk_alarm_update_indicators(gk_alarm_manager *m, double dt)
{
    if (m == NULL) {
        return;
    }
    if (m->active_count == 0) {
        m->buzzer_on = 0;
        m->lamp_on = 0;
        m->lamp_phase = 0.0;
        return;
    }
    m->lamp_phase += dt;
    if (m->lamp_phase >= 1.0) {
        m->lamp_phase -= 1.0;
    }
    m->lamp_on = m->lamp_phase < 0.5 ? 1 : 0;
}

int gk_alarm_buzzer_on(const gk_alarm_manager *m)
{
    return m != NULL ? m->buzzer_on : 0;
}

int gk_alarm_lamp_on(const gk_alarm_manager *m)
{
    return m != NULL ? m->lamp_on : 0;
}

/* ---------------- program checks ---------------- */

gk_status gk_alarm_check_syntax(const char *line, gk_alarm *out)
{
    const char *p;
    int paren = 0;
    if (line == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (p = line; *p != '\0'; ++p) {
        if (*p == '(') {
            paren++;
        } else if (*p == ')') {
            if (paren == 0) {
                gk__fill(out, GK_ALARM_SYNTAX, -1, 0.0, 0.0,
                         "unmatched ')'");
                return GK_ERR_PARSE;
            }
            paren--;
        }
    }
    if (paren != 0) {
        gk__fill(out, GK_ALARM_SYNTAX, -1, 0.0, 0.0, "unmatched '('");
        return GK_ERR_PARSE;
    }
    return GK_OK;
}

gk_status gk_alarm_check_gcode(int g, gk_alarm *out)
{
    static const int known[] = {
        0, 1, 2, 3, 4, 9, 10, 17, 18, 19, 20, 21, 28, 40, 41, 42, 43, 44, 49,
        53, 54, 55, 56, 57, 58, 59, 73, 74, 76, 80, 81, 82, 83, 84, 85, 86,
        87, 88, 89, 90, 91, 92, 94, 96, 97, 98, 99
    };
    size_t i;
    for (i = 0; i < sizeof(known) / sizeof(known[0]); ++i) {
        if (known[i] == g) {
            return GK_OK;
        }
    }
    gk__fill(out, GK_ALARM_UNDEFINED_G, -1, (double)g, 0.0, NULL);
    snprintf(out->text, sizeof(out->text), "undefined-g: G%02d", g);
    return GK_ERR_NOT_FOUND;
}

gk_status gk_alarm_check_mcode(int m, gk_alarm *out)
{
    static const int known[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15, 16, 17, 18, 19,
        30, 31, 48, 49, 50, 51, 52, 60, 98, 99
    };
    size_t i;
    for (i = 0; i < sizeof(known) / sizeof(known[0]); ++i) {
        if (known[i] == m) {
            return GK_OK;
        }
    }
    gk__fill(out, GK_ALARM_UNDEFINED_M, -1, (double)m, 0.0, NULL);
    snprintf(out->text, sizeof(out->text), "undefined-m: M%02d", m);
    return GK_ERR_NOT_FOUND;
}

/* ---------------- fault tree ---------------- */

gk_status gk_fault_tree_add(gk_fault_tree *t, int id, const char *name,
                            int parent, double probability)
{
    gk_fault_node *n;
    if (t == NULL || name == NULL || t->count >= GK_FAULT_MAX_NODES) {
        return GK_ERR_INVALID_ARG;
    }
    n = &t->nodes[t->count++];
    memset(n, 0, sizeof(*n));
    n->id = id;
    strncpy(n->name, name, sizeof(n->name) - 1);
    n->parent = parent;
    n->probability = probability;
    return GK_OK;
}

static const gk_fault_node *gk__node_by_id(const gk_fault_tree *t, int id)
{
    int i;
    if (t == NULL) {
        return NULL;
    }
    for (i = 0; i < t->count; ++i) {
        if (t->nodes[i].id == id) {
            return &t->nodes[i];
        }
    }
    return NULL;
}

int gk_fault_tree_subtree_count(const gk_fault_tree *t, int node_id)
{
    const gk_fault_node *root;
    int i;
    int count = 0;
    if (t == NULL) {
        return 0;
    }
    root = gk__node_by_id(t, node_id);
    if (root == NULL) {
        return 0;
    }
    count = 1;
    for (i = 0; i < t->count; ++i) {
        if (t->nodes[i].parent == node_id) {
            count += gk_fault_tree_subtree_count(t, t->nodes[i].id);
        }
    }
    return count;
}

double gk_fault_tree_probability(const gk_fault_tree *t, int node_id)
{
    double no_fail = 1.0;
    int has_child = 0;
    int i;
    if (t == NULL) {
        return 0.0;
    }
    for (i = 0; i < t->count; ++i) {
        if (t->nodes[i].parent == node_id) {
            has_child = 1;
            no_fail *= (1.0 - t->nodes[i].probability);
        }
    }
    if (!has_child) {
        const gk_fault_node *n = gk__node_by_id(t, node_id);
        return n != NULL ? n->probability : 0.0;
    }
    return 1.0 - no_fail;
}

/* ---------------- fault simulation / injection ---------------- */

const char *gk_fault_injection_name(gk_fault_injection_kind k)
{
    switch (k) {
    case GK_FAULT_INJ_SENSOR: return "sensor";
    case GK_FAULT_INJ_ACTUATOR: return "actuator";
    case GK_FAULT_INJ_CONTROLLER: return "controller";
    case GK_FAULT_INJ_POWER: return "power";
    default: return "none";
    }
}

void gk_fault_sim_init(gk_fault_sim *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->enabled = 1;
}

static gk_alarm_category gk__category_for(gk_fault_injection_kind k)
{
    switch (k) {
    case GK_FAULT_INJ_SENSOR: return GK_ALARM_SERVO_OVERLOAD;
    case GK_FAULT_INJ_ACTUATOR: return GK_ALARM_SPINDLE_OVERLOAD;
    case GK_FAULT_INJ_CONTROLLER: return GK_ALARM_UNDEFINED_G;
    case GK_FAULT_INJ_POWER: return GK_ALARM_POWER_LOSS;
    default: return GK_ALARM_NONE;
    }
}

gk_status gk_fault_sim_add(gk_fault_sim *s, gk_fault_injection_kind kind,
                           double start_time, double duration)
{
    gk_fault_injection *inj;
    if (s == NULL || s->count >= GK_FAULT_MAX_NODES) {
        return GK_ERR_INVALID_ARG;
    }
    inj = &s->injections[s->count++];
    memset(inj, 0, sizeof(*inj));
    inj->kind = kind;
    inj->category = gk__category_for(kind);
    inj->start_time = start_time;
    inj->duration = duration;
    inj->active = 0;
    return GK_OK;
}

int gk_fault_sim_step(gk_fault_sim *s, double now, gk_alarm *out, int max_out)
{
    int emitted = 0;
    int i;
    if (s == NULL || !s->enabled || out == NULL || max_out <= 0) {
        return 0;
    }
    for (i = 0; i < s->count && emitted < max_out; ++i) {
        gk_fault_injection *inj = &s->injections[i];
        int in_window;
        if (inj->duration > 0.0) {
            in_window = now >= inj->start_time &&
                        now < inj->start_time + inj->duration;
        } else {
            in_window = now >= inj->start_time;
        }
        if (in_window && !inj->active) {
            gk__fill(&out[emitted], inj->category, -1, now, 0.0,
                     gk_fault_injection_name(inj->kind));
            out[emitted].time = now;
            emitted++;
            inj->active = 1;
        } else if (!in_window && inj->active) {
            inj->active = 0;
        }
    }
    return emitted;
}

/* ---------------- diagnosis ---------------- */

gk_status gk_alarm_diagnose(gk_alarm_category c, gk_diagnosis *out)
{
    const gk_alarm_manual_entry *e;
    if (out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    memset(out, 0, sizeof(*out));
    e = gk_alarm_manual_lookup(c);
    if (e == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    gk__copy(out->summary, sizeof(out->summary), e->cause);
    gk__copy(out->action, sizeof(out->action), e->remedy);
    return GK_OK;
}
