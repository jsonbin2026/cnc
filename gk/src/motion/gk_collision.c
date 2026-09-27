#include "gk/gk_collision.h"

#include <string.h>

const char *gk_body_kind_name(gk_body_kind k)
{
    switch (k) {
    case GK_BODY_NONE:
        return "none";
    case GK_BODY_TOOL:
        return "tool";
    case GK_BODY_HOLDER:
        return "holder";
    case GK_BODY_SPINDLE:
        return "spindle";
    case GK_BODY_WORKPIECE:
        return "workpiece";
    case GK_BODY_FIXTURE:
        return "fixture";
    case GK_BODY_TABLE:
        return "table";
    default:
        return "unknown";
    }
}

const char *gk_collision_type_name(gk_collision_type t)
{
    switch (t) {
    case GK_COLLISION_NONE:
        return "none";
    case GK_COLLISION_TOOL_WORKPIECE:
        return "tool-workpiece";
    case GK_COLLISION_TOOL_FIXTURE:
        return "tool-fixture";
    case GK_COLLISION_SPINDLE_WORKPIECE:
        return "spindle-workpiece";
    case GK_COLLISION_HOLDER_WORKPIECE:
        return "holder-workpiece";
    case GK_COLLISION_TABLE_SPINDLE:
        return "table-spindle";
    case GK_COLLISION_CRASH:
        return "crash";
    default:
        return "unknown";
    }
}

void gk_collision_world_init(gk_collision_world *w)
{
    if (w == NULL) {
        return;
    }
    memset(w, 0, sizeof(*w));
}

gk_status gk_collision_world_add(gk_collision_world *w, gk_body_kind kind,
                                 gk_aabb box, const char *name)
{
    gk_collision_body *b;
    if (w == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (w->body_count >= GK_COLLISION_MAX_BODIES) {
        return GK_ERR_OVERFLOW;
    }
    b = &w->bodies[w->body_count];
    memset(b, 0, sizeof(*b));
    b->kind = kind;
    b->enabled = 1;
    b->box = box;
    if (name != NULL) {
        size_t i;
        for (i = 0; i < sizeof(b->name) - 1 && name[i] != '\0'; ++i) {
            b->name[i] = name[i];
        }
        b->name[i] = '\0';
    }
    w->body_count += 1;
    return GK_OK;
}

gk_status gk_collision_world_move(gk_collision_world *w, size_t index,
                                  gk_aabb box)
{
    if (w == NULL || index >= w->body_count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    w->bodies[index].box = box;
    return GK_OK;
}

static gk_collision_type classify(gk_body_kind a, gk_body_kind b)
{
    gk_body_kind lo = a < b ? a : b;
    gk_body_kind hi = a < b ? b : a;
    if (lo == GK_BODY_TOOL && hi == GK_BODY_WORKPIECE) {
        return GK_COLLISION_TOOL_WORKPIECE;
    }
    if (lo == GK_BODY_TOOL && hi == GK_BODY_FIXTURE) {
        return GK_COLLISION_TOOL_FIXTURE;
    }
    if (lo == GK_BODY_SPINDLE && hi == GK_BODY_WORKPIECE) {
        return GK_COLLISION_SPINDLE_WORKPIECE;
    }
    if (lo == GK_BODY_HOLDER && hi == GK_BODY_WORKPIECE) {
        return GK_COLLISION_HOLDER_WORKPIECE;
    }
    if (lo == GK_BODY_SPINDLE && hi == GK_BODY_TABLE) {
        return GK_COLLISION_TABLE_SPINDLE;
    }
    return GK_COLLISION_CRASH;
}

int gk_collision_test_pair(const gk_collision_body *a,
                           const gk_collision_body *b,
                           gk_collision_event *event)
{
    gk_aabb o;
    if (a == NULL || b == NULL || !a->enabled || !b->enabled) {
        return 0;
    }
    if (!gk_aabb_overlaps(&a->box, &b->box)) {
        return 0;
    }
    o.min.x = a->box.min.x > b->box.min.x ? a->box.min.x : b->box.min.x;
    o.min.y = a->box.min.y > b->box.min.y ? a->box.min.y : b->box.min.y;
    o.min.z = a->box.min.z > b->box.min.z ? a->box.min.z : b->box.min.z;
    o.max.x = a->box.max.x < b->box.max.x ? a->box.max.x : b->box.max.x;
    o.max.y = a->box.max.y < b->box.max.y ? a->box.max.y : b->box.max.y;
    o.max.z = a->box.max.z < b->box.max.z ? a->box.max.z : b->box.max.z;
    if (event != NULL) {
        double dx = o.max.x - o.min.x;
        double dy = o.max.y - o.min.y;
        double dz = o.max.z - o.min.z;
        event->type = classify(a->kind, b->kind);
        event->a_index = 0;
        event->b_index = 0;
        event->overlap = o;
        event->penetration = dx < dy ? (dx < dz ? dx : dz) : (dy < dz ? dy : dz);
        if (event->penetration < 0.0) {
            event->penetration = 0.0;
        }
        event->severity = event->penetration > 5.0
                              ? 1.0
                              : event->penetration / 5.0;
    }
    return 1;
}

size_t gk_collision_check(const gk_collision_world *w,
                          gk_collision_event *events, size_t max_events)
{
    size_t i, j, n = 0;
    if (w == NULL) {
        return 0;
    }
    for (i = 0; i < w->body_count; ++i) {
        for (j = i + 1; j < w->body_count; ++j) {
            gk_collision_event ev;
            if (gk_collision_test_pair(&w->bodies[i], &w->bodies[j], &ev)) {
                ev.a_index = i;
                ev.b_index = j;
                if (events != NULL && n < max_events) {
                    events[n] = ev;
                }
                n += 1;
            }
        }
    }
    return n;
}

double gk_crash_damage(const gk_collision_event *events, size_t count)
{
    double worst = 0.0;
    size_t i;
    if (events == NULL) {
        return 0.0;
    }
    for (i = 0; i < count; ++i) {
        double s = events[i].severity;
        double weight = 1.0;
        if (events[i].type == GK_COLLISION_CRASH) {
            weight = 1.5;
        } else if (events[i].type == GK_COLLISION_TABLE_SPINDLE) {
            weight = 1.3;
        }
        if (s * weight > worst) {
            worst = s * weight;
        }
    }
    return worst > 1.0 ? 1.0 : worst;
}

int gk_collision_alarm(const gk_collision_event *event, double threshold)
{
    if (event == NULL) {
        return 0;
    }
    return (event->severity >= threshold) ? 1 : 0;
}
