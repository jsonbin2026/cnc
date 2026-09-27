#include "gk/gk_machine.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ---- machine types ---- */

typedef struct {
    const char *name;
    gk_process_kind process;
    int linear;
    int rotary;
    int simultaneous;
    int atc;
    int dresser;
    int tailstock;
    double rpm;
    double tx;
    double ty;
    double tz;
} machine_row;

static const machine_row g_machines[GK_MACHINE_TYPE_COUNT] = {
    /* 179 VMC */ {"VMC", GK_PROCESS_MILL, 3, 0, 3, 1, 0, 0, 12000, 800, 500, 500},
    /* 180 HMC */ {"HMC", GK_PROCESS_MILL, 3, 1, 3, 1, 0, 0, 12000, 700, 700, 700},
    /* 181 gantry */ {"gantry", GK_PROCESS_MILL, 3, 0, 3, 1, 0, 0, 8000, 3000, 2000, 1000},
    /* 182 5ax head */ {"5-axis-head", GK_PROCESS_MILL, 3, 2, 5, 1, 0, 0, 18000, 500, 500, 400},
    /* 183 5ax table */ {"5-axis-table", GK_PROCESS_MILL, 3, 2, 5, 1, 0, 0, 15000, 400, 400, 400},
    /* 184 5ax hybrid */ {"5-axis-hybrid", GK_PROCESS_MILL, 3, 2, 5, 1, 0, 0, 16000, 450, 450, 400},
    /* 185 lathe */ {"lathe", GK_PROCESS_TURN, 2, 1, 2, 1, 0, 1, 4000, 300, 200, 0},
    /* 186 turning center */ {"turning-center", GK_PROCESS_TURN, 2, 1, 3, 1, 0, 1, 5000, 400, 250, 0},
    /* 187 mill-turn */ {"mill-turn", GK_PROCESS_MILL, 3, 2, 5, 1, 0, 1, 10000, 500, 400, 400},
    /* 188 surface grinder */ {"surface-grinder", GK_PROCESS_GRIND, 3, 0, 2, 0, 1, 0, 3600, 600, 400, 300},
    /* 189 cyl grinder */ {"cylindrical-grinder", GK_PROCESS_GRIND, 2, 1, 2, 0, 1, 0, 3000, 500, 200, 0},
    /* 190 int grinder */ {"internal-grinder", GK_PROCESS_GRIND, 3, 1, 2, 0, 1, 0, 4000, 300, 200, 200},
    /* 191 centerless */ {"centerless-grinder", GK_PROCESS_GRIND, 2, 1, 2, 0, 1, 0, 3000, 400, 200, 0},
    /* 192 EDM */ {"EDM", GK_PROCESS_EDM, 3, 0, 3, 0, 0, 0, 0, 400, 300, 300},
    /* 193 WEDM */ {"WEDM", GK_PROCESS_EDM, 4, 1, 2, 0, 0, 0, 0, 500, 400, 300},
    /* 194 laser */ {"laser", GK_PROCESS_LASER, 3, 0, 2, 0, 0, 0, 0, 3000, 1500, 200},
    /* 195 plasma */ {"plasma", GK_PROCESS_CUT, 3, 0, 2, 0, 0, 0, 0, 3000, 1500, 200},
    /* 196 waterjet */ {"waterjet", GK_PROCESS_CUT, 3, 0, 2, 0, 0, 0, 0, 3000, 2000, 200},
    /* 197 flame */ {"flame", GK_PROCESS_CUT, 3, 0, 2, 0, 0, 0, 0, 4000, 2000, 300},
    /* 198 FDM */ {"FDM", GK_PROCESS_ADDITIVE, 3, 0, 3, 0, 0, 0, 0, 400, 400, 400},
    /* 199 SLM */ {"SLM", GK_PROCESS_ADDITIVE, 3, 0, 2, 0, 0, 0, 0, 250, 250, 300},
    /* 200 FSW */ {"FSW", GK_PROCESS_WELD, 3, 1, 3, 0, 0, 0, 3000, 1000, 600, 400},
    /* 201 drill-tap */ {"drill-tap-center", GK_PROCESS_MILL, 3, 0, 3, 1, 0, 0, 8000, 500, 400, 400},
    /* 202 engrave */ {"engraving-mill", GK_PROCESS_MILL, 3, 0, 3, 0, 0, 0, 24000, 400, 300, 200},
    /* 203 deep hole */ {"deep-hole-drill", GK_PROCESS_MILL, 2, 1, 2, 0, 0, 1, 3000, 200, 0, 1200},
    /* 204 gear */ {"gear-machine", GK_PROCESS_GRIND, 3, 2, 4, 0, 0, 0, 3000, 400, 300, 300},
    /* 205 thread grinder */ {"thread-grinder", GK_PROCESS_GRIND, 2, 2, 3, 0, 1, 1, 3000, 500, 200, 0},
    /* 206 crank grinder */ {"crankshaft-grinder", GK_PROCESS_GRIND, 3, 1, 2, 0, 1, 0, 2500, 1000, 400, 300},
    /* 207 cam grinder */ {"cam-grinder", GK_PROCESS_GRIND, 2, 2, 3, 0, 1, 0, 3000, 600, 200, 0},
    /* 208 tool grinder */ {"tool-grinder", GK_PROCESS_GRIND, 4, 2, 5, 0, 1, 0, 6000, 400, 300, 300}
};

