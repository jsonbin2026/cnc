#ifndef GK_MOVE_H
#define GK_MOVE_H

#include "gk/gk_motion.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Internal helpers shared by the motion planner and interpolator. */
void gk_plane_point(gk_plane plane, double a, double b, double c,
                    gk_point3 *out);
void gk_plane_uv(gk_plane plane, gk_point3 p, double *u, double *v);

#ifdef __cplusplus
}
#endif

#endif
