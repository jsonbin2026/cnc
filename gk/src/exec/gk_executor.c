#include "gk/gk_executor.h"
#include "gk/gk_move.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const char *const k_exec_state_names[] = {
    "IDLE", "RUNNING", "PAUSED", "STOPPED", "FINISHED", "ERROR", "ESTOP",
};

const char *gk_exec_state_name(gk_exec_state st)
{
    if ((int)st < 0 || (int)st > (int)GK_EXEC_ESTOP) {
        return "UNKNOWN";
    }
    return k_exec_state_names[(int)st];
}

gk_status gk_executor_init(gk_executor *ex, const gk_allocator *alloc)
{
    if (ex == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    memset(ex, 0, sizeof(*ex));
    ex->alloc = alloc != NULL ? *alloc : gk_allocator_default();
    gk_state_init(&ex->state);
    gk_motion_config_default(&ex->config);
    gk_transform_init(&ex->transform);
    gk_tool_table_init(&ex->tools);
    ex->run_state = GK_EXEC_IDLE;
    ex->error = GK_OK;
    return GK_OK;
}

void gk_executor_destroy(gk_executor *ex)
{
    if (ex == NULL) {
        return;
    }
    gk_tool_table_free(&ex->tools);
    memset(ex, 0, sizeof(*ex));
}

static void set_error(gk_executor *ex, gk_status st, const char *fmt, ...)
{
    va_list args;
    ex->error = st;
    ex->run_state = GK_EXEC_ERROR;
    va_start(args, fmt);
    vsnprintf(ex->error_message, sizeof(ex->error_message), fmt, args);
    va_end(args);
}

gk_status gk_executor_load(gk_executor *ex, const gk_program *program)
{
    if (ex == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    ex->program = program;
    return gk_executor_reset(ex);
}

gk_status gk_executor_reset(gk_executor *ex)
{
    if (ex == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    gk_state_reset(&ex->state);
    gk_transform_init(&ex->transform);
    ex->cutter_comp = GK_COMP_CANCEL;
    ex->length_comp = GK_LEN_COMP_CANCEL;
    ex->comp_register = 0;
    ex->canned = GK_CANNED_NONE;
    ex->canned_pending = 0;
    ex->last_tool = 0;
    ex->tool_warning = 0;
    ex->cursor = 0;
    ex->program_end = 0;
    ex->run_state = GK_EXEC_IDLE;
    ex->single_block = 0;
    ex->optional_stop = 0;
    ex->sub_call_depth = 0;
    ex->call_stack_top = 0;
    ex->blocks_executed = 0;
    ex->move_count = 0;
    ex->elapsed_time = 0.0;
    ex->has_last_move = 0;
    ex->error = GK_OK;
    ex->error_message[0] = '\0';
    return GK_OK;
}

void gk_executor_start(gk_executor *ex)
{
    if (ex != NULL && ex->run_state != GK_EXEC_ERROR) {
        ex->run_state = GK_EXEC_RUNNING;
    }
}

void gk_executor_halt(gk_executor *ex)
{
    if (ex != NULL && (ex->run_state == GK_EXEC_RUNNING ||
                       ex->run_state == GK_EXEC_PAUSED)) {
        ex->run_state = GK_EXEC_STOPPED;
    }
}

void gk_executor_estop(gk_executor *ex)
{
    if (ex != NULL) {
        ex->run_state = GK_EXEC_ESTOP;
        ex->state.spindle_on = 0;
        ex->state.spindle_dir = 0;
        ex->state.coolant = 0;
    }
}

void gk_executor_resume(gk_executor *ex)
{
    if (ex != NULL && (ex->run_state == GK_EXEC_PAUSED ||
                       ex->run_state == GK_EXEC_ESTOP)) {
        ex->run_state = GK_EXEC_RUNNING;
    }
}

void gk_executor_set_single_block(gk_executor *ex, int enabled)
{
    if (ex != NULL) {
        ex->single_block = enabled ? 1 : 0;
    }
}

gk_status gk_executor_add_breakpoint(gk_executor *ex, size_t block_index)
{
    if (ex == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (gk_executor_has_breakpoint(ex, block_index)) {
        return GK_ERR_ALREADY_EXISTS;
    }
    if (ex->breakpoint_count >= 64) {
        return GK_ERR_OUT_OF_RANGE;
    }
    ex->breakpoints[ex->breakpoint_count].block_index = block_index;
    ex->breakpoints[ex->breakpoint_count].has_breakpoint = 1;
    ex->breakpoint_count += 1;
    return GK_OK;
}

gk_status gk_executor_remove_breakpoint(gk_executor *ex, size_t block_index)
{
    size_t i;
    if (ex == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < ex->breakpoint_count; ++i) {
        if (ex->breakpoints[i].block_index == block_index) {
            memmove(&ex->breakpoints[i], &ex->breakpoints[i + 1],
                    (ex->breakpoint_count - i - 1) * sizeof(gk_breakpoint));
            ex->breakpoint_count -= 1;
            return GK_OK;
        }
    }
    return GK_ERR_NOT_FOUND;
}

int gk_executor_has_breakpoint(const gk_executor *ex, size_t block_index)
{
    size_t i;
    if (ex == NULL) {
        return 0;
    }
    for (i = 0; i < ex->breakpoint_count; ++i) {
        if (ex->breakpoints[i].block_index == block_index) {
            return 1;
        }
    }
    return 0;
}

void gk_executor_clear_breakpoints(gk_executor *ex)
{
    if (ex != NULL) {
        ex->breakpoint_count = 0;
    }
}

/* Find a word by letter in a block. */
static const gk_word *find_word(const gk_block *b, char letter)
{
    size_t i;
    if (b == NULL) {
        return NULL;
    }
    for (i = 0; i < b->word_count; ++i) {
        if (b->words[i].letter == letter) {
            return &b->words[i];
        }
    }
    return NULL;
}

static gk_axis axis_from_letter(char letter)
{
    switch (letter) {
    case 'X': return GK_AXIS_X;
    case 'Y': return GK_AXIS_Y;
    case 'Z': return GK_AXIS_Z;
    case 'A': return GK_AXIS_A;
    case 'B': return GK_AXIS_B;
    case 'C': return GK_AXIS_C;
    default: return GK_AXIS_COUNT;
    }
}

/* Apply modal G codes and collect endpoints. Returns 1 if a motion was
 * requested, 0 if pure modal/non-motion block. */
static int apply_block(gk_executor *ex, const gk_block *b,
                       gk_point3 *target, int *has_motion)
{
    size_t i;
    gk_motion_mode motion = GK_MOTION_NONE;
    int ijk_present = 0;
    double i_val = 0.0;
    double j_val = 0.0;
    double k_val = 0.0;
    double r_val = 0.0;
    int has_r = 0;
    int canned_requested = 0;
    gk_point3 program_pos;
    gk_point3 machine_pos;
    gk_machine_state *s = &ex->state;

    *has_motion = 0;

    /* First pass: modal codes */
    for (i = 0; i < b->word_count; ++i) {
        const gk_word *w = &b->words[i];
        if (!w->is_gcode) {
            continue;
        }
        switch (w->ivalue) {
        case 0: motion = GK_MOTION_RAPID; break;
        case 1: motion = GK_MOTION_LINEAR; break;
        case 2: motion = GK_MOTION_CW; break;
        case 3: motion = GK_MOTION_CCW; break;
        case 17: gk_state_select_plane(s, GK_PLANE_XY); break;
        case 18: gk_state_select_plane(s, GK_PLANE_ZX); break;
        case 19: gk_state_select_plane(s, GK_PLANE_YZ); break;
        case 20: gk_state_set_unit(s, GK_UNIT_INCH); break;
        case 21: gk_state_set_unit(s, GK_UNIT_MM); break;
        case 40: ex->cutter_comp = GK_COMP_CANCEL; break;
        case 41: {
            const gk_word *dw = find_word(b, 'D');
            ex->cutter_comp = GK_COMP_LEFT;
            if (dw != NULL) {
                ex->comp_register = dw->ivalue;
            }
            break;
        }
        case 42: {
            const gk_word *dw = find_word(b, 'D');
            ex->cutter_comp = GK_COMP_RIGHT;
            if (dw != NULL) {
                ex->comp_register = dw->ivalue;
            }
            break;
        }
        case 43: {
            const gk_word *hw = find_word(b, 'H');
            ex->length_comp = GK_LEN_COMP_POS;
            if (hw != NULL) {
                ex->comp_register = hw->ivalue;
            }
            break;
        }
        case 44: {
            const gk_word *hw = find_word(b, 'H');
            ex->length_comp = GK_LEN_COMP_NEG;
            if (hw != NULL) {
                ex->comp_register = hw->ivalue;
            }
            break;
        }
        case 49: ex->length_comp = GK_LEN_COMP_CANCEL; break;
        case 69: gk_transform_cancel_rotation(&ex->transform); break;
        case 80: ex->canned = GK_CANNED_NONE; break;
        case 98: ex->canned_params.retract = GK_RETRACT_INITIAL; break;
        case 99: ex->canned_params.retract = GK_RETRACT_R_PLANE; break;
        case 50: {
            const gk_word *sv = find_word(b, 'S');
            if (sv != NULL) {
                s->spindle_max_rpm = sv->value;
            }
            break;
        }
        case 51: {
            const gk_word *pv = find_word(b, 'P');
            double scale = pv != NULL ? pv->value : 1.0;
            if (scale > GK_EPS) {
                gk_transform_set_scale(&ex->transform, scale);
            }
            break;
        }
        case 52: {
            gk_point3 off;
            const gk_word *x = find_word(b, 'X');
            const gk_word *y = find_word(b, 'Y');
            const gk_word *z = find_word(b, 'Z');
            off = gk_vec3_make(x != NULL ? x->value : 0.0,
                               y != NULL ? y->value : 0.0,
                               z != NULL ? z->value : 0.0);
            gk_transform_set_local(&ex->transform, off);
            break;
        }
        case 53: break; /* G53 machine coords: handled at motion */
        case 54: case 55: case 56: case 57: case 58: case 59:
            gk_state_set_work_offset(s, w->ivalue);
            break;
        case 68: {
            const gk_word *rv = find_word(b, 'R');
            gk_point3 center = gk_vec3_make(0.0, 0.0, 0.0);
            const gk_word *x = find_word(b, 'X');
            const gk_word *y = find_word(b, 'Y');
            if (x != NULL) center.x = x->value;
            if (y != NULL) center.y = y->value;
            gk_transform_set_rotation(&ex->transform,
                                      rv != NULL ? rv->value : 0.0, center);
            break;
        }
        case 90: gk_state_set_distance(s, GK_DIST_ABSOLUTE); break;
        case 91: gk_state_set_distance(s, GK_DIST_INCREMENTAL); break;
        case 92: {
            gk_point3 off;
            const gk_word *x = find_word(b, 'X');
            const gk_word *y = find_word(b, 'Y');
            const gk_word *z = find_word(b, 'Z');
            off = gk_vec3_make(x != NULL ? x->value : 0.0,
                               y != NULL ? y->value : 0.0,
                               z != NULL ? z->value : 0.0);
            gk_transform_set_g92(&ex->transform, off);
            break;
        }
        case 94: gk_state_set_feed_mode(s, GK_FEED_PER_MIN); break;
        case 95: gk_state_set_feed_mode(s, GK_FEED_PER_REV); break;
        case 96: gk_state_set_spindle_mode(s, GK_SPEED_CONST_SURFACE); break;
        case 97: gk_state_set_spindle_mode(s, GK_SPEED_CONST_RPM); break;
        default:
            if (gk_canned_is_cycle(w->ivalue)) {
                ex->canned = (gk_canned_cycle)w->ivalue;
                canned_requested = 1;
            }
            break;
        }
    }

    if (canned_requested || ex->canned != GK_CANNED_NONE) {
        const gk_word *xw = find_word(b, 'X');
        const gk_word *yw = find_word(b, 'Y');
        const gk_word *zw = find_word(b, 'Z');
        const gk_word *rw = find_word(b, 'R');
        const gk_word *qw = find_word(b, 'Q');
        const gk_word *pw = find_word(b, 'P');
        const gk_word *fw = find_word(b, 'F');
        gk_point3 hole;
        double prior_z = s->coord[GK_AXIS_Z];

        if (canned_requested && xw == NULL && yw == NULL &&
            zw == NULL && rw == NULL) {
            /* Cantilever: canned code with no data cancels nothing. */
        }

        hole = gk_vec3_make(s->coord[GK_AXIS_X], s->coord[GK_AXIS_Y], prior_z);
        if (xw != NULL) {
            hole.x = s->distance == GK_DIST_INCREMENTAL
                         ? s->coord[GK_AXIS_X] + xw->value : xw->value;
        }
        if (yw != NULL) {
            hole.y = s->distance == GK_DIST_INCREMENTAL
                         ? s->coord[GK_AXIS_Y] + yw->value : yw->value;
        }
        if (xw != NULL || yw != NULL) {
            ex->canned_pending = 1;
        }

        if (zw != NULL) {
            ex->canned_params.z_depth =
                s->distance == GK_DIST_INCREMENTAL ? prior_z - zw->value
                                                    : zw->value;
        }
        if (rw != NULL) {
            ex->canned_params.r_plane =
                s->distance == GK_DIST_INCREMENTAL ? prior_z + rw->value
                                                    : rw->value;
        }
        if (qw != NULL) ex->canned_params.q_peck = qw->value;
        if (pw != NULL) ex->canned_params.p_dwell = pw->value;
        if (fw != NULL) { ex->canned_params.f_feed = fw->value; s->feed = fw->value; }
        ex->canned_params.cycle = ex->canned;
        ex->canned_params.hole = hole;
        if (ex->canned == GK_CANNED_G74) {
            ex->canned_params.spindle_dir = -1;
        } else if (ex->canned == GK_CANNED_G84) {
            ex->canned_params.spindle_dir = 1;
        }

        /* Update XY and the current position. */
        s->coord[GK_AXIS_X] = hole.x;
        s->coord[GK_AXIS_Y] = hole.y;
        ex->canned_pending = 1;
        *has_motion = 1;
        return 1;
    }

    if (motion == GK_MOTION_NONE) {
        /* Pure modal block: still apply F/S/T and M below. */
        const gk_word *fw = find_word(b, 'F');
        const gk_word *sw = find_word(b, 'S');
        const gk_word *tw = find_word(b, 'T');
        if (fw != NULL) s->feed = fw->value;
        if (sw != NULL) s->spindle_rpm = sw->value;
        if (tw != NULL) s->tool = tw->ivalue;
        return 0;
    }

    /* Motion block: compute target in program coordinates. */
    program_pos = gk_vec3_make(s->coord[GK_AXIS_X], s->coord[GK_AXIS_Y],
                               s->coord[GK_AXIS_Z]);
    target->x = program_pos.x;
    target->y = program_pos.y;
    target->z = program_pos.z;

    for (i = 0; i < b->word_count; ++i) {
        const gk_word *w = &b->words[i];
        gk_axis ax;
        if (w->is_gcode || w->is_mcode || w->is_line_no || !w->has_value) {
            continue;
        }
        ax = axis_from_letter(w->letter);
        if (ax != GK_AXIS_COUNT) {
            double cur = s->coord[(int)ax];
            double val = s->distance == GK_DIST_INCREMENTAL
                             ? cur + w->value
                             : w->value;
            switch (ax) {
            case GK_AXIS_X: target->x = val; break;
            case GK_AXIS_Y: target->y = val; break;
            case GK_AXIS_Z: target->z = val; break;
            default: s->coord[(int)ax] = val; break;
            }
        } else if (w->letter == 'I') {
            i_val = w->value;
            ijk_present = 1;
        } else if (w->letter == 'J') {
            j_val = w->value;
            ijk_present = 1;
        } else if (w->letter == 'K') {
            k_val = w->value;
            ijk_present = 1;
        } else if (w->letter == 'R') {
            r_val = w->value;
            has_r = 1;
        } else if (w->letter == 'F') {
            s->feed = w->value;
        } else if (w->letter == 'S') {
            s->spindle_rpm = w->value;
        } else if (w->letter == 'T') {
            s->tool = w->ivalue;
        }
    }

    /* Apply tool compensation in program coordinates. */
    if (ex->length_comp != GK_LEN_COMP_CANCEL && ex->comp_register > 0) {
        gk_tool *tool = gk_tool_table_get(&ex->tools, ex->comp_register);
        if (tool != NULL) {
            target->z = gk_len_comp_apply(ex->length_comp, tool->length,
                                          target->z);
        }
    }
    if (ex->cutter_comp != GK_COMP_CANCEL && ex->comp_register > 0) {
        gk_tool *tool = gk_tool_table_get(&ex->tools, ex->comp_register);
        if (tool != NULL) {
            *target = gk_comp_apply(ex->cutter_comp,
                                    tool->diameter * 0.5 + tool->wear_radius,
                                    program_pos, program_pos, *target);
        }
    }

    /* Transform program coords to machine coords. */
    machine_pos = gk_transform_to_machine(&ex->transform, s, program_pos);
    {
        gk_point3 machine_target =
            gk_transform_to_machine(&ex->transform, s, *target);

        if (motion == GK_MOTION_CW || motion == GK_MOTION_CCW) {
            if (has_r) {
                gk_status st = gk_move_arc_radius(&ex->last_move, machine_pos,
                                                  machine_target, r_val,
                                                  s->feed, motion, s->plane);
                if (st != GK_OK) {
                    set_error(ex, st, "invalid arc (R form) at block %zu",
                              ex->cursor);
                    return -1;
                }
            } else if (ijk_present) {
                gk_point3 center = machine_pos;
                switch (s->plane) {
                case GK_PLANE_XY:
                    center.x = machine_pos.x + i_val;
                    center.y = machine_pos.y + j_val;
                    center.z = machine_pos.z + k_val;
                    break;
                case GK_PLANE_ZX:
                    center.z = machine_pos.z + i_val;
                    center.x = machine_pos.x + k_val;
                    center.y = machine_pos.y + j_val;
                    break;
                case GK_PLANE_YZ:
                default:
                    center.y = machine_pos.y + j_val;
                    center.z = machine_pos.z + k_val;
                    center.x = machine_pos.x + i_val;
                    break;
                }
                {
                    gk_status st = gk_move_arc_ijk(
                        &ex->last_move, machine_pos, machine_target, center,
                        s->feed, motion, s->plane);
                    if (st != GK_OK) {
                        set_error(ex, st, "invalid arc (I/J/K) at block %zu",
                                  ex->cursor);
                        return -1;
                    }
                }
            } else {
                set_error(ex, GK_ERR_PARSE,
                          "arc without R or I/J/K at block %zu", ex->cursor);
                return -1;
            }
        } else {
            gk_move_linear(&ex->last_move, machine_pos, machine_target,
                           s->feed, motion);
        }
    }

    s->motion = motion;
    s->last_motion = motion;
    s->coord[GK_AXIS_X] = target->x;
    s->coord[GK_AXIS_Y] = target->y;
    s->coord[GK_AXIS_Z] = target->z;
    ex->has_last_move = 1;
    ex->move_count += 1;
    *has_motion = 1;
    return 1;
}

/* Handle M codes that affect control flow. Returns:
 *  0 = continue, 1 = program end, 2 = pause, -1 = error. */
static int apply_mcodes(gk_executor *ex, const gk_block *b)
{
    size_t i;
    gk_machine_state *s = &ex->state;
    for (i = 0; i < b->word_count; ++i) {
        const gk_word *w = &b->words[i];
        if (!w->is_mcode) {
            continue;
        }
        switch (w->ivalue) {
        case 0: /* M00 program stop */
            ex->run_state = GK_EXEC_PAUSED;
            return 2;
        case 1: /* M01 optional stop */
            if (ex->optional_stop) {
                ex->run_state = GK_EXEC_PAUSED;
                return 2;
            }
            break;
        case 2: /* M02 program end */
            ex->program_end = 1;
            return 1;
        case 3: s->spindle_on = 1; s->spindle_dir = 1; break;
        case 4: s->spindle_on = 1; s->spindle_dir = -1; break;
        case 5: s->spindle_on = 0; s->spindle_dir = 0; break;
        case 6: {
            /* Automatic tool change: tool number was set by T. */
            ex->last_tool = s->tool;
            if (gk_tool_table_get(&ex->tools, s->tool) == NULL &&
                gk_tool_table_count(&ex->tools) > 0) {
                set_error(ex, GK_ERR_NOT_FOUND,
                          "tool T%02d not in tool table", s->tool);
                return -1;
            }
            break;
        }
        case 7: case 8: s->coolant = 1; break;
        case 9: s->coolant = 0; break;
        case 10: break;
        case 11: break;
        case 13: s->spindle_on = 1; s->spindle_dir = 1; s->coolant = 1; break;
        case 14: s->spindle_on = 1; s->spindle_dir = -1; s->coolant = 1; break;
        case 19: s->spindle_on = 0; s->spindle_dir = 0; break;
        case 29: break;
        case 30: /* M30 program end + reset */
            ex->program_end = 1;
            return 1;
        case 98: /* M98 subprogram call */
            if (ex->call_stack_top >= 16) {
                set_error(ex, GK_ERR_STATE, "subprogram nesting too deep");
                return -1;
            }
            break;
        case 99: /* M99 subprogram return */
            break;
        default: break;
        }
    }
    return 0;
}

gk_status gk_executor_step(gk_executor *ex)
{
    const gk_block *b;
    gk_point3 target;
    int has_motion;
    int mres;

    if (ex == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (ex->program == NULL) {
        return GK_ERR_STATE;
    }
    if (ex->run_state == GK_EXEC_ESTOP || ex->run_state == GK_EXEC_ERROR) {
        return GK_ERR_STATE;
    }
    if (ex->program_end || ex->cursor >= gk_program_block_count(ex->program)) {
        ex->program_end = 1;
        ex->run_state = GK_EXEC_FINISHED;
        return GK_OK;
    }

    ex->run_state = GK_EXEC_RUNNING;
    b = gk_program_block(ex->program, ex->cursor);
    if (b == NULL) {
        ex->program_end = 1;
        ex->run_state = GK_EXEC_FINISHED;
        return GK_OK;
    }

    if (apply_block(ex, b, &target, &has_motion) < 0) {
        return ex->error;
    }
    if (ex->canned_pending && ex->canned != GK_CANNED_NONE) {
        gk_canned_plan cplan;
        gk_status cst = gk_canned_expand(&ex->canned_params,
                                         ex->canned_params.hole,
                                         ex->state.coord[GK_AXIS_Z], &cplan);
        if (cst != GK_OK) {
            set_error(ex, cst, "invalid canned cycle at block %zu",
                      ex->cursor);
            return ex->error;
        }
        {
            size_t ai;
            double total_len = 0.0;
            for (ai = 0; ai < cplan.action_count; ++ai) {
                if (cplan.actions[ai].kind == GK_CANNED_MOVE_FEED_PLUNGE) {
                    total_len += fabs(cplan.params.r_plane -
                                      cplan.params.z_depth);
                }
            }
            if (ex->canned_params.f_feed > 0.0) {
                ex->elapsed_time += total_len / ex->canned_params.f_feed * 60.0;
            }
        }
        ex->state.coord[GK_AXIS_Z] = ex->canned_params.z_depth;
        ex->move_count += cplan.action_count;
        gk_canned_plan_free(&cplan);
        ex->canned_pending = 0;
    }
    mres = apply_mcodes(ex, b);
    if (mres < 0) {
        return ex->error;
    }

    if (has_motion && ex->last_move.mode != GK_MOTION_NONE) {
        gk_motion_plan plan;
        if (gk_plan_move(&ex->config, &ex->last_move, &plan) == GK_OK) {
            ex->elapsed_time += plan.duration;
            if (ex->last_tool > 0 && plan.duration > 0.0) {
                gk_tool_consume_time(&ex->tools, ex->last_tool,
                                     plan.duration / 60.0);
            }
        }
    }
    {
        gk_tool *active = gk_tool_table_get(&ex->tools, ex->last_tool);
        ex->tool_warning = (active != NULL && gk_tool_needs_warning(active))
                               ? 1 : 0;
    }

    ex->blocks_executed += 1;
    ex->cursor += 1;

    if (mres == 1) {
        /* M02/M30 */
        ex->run_state = GK_EXEC_FINISHED;
        ex->program_end = 1;
    } else if (mres == 2 || ex->single_block) {
        if (ex->run_state != GK_EXEC_FINISHED) {
            ex->run_state = GK_EXEC_PAUSED;
        }
    } else if (ex->cursor >= gk_program_block_count(ex->program)) {
        ex->program_end = 1;
        ex->run_state = GK_EXEC_FINISHED;
    }
    return GK_OK;
}

gk_status gk_executor_run(gk_executor *ex, size_t max_blocks)
{
    size_t n = 0;
    if (ex == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    while (ex->run_state == GK_EXEC_RUNNING ||
           ex->run_state == GK_EXEC_IDLE) {
        if (n >= max_blocks) {
            break;
        }
        if (gk_executor_has_breakpoint(ex, ex->cursor) &&
            ex->blocks_executed > 0) {
            ex->run_state = GK_EXEC_PAUSED;
            break;
        }
        if (gk_executor_step(ex) != GK_OK) {
            return ex->error;
        }
        n += 1;
        if (ex->run_state == GK_EXEC_PAUSED ||
            ex->run_state == GK_EXEC_FINISHED ||
            ex->run_state == GK_EXEC_ERROR ||
            ex->run_state == GK_EXEC_ESTOP) {
            break;
        }
    }
    return GK_OK;
}