const char *gk_machine_type_name(gk_machine_type t)
{
    if (t < 0 || t >= GK_MACHINE_TYPE_COUNT) {
        return "unknown";
    }
    return g_machines[t].name;
}

const char *gk_process_kind_name(gk_process_kind k)
{
    switch (k) {
    case GK_PROCESS_MILL:
        return "milling";
    case GK_PROCESS_TURN:
        return "turning";
    case GK_PROCESS_GRIND:
        return "grinding";
    case GK_PROCESS_EDM:
        return "edm";
    case GK_PROCESS_LASER:
        return "laser";
    case GK_PROCESS_ADDITIVE:
        return "additive";
    case GK_PROCESS_WELD:
        return "welding";
    case GK_PROCESS_CUT:
        return "cutting";
    default:
        return "unknown";
    }
}

int gk_machine_type_count(void)
{
    return GK_MACHINE_TYPE_COUNT;
}

const gk_machine_def *gk_machine_type_def(gk_machine_type t)
{
    static gk_machine_def def;
    const machine_row *r;
    if (t < 0 || t >= GK_MACHINE_TYPE_COUNT) {
        return NULL;
    }
    r = &g_machines[t];
    def.type = t;
    def.process = r->process;
    def.linear_axes = r->linear;
    def.rotary_axes = r->rotary;
    def.simultaneous_axes = r->simultaneous;
    def.has_atc = r->atc;
    def.has_dresser = r->dresser;
    def.has_tailstock = r->tailstock;
    def.max_spindle_rpm = r->rpm;
    def.travel[0] = r->tx;
    def.travel[1] = r->ty;
    def.travel[2] = r->tz;
    return &def;
}

int gk_machine_item_id(gk_machine_type t)
{
    if (t < 0 || t >= GK_MACHINE_TYPE_COUNT) {
        return -1;
    }
    return 179 + (int)t;
}

int gk_machine_type_from_item(int item_id, gk_machine_type *out)
{
    int idx;
    if (item_id < 179 || item_id > 208) {
        return 0;
    }
    idx = item_id - 179;
    if (out != NULL) {
        *out = (gk_machine_type)idx;
    }
    return 1;
}

int gk_machine_is_turning(gk_machine_type t)
{
    if (t < 0 || t >= GK_MACHINE_TYPE_COUNT) {
        return 0;
    }
    return g_machines[t].process == GK_PROCESS_TURN;
}

int gk_machine_is_grinding(gk_machine_type t)
{
    if (t < 0 || t >= GK_MACHINE_TYPE_COUNT) {
        return 0;
    }
    return g_machines[t].process == GK_PROCESS_GRIND;
}

int gk_machine_is_five_axis(gk_machine_type t)
{
    if (t < 0 || t >= GK_MACHINE_TYPE_COUNT) {
        return 0;
    }
    return g_machines[t].simultaneous >= 5;
}

