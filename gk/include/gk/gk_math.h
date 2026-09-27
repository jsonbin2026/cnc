#ifndef GK_MATH_H
#define GK_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GK_PI
#define GK_PI 3.14159265358979323846
#endif

#define GK_DEG2RAD(d) ((d) * (GK_PI / 180.0))
#define GK_RAD2DEG(r) ((r) * (180.0 / GK_PI))

#define GK_EPS 1e-9

double gk_clamp(double v, double lo, double hi);
double gk_lerp(double a, double b, double t);
int gk_approx_eq(double a, double b, double eps);
double gk_normalize_angle(double deg);
double gk_angle_diff(double a, double b);

typedef struct {
    double x;
    double y;
    double z;
} gk_vec3;

typedef gk_vec3 gk_point3;

typedef struct {
    double x;
    double y;
} gk_vec2;

gk_vec2 gk_vec2_make(double x, double y);
double gk_vec2_length(gk_vec2 v);
gk_vec2 gk_vec2_normalize(gk_vec2 v);

gk_vec3 gk_vec3_make(double x, double y, double z);
gk_vec3 gk_vec3_add(gk_vec3 a, gk_vec3 b);
gk_vec3 gk_vec3_sub(gk_vec3 a, gk_vec3 b);
gk_vec3 gk_vec3_scale(gk_vec3 v, double s);
double gk_vec3_dot(gk_vec3 a, gk_vec3 b);
gk_vec3 gk_vec3_cross(gk_vec3 a, gk_vec3 b);
double gk_vec3_length(gk_vec3 v);
double gk_vec3_length_sq(gk_vec3 v);
double gk_vec3_distance(gk_vec3 a, gk_vec3 b);
gk_vec3 gk_vec3_normalize(gk_vec3 v);

typedef struct {
    gk_point3 min;
    gk_point3 max;
} gk_aabb;

gk_aabb gk_aabb_empty(void);
gk_aabb gk_aabb_from_points(gk_point3 a, gk_point3 b);
gk_aabb gk_aabb_expand(gk_aabb box, gk_point3 p);
int gk_aabb_contains(const gk_aabb *box, gk_point3 p);
int gk_aabb_overlaps(const gk_aabb *a, const gk_aabb *b);
int gk_aabb_is_valid(const gk_aabb *box);

#ifdef __cplusplus
}
#endif

#endif
