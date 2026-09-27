#ifndef GK_CSYS_H
#define GK_CSYS_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_CSYS_NAME 64

/* ===================================================================
 * Batch 42: real CNC system details (1121-1150)
 * Prefix: gk_csys_
 * =================================================================== */

typedef enum {
    GK_CSYS_POWERED_OFF = 0,
    GK_CSYS_SELFTEST,     /* 1121 */
    GK_CSYS_LOADING,      /* 1122-1129 */
    GK_CSYS_READY,
    GK_CSYS_HOMING,       /* 1132 */
    GK_CSYS_RUNNING,      /* 1135 */
    GK_CSYS_PAUSED,       /* 1136 */
    GK_CSYS_FAULT,
    GK_CSYS_ESTOP
} gk_csys_state;

const char *gk_csys_state_name(gk_csys_state s);

typedef struct {
    gk_csys_state state;
    int selftest_passed;
    int params_loaded;
    int program_loaded;
    int tools_loaded;
    int wcs_loaded;
    int offset_loaded;
    int macros_loaded;
    int plc_running;
    int servos_enabled;
    int homed;
    int alarmed;
} gk_csys;

void gk_csys_init(gk_csys *c);

/* 1121 self test */
gk_status gk_csys_selftest(gk_csys *c, int passed);

/* 1122 parameter loading */
gk_status gk_csys_load_params(gk_csys *c, const char *file);

/* 1123 program loading */
gk_status gk_csys_load_program(gk_csys *c, const char *file);

/* 1124-1125 program verify / syntax check */
gk_status gk_csys_verify_program(gk_csys *c, int syntax_ok);

/* 1126 tool table loading */
gk_status gk_csys_load_tools(gk_csys *c, int count);

/* 1127 workpiece coordinate loading */
gk_status gk_csys_load_wcs(gk_csys *c, int count);

/* 1128 tool offset table */
gk_status gk_csys_load_offsets(gk_csys *c, int count);

/* 1129 macro program loading */
gk_status gk_csys_load_macros(gk_csys *c, int count);

/* 1130 PLC start */
gk_status gk_csys_plc_start(gk_csys *c);
gk_status gk_csys_plc_stop(gk_csys *c);

/* 1131 servo enable */
gk_status gk_csys_enable_servos(gk_csys *c);

/* 1132 homing */
gk_status gk_csys_home(gk_csys *c, int axis_count);

/* 1133 tool setting */
gk_status gk_csys_touch_off(gk_csys *c, double offset_mm, double *stored_mm);

/* 1134 trial cut */
gk_status gk_csys_trial_cut(gk_csys *c, double depth_mm, double *result_mm);

/* 1135 machining */
gk_status gk_csys_start_machining(gk_csys *c);

/* 1136-1137 pause / resume */
gk_status gk_csys_pause(gk_csys *c);
gk_status gk_csys_resume(gk_csys *c);

/* 1138 end */
gk_status gk_csys_end_program(gk_csys *c);

/* 1139 shutdown */
gk_status gk_csys_shutdown(gk_csys *c);

/* 1140-1141 parameter / program backup */
gk_status gk_csys_backup_params(const gk_csys *c, char *out, size_t out_cap);
gk_status gk_csys_backup_program(const char *program, char *out,
                                 size_t out_cap);

/* 1142 system upgrade */
gk_status gk_csys_upgrade(gk_csys *c, const char *version, double *new_version);

/* 1143 data restore */
gk_status gk_csys_restore(gk_csys *c, int backup_valid);

/* 1144-1148 resets */
gk_status gk_csys_reset_alarm(gk_csys *c);
gk_status gk_csys_reset_estop(gk_csys *c);
gk_status gk_csys_reset_overtravel(gk_csys *c, int *overtravel_axis);
gk_status gk_csys_reset_servo(gk_csys *c);
gk_status gk_csys_reset_system(gk_csys *c);

/* 1149-1150 cold / warm start */
gk_status gk_csys_cold_start(gk_csys *c);
gk_status gk_csys_warm_start(gk_csys *c);

/* overall readiness */
int gk_csys_ready(const gk_csys *c);

#ifdef __cplusplus
}
#endif

#endif /* GK_CSYS_H */