int gk_machine_supports_axis(gk_machine_type t, int axis_index)
{
    const machine_row *r;
    if (t < 0 || t >= GK_MACHINE_TYPE_COUNT || axis_index < 0) {
        return 0;
    }
    r = &g_machines[t];
    if (axis_index < 3) {
        return axis_index < r->linear;
    }
    return (axis_index - 3) < r->rotary;
}

/* ---- controllers ---- */

typedef struct {
    const char *name;
    const char *vendor;
    gk_dialect dialect;
    size_t param_pages;
} ctrl_row;

static const ctrl_row g_controllers[GK_CTRL_COUNT] = {
    /* 209 */ {"FANUC 0i", "FANUC", GK_DIALECT_FANUC, 6},
    /* 210 */ {"FANUC 31i", "FANUC", GK_DIALECT_FANUC, 8},
    /* 211 */ {"FANUC 35i", "FANUC", GK_DIALECT_FANUC, 9},
    /* 212 */ {"SIEMENS 808D", "SIEMENS", GK_DIALECT_SIEMENS, 5},
    /* 213 */ {"SIEMENS 828D", "SIEMENS", GK_DIALECT_SIEMENS, 7},
    /* 214 */ {"SIEMENS 840D", "SIEMENS", GK_DIALECT_SIEMENS, 10},
    /* 215 */ {"MITSUBISHI M70", "MITSUBISHI", GK_DIALECT_MITSUBISHI, 6},
    /* 216 */ {"MITSUBISHI M80", "MITSUBISHI", GK_DIALECT_MITSUBISHI, 7},
    /* 217 */ {"MAZAK Smooth", "MAZAK", GK_DIALECT_MAZAK, 8},
    /* 218 */ {"MAZAK Matrix", "MAZAK", GK_DIALECT_MAZAK, 7},
    /* 219 */ {"HAAS", "HAAS", GK_DIALECT_FANUC, 5},
    /* 220 */ {"OKUMA OSP", "OKUMA", GK_DIALECT_FANUC, 6},
    /* 221 */ {"HEIDENHAIN TNC", "HEIDENHAIN", GK_DIALECT_HEIDENHAIN, 9},
    /* 222 */ {"HNC-8", "HNC", GK_DIALECT_HNC, 6},
    /* 223 */ {"GSK", "GSK", GK_DIALECT_FANUC, 5},
    /* 224 */ {"SYNTEC", "SYNTEC", GK_DIALECT_FANUC, 5},
    /* 225 */ {"LNC", "LNC", GK_DIALECT_FANUC, 5},
    /* 226 */ {"KND", "KND", GK_DIALECT_FANUC, 4},
    /* 227 */ {"DIMA", "DIMA", GK_DIALECT_FANUC, 4}
};

const char *gk_controller_name(gk_controller c)
{
    if (c < 0 || c >= GK_CTRL_COUNT) {
        return "unknown";
    }
    return g_controllers[c].name;
}

const char *gk_controller_vendor(gk_controller c)
{
    if (c < 0 || c >= GK_CTRL_COUNT) {
        return "unknown";
    }
    return g_controllers[c].vendor;
}

gk_dialect gk_controller_dialect(gk_controller c)
{
    if (c < 0 || c >= GK_CTRL_COUNT) {
        return GK_DIALECT_FANUC;
    }
    return g_controllers[c].dialect;
}

const char *gk_dialect_name(gk_dialect d)
{
    switch (d) {
    case GK_DIALECT_FANUC:
        return "FANUC/ISO";
    case GK_DIALECT_SIEMENS:
        return "SIEMENS/DIN";
    case GK_DIALECT_HEIDENHAIN:
        return "HEIDENHAIN/Klartext";
    case GK_DIALECT_MITSUBISHI:
        return "MITSUBISHI";
    case GK_DIALECT_MAZAK:
        return "MAZAK";
    case GK_DIALECT_HNC:
        return "HNC";
    default:
        return "unknown";
    }
}

int gk_controller_count(void)
{
    return GK_CTRL_COUNT;
}

