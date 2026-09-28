/* Native CNC simulator bridge for Android.
 *
 * Wraps the gk engine: parse G-code, drive the executor block by block,
 * interpolate each motion segment and remove voxel material with the tool.
 * The Java side reads the voxel grid + tool position to render with GLES.
 */
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "gk/gk_executor.h"
#include "gk/gk_parser.h"
#include "gk/gk_motion.h"
#include "gk/gk_voxel.h"
#include "gk/gk_state.h"

#define SIM_NX 64
#define SIM_NY 64
#define SIM_NZ 32
#define SIM_CELL 1.0

typedef struct {
    gk_program program;
    gk_executor exec;
    gk_voxel_grid voxel;
    int loaded;
    int running;
    int finished;
    double tool_x, tool_y, tool_z;
    double tool_radius;
    size_t removed_total;
    size_t blocks_done;
    size_t total_blocks;
} sim_state;

static sim_state g_sim;
static int g_sim_inited = 0;

/* Interpolate a move and cut material along it. */
static void sim_cut_move(const gk_move *m, double tool_radius)
{
    double len = gk_move_length(m);
    size_t steps = (size_t)(len / (SIM_CELL * 0.75)) + 2;
    size_t i;
    gk_point3 prev = m->start;

    for (i = 1; i <= steps; i++) {
        gk_point3 pt;
        double t = (double)i / (double)steps;
        if (m->mode == GK_MOTION_CW || m->mode == GK_MOTION_CCW) {
            if (gk_interpolate_arc_point(m, t, &pt) != GK_OK) {
                break;
            }
        } else {
            pt.x = m->start.x + (m->end.x - m->start.x) * t;
            pt.y = m->start.y + (m->end.y - m->start.y) * t;
            pt.z = m->start.z + (m->end.z - m->start.z) * t;
        }
        /* Only cut when the tool is below the stock top (Z <= 0). */
        if (pt.z <= 0.0) {
            size_t removed = 0;
            gk_voxel_cut_segment(&g_sim.voxel, prev, pt, tool_radius, &removed);
            g_sim.removed_total += removed;
        }
        prev = pt;
    }
}

static void sim_init_grid(void)
{
    gk_point3 origin;
    gk_allocator alloc;
    origin.x = -32.0;
    origin.y = -32.0;
    origin.z = -(double)SIM_NZ;   /* stock top at Z=0 */
    memset(&alloc, 0, sizeof(alloc));
    gk_voxel_init(&g_sim.voxel, SIM_NX, SIM_NY, SIM_NZ, SIM_CELL, origin, &alloc);
}

static void sim_reset(void)
{
    if (!g_sim_inited) {
        memset(&g_sim, 0, sizeof(g_sim));
        g_sim.tool_radius = 1.5;
        g_sim_inited = 1;
    }
    if (g_sim.loaded) {
        gk_executor_destroy(&g_sim.exec);
        gk_program_free(&g_sim.program);
    }
    if (g_sim.voxel.cells != NULL) {
        gk_voxel_destroy(&g_sim.voxel);
    }
    memset(&g_sim, 0, sizeof(g_sim));
    g_sim.tool_radius = 1.5;
    sim_init_grid();
}

/* Default demo program: mill a pocket with an arc. */
static const char *DEMO_GCODE =
    "G21 G90 G17\n"
    "G00 X-20 Y-15 Z5\n"
    "G01 Z-2 F80\n"
    "G01 X20 F200\n"
    "G01 Y15\n"
    "G01 X-20\n"
    "G01 Y-15\n"
    "G00 Z5\n"
    "G00 X0 Y0\n"
    "G01 Z-3 F80\n"
    "G02 X0 Y0 I10 J0 F200\n"
    "G00 Z10\n"
    "M30\n";

JNIEXPORT void JNICALL
Java_com_example_gk_SimActivity_nativeSimInit(JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;
    sim_reset();
}

