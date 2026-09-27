#include "gk/gk_fscen.h"

#include <math.h>
#include <string.h>

static void gk__fscen_copy(char *dst, size_t cap, const char *src)
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

const char *gk_fscen_name(gk_fscen_kind k)
{
    switch (k) {
    case GK_FSCEN_POWER_LOSS: return "power-loss";
    case GK_FSCEN_VOLTAGE_FLUX: return "voltage-fluctuation";
    case GK_FSCEN_AIR_DROP: return "air-pressure-drop";
    case GK_FSCEN_HYD_LEAK: return "hydraulic-leak";
    case GK_FSCEN_COOLANT_LEAK: return "coolant-leak";
    case GK_FSCEN_LUBE_LOW: return "lubrication-low";
    case GK_FSCEN_BELT_BREAK: return "belt-break";
    case GK_FSCEN_COUPLING_LOOSE: return "coupling-loose";
    case GK_FSCEN_SCREW_JAM: return "screw-jam";
    case GK_FSCEN_GUIDE_DAMAGE: return "guide-damage";
    case GK_FSCEN_BEARING_DAMAGE: return "bearing-damage";
    case GK_FSCEN_MOTOR_OVERHEAT: return "motor-overheat";
    case GK_FSCEN_DRIVE_FAULT: return "drive-fault";
    case GK_FSCEN_ENCODER_FAULT: return "encoder-fault";
    case GK_FSCEN_LIMIT_FAULT: return "limit-fault";
    case GK_FSCEN_ESTOP_MISTOUCH: return "estop-mistouch";
    case GK_FSCEN_PROG_DELETED: return "program-deleted";
    case GK_FSCEN_PARAM_CHANGED: return "parameter-changed";
    case GK_FSCEN_TOOL_REVERSED: return "tool-reversed";
    case GK_FSCEN_PART_REVERSED: return "part-reversed";
    case GK_FSCEN_FIXTURE_LOOSE: return "fixture-loose";
    case GK_FSCEN_TOOLSET_ERROR: return "tool-setting-error";
    case GK_FSCEN_COORD_ERROR: return "coordinate-error";
    case GK_FSCEN_COMP_ERROR: return "compensation-error";
    case GK_FSCEN_PROG_ERROR: return "program-error";
    case GK_FSCEN_CRASH: return "crash";
    case GK_FSCEN_TOOL_BREAK: return "tool-break";
    case GK_FSCEN_TOOL_BURN: return "tool-burn";
    case GK_FSCEN_EDGE_CHIP: return "edge-chipping";
    case GK_FSCEN_PART_FLY: return "part-fly";
    case GK_FSCEN_FIRE: return "fire";
    case GK_FSCEN_LEAKAGE: return "electric-leakage";
    case GK_FSCEN_INJURY: return "personnel-injury";
    default: return "unknown";
    }
}

int gk_fscen_severity(gk_fscen_kind k)
{
    switch (k) {
    case GK_FSCEN_INJURY:
    case GK_FSCEN_FIRE:
    case GK_FSCEN_LEAKAGE:
    case GK_FSCEN_PART_FLY:
    case GK_FSCEN_CRASH:
        return 5;
    case GK_FSCEN_TOOL_BREAK:
    case GK_FSCEN_TOOL_BURN:
    case GK_FSCEN_EDGE_CHIP:
    case GK_FSCEN_DRIVE_FAULT:
    case GK_FSCEN_SCREW_JAM:
    case GK_FSCEN_BEARING_DAMAGE:
        return 4;
    case GK_FSCEN_HYD_LEAK:
    case GK_FSCEN_COOLANT_LEAK:
    case GK_FSCEN_MOTOR_OVERHEAT:
    case GK_FSCEN_ENCODER_FAULT:
    case GK_FSCEN_BELT_BREAK:
    case GK_FSCEN_GUIDE_DAMAGE:
    case GK_FSCEN_TOOLSET_ERROR:
    case GK_FSCEN_COORD_ERROR:
    case GK_FSCEN_COMP_ERROR:
    case GK_FSCEN_PROG_ERROR:
    case GK_FSCEN_TOOL_REVERSED:
    case GK_FSCEN_PART_REVERSED:
    case GK_FSCEN_FIXTURE_LOOSE:
        return 3;
    case GK_FSCEN_POWER_LOSS:
    case GK_FSCEN_VOLTAGE_FLUX:
    case GK_FSCEN_AIR_DROP:
    case GK_FSCEN_LUBE_LOW:
    case GK_FSCEN_COUPLING_LOOSE:
    case GK_FSCEN_LIMIT_FAULT:
    case GK_FSCEN_PROG_DELETED:
    case GK_FSCEN_PARAM_CHANGED:
        return 2;
    case GK_FSCEN_ESTOP_MISTOUCH:
        return 1;
    default:
        return 1;
    }
}

int gk_fscen_is_safety(gk_fscen_kind k)
{
    switch (k) {
    case GK_FSCEN_INJURY:
    case GK_FSCEN_FIRE:
    case GK_FSCEN_LEAKAGE:
    case GK_FSCEN_PART_FLY:
    case GK_FSCEN_CRASH:
    case GK_FSCEN_ESTOP_MISTOUCH:
        return 1;
    default:
        return 0;
    }
}

