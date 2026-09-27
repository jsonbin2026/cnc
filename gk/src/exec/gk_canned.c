#include "gk/gk_canned.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static const char *const k_canned_names[] = {
    "none", "G73", "G74", "G76", "G81", "G82", "G83", "G84",
    "G85", "G86", "G87", "G88", "G89",
};

const char *gk_canned_name(gk_canned_cycle cycle)
{
    if ((int)cycle < 0 || (int)cycle > (int)GK_CANNED_G89) {
        return "unknown";
    }
    return k_canned_names[(int)cycle];
}

int gk_canned_is_cycle(int gcode)
{
    switch (gcode) {
    case 73: case 74: case 76:
    case 81: case 82: case 83: case 84: case 85:
    case 86: case 87: case 88: case 89:
        return 1;
    default:
        return 0;
    }
}

void gk_canned_plan_init(gk_canned_plan *plan)
{
    if (plan == NULL) {
        return;
    }
    memset(plan, 0, sizeof(*plan));
}

void gk_canned_plan_free(gk_canned_plan *plan)
{
    if (plan == NULL) {
        return;
    }
    free(plan->actions);
    plan->actions = NULL;
    plan->action_count = 0;
    plan->action_cap = 0;
}

static gk_status push_action(gk_canned_plan *plan, gk_canned_action_kind kind,
                             gk_point3 target, double dwell, double feed)
{
    gk_canned_action *mem;
    if (plan->action_count == plan->action_cap) {
        size_t next = plan->action_cap == 0 ? 16 : plan->action_cap * 2;
        mem = realloc(plan->actions, next * sizeof(*mem));
        if (mem == NULL) {
            return GK_ERR_NO_MEMORY;
        }
        plan->actions = mem;
        plan->action_cap = next;
    }
    plan->actions[plan->action_count].kind = kind;
    plan->actions[plan->action_count].target = target;
    plan->actions[plan->action_count].dwell = dwell;
    plan->actions[plan->action_count].feed = feed;
    plan->action_count += 1;
    return GK_OK;
}

/* Determine the retract height: initial point for G98, R plane for G99. */
static double retract_height(const gk_canned_params *p, double prior_z)
{
    return p->retract == GK_RETRACT_R_PLANE ? p->r_plane : prior_z;
}

static gk_status emit_peck(gk_canned_plan *plan, const gk_canned_params *p,
                           gk_point3 xy, double retract_z, int full_retract)
{
    double current = p->r_plane;
    gk_point3 pt = xy;
    double depth = p->z_depth;

    if (depth > current) {
        return GK_ERR_OUT_OF_RANGE;
    }

    if (p->q_peck <= 0.0) {
        pt.z = depth;
        if (push_action(plan, GK_CANNED_MOVE_FEED_PLUNGE, pt, 0.0,
                        p->f_feed) != GK_OK) {
            return GK_ERR_NO_MEMORY;
        }
        if (p->cycle == GK_CANNED_G82 || p->cycle == GK_CANNED_G89 ||
            p->cycle == GK_CANNED_G76) {
            pt.z = depth;
            if (push_action(plan, GK_CANNED_MOVE_DWELL, pt, p->p_dwell,
                            0.0) != GK_OK) {
                return GK_ERR_NO_MEMORY;
            }
        }
        return GK_OK;
    }

    while (current > depth) {
        current -= p->q_peck;
        if (current < depth) {
            current = depth;
        }
        pt.z = current;
        if (push_action(plan, GK_CANNED_MOVE_FEED_PLUNGE, pt, 0.0,
                        p->f_feed) != GK_OK) {
            return GK_ERR_NO_MEMORY;
        }
        if (current <= depth) {
            break;
        }
        if (full_retract) {
            pt.z = retract_z;
            if (push_action(plan, GK_CANNED_MOVE_RAPID_OUT, pt, 0.0,
                            0.0) != GK_OK) {
                return GK_ERR_NO_MEMORY;
            }
            pt.z = current;
            if (push_action(plan, GK_CANNED_MOVE_RAPID_TO_R, pt, 0.0,
                            0.0) != GK_OK) {
                return GK_ERR_NO_MEMORY;
            }
        } else {
            pt.z = current + p->q_peck * 0.5;
            if (push_action(plan, GK_CANNED_MOVE_PECK_RETRACT, pt, 0.0,
                            0.0) != GK_OK) {
                return GK_ERR_NO_MEMORY;
            }
            pt.z = current;
            if (push_action(plan, GK_CANNED_MOVE_RAPID_TO_R, pt, 0.0,
                            0.0) != GK_OK) {
                return GK_ERR_NO_MEMORY;
            }
        }
    }
    return GK_OK;
}

gk_status gk_canned_expand(const gk_canned_params *p, gk_point3 hole_xy,
                           double prior_z, gk_canned_plan *out)
{
    double retract_z;
    gk_point3 pt;
    gk_status st;

    if (p == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (p->cycle == GK_CANNED_NONE) {
        return GK_ERR_INVALID_ARG;
    }
    if (p->z_depth > p->r_plane) {
        return GK_ERR_OUT_OF_RANGE;
    }

    gk_canned_plan_init(out);
    out->params = *p;
    retract_z = retract_height(p, prior_z);

    pt = hole_xy;
    pt.z = retract_z;
    st = push_action(out, GK_CANNED_MOVE_RAPID_TO_XY, pt, 0.0, 0.0);
    if (st != GK_OK) {
        return st;
    }

    /* Spindle reversals for tapping cycles. */
    if (p->cycle == GK_CANNED_G84) {
        st = push_action(out, GK_CANNED_MOVE_SPINDLE_ON, pt, 0.0, 0.0);
        if (st != GK_OK) {
            return st;
        }
    } else if (p->cycle == GK_CANNED_G74) {
        st = push_action(out, GK_CANNED_MOVE_SPINDLE_REVERSE, pt, 0.0, 0.0);
        if (st != GK_OK) {
            return st;
        }
    }

    pt.z = p->r_plane;
    st = push_action(out, GK_CANNED_MOVE_RAPID_TO_R, pt, 0.0, 0.0);
    if (st != GK_OK) {
        return st;
    }

    switch (p->cycle) {
    case GK_CANNED_G73:
        st = emit_peck(out, p, hole_xy, retract_z, 0);
        break;
    case GK_CANNED_G83:
        st = emit_peck(out, p, hole_xy, retract_z, 1);
        break;
    case GK_CANNED_G81:
    case GK_CANNED_G82:
    case GK_CANNED_G84:
    case GK_CANNED_G85:
    case GK_CANNED_G86:
    case GK_CANNED_G87:
    case GK_CANNED_G88:
    case GK_CANNED_G89:
    case GK_CANNED_G74:
    case GK_CANNED_G76:
    default:
        st = emit_peck(out, p, hole_xy, retract_z, 0);
        break;
    }
    if (st != GK_OK) {
        return st;
    }

    /* Retraction behavior differs by cycle. */
    pt.z = retract_z;
    switch (p->cycle) {
    case GK_CANNED_G85:
    case GK_CANNED_G86:
    case GK_CANNED_G87:
    case GK_CANNED_G88:
    case GK_CANNED_G89:
        st = push_action(out, GK_CANNED_MOVE_FEED_OUT, pt, 0.0, p->f_feed);
        break;
    default:
        st = push_action(out, GK_CANNED_MOVE_RAPID_OUT, pt, 0.0, 0.0);
        break;
    }
    return st;
}
