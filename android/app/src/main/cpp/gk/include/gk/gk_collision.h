#ifndef GK_COLLISION_H
#define GK_COLLISION_H

#include <stddef.h>
#include "gk/gk_error.h"
#include "gk/gk_math.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_COLLISION_MAX_BODIES 32

typedef enum {
    GK_BODY_NONE = 0,
    GK_BODY_TOOL,
    GK_BODY_HOLDER,
    GK_BODY_SPINDLE,
    GK_BODY_WORKPIECE,
    GK_BODY_FIXTURE,
    GK_BODY_TABLE
} gk_body_kind;

const char *gk_body_kind_name(gk_body_kind k);

typedef struct {
    gk_body_kind kind;
    int enabled;
    gk_aabb box;
    char name[32];
} gk_collision_body;

typedef enum {
    GK_COLLISION_NONE = 0,
    GK_COLLISION_TOOL_WORKPIECE,
    GK_COLLISION_TOOL_FIXTURE,
    GK_COLLISION_SPINDLE_WORKPIECE,
    GK_COLLISION_HOLDER_WORKPIECE,
    GK_COLLISION_TABLE_SPINDLE,
    GK_COLLISION_CRASH
} gk_collision_type;

const char *gk_collision_type_name(gk_collision_type t);

typedef struct {
    gk_collision_type type;
    size_t a_index;
    size_t b_index;
    gk_aabb overlap;
    double penetration;     /* mm */
    double severity;        /* 0..1 */
} gk_collision_event;

typedef struct {
    gk_collision_body bodies[GK_COLLISION_MAX_BODIES];
    size_t body_count;
} gk_collision_world;

void gk_collision_world_init(gk_collision_world *w);
gk_status gk_collision_world_add(gk_collision_world *w, gk_body_kind kind,
                                 gk_aabb box, const char *name);
gk_status gk_collision_world_move(gk_collision_world *w, size_t index,
                                  gk_aabb box);

/* Test a single pair. Returns 1 on collision, fills event if non-NULL. */
int gk_collision_test_pair(const gk_collision_body *a,
                           const gk_collision_body *b,
                           gk_collision_event *event);

/* Test the whole world; returns number of events found (up to max). */
size_t gk_collision_check(const gk_collision_world *w,
                          gk_collision_event *events, size_t max_events);

/* Machine-crash evaluation: overall damage estimate 0..1. */
double gk_crash_damage(const gk_collision_event *events, size_t count);

/* Collision alarm threshold check. */
int gk_collision_alarm(const gk_collision_event *event, double threshold);

#ifdef __cplusplus
}
#endif

#endif
