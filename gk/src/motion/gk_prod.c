#include "gk/gk_prod.h"

#include <math.h>
#include <string.h>

static void gk__copy(char *dst, size_t cap, const char *src)
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

/* ===================================================================
 * Part A: shop-floor integration
 * =================================================================== */

void gk_prod_cell_init(gk_prod_cell *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
}

int gk_prod_cell_add(gk_prod_cell *c, const char *name, double available_h)
{
    if (c == NULL || name == NULL || available_h <= 0.0 ||
        c->count >= GK_PROD_MAX_ITEMS) {
        return -1;
    }
    gk__copy(c->machines[c->count].name, sizeof(c->machines[c->count].name),
             name);
    c->machines[c->count].available_h = available_h;
    c->count++;
    return c->count - 1;
}

int gk_prod_cell_assign(gk_prod_cell *c, double hours)
{
    int i;
    int best = -1;
    double best_remaining = -1.0;
    if (c == NULL || hours < 0.0) {
        return -1;
    }
    for (i = 0; i < c->count; i++) {
        double remaining = c->machines[i].available_h -
                           c->machines[i].assigned_h;
        if (remaining < hours) {
            continue;
        }
        if (best < 0 || remaining > best_remaining) {
            best = i;
            best_remaining = remaining;
        }
    }
    if (best < 0) {
        return -1;
    }
    c->machines[best].assigned_h += hours;
    c->machines[best].jobs++;
    return best;
}

double gk_prod_cell_utilization(const gk_prod_cell *c, int index)
{
    if (c == NULL || index < 0 || index >= c->count ||
        c->machines[index].available_h <= 0.0) {
        return 0.0;
    }
    return c->machines[index].assigned_h / c->machines[index].available_h;
}

void gk_prod_line_init(gk_prod_line *l)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
}

int gk_prod_line_add(gk_prod_line *l, const char *name, double cycle_time_s)
{
    if (l == NULL || name == NULL || cycle_time_s <= 0.0 ||
        l->count >= GK_PROD_MAX_ITEMS) {
        return -1;
    }
    gk__copy(l->stations[l->count].name, sizeof(l->stations[l->count].name),
             name);
    l->stations[l->count].cycle_time_s = cycle_time_s;
    l->stations[l->count].capable = 1;
    l->count++;
    return l->count - 1;
}

int gk_prod_line_bottleneck(const gk_prod_line *l)
{
    int i;
    int best = -1;
    double slowest = 0.0;
    if (l == NULL) {
        return -1;
    }
    for (i = 0; i < l->count; i++) {
        if (l->stations[i].capable &&
            l->stations[i].cycle_time_s > slowest) {
            slowest = l->stations[i].cycle_time_s;
            best = i;
        }
    }
    return best;
}

double gk_prod_line_takt(const gk_prod_line *l)
{
    int b = gk_prod_line_bottleneck(l);
    if (l == NULL || b < 0) {
        return 0.0;
    }
    return l->stations[b].cycle_time_s;
}

void gk_prod_agv_init(gk_prod_agv *a, double speed_mps)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    a->speed_mps = (speed_mps > 0.0) ? speed_mps : 1.0;
    a->battery_pct = 100.0;
    a->charge_rate_pct_per_s = 1.0;
}

double gk_prod_agv_travel_time(const gk_prod_agv *a, double target_m)
{
    double dist;
    if (a == NULL || a->speed_mps <= 0.0) {
        return 0.0;
    }
    dist = fabs(target_m - a->position_m);
    return dist / a->speed_mps;
}

gk_status gk_prod_agv_move(gk_prod_agv *a, double target_m)
{
    double t;
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    t = gk_prod_agv_travel_time(a, target_m);
    a->battery_pct -= t * 0.05;
    if (a->battery_pct < 0.0) {
        a->battery_pct = 0.0;
    }
    a->position_m = target_m;
    return GK_OK;
}