int gk_controller_item_id(gk_controller c)
{
    if (c < 0 || c >= GK_CTRL_COUNT) {
        return -1;
    }
    return 209 + (int)c;
}

int gk_controller_from_item(int item_id, gk_controller *out)
{
    if (item_id < 209 || item_id > 227) {
        return 0;
    }
    if (out != NULL) {
        *out = (gk_controller)(item_id - 209);
    }
    return 1;
}

/* ---- alarms ---- */

static const gk_alarm_entry g_fanuc_alarms[] = {
    {1, "T01 OVER TRAVEL"},
    {2, "T02 OVER TRAVEL"},
    {100, "PARAMETER WRITE ENABLE"},
    {300, "APC ALARM"},
    {401, "SERVO ALARM"},
    {500, "OVER TRAVEL"}
};

static const gk_alarm_entry g_siemens_alarms[] = {
    {700000, "Emergency stop"},
    {700001, "Drive not ready"},
    {1010, "Block not available"},
    {14011, "Tool not in magazine"},
    {25000, "Axis overtravel"}
};

static const gk_alarm_entry g_heidenhain_alarms[] = {
    {1, "Wrong operating mode"},
    {100, "Spindle reference mark missing"},
    {204, "Reference not reachable"},
    {500, "Power interrupted"}
};

static const gk_alarm_entry g_mitsubishi_alarms[] = {
    {10, "SERVO ALARM"},
    {30, "SPINDLE ALARM"},
    {100, "PROGRAM ERROR"},
    {120, "OVER TRAVEL"}
};

static const gk_alarm_entry g_mazak_alarms[] = {
    {1, "Over travel"},
    {20, "Servo alarm"},
    {100, "Program error"},
    {201, "Spindle alarm"}
};

static const gk_alarm_entry g_hnc_alarms[] = {
    {1, "伺服报警"},
    {2, "过行程报警"},
    {10, "程序错误"},
    {100, "主轴报警"}
};

typedef struct {
    const gk_alarm_entry *entries;
    size_t count;
} gk_alarm_set;

static const gk_alarm_set g_alarm_sets[6] = {
    {g_fanuc_alarms, sizeof(g_fanuc_alarms) / sizeof(g_fanuc_alarms[0])},
    {g_siemens_alarms, sizeof(g_siemens_alarms) / sizeof(g_siemens_alarms[0])},
    {g_heidenhain_alarms,
     sizeof(g_heidenhain_alarms) / sizeof(g_heidenhain_alarms[0])},
    {g_mitsubishi_alarms,
     sizeof(g_mitsubishi_alarms) / sizeof(g_mitsubishi_alarms[0])},
    {g_mazak_alarms, sizeof(g_mazak_alarms) / sizeof(g_mazak_alarms[0])},
    {g_hnc_alarms, sizeof(g_hnc_alarms) / sizeof(g_hnc_alarms[0])}
};

const gk_alarm_entry *gk_controller_alarms(gk_controller c, size_t *count)
{
    gk_dialect d = gk_controller_dialect(c);
    const gk_alarm_set *set = &g_alarm_sets[d];
    if (count != NULL) {
        *count = set->count;
    }
    return set->entries;
}

const char *gk_controller_alarm_message(gk_controller c, int code)
{
    size_t n, i;
    const gk_alarm_entry *e = gk_controller_alarms(c, &n);
    for (i = 0; i < n; ++i) {
        if (e[i].code == code) {
            return e[i].message;
        }
    }
    return NULL;
}

/* ---- parameter pages ---- */

static const char *const g_fanuc_pages[] = {
    "SETTING", "OFFSET", "WORK", "TIMER", "SYSTEM", "DIAGNOSTIC"
};
static const char *const g_siemens_pages[] = {
    "MACHINE DATA", "SETTING DATA", "TOOL DATA", "WORK OFFSET",
    "SERVICE DISPLAY", "DRIVE PARAMS", "PLC STATUS"
};
static const char *const g_heidenhaid_pages[] = {
    "MOD", "MP", "CODE", "TRACING", "TOOL TABLE", "POCKET TABLE",
    "SPINDLE", "FEED", "PLC"
};
static const char *const g_mitsubishi_pages[] = {
    "PARAM", "OFFSET", "WORK", "TOOL", "DIAGN", "I/F DIAGN"
};
static const char *const g_mazak_pages[] = {
    "MAINTENANCE", "TUNING", "PARAMETER", "TOOL DATA", "WORK OFFSET",
    "PC", "SYSTEM", "ALARM"
};
static const char *const g_hnc_pages[] = {
    "参数", "刀补", "坐标系", "诊断", "PLC", "伺服"
};

