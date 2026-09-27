#ifndef GK_EXECUTOR_H
#define GK_EXECUTOR_H

#include <stddef.h>
#include "gk/gk_error.h"
#include "gk/gk_parser.h"
#include "gk/gk_state.h"
#include "gk/gk_motion.h"
#include "gk/gk_transform.h"
#include "gk/gk_tool.h"
#include "gk/gk_canned.h"
#include "gk/gk_mem.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GK_EXEC_IDLE = 0,
    GK_EXEC_RUNNING,
    GK_EXEC_PAUSED,
    GK_EXEC_STOPPED,
    GK_EXEC_FINISHED,
    GK_EXEC_ERROR,
    GK_EXEC_ESTOP
} gk_exec_state;

typedef struct {
    size_t block_index;      /* next block to execute */
    int has_breakpoint;
    char label[32];
} gk_breakpoint;

typedef struct {
    gk_machine_state state;
    gk_motion_config config;
    gk_transform transform;
    gk_tool_table tools;

    gk_cutter_comp cutter_comp;
    gk_length_comp length_comp;
    int comp_register;       /* D/H register (tool number) */
    gk_canned_cycle canned;  /* active canned cycle, 0 = none */
    gk_canned_params canned_params;
    int canned_pending;      /* new hole position pending in current block */
    int last_tool;           /* tool currently in spindle */
    int tool_warning;

    const gk_program *program;
    gk_exec_state run_state;
    size_t cursor;            /* current block index */
    int single_block;
    int optional_stop;        /* M01 enabled */
    int program_end;
    int sub_call_depth;
    size_t call_stack[16];
    size_t call_stack_top;

    gk_allocator alloc;

    gk_breakpoint breakpoints[64];
    size_t breakpoint_count;

    /* last executed block info */
    gk_move last_move;
    int has_last_move;
    size_t blocks_executed;
    size_t move_count;
    double elapsed_time;

    gk_status error;
    char error_message[160];
} gk_executor;

gk_status gk_executor_init(gk_executor *ex, const gk_allocator *alloc);
void gk_executor_destroy(gk_executor *ex);
gk_status gk_executor_load(gk_executor *ex, const gk_program *program);
gk_status gk_executor_reset(gk_executor *ex);

void gk_executor_start(gk_executor *ex);
void gk_executor_halt(gk_executor *ex);
void gk_executor_estop(gk_executor *ex);
void gk_executor_resume(gk_executor *ex);
void gk_executor_set_single_block(gk_executor *ex, int enabled);

gk_status gk_executor_add_breakpoint(gk_executor *ex, size_t block_index);
gk_status gk_executor_remove_breakpoint(gk_executor *ex, size_t block_index);
int gk_executor_has_breakpoint(const gk_executor *ex, size_t block_index);
void gk_executor_clear_breakpoints(gk_executor *ex);

/* Execute one block. Returns GK_OK even when the program finishes; sets
 * run_state to GK_EXEC_* accordingly. */
gk_status gk_executor_step(gk_executor *ex);
/* Run until a breakpoint, pause, end, or error. */
gk_status gk_executor_run(gk_executor *ex, size_t max_blocks);

const char *gk_exec_state_name(gk_exec_state st);

#ifdef __cplusplus
}
#endif

#endif