gk_status gk_prod_agv_charge(gk_prod_agv *a, double seconds)
{
    if (a == NULL || seconds < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    a->battery_pct += seconds * a->charge_rate_pct_per_s;
    if (a->battery_pct > 100.0) {
        a->battery_pct = 100.0;
    }
    return GK_OK;
}

void gk_prod_robot_init(gk_prod_robot *r)
{
    if (r == NULL) {
        return;
    }
    r->approach_s = 0.5;
    r->grip_s = 0.3;
    r->transfer_s = 1.0;
    r->release_s = 0.3;
}

double gk_prod_robot_cycle_time(const gk_prod_robot *r, double distance_m,
                                double speed_mps)
{
    double move;
    if (r == NULL || speed_mps <= 0.0) {
        return 0.0;
    }
    move = 2.0 * distance_m / speed_mps;
    return r->approach_s + r->grip_s + r->transfer_s + r->release_s + move;
}

gk_status gk_prod_vision_find(const double *xs, const double *ys, int count,
                              double *out_cx, double *out_cy)
{
    int i;
    double sx = 0.0, sy = 0.0;
    if (xs == NULL || ys == NULL || out_cx == NULL || out_cy == NULL ||
        count <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < count; i++) {
        sx += xs[i];
        sy += ys[i];
    }
    *out_cx = sx / (double)count;
    *out_cy = sy / (double)count;
    return GK_OK;
}

double gk_prod_vision_offset(double detected, double nominal)
{
    return detected - nominal;
}

void gk_prod_rfid_init(gk_prod_rfid *t, const char *tag_id)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    gk__copy(t->tag_id, sizeof(t->tag_id), tag_id);
}

gk_status gk_prod_rfid_write(gk_prod_rfid *t, const char *payload)
{
    if (t == NULL || payload == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    gk__copy(t->payload, sizeof(t->payload), payload);
    t->valid = 1;
    return GK_OK;
}

gk_status gk_prod_rfid_read(const gk_prod_rfid *t, char *out, size_t out_cap)
{
    if (t == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    if (!t->valid) {
        return GK_ERR_STATE;
    }
    gk__copy(out, out_cap, t->payload);
    return GK_OK;
}

const char *gk_prod_mes_state_name(gk_prod_mes_state s)
{
    switch (s) {
    case GK_PROD_MES_PLANNED: return "planned";
    case GK_PROD_MES_RELEASED: return "released";
    case GK_PROD_MES_RUNNING: return "running";
    case GK_PROD_MES_DONE: return "done";
    default: return "unknown";
    }
}

void gk_prod_mes_init(gk_prod_mes *m, const char *order)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    gk__copy(m->order, sizeof(m->order), order);
    m->state = GK_PROD_MES_PLANNED;
}

gk_status gk_prod_mes_advance(gk_prod_mes *m)
{
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (m->state >= GK_PROD_MES_DONE) {
        return GK_ERR_STATE;
    }
    m->state = (gk_prod_mes_state)(m->state + 1);
    return GK_OK;
}

gk_status gk_prod_mes_report(gk_prod_mes *m, int good, int scrap)
{
    if (m == NULL || good < 0 || scrap < 0) {
        return GK_ERR_INVALID_ARG;
    }
    if (m->state != GK_PROD_MES_RUNNING) {
        return GK_ERR_STATE;
    }
    m->good += good;
    m->scrap += scrap;
    return GK_OK;
}

void gk_prod_erp_init(gk_prod_erp *e, const char *order, int quantity,
                      long due_day)
{
    if (e == NULL) {
        return;
    }
    memset(e, 0, sizeof(*e));
    gk__copy(e->order, sizeof(e->order), order);
    e->quantity = quantity;
    e->due_day = due_day;
}

double gk_prod_erp_progress(const gk_prod_erp *e)
{
    if (e == NULL || e->quantity <= 0) {
        return 0.0;
    }
    return (double)e->produced / (double)e->quantity;
}

gk_status gk_prod_erp_receive(gk_prod_erp *e, int qty)
{
    if (e == NULL || qty < 0) {
        return GK_ERR_INVALID_ARG;
    }
    e->produced += qty;
    return GK_OK;
}

void gk_prod_wms_init(gk_prod_wms *w, const char *sku, int on_hand)
{
    if (w == NULL) {
        return;
    }
    memset(w, 0, sizeof(*w));
    gk__copy(w->sku, sizeof(w->sku), sku);
    w->on_hand = on_hand;
}

int gk_prod_wms_available(const gk_prod_wms *w)
{
    if (w == NULL) {
        return 0;
    }
    return w->on_hand - w->reserved;
}

gk_status gk_prod_wms_reserve(gk_prod_wms *w, int qty)
{
    if (w == NULL || qty < 0) {
        return GK_ERR_INVALID_ARG;
    }
    if (qty > gk_prod_wms_available(w)) {
        return GK_ERR_STATE;
    }
    w->reserved += qty;
    return GK_OK;
}

gk_status gk_prod_wms_pick(gk_prod_wms *w, int qty)
{
    if (w == NULL || qty < 0) {
        return GK_ERR_INVALID_ARG;
    }
    if (qty > w->reserved || qty > w->on_hand) {
        return GK_ERR_STATE;
    }
    w->reserved -= qty;
    w->on_hand -= qty;
    return GK_OK;
}

void gk_prod_asrs_init(gk_prod_asrs *a, int bins)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    a->bins = bins;
    a->aisle_time_s = 20.0;
}

