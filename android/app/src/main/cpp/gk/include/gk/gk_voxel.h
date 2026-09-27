#ifndef GK_VOXEL_H
#define GK_VOXEL_H

#include <stddef.h>
#include "gk/gk_error.h"
#include "gk/gk_math.h"
#include "gk/gk_mem.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char *cells;   /* 0 = removed, 1 = present */
    size_t nx;
    size_t ny;
    size_t nz;
    double cell_size;       /* mm per voxel edge */
    gk_point3 origin;       /* min corner, machine coords */
    gk_allocator alloc;
} gk_voxel_grid;

gk_status gk_voxel_init(gk_voxel_grid *g, size_t nx, size_t ny, size_t nz,
                        double cell_size, gk_point3 origin,
                        const gk_allocator *alloc);
void gk_voxel_destroy(gk_voxel_grid *g);
size_t gk_voxel_count(const gk_voxel_grid *g);
size_t gk_voxel_present_count(const gk_voxel_grid *g);
int gk_voxel_index(const gk_voxel_grid *g, size_t ix, size_t iy, size_t iz,
                   size_t *out_index);
int gk_voxel_get(const gk_voxel_grid *g, size_t ix, size_t iy, size_t iz);
gk_status gk_voxel_set(gk_voxel_grid *g, size_t ix, size_t iy, size_t iz,
                       int present);

/* Remove material swept by a sphere (cutting tool) travelling from a to b. */
gk_status gk_voxel_cut_segment(gk_voxel_grid *g, gk_point3 from, gk_point3 to,
                               double tool_radius, size_t *out_removed);
/* Remove material swept by a sphere at a single point. */
gk_status gk_voxel_cut_point(gk_voxel_grid *g, gk_point3 center,
                             double tool_radius, size_t *out_removed);

gk_aabb gk_voxel_bounds(const gk_voxel_grid *g);

#ifdef __cplusplus
}
#endif

#endif
