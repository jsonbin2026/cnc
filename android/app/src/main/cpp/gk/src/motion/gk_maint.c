#include "gk/gk_maint.h"

#include <math.h>
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

/* ---- periods ---- */

const char *gk_maint_period_name(gk_maint_period p)
{
    switch (p) {
    case GK_MAINT_DAILY: return "daily";
    case GK_MAINT_WEEKLY: return "weekly";
    case GK_MAINT_MONTHLY: return "monthly";
    case GK_MAINT_YEARLY: return "yearly";
    case GK_MAINT_INTERVAL: return "interval";
    default: return "unknown";
    }
}

double gk_maint_period_hours(gk_maint_period p)
{
    switch (p) {
    case GK_MAINT_DAILY: return 8.0;
    case GK_MAINT_WEEKLY: return 40.0;
    case GK_MAINT_MONTHLY: return 160.0;
    case GK_MAINT_YEARLY: return 2000.0;
    case GK_MAINT_INTERVAL: return 500.0;
    default: return 0.0;
    }
}

/* ---- schedule ---- */

void gk_maint_schedule_init(gk_maint_schedule *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->next_id = 1;
}

int gk_maint_task_add(gk_maint_schedule *s, const char *name,
                      gk_maint_period period)
{
    gk_maint_task *t;
    if (s == NULL || name == NULL || s->count >= GK_MAINT_MAX_TASKS) {
        return -1;
    }
    t = &s->tasks[s->count++];
    memset(t, 0, sizeof(*t));
    t->id = s->next_id++;
    gk__copy(t->name, sizeof(t->name), name);
    t->period = period;
    t->interval_hours = gk_maint_period_hours(period);
    return t->id;
}

static gk_maint_task *gk__task(gk_maint_schedule *s, int id)
{
    int i;
    if (s == NULL) {
        return NULL;
    }
    for (i = 0; i < s->count; ++i) {
        if (s->tasks[i].id == id) {
            return &s->tasks[i];
        }
    }
    return NULL;
}

gk_status gk_maint_task_done(gk_maint_schedule *s, int id, double now_hours)
{
    gk_maint_task *t = gk__task(s, id);
    if (t == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    t->last_hours = now_hours;
    t->done_count++;
    return GK_OK;
}

double gk_maint_task_due_in(const gk_maint_schedule *s, int id,
                            double now_hours)
{
    int i;
    if (s == NULL) {
        return 0.0;
    }
    for (i = 0; i < s->count; ++i) {
        if (s->tasks[i].id == id) {
            double due = s->tasks[i].last_hours + s->tasks[i].interval_hours;
            return due - now_hours;
        }
    }
    return 0.0;
}

int gk_maint_task_is_due(const gk_maint_schedule *s, int id, double now_hours)
{
    return gk_maint_task_due_in(s, id, now_hours) <= 0.0;
}

int gk_maint_count_by_period(const gk_maint_schedule *s, gk_maint_period p)
{
    int i;
    int n = 0;
    if (s == NULL) {
        return 0;
    }
    for (i = 0; i < s->count; ++i) {
        if (s->tasks[i].period == p) n++;
    }
    return n;
}

int gk_maint_due_count(const gk_maint_schedule *s, double now_hours)
{
    int i;
    int n = 0;
    if (s == NULL) {
        return 0;
    }
    for (i = 0; i < s->count; ++i) {
        double due = s->tasks[i].last_hours + s->tasks[i].interval_hours;
        if (due - now_hours <= 0.0) n++;
    }
    return n;
}

/* ---- lube ---- */

void gk_lube_system_init(gk_lube_system *l, double capacity, double threshold)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
    l->capacity_l = capacity;
    l->level_l = capacity;
    l->low_threshold = threshold;
}