gk_status gk_prod_asrs_store(gk_prod_asrs *a)
{
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (a->stored >= a->bins) {
        return GK_ERR_STATE;
    }
    a->stored++;
    return GK_OK;
}

gk_status gk_prod_asrs_retrieve(gk_prod_asrs *a)
{
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (a->stored <= 0) {
        return GK_ERR_STATE;
    }
    a->stored--;
    return GK_OK;
}

double gk_prod_asrs_cycle_time(const gk_prod_asrs *a)
{
    if (a == NULL) {
        return 0.0;
    }
    return 2.0 * a->aisle_time_s;
}

/* ===================================================================
 * Part B: optimisation
 * =================================================================== */

gk_status gk_prod_schedule(const gk_prod_job *jobs, int count, int *order)
{
    int i, j;
    if (jobs == NULL || order == NULL || count <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < count; i++) {
        order[i] = i;
    }
    for (i = 1; i < count; i++) {
        int key = order[i];
        j = i - 1;
        while (j >= 0) {
            const gk_prod_job *a = &jobs[order[j]];
            const gk_prod_job *b = &jobs[key];
            int before;
            if (a->priority != b->priority) {
                before = a->priority >= b->priority;
            } else {
                before = a->due_day <= b->due_day;
            }
            if (before) {
                break;
            }
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = key;
    }
    return GK_OK;
}

gk_status gk_prod_optimize_process(const gk_prod_operation *ops, int count,
                                   int *order)
{
    int i, n = 0;
    int done[GK_PROD_MAX_ITEMS];
    if (ops == NULL || order == NULL || count <= 0 ||
        count > GK_PROD_MAX_ITEMS) {
        return GK_ERR_INVALID_ARG;
    }
    memset(done, 0, sizeof(done));
    while (n < count) {
        int progressed = 0;
        for (i = 0; i < count; i++) {
            if (done[i]) {
                continue;
            }
            if (ops[i].predecessor < 0 ||
                (ops[i].predecessor < count && done[ops[i].predecessor])) {
                order[n++] = i;
                done[i] = 1;
                progressed = 1;
            }
        }
        if (!progressed) {
            return GK_ERR_STATE;   /* cyclic precedence */
        }
    }
    return GK_OK;
}

gk_status gk_prod_optimize_path(const double *xs, const double *ys, int count,
                                int *order, double *out_length)
{
    int i, used[GK_PROD_MAX_ITEMS];
    int current;
    double length = 0.0;
    if (xs == NULL || ys == NULL || order == NULL || count <= 0 ||
        count > GK_PROD_MAX_ITEMS) {
        return GK_ERR_INVALID_ARG;
    }
    memset(used, 0, sizeof(used));
    order[0] = 0;
    used[0] = 1;
    current = 0;
    for (i = 1; i < count; i++) {
        int best = -1;
        double best_d = 0.0;
        int k;
        for (k = 0; k < count; k++) {
            double dx, dy, d;
            if (used[k]) {
                continue;
            }
            dx = xs[k] - xs[current];
            dy = ys[k] - ys[current];
            d = dx * dx + dy * dy;
            if (best < 0 || d < best_d) {
                best = k;
                best_d = d;
            }
        }
        length += sqrt(best_d);
        order[i] = best;
        used[best] = 1;
        current = best;
    }
    if (out_length != NULL) {
        *out_length = length;
    }
    return GK_OK;
}

gk_status gk_prod_optimize(double lo, double hi, int steps,
                           gk_prod_objective f, void *ctx, gk_prod_goal goal,
                           gk_prod_result *out)
{
    int i;
    double best_x = lo;
    double best_v = 0.0;
    if (f == NULL || out == NULL || steps < 1 || hi < lo) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < steps; i++) {
        double x = lo + (hi - lo) * (double)i / (double)(steps - 1 > 0
                                                             ? steps - 1
                                                             : 1);
        double v = f(x, ctx);
        if (i == 0) {
            best_x = x;
            best_v = v;
        } else if ((goal == GK_PROD_MINIMISE && v < best_v) ||
                   (goal == GK_PROD_MAXIMISE && v > best_v)) {
            best_x = x;
            best_v = v;
        }
    }
    out->x = best_x;
    out->value = best_v;
    out->evaluations = steps;
    return GK_OK;
}

