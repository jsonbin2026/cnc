#ifndef GK_CADCAM_H
#define GK_CADCAM_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_CAD_MAX_POINTS 256
#define GK_CAD_MAX_ENTITIES 128
#define GK_CAD_MAX_PATH 512
#define GK_CAD_MAX_TEXT 2048
#define GK_CAD_NAME 64

/* ===================================================================
 * Part A: 2D drawing (820-824)
 * =================================================================== */

typedef enum {
    GK_CAD_LINE = 0,      /* 821 */
    GK_CAD_ARC,           /* 822 */
    GK_CAD_FILLET,        /* 823 */
    GK_CAD_CHAMFER,       /* 824 */
    GK_CAD_ENTITY_COUNT
} gk_cad_entity_kind;

const char *gk_cad_entity_name(gk_cad_entity_kind k);

typedef struct {
    gk_cad_entity_kind kind;
    double x1, y1;
    double x2, y2;
    double radius;        /* arc / fillet radius */
    double cx, cy;        /* arc centre */
    int clockwise;
} gk_cad_entity;

typedef struct {
    gk_cad_entity entities[GK_CAD_MAX_ENTITIES];
    int count;
} gk_cad_sketch;

void gk_cad_sketch_init(gk_cad_sketch *s);
int gk_cad_add_line(gk_cad_sketch *s, double x1, double y1, double x2,
                    double y2);
int gk_cad_add_arc(gk_cad_sketch *s, double cx, double cy, double radius,
                   int clockwise);
gk_status gk_cad_add_fillet(gk_cad_sketch *s, double radius);
gk_status gk_cad_add_chamfer(gk_cad_sketch *s, double distance);
double gk_cad_sketch_length(const gk_cad_sketch *s);

/* ===================================================================
 * Part B: 3D modelling (825-828)
 * =================================================================== */

typedef struct {
    double x1, y1, z1;
    double x2, y2, z2;
} gk_cad_box;

typedef struct {
    gk_cad_entity profile[GK_CAD_MAX_ENTITIES];
    int profile_count;
    double height;        /* extrude height */
    int revolve_steps;    /* facets for revolve */
    double revolve_angle; /* degrees */
} gk_cad_solid;

void gk_cad_solid_init(gk_cad_solid *s);
/* 826: extrude a closed profile to a box / prism */
gk_status gk_cad_extrude(gk_cad_solid *s, double height, gk_cad_box *out);
/* 827: revolve a profile about the Z axis, returning the swept volume */
gk_status gk_cad_revolve(gk_cad_solid *s, double angle_deg, double radius,
                         double *out_volume);
/* 828: boolean operations on axis-aligned boxes */
typedef enum {
    GK_CAD_BOOL_UNION = 0,
    GK_CAD_BOOL_INTERSECT,
    GK_CAD_BOOL_SUBTRACT
} gk_cad_boolean_op;

gk_status gk_cad_boolean(const gk_cad_box *a, const gk_cad_box *b,
                         gk_cad_boolean_op op, gk_cad_box *out);
double gk_cad_box_volume(const gk_cad_box *b);

/* ===================================================================
 * Part C: CAM (829-836)
 * =================================================================== */

typedef enum {
    GK_CAM_POCKET = 0,      /* 830 */
    GK_CAM_CONTOUR_Z,       /* 831 waterline */
    GK_CAM_PARALLEL,        /* 832 */
    GK_CAM_PROFILE,         /* 833 */
    GK_CAM_DRILL,           /* 834 */
    GK_CAM_THREAD_MILL,     /* 835 */
    GK_CAM_TAP,             /* 836 */
    GK_CAM_OP_COUNT
} gk_cam_op_kind;

const char *gk_cam_op_name(gk_cam_op_kind k);

typedef struct {
    gk_cam_op_kind kind;
    double x;
    double y;
    double z_top;
    double z_bottom;
    double tool_diameter;
    double stepover;
    double feed;
    double rpm;
    double thread_pitch;
    int holes;
} gk_cam_op;

typedef struct {
    gk_cam_op ops[GK_CAD_MAX_ENTITIES];
    int count;
    char program[GK_CAD_MAX_TEXT];
} gk_cam_job;

