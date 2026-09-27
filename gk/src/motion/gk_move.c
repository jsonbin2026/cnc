#include "gk/gk_move.h"

#include <math.h>
#include <string.h>

static void set_point(gk_point3 *p, double x, double y, double z)
{
    p->x = x;
    p->y = y;
    p->z = z;
}

gk_status gk_move_linear(gk_move *m, gk_point3 from, gk_point3 to,
                         double feed, gk_motion_mode mode)
{
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (mode != GK_MOTION_RAPID && mode != GK_MOTION_LINEAR) {
        return GK_ERR_INVALID_ARG;
    }
    memset(m, 0, sizeof(*m));
    m->start = from;
    m->end = to;
    m->feed = feed;
    m->mode = mode;
    m->plane = GK_PLANE_XY;
    return GK_OK;
}

/* Map plane coordinates to full 3D. */
void gk_plane_point(gk_plane plane, double a, double b, double c,
                        gk_point3 *out)
{
    switch (plane) {
    case GK_PLANE_XY:
        set_point(out, a, b, c);
        break;
    case GK_PLANE_ZX:
        set_point(out, b, c, a);
        break;
    case GK_PLANE_YZ:
    default:
        set_point(out, c, a, b);
        break;
    }
}

/* Extract the two in-plane coordinates for a given plane. */
void gk_plane_uv(gk_plane plane, gk_point3 p, double *u, double *v)
{
    switch (plane) {
    case GK_PLANE_XY:
        *u = p.x;
        *v = p.y;
        break;
    case GK_PLANE_ZX:
        *u = p.z;
        *v = p.x;
        break;
    case GK_PLANE_YZ:
    default:
        *u = p.y;
        *v = p.z;
        break;
    }
}

gk_status gk_move_arc_ijk(gk_move *m, gk_point3 from, gk_point3 to,
                          gk_point3 center, double feed, gk_motion_mode mode,
                          gk_plane plane)
{
    double du, dv, cu, cv;
    double ru, rv;
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (mode != GK_MOTION_CW && mode != GK_MOTION_CCW) {
        return GK_ERR_INVALID_ARG;
    }
    memset(m, 0, sizeof(*m));
    m->start = from;
    m->end = to;
    m->center = center;
    m->feed = feed;
    m->mode = mode;
    m->plane = plane;
    m->use_radius = 0;

    gk_plane_uv(plane, from, &du, &dv);
    gk_plane_uv(plane, center, &cu, &cv);
    ru = du - cu;
    rv = dv - cv;
    m->radius = sqrt(ru * ru + rv * rv);
    if (m->radius <= GK_EPS) {
        return GK_ERR_INVALID_ARG;
    }
    m->normal_angle = atan2(rv, ru);
    return GK_OK;
}

gk_status gk_move_arc_radius(gk_move *m, gk_point3 from, gk_point3 to,
                             double radius, double feed,
                             gk_motion_mode mode, gk_plane plane)
{
    double au, av, bu, bv;
    double dx, dy, d;
    double h;
    double mx, my;
    double cx, cy;
    double r;
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (mode != GK_MOTION_CW && mode != GK_MOTION_CCW) {
        return GK_ERR_INVALID_ARG;
    }
    if (fabs(radius) <= GK_EPS) {
        return GK_ERR_INVALID_ARG;
    }
    memset(m, 0, sizeof(*m));
    m->start = from;
    m->end = to;
    m->feed = feed;
    m->mode = mode;
    m->plane = plane;
    m->use_radius = 1;
    m->radius_negative = radius < 0.0;

    gk_plane_uv(plane, from, &au, &av);
    gk_plane_uv(plane, to, &bu, &bv);
    dx = bu - au;
    dy = bv - av;
    d = sqrt(dx * dx + dy * dy);
    if (d <= GK_EPS) {
        return GK_ERR_INVALID_ARG;
    }
    r = fabs(radius);
    if (d > 2.0 * r + 1e-6) {
        return GK_ERR_OUT_OF_RANGE;
    }
    h = sqrt(r * r - (d * d) / 4.0);
    mx = (au + bu) / 2.0;
    my = (av + bv) / 2.0;
    /* Center offset perpendicular to the chord. Direction depends on the
     * arc sense and the sign of R. */
    {
        double nx = -dy / d;
        double ny = dx / d;
        int want_center_left = (mode == GK_MOTION_CW);
        if (m->radius_negative) {
            want_center_left = !want_center_left;
        }
        if (!want_center_left) {
            nx = -nx;
            ny = -ny;
        }
        cx = mx + nx * h;
        cy = my + ny * h;
    }
    gk_plane_point(plane, cx, cy, 0.0, &m->center);
    {
        /* Set the missing 3rd coordinate of the center from start. */
        switch (plane) {
        case GK_PLANE_XY:
            m->center.z = from.z;
            break;
        case GK_PLANE_ZX:
            m->center.y = from.y;
            break;
        case GK_PLANE_YZ:
        default:
            m->center.x = from.x;
            break;
        }
    }
    m->radius = r;
    {
        double ru = au - cx;
        double rv = av - cy;
        m->normal_angle = atan2(rv, ru);
    }
    return GK_OK;
}