gk_status gk_lube_refill(gk_lube_system *l, double amount)
{
    if (l == NULL || amount < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    l->level_l += amount;
    if (l->level_l > l->capacity_l) {
        l->level_l = l->capacity_l;
    }
    return GK_OK;
}

gk_status gk_lube_consume(gk_lube_system *l, double hours)
{
    if (l == NULL || hours < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    l->level_l -= l->consumption_lph * hours;
    if (l->level_l < 0.0) {
        l->level_l = 0.0;
    }
    return GK_OK;
}

double gk_lube_level_fraction(const gk_lube_system *l)
{
    if (l == NULL || l->capacity_l <= 0.0) {
        return 0.0;
    }
    return l->level_l / l->capacity_l;
}

int gk_lube_level_low(const gk_lube_system *l)
{
    if (l == NULL) {
        return 1;
    }
    return gk_lube_level_fraction(l) <= l->low_threshold;
}

int gk_lube_auto_pulse(const gk_lube_system *l, double hours,
                       double pulse_volume)
{
    double total;
    if (l == NULL || hours <= 0.0 || pulse_volume <= 0.0) {
        return 0;
    }
    total = l->consumption_lph * hours;
    return (int)ceil(total / pulse_volume);
}

/* ---- guide cleaning ---- */

void gk_guide_clean_init(gk_guide_clean *g, double interval_hours)
{
    if (g == NULL) {
        return;
    }
    memset(g, 0, sizeof(*g));
    g->interval_hours = interval_hours;
}

gk_status gk_guide_clean_run(gk_guide_clean *g, double now_hours)
{
    if (g == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    g->last_clean_hours = now_hours;
    g->cleaned = 1;
    return GK_OK;
}

int gk_guide_clean_due(const gk_guide_clean *g, double now_hours)
{
    if (g == NULL || !g->cleaned) {
        return 1;
    }
    return (now_hours - g->last_clean_hours) >= g->interval_hours;
}

/* ---- filter ---- */

void gk_filter_init(gk_filter *f, double life_hours, double max_drop)
{
    if (f == NULL) {
        return;
    }
    memset(f, 0, sizeof(*f));
    f->life_hours = life_hours;
    f->max_pressure_drop = max_drop;
}

gk_status gk_filter_replace(gk_filter *f, double now_hours)
{
    if (f == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    f->installed_hours = now_hours;
    f->pressure_drop = 0.0;
    f->replaced = 1;
    return GK_OK;
}

int gk_filter_needs_change(const gk_filter *f, double now_hours)
{
    if (f == NULL) {
        return 0;
    }
    if (!f->replaced) {
        return 1;
    }
    return (now_hours - f->installed_hours) >= f->life_hours ||
           f->pressure_drop >= f->max_pressure_drop;
}

/* ---- belt ---- */

void gk_belt_init(gk_belt *b, double min_tension, double max_tension)
{
    if (b == NULL) {
        return;
    }
    memset(b, 0, sizeof(*b));
    b->min_tension = min_tension;
    b->max_tension = max_tension;
}

gk_status gk_belt_check(gk_belt *b, double tension, double wear)
{
    if (b == NULL || tension < 0.0 || wear < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    b->tension_n = tension;
    b->wear = wear;
    b->checked = 1;
    return GK_OK;
}

int gk_belt_ok(const gk_belt *b)
{
    if (b == NULL || !b->checked) {
        return 0;
    }
    return b->tension_n >= b->min_tension && b->tension_n <= b->max_tension &&
           b->wear < 0.8;
}

/* ---- precision check ---- */

void gk_precision_check_init(gk_precision_check *c)
{
    if (c != NULL) {
        memset(c, 0, sizeof(*c));
    }
}

gk_status gk_precision_add(gk_precision_check *c, double nominal,
                           double measured, double allowed)
{
    gk_precision_axis *a;
    if (c == NULL || allowed < 0.0 || c->count >= 8) {
        return GK_ERR_INVALID_ARG;
    }
    a = &c->axes[c->count++];
    a->nominal = nominal;
    a->measured = measured;
    a->allowed = allowed;
    a->error = measured - nominal;
    return GK_OK;
}

int gk_precision_all_pass(const gk_precision_check *c)
{
    int i;
    if (c == NULL) {
        return 0;
    }
    for (i = 0; i < c->count; ++i) {
        if (fabs(c->axes[i].error) > c->axes[i].allowed) {
            return 0;
        }
    }
    return 1;
}

double gk_precision_max_error(const gk_precision_check *c)
{
    double best = 0.0;
    int i;
    if (c == NULL) {
        return 0.0;
    }
    for (i = 0; i < c->count; ++i) {
        double e = fabs(c->axes[i].error);
        if (e > best) best = e;
    }
    return best;
}

/* ---- interferometer ---- */

void gk_interferometer_init(gk_interferometer *i)
{
    if (i == NULL) {
        return;
    }
    memset(i, 0, sizeof(*i));
    i->wavelength = 632.8;
}

gk_status gk_interferometer_sample(gk_interferometer *i, double position,
                                   double deviation)
{
    if (i == NULL || i->count >= 64) {
        return GK_ERR_OUT_OF_RANGE;
    }
    i->position[i->count] = position;
    i->deviation[i->count] = deviation;
    i->count++;
    return GK_OK;
}

double gk_interferometer_linear_error(const gk_interferometer *i)
{
    double sum = 0.0;
    int n = 0;
    int k;
    if (i == NULL || i->count < 2) {
        return 0.0;
    }
    /* slope of deviation vs position by least squares */
    {
        double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
        for (k = 0; k < i->count; ++k) {
            sx += i->position[k];
            sy += i->deviation[k];
            sxx += i->position[k] * i->position[k];
            sxy += i->position[k] * i->deviation[k];
        }
        n = i->count;
        sum = (n * sxy - sx * sy) / (n * sxx - sx * sx);
    }
    return sum;
}

/* ---- ballbar ---- */

void gk_ballbar_init(gk_ballbar *b, double radius)
{
    if (b == NULL) {
        return;
    }
    memset(b, 0, sizeof(*b));
    b->radius = radius;
}

gk_status gk_ballbar_run(gk_ballbar *b, double radial_error, double backlash)
{
    if (b == NULL || radial_error < 0.0 || backlash < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    b->radial_error = radial_error;
    b->backlash = backlash;
    b->circularity = radial_error * 2.0 + backlash;
    return GK_OK;
}

int gk_ballbar_pass(const gk_ballbar *b, double tolerance)
{
    if (b == NULL) {
        return 0;
    }
    return b->circularity <= tolerance;
}

/* ---- fault tree ---- */

void gk_maint_fault_tree_init(gk_maint_fault_tree *t)
{
    if (t != NULL) {
        memset(t, 0, sizeof(*t));
    }
}

gk_status gk_maint_fault_node_add(gk_maint_fault_tree *t, int id, int parent,
                                  const char *name, double prob, int is_leaf)
{
    gk_maint_fault_node *n;
    if (t == NULL || name == NULL || t->count >= GK_MAINT_MAX_PARTS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    n = &t->nodes[t->count++];
    memset(n, 0, sizeof(*n));
    n->id = id;
    n->parent = parent;
    n->probability = prob;
    n->is_leaf = is_leaf ? 1 : 0;
    gk__copy(n->name, sizeof(n->name), name);
    return GK_OK;
}

double gk_maint_fault_probability(const gk_maint_fault_tree *t, int node_id)
{
    double prod = 1.0;
    int found = 0;
    int i;
    if (t == NULL) {
        return 0.0;
    }
    /* leaf: return its own probability */
    for (i = 0; i < t->count; ++i) {
        if (t->nodes[i].id == node_id && t->nodes[i].is_leaf) {
            return t->nodes[i].probability;
        }
    }
    /* internal: OR over children */
    for (i = 0; i < t->count; ++i) {
        if (t->nodes[i].parent == node_id) {
            double p = gk_maint_fault_probability(t, t->nodes[i].id);
            prod *= (1.0 - p);
            found = 1;
        }
    }
    if (!found) {
        return 0.0;
    }
    return 1.0 - prod;
}

/* ---- spares ---- */

void gk_spare_store_init(gk_spare_store *s)
{
    if (s != NULL) {
        memset(s, 0, sizeof(*s));
    }
}

int gk_spare_add(gk_spare_store *s, const char *name, int stock, int min_stock,
                 double price)
{
    gk_spare_part *p;
    if (s == NULL || name == NULL || s->count >= GK_MAINT_MAX_PARTS) {
        return -1;
    }
    p = &s->parts[s->count++];
    memset(p, 0, sizeof(*p));
    p->id = s->count;
    gk__copy(p->name, sizeof(p->name), name);
    p->stock = stock;
    p->min_stock = min_stock;
    p->unit_price = price;
    return p->id;
}

static gk_spare_part *gk__spare(gk_spare_store *s, int id)
{
    int i;
    if (s == NULL) {
        return NULL;
    }
    for (i = 0; i < s->count; ++i) {
        if (s->parts[i].id == id) {
            return &s->parts[i];
        }
    }
    return NULL;
}

gk_status gk_spare_consume(gk_spare_store *s, int id, int qty)
{
    gk_spare_part *p = gk__spare(s, id);
    if (p == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    if (qty < 0 || qty > p->stock) {
        return GK_ERR_OUT_OF_RANGE;
    }
    p->stock -= qty;
    return GK_OK;
}

gk_status gk_spare_restock(gk_spare_store *s, int id, int qty)
{
    gk_spare_part *p = gk__spare(s, id);
    if (p == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    if (qty < 0) {
        return GK_ERR_INVALID_ARG;
    }
    p->stock += qty;
    return GK_OK;
}

int gk_spare_below_min(const gk_spare_store *s)
{
    int i;
    int n = 0;
    if (s == NULL) {
        return 0;
    }
    for (i = 0; i < s->count; ++i) {
        if (s->parts[i].stock < s->parts[i].min_stock) n++;
    }
    return n;
}

double gk_spare_value(const gk_spare_store *s)
{
    double sum = 0.0;
    int i;
    if (s == NULL) {
        return 0.0;
    }
    for (i = 0; i < s->count; ++i) {
        sum += s->parts[i].stock * s->parts[i].unit_price;
    }
    return sum;
}

/* ---- work orders ---- */

const char *gk_work_order_status_name(gk_work_order_status s)
{
    switch (s) {
    case GK_WO_OPEN: return "open";
    case GK_WO_IN_PROGRESS: return "in-progress";
    case GK_WO_DONE: return "done";
    default: return "unknown";
    }
}

void gk_work_order_book_init(gk_work_order_book *b)
{
    if (b == NULL) {
        return;
    }
    memset(b, 0, sizeof(*b));
    b->next_id = 1;
}

int gk_work_order_create(gk_work_order_book *b, const char *desc, int priority)
{
    gk_work_order *w;
    if (b == NULL || desc == NULL || b->count >= GK_MAINT_MAX_TASKS) {
        return -1;
    }
    w = &b->orders[b->count++];
    memset(w, 0, sizeof(*w));
    w->id = b->next_id++;
    gk__copy(w->description, sizeof(w->description), desc);
    w->priority = priority;
    w->status = GK_WO_OPEN;
    return w->id;
}

static gk_work_order *gk__wo(gk_work_order_book *b, int id)
{
    int i;
    if (b == NULL) {
        return NULL;
    }
    for (i = 0; i < b->count; ++i) {
        if (b->orders[i].id == id) {
            return &b->orders[i];
        }
    }
    return NULL;
}

gk_status gk_work_order_assign(gk_work_order_book *b, int id, int assignee)
{
    gk_work_order *w = gk__wo(b, id);
    if (w == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    if (w->status == GK_WO_DONE) {
        return GK_ERR_STATE;
    }
    w->assignee = assignee;
    w->status = GK_WO_IN_PROGRESS;
    return GK_OK;
}

gk_status gk_work_order_log(gk_work_order_book *b, int id, double hours)
{
    gk_work_order *w = gk__wo(b, id);
    if (w == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    if (hours < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    w->hours_spent += hours;
    return GK_OK;
}

gk_status gk_work_order_complete(gk_work_order_book *b, int id)
{
    gk_work_order *w = gk__wo(b, id);
    if (w == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    w->status = GK_WO_DONE;
    return GK_OK;
}

int gk_work_order_open_count(const gk_work_order_book *b)
{
    int i;
    int n = 0;
    if (b == NULL) {
        return 0;
    }
    for (i = 0; i < b->count; ++i) {
        if (b->orders[i].status != GK_WO_DONE) n++;
    }
    return n;
}

/* ---- repair history ---- */

void gk_repair_history_init(gk_repair_history *h)
{
    if (h != NULL) {
        memset(h, 0, sizeof(*h));
    }
}

gk_status gk_repair_record_add(gk_repair_history *h, int work_order_id,
                               const char *note, double hours, double cost)
{
    gk_repair_log *r;
    if (h == NULL || hours < 0.0 || cost < 0.0 ||
        h->count >= GK_MAINT_MAX_TASKS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    r = &h->entries[h->count++];
    memset(r, 0, sizeof(*r));
    r->work_order_id = work_order_id;
    gk__copy(r->note, sizeof(r->note), note);
    r->hours = hours;
    r->cost = cost;
    h->total_cost += cost;
    return GK_OK;
}

double gk_repair_total_hours(const gk_repair_history *h)
{
    double sum = 0.0;
    int i;
    if (h == NULL) {
        return 0.0;
    }
    for (i = 0; i < h->count; ++i) {
        sum += h->entries[i].hours;
    }
    return sum;
}

/* ---- reminder ---- */

double gk_maint_next_due(const gk_maint_task *t, double now_hours)
{
    if (t == NULL) {
        return 0.0;
    }
    (void)now_hours;
    return t->last_hours + t->interval_hours;
}

/* ---- life prediction ---- */

void gk_component_life_init(gk_component_life *l, double design_hours)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
    l->design_hours = design_hours;
    l->degradation = 1.0;
}

double gk_component_rul(const gk_component_life *l)
{
    double effective;
    if (l == NULL || l->degradation <= 0.0) {
        return 0.0;
    }
    effective = l->used_hours * l->degradation;
    if (effective >= l->design_hours) {
        return 0.0;
    }
    return (l->design_hours - effective) / l->degradation;
}

double gk_component_health(const gk_component_life *l)
{
    double rul;
    if (l == NULL || l->design_hours <= 0.0) {
        return 0.0;
    }
    rul = gk_component_rul(l);
    if (rul > l->design_hours) {
        return 1.0;
    }
    return rul / l->design_hours;
}