void gk_cam_job_init(gk_cam_job *j);
int gk_cam_add_op(gk_cam_job *j, const gk_cam_op *op);
/* 829-836: generate G-code for all queued operations */
gk_status gk_cam_generate(gk_cam_job *j);
/* rough path length / cycle time estimate for an operation */
double gk_cam_op_time(const gk_cam_op *op);

/* ===================================================================
 * Part D: verification, post, drawing (837-839)
 * =================================================================== */

typedef struct {
    double min_x, min_y, min_z;
    double max_x, max_y, max_z;
} gk_cam_bounds;

typedef struct {
    int collisions;
    int rapid_into_material;
    double rapid_distance;
    double cutting_distance;
} gk_cam_verify;

void gk_cam_verify_init(gk_cam_verify *v);
/* 837: collect bounding box from a G-code program's coordinates */
gk_status gk_cam_verify_run(const char *program, gk_cam_bounds *bounds,
                            gk_cam_verify *v);

/* 838: post-processor flavour */
typedef enum {
    GK_CAM_POST_FANUC = 0,
    GK_CAM_POST_SIEMENS,
    GK_CAM_POST_HAAS,
    GK_CAM_POST_LINUXCNC
} gk_cam_post;

const char *gk_cam_post_name(gk_cam_post p);
gk_status gk_cam_post_transform(gk_cam_post post, const char *in,
                                char *out, size_t out_cap);

/* 839: engineering drawing generation */
typedef struct {
    char title[GK_CAD_NAME];
    char drawing_number[GK_CAD_NAME];
    double scale;
    int views;          /* front/top/side bitmask */
    int projection;     /* 0 = first angle, 1 = third angle */
} gk_cad_drawing;

void gk_cad_drawing_init(gk_cad_drawing *d, const char *title);
gk_status gk_cad_drawing_generate(const gk_cad_drawing *d,
                                  const gk_cam_bounds *bounds, char *out,
                                  size_t out_cap);

/* ===================================================================
 * Part E: CAD import (840-846)
 * =================================================================== */

typedef enum {
    GK_CAD_IMPORT_DXF = 0,      /* 840 */
    GK_CAD_IMPORT_STEP,         /* 841 */
    GK_CAD_IMPORT_IGES,         /* 842 */
    GK_CAD_IMPORT_STL,          /* 843 */
    GK_CAD_IMPORT_OBJ,          /* 844 */
    GK_CAD_IMPORT_3MF,          /* 845 */
    GK_CAD_IMPORT_PARASOLID,    /* 846 */
    GK_CAD_IMPORT_COUNT
} gk_cad_import_format;

const char *gk_cad_import_name(gk_cad_import_format f);
/* format signature detection from a file name / header */
gk_cad_import_format gk_cad_import_detect(const char *filename);
/* returns the number of triangles/entities found in the header text */
gk_status gk_cad_import_probe(const char *header, gk_cad_import_format fmt,
                              int *out_entities);

/* ===================================================================
 * Part F: platform & testing (847-850)
 * =================================================================== */

typedef enum {
    GK_CAD_PLATFORM_WINDOWS = 0,  /* 847 */
    GK_CAD_PLATFORM_LINUX,        /* 848 */
    GK_CAD_PLATFORM_MACOS         /* 849 */
} gk_cad_platform;

const char *gk_cad_platform_name(gk_cad_platform p);
/* returns the current platform this build targets */
gk_cad_platform gk_cad_platform_current(void);
int gk_cad_platform_supported(gk_cad_platform p);

/* 850: minimal unit-test helper registry */
typedef int (*gk_cad_test_fn)(void);

typedef struct {
    char name[GK_CAD_NAME];
    gk_cad_test_fn fn;
} gk_cad_test_case;

typedef struct {
    gk_cad_test_case cases[GK_CAD_MAX_ENTITIES];
    int count;
    int passed;
    int failed;
} gk_cad_test_suite;

void gk_cad_test_suite_init(gk_cad_test_suite *t);
int gk_cad_test_add(gk_cad_test_suite *t, const char *name, gk_cad_test_fn fn);
int gk_cad_test_run(gk_cad_test_suite *t);

#ifdef __cplusplus
}
#endif

#endif /* GK_CADCAM_H */