double gk_move_length(const gk_move *m)
{
    if (m == NULL) {
        return 0.0;
    }
    if (m->mode == GK_MOTION_RAPID || m->mode == GK_MOTION_LINEAR) {
        return gk_vec3_distance(m->start, m->end);
    }
    if (m->mode == GK_MOTION_CW || m->mode == GK_MOTION_CCW) {
        double start_angle;
        double end_angle;
        double sweep;
        double su, sv, eu, ev, cu, cv;

        gk_plane_uv(m->plane, m->start, &su, &sv);
        gk_plane_uv(m->plane, m->end, &eu, &ev);
        gk_plane_uv(m->plane, m->center, &cu, &cv);
        start_angle = atan2(sv - cv, su - cu);
        end_angle = atan2(ev - cv, eu - cu);
        sweep = end_angle - start_angle;
        if (m->mode == GK_MOTION_CCW) {
            while (sweep <= 0.0) {
                sweep += 2.0 * GK_PI;
            }
        } else {
            while (sweep >= 0.0) {
                sweep -= 2.0 * GK_PI;
            }
        }
        return fabs(sweep) * m->radius;
    }
    return 0.0;
}

gk_status gk_interpolate_arc_point(const gk_move *m, double t,
                                   gk_point3 *out)
{
    double su, sv, cu, cv, eu, ev;
    double start_angle;
    double end_angle;
    double sweep;
    double angle;
    double u, v;
    if (m == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (m->mode != GK_MOTION_CW && m->mode != GK_MOTION_CCW) {
        return GK_ERR_INVALID_ARG;
    }
    t = gk_clamp(t, 0.0, 1.0);
    gk_plane_uv(m->plane, m->start, &su, &sv);
    gk_plane_uv(m->plane, m->end, &eu, &ev);
    gk_plane_uv(m->plane, m->center, &cu, &cv);
    start_angle = atan2(sv - cv, su - cu);
    end_angle = atan2(ev - cv, eu - cu);
    sweep = end_angle - start_angle;
    if (m->mode == GK_MOTION_CCW) {
        while (sweep <= 0.0) {
            sweep += 2.0 * GK_PI;
        }
    } else {
        while (sweep >= 0.0) {
            sweep -= 2.0 * GK_PI;
        }
    }
    angle = start_angle + sweep * t;
    u = cu + m->radius * cos(angle);
    v = cv + m->radius * sin(angle);

    switch (m->plane) {
    case GK_PLANE_XY:
        out->x = u;
        out->y = v;
        out->z = m->start.z + (m->end.z - m->start.z) * t;
        break;
    case GK_PLANE_ZX:
        out->z = u;
        out->x = v;
        out->y = m->start.y + (m->end.y - m->start.y) * t;
        break;
    case GK_PLANE_YZ:
    default:
        out->y = u;
        out->z = v;
        out->x = m->start.x + (m->end.x - m->start.x) * t;
        break;
    }
    return GK_OK;
}