static const char *const *const g_page_sets[6] = {
    g_fanuc_pages, g_siemens_pages, g_heidenhaid_pages,
    g_mitsubishi_pages, g_mazak_pages, g_hnc_pages
};

static const size_t g_page_counts[6] = {6, 7, 9, 6, 8, 6};

size_t gk_controller_param_page_count(gk_controller c)
{
    return g_page_counts[gk_controller_dialect(c)];
}

const char *gk_controller_param_page_name(gk_controller c, size_t index)
{
    gk_dialect d = gk_controller_dialect(c);
    if (index >= g_page_counts[d]) {
        return NULL;
    }
    return g_page_sets[d][index];
}

/* ---- dialect conversion ---- */

size_t gk_dialect_convert(const char *block, gk_controller from,
                          gk_controller to, char *out, size_t out_size)
{
    gk_dialect df, dt;
    if (block == NULL || out == NULL || out_size == 0) {
        return 0;
    }
    df = gk_controller_dialect(from);
    dt = gk_controller_dialect(to);
    if (df == dt) {
        size_t len = strlen(block);
        if (len >= out_size) {
            len = out_size - 1;
        }
        memcpy(out, block, len);
        out[len] = '\0';
        return len;
    }
    /* Handle a few well-known differences:
     * - rigid tapping: G84 (FANUC) <-> G841 (SIEMENS)
     * - reference return: G28 (FANUC) <-> G74 (SIEMENS)
     * - Tool call: T01 M06 (FANUC) <-> T="..." M6 (SIEMENS)
     */
    {
        const char *src = block;
        size_t written = 0;
        while (*src != '\0' && written + 1 < out_size) {
            if (dt == GK_DIALECT_SIEMENS) {
                if (strncmp(src, "G84", 3) == 0) {
                    written += (size_t)snprintf(out + written,
                                                out_size - written, "G841");
                    src += 3;
                    continue;
                }
                if (strncmp(src, "G28", 3) == 0) {
                    written += (size_t)snprintf(out + written,
                                                out_size - written, "G74");
                    src += 3;
                    continue;
                }
            }
            if (df == GK_DIALECT_SIEMENS && dt == GK_DIALECT_FANUC) {
                if (strncmp(src, "G841", 4) == 0) {
                    written += (size_t)snprintf(out + written,
                                                out_size - written, "G84");
                    src += 4;
                    continue;
                }
                if (strncmp(src, "G74", 3) == 0) {
                    written += (size_t)snprintf(out + written,
                                                out_size - written, "G28");
                    src += 3;
                    continue;
                }
            }
            out[written++] = *src++;
        }
        out[written] = '\0';
        return written;
    }
}

/* ---- comparison ---- */

gk_status gk_controller_compare(gk_controller a, gk_controller b,
                                gk_ctrl_diff *out)
{
    if (out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (a < 0 || a >= GK_CTRL_COUNT || b < 0 || b >= GK_CTRL_COUNT) {
        return GK_ERR_OUT_OF_RANGE;
    }
    memset(out, 0, sizeof(*out));
    out->same_dialect = gk_controller_dialect(a) == gk_controller_dialect(b);
    out->same_alarm_scheme = out->same_dialect;
    out->same_param_page_count =
        gk_controller_param_page_count(a) ==
        gk_controller_param_page_count(b);
    if (!out->same_dialect) {
        out->notes[out->diff_count++] = "different G-code dialect";
    }
    if (!out->same_alarm_scheme) {
        out->notes[out->diff_count++] = "alarm codes differ";
    }
    if (!out->same_param_page_count) {
        out->notes[out->diff_count++] = "parameter page layout differs";
    }
    return GK_OK;
}
