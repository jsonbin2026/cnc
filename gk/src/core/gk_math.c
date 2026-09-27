#include "gk/gk_math.h"

#include <math.h>

double gk_clamp(double v, double lo, double hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

double gk_lerp(double a, double b, double t)
{
    return a + (b - a) * t;
}

int gk_approx_eq(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

double gk_normalize_angle(double deg)
{
    double r = fmod(deg, 360.0);
    if (r < 0.0) {
        r += 360.0;
    }
    return r;
}

double gk_angle_diff(double a, double b)
{
    double d = gk_normalize_angle(a - b);
    if (d > 180.0) {
        d -= 360.0;
    }
    return d;
}

gk_vec3 gk_vec3_make(double x, double y, double z)
{
    gk_vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

gk_vec2 gk_vec2_make(double x, double y)
{
    gk_vec2 v;
    v.x = x;
    v.y = y;
    return v;
}

double gk_vec2_length(gk_vec2 v)
{
    return sqrt(v.x * v.x + v.y * v.y);
}

gk_vec2 gk_vec2_normalize(gk_vec2 v)
{
    double len = gk_vec2_length(v);
    if (len <= GK_EPS) {
        return gk_vec2_make(0.0, 0.0);
    }
    return gk_vec2_make(v.x / len, v.y / len);
}

gk_vec3 gk_vec3_add(gk_vec3 a, gk_vec3 b)
{
    return gk_vec3_make(a.x + b.x, a.y + b.y, a.z + b.z);
}

gk_vec3 gk_vec3_sub(gk_vec3 a, gk_vec3 b)
{
    return gk_vec3_make(a.x - b.x, a.y - b.y, a.z - b.z);
}

gk_vec3 gk_vec3_scale(gk_vec3 v, double s)
{
    return gk_vec3_make(v.x * s, v.y * s, v.z * s);
}

double gk_vec3_dot(gk_vec3 a, gk_vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

gk_vec3 gk_vec3_cross(gk_vec3 a, gk_vec3 b)
{
    return gk_vec3_make(a.y * b.z - a.z * b.y,
                        a.z * b.x - a.x * b.z,
                        a.x * b.y - a.y * b.x);
}

double gk_vec3_length_sq(gk_vec3 v)
{
    return gk_vec3_dot(v, v);
}

double gk_vec3_length(gk_vec3 v)
{
    return sqrt(gk_vec3_length_sq(v));
}

double gk_vec3_distance(gk_vec3 a, gk_vec3 b)
{
    return gk_vec3_length(gk_vec3_sub(a, b));
}

gk_vec3 gk_vec3_normalize(gk_vec3 v)
{
    double len = gk_vec3_length(v);
    if (len <= GK_EPS) {
        return gk_vec3_make(0.0, 0.0, 0.0);
    }
    return gk_vec3_scale(v, 1.0 / len);
}

gk_aabb gk_aabb_empty(void)
{
    gk_aabb box;
    box.min = gk_vec3_make(0.0, 0.0, 0.0);
    box.max = gk_vec3_make(0.0, 0.0, 0.0);
    return box;
}

gk_aabb gk_aabb_from_points(gk_point3 a, gk_point3 b)
{
    gk_aabb box;
    box.min.x = a.x < b.x ? a.x : b.x;
    box.min.y = a.y < b.y ? a.y : b.y;
    box.min.z = a.z < b.z ? a.z : b.z;
    box.max.x = a.x > b.x ? a.x : b.x;
    box.max.y = a.y > b.y ? a.y : b.y;
    box.max.z = a.z > b.z ? a.z : b.z;
    return box;
}

gk_aabb gk_aabb_expand(gk_aabb box, gk_point3 p)
{
    if (p.x < box.min.x) box.min.x = p.x;
    if (p.y < box.min.y) box.min.y = p.y;
    if (p.z < box.min.z) box.min.z = p.z;
    if (p.x > box.max.x) box.max.x = p.x;
    if (p.y > box.max.y) box.max.y = p.y;
    if (p.z > box.max.z) box.max.z = p.z;
    return box;
}

int gk_aabb_contains(const gk_aabb *box, gk_point3 p)
{
    if (box == 0) {
        return 0;
    }
    return p.x >= box->min.x && p.x <= box->max.x &&
           p.y >= box->min.y && p.y <= box->max.y &&
           p.z >= box->min.z && p.z <= box->max.z;
}

int gk_aabb_overlaps(const gk_aabb *a, const gk_aabb *b)
{
    if (a == 0 || b == 0) {
        return 0;
    }
    if (a->max.x < b->min.x || b->max.x < a->min.x) return 0;
    if (a->max.y < b->min.y || b->max.y < a->min.y) return 0;
    if (a->max.z < b->min.z || b->max.z < a->min.z) return 0;
    return 1;
}

int gk_aabb_is_valid(const gk_aabb *box)
{
    if (box == 0) {
        return 0;
    }
    return box->min.x <= box->max.x && box->min.y <= box->max.y &&
           box->min.z <= box->max.z;
}
