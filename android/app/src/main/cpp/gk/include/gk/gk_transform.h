#ifndef GK_TRANSFORM_H
#define GK_TRANSFORM_H

#include "gk/gk_error.h"
#include "gk/gk_math.h"
#include "gk/gk_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gk_point3 work_offsets[6];   /* G54..G59 */
    gk_point3 g92_offset;        /* G92 coordinate shift */
    gk_point3 local_offset;      /* G52 local coordinate system */
    double scale;                /* G51 scaling factor */
    double rotation;             /* G68 rotation, degrees */
    gk_point3 rotation_center;
    int rotation_active;
    int scale_active;
    gk_point3 machine_zero;      /* machine reference (G53) */
} gk_transform;

void gk_transform_init(gk_transform *t);
gk_status gk_transform_set_work_offset(gk_transform *t, int index,
                                       gk_point3 offset);
gk_status gk_transform_get_work_offset(const gk_transform *t, int index,
                                       gk_point3 *out);
gk_status gk_transform_set_g92(gk_transform *t, gk_point3 offset);
gk_status gk_transform_set_local(gk_transform *t, gk_point3 offset);
gk_status gk_transform_set_scale(gk_transform *t, double scale);
gk_status gk_transform_set_rotation(gk_transform *t, double deg,
                                    gk_point3 center);
gk_status gk_transform_cancel_rotation(gk_transform *t);
gk_status gk_transform_cancel_scale(gk_transform *t);

/* Program coords -> machine coords, applying scale, rotation, work offset,
 * local offset and G92. */
gk_point3 gk_transform_to_machine(const gk_transform *t,
                                  const gk_machine_state *s,
                                  gk_point3 program);

#ifdef __cplusplus
}
#endif

#endif