gk_status gk_prod_optimize_params(gk_prod_objective f, void *ctx, double lo,
                                  double hi, int steps, gk_prod_result *out)
{
    return gk_prod_optimize(lo, hi, steps, f, ctx, GK_PROD_MAXIMISE, out);
}

gk_status gk_prod_optimize_tech(gk_prod_objective f, void *ctx, double lo,
                                double hi, int steps, gk_prod_result *out)
{
    return gk_prod_optimize(lo, hi, steps, f, ctx, GK_PROD_MAXIMISE, out);
}

gk_status gk_prod_optimize_cost(gk_prod_objective f, void *ctx, double lo,
                                double hi, int steps, gk_prod_result *out)
{
    return gk_prod_optimize(lo, hi, steps, f, ctx, GK_PROD_MINIMISE, out);
}

gk_status gk_prod_optimize_energy(gk_prod_objective f, void *ctx, double lo,
                                  double hi, int steps, gk_prod_result *out)
{
    return gk_prod_optimize(lo, hi, steps, f, ctx, GK_PROD_MINIMISE, out);
}

gk_status gk_prod_optimize_quality(gk_prod_objective f, void *ctx, double lo,
                                   double hi, int steps, gk_prod_result *out)
{
    return gk_prod_optimize(lo, hi, steps, f, ctx, GK_PROD_MAXIMISE, out);
}

gk_status gk_prod_optimize_efficiency(gk_prod_objective f, void *ctx,
                                      double lo, double hi, int steps,
                                      gk_prod_result *out)
{
    return gk_prod_optimize(lo, hi, steps, f, ctx, GK_PROD_MAXIMISE, out);
}

gk_status gk_prod_optimize_comprehensive(gk_prod_objective f, void *ctx,
                                         double lo, double hi, int steps,
                                         gk_prod_result *out)
{
    return gk_prod_optimize(lo, hi, steps, f, ctx, GK_PROD_MAXIMISE, out);
}