JNIEXPORT void JNICALL
Java_com_example_gk_SimActivity_nativeSimLoadDemo(JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;
    sim_reset();
    gk_program_init(&g_sim.program, NULL);
    if (gk_program_parse(&g_sim.program, DEMO_GCODE, strlen(DEMO_GCODE)) != GK_OK) {
        return;
    }
    gk_executor_init(&g_sim.exec, NULL);
    gk_executor_load(&g_sim.exec, &g_sim.program);
    gk_executor_start(&g_sim.exec);
    g_sim.loaded = 1;
    g_sim.running = 1;
    g_sim.finished = 0;
    g_sim.blocks_done = 0;
    g_sim.total_blocks = g_sim.program.block_count;
    gk_executor_reset(&g_sim.exec);
    gk_executor_start(&g_sim.exec);
}

/* Execute the next block; cut material for any motion it produced. */
JNIEXPORT jboolean JNICALL
Java_com_example_gk_SimActivity_nativeSimStep(JNIEnv *env, jclass cls)
{
    gk_status st;
    (void)env; (void)cls;
    if (!g_sim.loaded || g_sim.finished) {
        return JNI_FALSE;
    }
    st = gk_executor_step(&g_sim.exec);
    if (g_sim.exec.has_last_move) {
        sim_cut_move(&g_sim.exec.last_move, g_sim.tool_radius);
    }
    g_sim.tool_x = g_sim.exec.state.coord[GK_AXIS_X];
    g_sim.tool_y = g_sim.exec.state.coord[GK_AXIS_Y];
    g_sim.tool_z = g_sim.exec.state.coord[GK_AXIS_Z];
    g_sim.blocks_done++;
    if (g_sim.exec.run_state == GK_EXEC_FINISHED
            || g_sim.exec.run_state == GK_EXEC_STOPPED
            || g_sim.exec.run_state == GK_EXEC_ERROR
            || st != GK_OK) {
        g_sim.finished = 1;
        g_sim.running = 0;
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

JNIEXPORT jdoubleArray JNICALL
Java_com_example_gk_SimActivity_nativeSimToolPos(JNIEnv *env, jclass cls)
{
    jdoubleArray arr = (*env)->NewDoubleArray(env, 3);
    jdouble vals[3];
    (void)cls;
    vals[0] = g_sim.tool_x;
    vals[1] = g_sim.tool_y;
    vals[2] = g_sim.tool_z;
    if (arr != NULL) {
        (*env)->SetDoubleArrayRegion(env, arr, 0, 3, vals);
    }
    return arr;
}

JNIEXPORT jintArray JNICALL
Java_com_example_gk_SimActivity_nativeSimVoxels(JNIEnv *env, jclass cls)
{
    jint n = SIM_NX * SIM_NY * SIM_NZ;
    jintArray arr = (*env)->NewIntArray(env, n);
    jint *buf;
    size_t x, y, z, k = 0;
    (void)cls;
    if (arr == NULL) {
        return NULL;
    }
    buf = (jint *)malloc(sizeof(jint) * (size_t)n);
    if (buf == NULL) {
        return arr;
    }
    for (x = 0; x < SIM_NX; x++) {
        for (y = 0; y < SIM_NY; y++) {
            for (z = 0; z < SIM_NZ; z++) {
                size_t idx = 0;
                int present = 0;
                if (gk_voxel_index(&g_sim.voxel, x, y, z, &idx) == 1) {
                    present = g_sim.voxel.cells[idx] ? 1 : 0;
                }
                buf[k++] = present;
            }
        }
    }
    (*env)->SetIntArrayRegion(env, arr, 0, n, buf);
    free(buf);
    return arr;
}

JNIEXPORT jint JNICALL
Java_com_example_gk_SimActivity_nativeSimRemoved(JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;
    return (jint)g_sim.removed_total;
}

JNIEXPORT jint JNICALL
Java_com_example_gk_SimActivity_nativeSimBlocksDone(JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;
    return (jint)g_sim.blocks_done;
}

JNIEXPORT jboolean JNICALL
Java_com_example_gk_SimActivity_nativeSimFinished(JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;
    return g_sim.finished ? JNI_TRUE : JNI_FALSE;
}
