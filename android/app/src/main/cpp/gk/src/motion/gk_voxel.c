#include "gk/gk_voxel.h"

#include <math.h>
#include <string.h>

gk_status gk_voxel_init(gk_voxel_grid *g, size_t nx, size_t ny, size_t nz,
                        double cell_size, gk_point3 origin,
                        const gk_allocator *alloc)
{
    size_t total;
    if (g == NULL || nx == 0 || ny == 0 || nz == 0 || cell_size <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    if (nx > (size_t)-1 / ny || nx * ny > (size_t)-1 / nz) {
        return GK_ERR_OVERFLOW;
    }
    total = nx * ny * nz;
    memset(g, 0, sizeof(*g));
    g->alloc = alloc != NULL ? *alloc : gk_allocator_default();
    g->cells = gk_calloc(&g->alloc, total, sizeof(unsigned char));
    if (g->cells == NULL) {
        return GK_ERR_NO_MEMORY;
    }
    g->nx = nx;
    g->ny = ny;
    g->nz = nz;
    g->cell_size = cell_size;
    g->origin = origin;
    memset(g->cells, 1, total);
    return GK_OK;
}

void gk_voxel_destroy(gk_voxel_grid *g)
{
    if (g == NULL) {
        return;
    }
    gk_free(&g->alloc, g->cells);
    g->cells = NULL;
    g->nx = g->ny = g->nz = 0;
}

size_t gk_voxel_count(const gk_voxel_grid *g)
{
    if (g == NULL) {
        return 0;
    }
    return g->nx * g->ny * g->nz;
}

size_t gk_voxel_present_count(const gk_voxel_grid *g)
{
    size_t i;
    size_t n;
    size_t present = 0;
    if (g == NULL) {
        return 0;
    }
    n = gk_voxel_count(g);
    for (i = 0; i < n; ++i) {
        if (g->cells[i]) {
            present += 1;
        }
    }
    return present;
}

int gk_voxel_index(const gk_voxel_grid *g, size_t ix, size_t iy, size_t iz,
                   size_t *out_index)
{
    if (g == NULL || out_index == NULL) {
        return 0;
    }
    if (ix >= g->nx || iy >= g->ny || iz >= g->nz) {
        return 0;
    }
    *out_index = (iz * g->ny + iy) * g->nx + ix;
    return 1;
}

int gk_voxel_get(const gk_voxel_grid *g, size_t ix, size_t iy, size_t iz)
{
    size_t idx;
    if (!gk_voxel_index(g, ix, iy, iz, &idx)) {
        return 0;
    }
    return g->cells[idx] ? 1 : 0;
}

gk_status gk_voxel_set(gk_voxel_grid *g, size_t ix, size_t iy, size_t iz,
                       int present)
{
    size_t idx;
    if (g == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!gk_voxel_index(g, ix, iy, iz, &idx)) {
        return GK_ERR_OUT_OF_RANGE;
    }
    g->cells[idx] = present ? 1 : 0;
    return GK_OK;
}

gk_aabb gk_voxel_bounds(const gk_voxel_grid *g)
{
    gk_aabb box = gk_aabb_empty();
    if (g == NULL) {
        return box;
    }
    box.min = g->origin;
    box.max.x = g->origin.x + (double)g->nx * g->cell_size;
    box.max.y = g->origin.y + (double)g->ny * g->cell_size;
    box.max.z = g->origin.z + (double)g->nz * g->cell_size;
    return box;
}

/* Cut all voxels whose center lies within tool_radius of point p. */
static size_t cut_sphere(gk_voxel_grid *g, gk_point3 center, double radius)
{
    size_t removed = 0;
    long ix0, ix1, iy0, iy1, iz0, iz1;
    long ix, iy, iz;
    double r2 = radius * radius;

    if (g == NULL || radius <= 0.0) {
        return 0;
    }
    ix0 = (long)floor((center.x - radius - g->origin.x) / g->cell_size);
    ix1 = (long)ceil((center.x + radius - g->origin.x) / g->cell_size);
    iy0 = (long)floor((center.y - radius - g->origin.y) / g->cell_size);
    iy1 = (long)ceil((center.y + radius - g->origin.y) / g->cell_size);
    iz0 = (long)floor((center.z - radius - g->origin.z) / g->cell_size);
    iz1 = (long)ceil((center.z + radius - g->origin.z) / g->cell_size);

    if (ix0 < 0) ix0 = 0;
    if (iy0 < 0) iy0 = 0;
    if (iz0 < 0) iz0 = 0;
    if (ix1 >= (long)g->nx) ix1 = (long)g->nx - 1;
    if (iy1 >= (long)g->ny) iy1 = (long)g->ny - 1;
    if (iz1 >= (long)g->nz) iz1 = (long)g->nz - 1;

    for (iz = iz0; iz <= iz1; ++iz) {
        for (iy = iy0; iy <= iy1; ++iy) {
            for (ix = ix0; ix <= ix1; ++ix) {
                size_t idx;
                double cx = g->origin.x + ((double)ix + 0.5) * g->cell_size;
                double cy = g->origin.y + ((double)iy + 0.5) * g->cell_size;
                double cz = g->origin.z + ((double)iz + 0.5) * g->cell_size;
                double dx = cx - center.x;
                double dy = cy - center.y;
                double dz = cz - center.z;
                if (dx * dx + dy * dy + dz * dz > r2) {
                    continue;
                }
                if (!gk_voxel_index(g, (size_t)ix, (size_t)iy, (size_t)iz,
                                    &idx)) {
                    continue;
                }
                if (g->cells[idx]) {
                    g->cells[idx] = 0;
                    removed += 1;
                }
            }
        }
    }
    return removed;
}

gk_status gk_voxel_cut_point(gk_voxel_grid *g, gk_point3 center,
                             double tool_radius, size_t *out_removed)
{
    if (g == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (out_removed != NULL) {
        *out_removed = cut_sphere(g, center, tool_radius);
    } else {
        (void)cut_sphere(g, center, tool_radius);
    }
    return GK_OK;
}

gk_status gk_voxel_cut_segment(gk_voxel_grid *g, gk_point3 from, gk_point3 to,
                               double tool_radius, size_t *out_removed)
{
    double dist;
    size_t steps;
    size_t i;
    size_t removed = 0;
    if (g == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    dist = gk_vec3_distance(from, to);
    /* Step at most half a voxel to avoid gaps. */
    steps = (size_t)(dist / (g->cell_size * 0.5)) + 1;
    if (steps > 1000000) {
        steps = 1000000;
    }
    for (i = 0; i <= steps; ++i) {
        double t = steps == 0 ? 0.0 : (double)i / (double)steps;
        gk_point3 p;
        p.x = gk_lerp(from.x, to.x, t);
        p.y = gk_lerp(from.y, to.y, t);
        p.z = gk_lerp(from.z, to.z, t);
        removed += cut_sphere(g, p, tool_radius);
    }
    if (out_removed != NULL) {
        *out_removed = removed;
    }
    return GK_OK;
}