gk_status gk_fscen_trigger(gk_fscen_event *e, gk_fscen_kind k)
{
    if (e == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    memset(e, 0, sizeof(*e));
    e->kind = k;
    /* higher severity tends to be rarer */
    e->probability = 0.5 / (double)gk_fscen_severity(k);
    gk__fscen_copy(e->description, sizeof(e->description), gk_fscen_name(k));
    return GK_OK;
}

gk_status gk_fscen_detect(gk_fscen_event *e, int detected)
{
    if (e == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    e->detected = detected ? 1 : 0;
    return GK_OK;
}

gk_status gk_fscen_respond(gk_fscen_event *e, int stopped)
{
    if (e == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!e->detected) {
        return GK_ERR_STATE;
    }
    e->stopped = stopped ? 1 : 0;
    return GK_OK;
}

int gk_fscen_handled(const gk_fscen_event *e)
{
    if (e == NULL) {
        return 0;
    }
    return e->detected && e->stopped;
}

const char *gk_fscen_recovery(gk_fscen_kind k)
{
    switch (k) {
    case GK_FSCEN_POWER_LOSS: return "check power, restore, re-home";
    case GK_FSCEN_VOLTAGE_FLUX: return "stabilize supply, verify";
    case GK_FSCEN_AIR_DROP: return "restore air pressure";
    case GK_FSCEN_HYD_LEAK: return "stop, repair seal, refill";
    case GK_FSCEN_COOLANT_LEAK: return "repair line, refill coolant";
    case GK_FSCEN_LUBE_LOW: return "refill lubricant";
    case GK_FSCEN_BELT_BREAK: return "replace belt, realign";
    case GK_FSCEN_COUPLING_LOOSE: return "tighten coupling, dialectic";
    case GK_FSCEN_SCREW_JAM: return "free screw, inspect nut";
    case GK_FSCEN_GUIDE_DAMAGE: return "inspect guide, replace";
    case GK_FSCEN_BEARING_DAMAGE: return "replace bearing";
    case GK_FSCEN_MOTOR_OVERHEAT: return "cool down, check load";
    case GK_FSCEN_DRIVE_FAULT: return "reset drive, check wiring";
    case GK_FSCEN_ENCODER_FAULT: return "check encoder cable, replace";
    case GK_FSCEN_LIMIT_FAULT: return "replace limit switch";
    case GK_FSCEN_ESTOP_MISTOUCH: return "reset estop, verify safe";
    case GK_FSCEN_PROG_DELETED: return "restore from backup";
    case GK_FSCEN_PARAM_CHANGED: return "restore parameters";
    case GK_FSCEN_TOOL_REVERSED: return "reverse tool, re-set";
    case GK_FSCEN_PART_REVERSED: return "reverse part, re-clamp";
    case GK_FSCEN_FIXTURE_LOOSE: return "re-clamp fixture";
    case GK_FSCEN_TOOLSET_ERROR: return "re-measure offsets";
    case GK_FSCEN_COORD_ERROR: return "re-set work coordinate";
    case GK_FSCEN_COMP_ERROR: return "fix tool compensation";
    case GK_FSCEN_PROG_ERROR: return "debug and verify program";
    case GK_FSCEN_CRASH: return "inspect damage, realign, re-home";
    case GK_FSCEN_TOOL_BREAK: return "replace tool, check offset";
    case GK_FSCEN_TOOL_BURN: return "reduce speed, add coolant";
    case GK_FSCEN_EDGE_CHIP: return "reduce feed, inspect edge";
    case GK_FSCEN_PART_FLY: return "secure clamping, upgrade fixture";
    case GK_FSCEN_FIRE: return "extinguish, evacuate, inspect";
    case GK_FSCEN_LEAKAGE: return "power off, ground, inspect";
    case GK_FSCEN_INJURY: return "first aid, evacuate, report";
    default: return "unknown";
    }
}

void gk_fscen_stats_init(gk_fscen_stats *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
}

gk_status gk_fscen_stats_add(gk_fscen_stats *s, double uptime_hours,
                             double repair_hours)
{
    if (s == NULL || uptime_hours < 0.0 || repair_hours < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    s->total_hours += uptime_hours;
    s->repair_hours += repair_hours;
    s->failure_count++;
    return GK_OK;
}

double gk_fscen_mtbf(const gk_fscen_stats *s)
{
    if (s == NULL || s->failure_count == 0) {
        return 0.0;
    }
    return s->total_hours / (double)s->failure_count;
}

double gk_fscen_mttr(const gk_fscen_stats *s)
{
    if (s == NULL || s->failure_count == 0) {
        return 0.0;
    }
    return s->repair_hours / (double)s->failure_count;
}

double gk_fscen_availability(const gk_fscen_stats *s)
{
    double up, down;
    if (s == NULL) {
        return 0.0;
    }
    up = s->total_hours;
    down = s->repair_hours;
    if (up + down <= 0.0) {
        return 0.0;
    }
    return up / (up + down);
}
