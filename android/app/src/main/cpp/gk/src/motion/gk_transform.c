#include "gk/gk_transform.h"

#include <math.h>
#include <string.h>

void gk_transform_init(gk_transform *t)
{
    int i;
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    for (i = 0; i < 6; ++i) {
        t->work_offsets[i] = gk_vec3_make(0.0, 0.0, 0.0);
    }
    t->scale = 1.0;
    t->rotation = 0.0;
    t->rotation_active = 0;
    t->scale_active = 0;
}

gk_status gk_transform_set_work_offset(gk_transform *t, int index,
                                       gk_point3 offset)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (index < 0 || index >= 6) {
        return GK_ERR_OUT_OF_RANGE;
    }
    t->work_offsets[index] = offset;
    return GK_OK;
}

gk_status gk_transform_get_work_offset(const gk_transform *t, int index,
                                       gk_point3 *out)
{
    if (t == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (index < 0 || index >= 6) {
        return GK_ERR_OUT_OF_RANGE;
    }
    *out = t->work_offsets[index];
    return GK_OK;
}

gk_status gk_transform_set_g92(gk_transform *t, gk_point3 offset)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    t->g92_offset = offset;
    return GK_OK;
}

gk_status gk_transform_set_local(gk_transform *t, gk_point3 offset)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    t->local_offset = offset;
    return GK_OK;
}

gk_status gk_transform_set_scale(gk_transform *t, double scale)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (fabs(scale) <= GK_EPS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    t->scale = scale;
    t->scale_active = 1;
    return GK_OK;
}

gk_status gk_transform_set_rotation(gk_transform *t, double deg,
                                    gk_point3 center)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    t->rotation = deg;
    t->rotation_center = center;
    t->rotation_active = 1;
    return GK_OK;
}

gk_status gk_transform_cancel_rotation(gk_transform *t)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    t->rotation = 0.0;
    t->rotation_active = 0;
    return GK_OK;
}

gk_status gk_transform_cancel_scale(gk_transform *t)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    t->scale = 1.0;
    t->scale_active = 0;
    return GK_OK;
}

gk_point3 gk_transform_to_machine(const gk_transform *t,
                                  const gk_machine_state *s,
                                  gk_point3 program)
{
    gk_point3 p = program;
    gk_point3 origin;

    if (t == NULL) {
        return p;
    }

    /* G51 scaling about the work origin. */
    if (t->scale_active && fabs(t->scale - 1.0) > GK_EPS) {
        p.x *= t->scale;
        p.y *= t->scale;
        p.z *= t->scale;
    }

    /* G68 rotation in the active plane (XY). */
    if (t->rotation_active && fabs(t->rotation) > GK_EPS) {
        double rad = GK_DEG2RAD(t->rotation);
        double c = cos(rad);
        double sn = sin(rad);
        double dx = p.x - t->rotation_center.x;
        double dy = p.y - t->rotation_center.y;
        p.x = t->rotation_center.x + dx * c - dy * sn;
        p.y = t->rotation_center.y + dx * sn + dy * c;
    }

    /* Work offset + local offset + G92. */
    origin = gk_vec3_make(0.0, 0.0, 0.0);
    if (s != NULL && s->work_offset >= 0 && s->work_offset < 6) {
        origin = gk_vec3_add(origin, t->work_offsets[s->work_offset]);
    }
    origin = gk_vec3_add(origin, t->local_offset);
    origin = gk_vec3_add(origin, t->g92_offset);
    p = gk_vec3_add(p, origin);
    return p;
}
