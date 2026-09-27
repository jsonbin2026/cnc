#ifndef GK_CANNED_H
#define GK_CANNED_H

#include <stddef.h>
#include "gk/gk_error.h"
#include "gk/gk_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GK_CANNED_NONE = 0,
    GK_CANNED_G73,   /* high-speed peck drilling */
    GK_CANNED_G74,   /* left-hand tapping */
    GK_CANNED_G76,   /* fine boring */
    GK_CANNED_G81,   /* drilling */
    GK_CANNED_G82,   /* drilling with dwell */
    GK_CANNED_G83,   /* deep-hole peck drilling */
    GK_CANNED_G84,   /* tapping */
    GK_CANNED_G85,   /* boring */
    GK_CANNED_G86,   /* boring (spindle stop, rapid out) */
    GK_CANNED_G87,   /* back boring */
    GK_CANNED_G88,   /* boring (manual retract) */
    GK_CANNED_G89    /* boring with dwell */
} gk_canned_cycle;

typedef enum {
    GK_RETRACT_INITIAL = 0,  /* G98 */
    GK_RETRACT_R_PLANE       /* G99 */
} gk_retract_mode;

typedef struct {
    gk_canned_cycle cycle;
    gk_retract_mode retract;
    gk_point3 hole;          /* hole position in the active plane */
    double r_plane;          /* R point */
    double z_depth;          /* Z depth (absolute) */
    double q_peck;           /* peck increment, 0 = single plunge */
    double p_dwell;          /* dwell seconds */
    double f_feed;
    int    repeat;           /* K repeats */
    int    spindle_dir;      /* +1 for G84, -1 for G74 */
} gk_canned_params;

typedef enum {
    GK_CANNED_MOVE_RAPID_TO_XY = 0,
    GK_CANNED_MOVE_RAPID_TO_R,
    GK_CANNED_MOVE_FEED_PLUNGE,
    GK_CANNED_MOVE_PECK_RETRACT,
    GK_CANNED_MOVE_DWELL,
    GK_CANNED_MOVE_FEED_OUT,
    GK_CANNED_MOVE_RAPID_OUT,
    GK_CANNED_MOVE_SPINDLE_ON,
    GK_CANNED_MOVE_SPINDLE_REVERSE
} gk_canned_action_kind;

typedef struct {
    gk_canned_action_kind kind;
    gk_point3 target;
    double dwell;
    double feed;
} gk_canned_action;

typedef struct {
    gk_canned_params params;
    gk_canned_action *actions;
    size_t action_count;
    size_t action_cap;
} gk_canned_plan;

void gk_canned_plan_init(gk_canned_plan *plan);
void gk_canned_plan_free(gk_canned_plan *plan);

/* Expand a canned cycle into a concrete action list.
 * hole_xy is the (x,y) target; prior_z is the current Z when the cycle
 * starts (used as the initial point for G98). */
gk_status gk_canned_expand(const gk_canned_params *p, gk_point3 hole_xy,
                           double prior_z, gk_canned_plan *out);

const char *gk_canned_name(gk_canned_cycle cycle);
int gk_canned_is_cycle(int gcode);

#ifdef __cplusplus
}
#endif

#endif
