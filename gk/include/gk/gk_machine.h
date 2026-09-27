#ifndef GK_MACHINE_H
#define GK_MACHINE_H

#include <stddef.h>
#include "gk/gk_error.h"
#include "gk/gk_math.h"
#include "gk/gk_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- machine types (items 179..208) ---- */

typedef enum {
    GK_MACHINE_VMC = 0,          /* 179 vertical machining center */
    GK_MACHINE_HMC,              /* 180 horizontal machining center */
    GK_MACHINE_GANTRY,           /* 181 gantry machining center */
    GK_MACHINE_5AX_HEAD,         /* 182 5-axis double swivel head */
    GK_MACHINE_5AX_TABLE,        /* 183 5-axis swivel table */
    GK_MACHINE_5AX_HYBRID,       /* 184 5-axis hybrid */
    GK_MACHINE_TURN,             /* 185 CNC lathe */
    GK_MACHINE_TURN_MILL,        /* 186 turning center */
    GK_MACHINE_MILL_TURN,        /* 187 mill-turn */
    GK_MACHINE_SURFACE_GRINDER,  /* 188 surface grinder */
    GK_MACHINE_CYL_GRINDER,      /* 189 cylindrical grinder */
    GK_MACHINE_INT_GRINDER,      /* 190 internal grinder */
    GK_MACHINE_CENTERLESS,       /* 191 centerless grinder */
    GK_MACHINE_EDM,              /* 192 EDM */
    GK_MACHINE_WEDM,             /* 193 wire EDM */
    GK_MACHINE_LASER,            /* 194 laser cutting */
    GK_MACHINE_PLASMA,           /* 195 plasma cutting */
    GK_MACHINE_WATERJET,         /* 196 waterjet */
    GK_MACHINE_FLAME,            /* 197 flame cutting */
    GK_MACHINE_FDM,              /* 198 additive FDM */
    GK_MACHINE_SLM,              /* 199 additive SLM */
    GK_MACHINE_FSW,              /* 200 friction stir welding */
    GK_MACHINE_DRILL_TAP,        /* 201 drilling-tapping center */
    GK_MACHINE_ENGRAVE,          /* 202 engraving mill */
    GK_MACHINE_DEEP_HOLE,        /* 203 deep-hole drilling machine */
    GK_MACHINE_GEAR,             /* 204 gear machine */
    GK_MACHINE_THREAD_GRINDER,   /* 205 thread grinder */
    GK_MACHINE_CRANK_GRINDER,    /* 206 crankshaft grinder */
    GK_MACHINE_CAM_GRINDER,      /* 207 cam grinder */
    GK_MACHINE_TOOL_GRINDER,     /* 208 tool grinder */
    GK_MACHINE_TYPE_COUNT
} gk_machine_type;

typedef enum {
    GK_PROCESS_MILL = 0,
    GK_PROCESS_TURN,
    GK_PROCESS_GRIND,
    GK_PROCESS_EDM,
    GK_PROCESS_LASER,
    GK_PROCESS_ADDITIVE,
    GK_PROCESS_WELD,
    GK_PROCESS_CUT
} gk_process_kind;

const char *gk_machine_type_name(gk_machine_type t);
const char *gk_process_kind_name(gk_process_kind k);
int gk_machine_type_count(void);

typedef struct {
    gk_machine_type type;
    gk_process_kind process;
    int linear_axes;       /* number of linear axes */
    int rotary_axes;       /* number of rotary axes */
    int simultaneous_axes; /* max simultaneously interpolated axes */
    int has_atc;           /* automatic tool changer */
    int has_dresser;       /* wheel dresser (grinders) */
    int has_tailstock;     /* lathes */
    double max_spindle_rpm;
    double travel[3];      /* X/Y/Z travel in mm */
} gk_machine_def;

/* Returns NULL if the type is out of range. */
const gk_machine_def *gk_machine_type_def(gk_machine_type t);
/* Index 0..29 -> item id 179..208 and back. */
int gk_machine_item_id(gk_machine_type t);
int gk_machine_type_from_item(int item_id, gk_machine_type *out);

/* Kinematic layout helpers. */
int gk_machine_is_turning(gk_machine_type t);
int gk_machine_is_grinding(gk_machine_type t);
int gk_machine_is_five_axis(gk_machine_type t);
int gk_machine_supports_axis(gk_machine_type t, int axis_index);

/* ---- CNC control systems (items 209..231) ---- */

typedef enum {
    GK_CTRL_FANUC_0I = 0,   /* 209 */
    GK_CTRL_FANUC_31I,      /* 210 */
    GK_CTRL_FANUC_35I,      /* 211 */
    GK_CTRL_SIEMENS_808D,   /* 212 */
    GK_CTRL_SIEMENS_828D,   /* 213 */
    GK_CTRL_SIEMENS_840D,   /* 214 */
    GK_CTRL_MITSUBISHI_M70, /* 215 */
    GK_CTRL_MITSUBISHI_M80, /* 216 */
    GK_CTRL_MAZAK_SMOOTH,   /* 217 */
    GK_CTRL_MAZAK_MATRIX,   /* 218 */
    GK_CTRL_HAAS,           /* 219 */
    GK_CTRL_OKUMA_OSP,      /* 220 */
    GK_CTRL_HEIDENHAIN_TNC, /* 221 */
    GK_CTRL_HNC8,           /* 222 */
    GK_CTRL_GSK,            /* 223 */
    GK_CTRL_SYNTEC,         /* 224 */
    GK_CTRL_LNC,            /* 225 */
    GK_CTRL_KND,            /* 226 */
    GK_CTRL_DIMA,           /* 227 */
    GK_CTRL_COUNT
} gk_controller;

typedef enum {
    GK_DIALECT_FANUC = 0,   /* ISO / FANUC */
    GK_DIALECT_SIEMENS,     /* DIN 66025 / ShopMill */
    GK_DIALECT_HEIDENHAIN,  /* Klartext */
    GK_DIALECT_MITSUBISHI,
    GK_DIALECT_MAZAK,
    GK_DIALECT_HNC
} gk_dialect;

const char *gk_controller_name(gk_controller c);
const char *gk_controller_vendor(gk_controller c);
gk_dialect gk_controller_dialect(gk_controller c);
const char *gk_dialect_name(gk_dialect d);
int gk_controller_count(void);
int gk_controller_item_id(gk_controller c);
int gk_controller_from_item(int item_id, gk_controller *out);

/* Alarm code differences (item 229). Sorted ascending by code. */
typedef struct {
    int code;
    const char *message;
} gk_alarm_entry;

const gk_alarm_entry *gk_controller_alarms(gk_controller c, size_t *count);
const char *gk_controller_alarm_message(gk_controller c, int code);

/* Parameter page differences (item 230). */
size_t gk_controller_param_page_count(gk_controller c);
const char *gk_controller_param_page_name(gk_controller c, size_t index);

/* G-code dialect conversion (item 228).
 * Converts a single block's textual form between dialects where they differ.
 * Returns the number of characters written (excluding NUL). */
size_t gk_dialect_convert(const char *block, gk_controller from,
                          gk_controller to, char *out, size_t out_size);

/* Multi-controller comparison (item 231). */
#define GK_CTRL_TRAIT_COUNT 4

typedef struct {
    int same_dialect;
    int same_alarm_scheme;
    int same_param_page_count;
    size_t diff_count;
    const char *notes[GK_CTRL_TRAIT_COUNT];
} gk_ctrl_diff;

gk_status gk_controller_compare(gk_controller a, gk_controller b,
                                gk_ctrl_diff *out);

#ifdef __cplusplus
}
#endif

#endif
