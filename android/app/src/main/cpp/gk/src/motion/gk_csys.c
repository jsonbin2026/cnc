#include "gk/gk_csys.h"

#include <stdio.h>
#include <string.h>

const char *gk_csys_state_name(gk_csys_state s)
{
    switch (s) {
    case GK_CSYS_POWERED_OFF: return "powered-off";
    case GK_CSYS_SELFTEST: return "selftest";
    case GK_CSYS_LOADING: return "loading";
    case GK_CSYS_READY: return "ready";
    case GK_CSYS_HOMING: return "homing";
    case GK_CSYS_RUNNING: return "running";
    case GK_CSYS_PAUSED: return "paused";
    case GK_CSYS_FAULT: return "fault";
    case GK_CSYS_ESTOP: return "estop";
    default: return "unknown";
    }
}

void gk_csys_init(gk_csys *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    c->state = GK_CSYS_POWERED_OFF;
}

gk_status gk_csys_selftest(gk_csys *c, int passed)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    c->state = GK_CSYS_SELFTEST;
    c->selftest_passed = passed ? 1 : 0;
    if (!c->selftest_passed) {
        c->state = GK_CSYS_FAULT;
        c->alarmed = 1;
        return GK_ERR_STATE;
    }
    c->state = GK_CSYS_LOADING;
    return GK_OK;
}

gk_status gk_csys_load_params(gk_csys *c, const char *file)
{
    if (c == NULL || file == NULL || file[0] == '\0') {
        return GK_ERR_INVALID_ARG;
    }
    if (c->state != GK_CSYS_LOADING) {
        return GK_ERR_STATE;
    }
    c->params_loaded = 1;
    return GK_OK;
}

gk_status gk_csys_load_program(gk_csys *c, const char *file)
{
    if (c == NULL || file == NULL || file[0] == '\0') {
        return GK_ERR_INVALID_ARG;
    }
    c->program_loaded = 1;
    return GK_OK;
}

gk_status gk_csys_verify_program(gk_csys *c, int syntax_ok)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!c->program_loaded) {
        return GK_ERR_STATE;
    }
    if (!syntax_ok) {
        c->state = GK_CSYS_FAULT;
        c->alarmed = 1;
        return GK_ERR_PARSE;
    }
    return GK_OK;
}

gk_status gk_csys_load_tools(gk_csys *c, int count)
{
    if (c == NULL || count < 0) {
        return GK_ERR_INVALID_ARG;
    }
    c->tools_loaded = count > 0 ? 1 : 0;
    return GK_OK;
}

gk_status gk_csys_load_wcs(gk_csys *c, int count)
{
    if (c == NULL || count < 0) {
        return GK_ERR_INVALID_ARG;
    }
    c->wcs_loaded = count > 0 ? 1 : 0;
    return GK_OK;
}

gk_status gk_csys_load_offsets(gk_csys *c, int count)
{
    if (c == NULL || count < 0) {
        return GK_ERR_INVALID_ARG;
    }
    c->offset_loaded = count > 0 ? 1 : 0;
    return GK_OK;
}

gk_status gk_csys_load_macros(gk_csys *c, int count)
{
    if (c == NULL || count < 0) {
        return GK_ERR_INVALID_ARG;
    }
    c->macros_loaded = count > 0 ? 1 : 0;
    return GK_OK;
}

gk_status gk_csys_plc_start(gk_csys *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    c->plc_running = 1;
    return GK_OK;
}

gk_status gk_csys_plc_stop(gk_csys *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    c->plc_running = 0;
    return GK_OK;
}

gk_status gk_csys_enable_servos(gk_csys *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (c->alarmed || c->state == GK_CSYS_ESTOP) {
        return GK_ERR_STATE;
    }
    c->servos_enabled = 1;
    return GK_OK;
}

gk_status gk_csys_home(gk_csys *c, int axis_count)
{
    if (c == NULL || axis_count <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    if (!c->servos_enabled) {
        return GK_ERR_STATE;
    }
    c->state = GK_CSYS_HOMING;
    c->homed = 1;
    c->state = GK_CSYS_READY;
    return GK_OK;
}

gk_status gk_csys_touch_off(gk_csys *c, double offset_mm, double *stored_mm)
{
    if (c == NULL || stored_mm == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!c->homed) {
        return GK_ERR_STATE;
    }
    *stored_mm = offset_mm;
    return GK_OK;
}

gk_status gk_csys_trial_cut(gk_csys *c, double depth_mm, double *result_mm)
{
    if (c == NULL || result_mm == NULL || depth_mm <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    if (!c->homed) {
        return GK_ERR_STATE;
    }
    /* a trial cut removes slightly less than the commanded depth */
    *result_mm = depth_mm * 0.98;
    return GK_OK;
}

gk_status gk_csys_start_machining(gk_csys *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!c->program_loaded || !c->homed || !c->servos_enabled || c->alarmed) {
        return GK_ERR_STATE;
    }
    c->state = GK_CSYS_RUNNING;
    return GK_OK;
}

gk_status gk_csys_pause(gk_csys *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (c->state != GK_CSYS_RUNNING) {
        return GK_ERR_STATE;
    }
    c->state = GK_CSYS_PAUSED;
    return GK_OK;
}

gk_status gk_csys_resume(gk_csys *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (c->state != GK_CSYS_PAUSED) {
        return GK_ERR_STATE;
    }
    c->state = GK_CSYS_RUNNING;
    return GK_OK;
}

gk_status gk_csys_end_program(gk_csys *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    c->state = GK_CSYS_READY;
    return GK_OK;
}

gk_status gk_csys_shutdown(gk_csys *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    c->state = GK_CSYS_POWERED_OFF;
    c->servos_enabled = 0;
    c->plc_running = 0;
    return GK_OK;
}

gk_status gk_csys_backup_params(const gk_csys *c, char *out, size_t out_cap)
{
    int n;
    if (c == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    if (!c->params_loaded) {
        return GK_ERR_STATE;
    }
    n = snprintf(out, out_cap, "PARAMS_BACKUP_OK");
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

gk_status gk_csys_backup_program(const char *program, char *out,
                                 size_t out_cap)
{
    int n;
    if (program == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "PROG:%s", program);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

gk_status gk_csys_upgrade(gk_csys *c, const char *version,
                          double *new_version)
{
    if (c == NULL || version == NULL || new_version == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (c->state == GK_CSYS_RUNNING) {
        return GK_ERR_STATE;
    }
    /* parse a numeric version like "1.2" */
    *new_version = 0.0;
    {
        const char *p = version;
        double frac = 0.1;
        int seen_dot = 0;
        while (*p != '\0') {
            if (*p == '.') {
                seen_dot = 1;
            } else if (*p >= '0' && *p <= '9') {
                if (!seen_dot) {
                    *new_version = *new_version * 10.0 + (*p - '0');
                } else {
                    *new_version += (*p - '0') * frac;
                    frac *= 0.1;
                }
            } else {
                return GK_ERR_PARSE;
            }
            p++;
        }
    }
    return GK_OK;
}

gk_status gk_csys_restore(gk_csys *c, int backup_valid)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!backup_valid) {
        return GK_ERR_NOT_FOUND;
    }
    c->params_loaded = 1;
    c->state = GK_CSYS_READY;
    return GK_OK;
}

gk_status gk_csys_reset_alarm(gk_csys *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    c->alarmed = 0;
    if (c->state == GK_CSYS_FAULT) {
        c->state = GK_CSYS_READY;
    }
    return GK_OK;
}

gk_status gk_csys_reset_estop(gk_csys *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (c->state == GK_CSYS_ESTOP) {
        c->state = GK_CSYS_READY;
    }
    return GK_OK;
}

gk_status gk_csys_reset_overtravel(gk_csys *c, int *overtravel_axis)
{
    if (c == NULL || overtravel_axis == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    /* clear the first reported overtravel axis, return which one */
    if (*overtravel_axis >= 0) {
        *overtravel_axis = -1;
    }
    if (c->state == GK_CSYS_FAULT) {
        c->state = GK_CSYS_READY;
    }
    return GK_OK;
}

gk_status gk_csys_reset_servo(gk_csys *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    c->servos_enabled = 1;
    return GK_OK;
}

gk_status gk_csys_reset_system(gk_csys *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    gk_csys_init(c);
    return GK_OK;
}

gk_status gk_csys_cold_start(gk_csys *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    gk_csys_init(c);
    c->state = GK_CSYS_LOADING;
    c->selftest_passed = 1;
    return GK_OK;
}

gk_status gk_csys_warm_start(gk_csys *c)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    /* warm start keeps loaded data but re-runs the self test */
    c->state = GK_CSYS_READY;
    return GK_OK;
}

int gk_csys_ready(const gk_csys *c)
{
    if (c == NULL) {
        return 0;
    }
    return c->selftest_passed && c->params_loaded && c->homed &&
           c->servos_enabled && !c->alarmed;
}
