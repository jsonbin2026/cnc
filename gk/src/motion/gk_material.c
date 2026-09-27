#include "gk/gk_material.h"

#include <math.h>
#include <string.h>

static double clamp01(double v)
{
    if (v < 0.0) {
        return 0.0;
    }
    if (v > 1.0) {
        return 1.0;
    }
    return v;
}

const char *gk_coolant_name(gk_coolant_type t)
{
    switch (t) {
    case GK_COOLANT_OFF:
        return "OFF";
    case GK_COOLANT_FLOOD:
        return "FLOOD";
    case GK_COOLANT_MIST:
        return "MIST";
    case GK_COOLANT_THROUGH:
        return "THROUGH";
    case GK_COOLANT_AIR:
        return "AIR";
    default:
        return "unknown";
    }
}

const char *gk_chip_form_name(gk_chip_form f)
{
    switch (f) {
    case GK_CHIP_CONTINUOUS:
        return "continuous";
    case GK_CHIP_SEGMENTED:
        return "segmented";
    case GK_CHIP_DISCONTINUOUS:
        return "discontinuous";
    case GK_CHIP_BUILT_UP:
        return "built-up";
    default:
        return "unknown";
    }
}

gk_status gk_coolant_jet_init(gk_coolant_jet *j, gk_coolant_type type,
                              gk_vec3 dir, double pressure, double flow)
{
    if (j == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    j->type = type;
    j->nozzle_dir = gk_vec3_normalize(dir);
    j->pressure = pressure;
    j->flow = flow;
    return GK_OK;
}

double gk_coolant_effectiveness(const gk_coolant_jet *j, gk_point3 tool,
                                gk_point3 cut_point)
{
    gk_vec3 aim;
    gk_vec3 to_cut;
    double dot;
    if (j == NULL || j->type == GK_COOLANT_OFF) {
        return 0.0;
    }
    aim = j->nozzle_dir;
    to_cut = gk_vec3_normalize(gk_vec3_sub(cut_point, tool));
    dot = gk_vec3_dot(aim, to_cut);
    if (dot < 0.0) {
        dot = 0.0;
    }
    {
        double base = 0.5;
        switch (j->type) {
        case GK_COOLANT_THROUGH:
            base = 1.0;
            break;
        case GK_COOLANT_FLOOD:
            base = 0.85;
            break;
        case GK_COOLANT_MIST:
            base = 0.55;
            break;
        case GK_COOLANT_AIR:
            base = 0.35;
            break;
        default:
            break;
        }
        return clamp01(base * (0.4 + 0.6 * dot));
    }
}

/* ---- height map ---- */

gk_status gk_height_map_init(gk_height_map *h, size_t nx, size_t ny,
                             double cell_size, double fill,
                             const gk_allocator *alloc)
{
    size_t total;
    size_t i;
    if (h == NULL || nx == 0 || ny == 0 || cell_size <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    memset(h, 0, sizeof(*h));
    h->alloc = alloc != NULL ? *alloc : gk_allocator_default();
    total = nx * ny;
    h->heights = (double *)gk_alloc(&h->alloc, total * sizeof(double));
    if (h->heights == NULL) {
        return GK_ERR_NO_MEMORY;
    }
    for (i = 0; i < total; ++i) {
        h->heights[i] = fill;
    }
    h->nx = nx;
    h->ny = ny;
    h->cell_size = cell_size;
    h->origin = gk_vec3_make(0, 0, 0);
    return GK_OK;
}

void gk_height_map_destroy(gk_height_map *h)
{
    if (h == NULL) {
        return;
    }
    if (h->heights != NULL) {
        gk_free(&h->alloc, h->heights);
    }
    h->heights = NULL;
    h->nx = h->ny = 0;
}

gk_status gk_height_map_at(const gk_height_map *h, size_t ix, size_t iy,
                           double *out)
{
    if (h == NULL || out == NULL || ix >= h->nx || iy >= h->ny) {
        return GK_ERR_OUT_OF_RANGE;
    }
    *out = h->heights[iy * h->nx + ix];
    return GK_OK;
}

int gk_height_map_cut(gk_height_map *h, size_t ix, size_t iy, double z)
{
    double *p;
    if (h == NULL || ix >= h->nx || iy >= h->ny) {
        return 0;
    }
    p = &h->heights[iy * h->nx + ix];
    if (z < *p) {
        *p = z;
        return 1;
    }
    return 0;
}

gk_status gk_height_map_cut_segment(gk_height_map *h, gk_point3 from,
                                    gk_point3 to, double radius)
{
    double dx, dy, dist, steps;
    size_t n, k;
    if (h == NULL || radius <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    dx = to.x - from.x;
    dy = to.y - from.y;
    dist = sqrt(dx * dx + dy * dy);
    steps = dist / (h->cell_size * 0.5);
    n = (size_t)(steps + 1.0);
    if (n < 1) {
        n = 1;
    }
    for (k = 0; k <= n; ++k) {
        double t = n == 0 ? 0.0 : (double)k / (double)n;
        double cx = from.x + dx * t;
        double cy = from.y + dy * t;
        double cell_radius = radius / h->cell_size;
        long r = (long)ceil(cell_radius);
        long ix0 = (long)((cx - h->origin.x) / h->cell_size - cell_radius);
        long iy0 = (long)((cy - h->origin.y) / h->cell_size - cell_radius);
        long ix1 = (long)((cx - h->origin.x) / h->cell_size + cell_radius);
        long iy1 = (long)((cy - h->origin.y) / h->cell_size + cell_radius);
        long ix, iy;
        for (iy = iy0; iy <= iy1; ++iy) {
            for (ix = ix0; ix <= ix1; ++ix) {
                double px, py, d2;
                if (ix < 0 || iy < 0 || (size_t)ix >= h->nx ||
                    (size_t)iy >= h->ny) {
                    continue;
                }
                px = h->origin.x + ((double)ix + 0.5) * h->cell_size;
                py = h->origin.y + ((double)iy + 0.5) * h->cell_size;
                d2 = (px - cx) * (px - cx) + (py - cy) * (py - cy);
                if (d2 <= radius * radius) {
                    gk_height_map_cut(h, (size_t)ix, (size_t)iy, from.z);
                }
            }
        }
        (void)r;
    }
    return GK_OK;
}

/* ---- CSG ---- */

gk_status gk_voxel_csg(gk_voxel_grid *dst, const gk_voxel_grid *other,
                       gk_csg_op op)
{
    size_t i;
    if (dst == NULL || other == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (dst->nx != other->nx || dst->ny != other->ny ||
        dst->nz != other->nz) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < gk_voxel_count(dst); ++i) {
        int a = dst->cells[i] ? 1 : 0;
        int b = other->cells[i] ? 1 : 0;
        int r = 0;
        switch (op) {
        case GK_CSG_UNION:
            r = a || b;
            break;
        case GK_CSG_INTERSECT:
            r = a && b;
            break;
        case GK_CSG_DIFFERENCE:
            r = a && !b;
            break;
        default:
            return GK_ERR_INVALID_ARG;
        }
        dst->cells[i] = (unsigned char)(r ? 1 : 0);
    }
    return GK_OK;
}

/* ---- surface quality ---- */

double gk_scallop_height(double tool_radius, double step_over)
{
    if (tool_radius <= 0.0) {
        return 0.0;
    }
    if (step_over >= 2.0 * tool_radius) {
        return tool_radius;
    }
    return tool_radius - sqrt(tool_radius * tool_radius -
                              step_over * step_over / 4.0);
}

gk_status gk_surface_roughness(double feed_per_tooth, double tool_radius,
                               double step_over, double nose_radius,
                               gk_surface_quality *out)
{
    double scallop;
    double ra;
    if (out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (tool_radius <= 0.0 || nose_radius <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    scallop = gk_scallop_height(tool_radius, step_over);
    /* cusp from feed marks: f^2 / (8 * nose_radius) */
    ra = (feed_per_tooth * feed_per_tooth) / (8.0 * nose_radius);
    out->scallop_height = scallop;
    out->step_over = step_over;
    out->ra = (ra + scallop * 0.25) * 1000.0; /* mm -> um */
    out->rz = out->ra * 4.0;
    return GK_OK;
}

double gk_burr_height(double exit_angle_deg, double depth_of_cut,
                      double tool_radius)
{
    double a = GK_DEG2RAD(exit_angle_deg);
    double f;
    if (tool_radius <= 0.0) {
        return 0.0;
    }
    f = fabs(sin(a)) * depth_of_cut;
    return f * 0.05;
}

/* ---- cut quality ---- */

int gk_overcut_detect(double target, double actual, double tolerance)
{
    return (actual < target - tolerance) ? 1 : 0;
}

int gk_undercut_detect(double target, double actual, double tolerance)
{
    return (actual > target + tolerance) ? 1 : 0;
}

gk_cut_check gk_cut_check_eval(double target, double actual,
                               double tolerance)
{
    gk_cut_check c;
    c.target = target;
    c.actual = actual;
    c.error = actual - target;
    c.tolerance = tolerance;
    return c;
}

/* ---- cutting force / wear ---- */

gk_status gk_cut_force_model(const gk_cut_force_params *p, double spindle_rpm,
                             gk_cut_force *out)
{
    double area;
    double ft;
    double vc;
    if (p == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (p->diameter <= 0.0 || p->flutes <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    area = p->depth_of_cut * p->feed_per_tooth;
    ft = p->kc * area;
    out->tangential = ft;
    out->radial = ft * 0.4;
    out->axial = ft * 0.25;
    vc = GK_PI * p->diameter * spindle_rpm / 1000.0; /* m/min */
    out->power_kw = out->tangential * vc / 60000.0;
    out->torque = out->tangential * (p->diameter * 0.5) / 1000.0;
    return GK_OK;
}

double gk_tool_wear(const gk_wear_model *m, double cutting_time,
                    double speed_factor)
{
    double wear;
    if (m == NULL || cutting_time < 0.0) {
        return 0.0;
    }
    wear = m->initial_wear + m->rate * cutting_time * speed_factor;
    if (wear > m->max_wear) {
        wear = m->max_wear;
    }
    return wear;
}

double gk_taylor_life(double cutting_speed, double c, double n)
{
    if (cutting_speed <= 0.0 || n <= 0.0) {
        return 0.0;
    }
    return pow(c / cutting_speed, 1.0 / n);
}

int gk_tool_life_alarm(double wear, const gk_wear_model *m)
{
    if (m == NULL) {
        return 0;
    }
    return wear >= m->max_wear ? 1 : 0;
}

int gk_tool_break_detect(double measured_force, double nominal_force,
                         double threshold_ratio)
{
    if (nominal_force <= 0.0) {
        return 0;
    }
    return (measured_force > nominal_force * threshold_ratio) ? 1 : 0;
}

gk_chip_form gk_chip_form_for(double ductility, double speed, double feed)
{
    if (ductility > 0.7 && speed < 60.0) {
        return GK_CHIP_BUILT_UP;
    }
    if (ductility > 0.6) {
        return GK_CHIP_CONTINUOUS;
    }
    if (ductility > 0.3) {
        return speed > 100.0 ? GK_CHIP_SEGMENTED : GK_CHIP_CONTINUOUS;
    }
    (void)feed;
    return GK_CHIP_DISCONTINUOUS;
}

double gk_chip_curl_radius(double depth_of_cut, double rake_angle_deg)
{
    double a = GK_DEG2RAD(rake_angle_deg);
    if (depth_of_cut <= 0.0) {
        return 0.0;
    }
    return depth_of_cut / (2.0 * (sin(a) + 0.1));
}

int gk_chip_breaks(double curl_radius, double thickness, double max_curvature)
{
    double curvature;
    if (curl_radius <= 0.0) {
        return 1;
    }
    curvature = thickness / curl_radius;
    return (curvature > max_curvature) ? 1 : 0;
}

double gk_bue_tendency(double speed, double temperature, double ductility)
{
    double s = 1.0 - clamp01(speed / 120.0);
    double t = clamp01((temperature - 200.0) / 400.0);
    return clamp01(ductility * (0.5 * s + 0.5 * t));
}

double gk_edge_radius_effect(double hone_radius, double depth_of_cut,
                             double rake_deg)
{
    double a;
    if (depth_of_cut <= 0.0) {
        return 0.0;
    }
    a = atan2(hone_radius, depth_of_cut);
    return GK_DEG2RAD(rake_deg) - a * 0.5;
}

double gk_hardness_at(double base_hardness, gk_point3 p, gk_point3 center,
                      double scale)
{
    double d;
    if (scale <= 0.0) {
        return base_hardness;
    }
    d = gk_vec3_distance(p, center) / scale;
    return base_hardness * (1.0 + 0.5 * exp(-d * d));
}

/* ---- thermal ---- */

double gk_thermal_expansion(const gk_thermal_model *m)
{
    if (m == NULL) {
        return 0.0;
    }
    return m->alpha * m->delta_t * m->length;
}

double gk_spindle_thermal_growth(double rise, double length, double alpha)
{
    return alpha * rise * length;
}

double gk_ballscrew_thermal_growth(double rise, double length, double alpha)
{
    return alpha * rise * length;
}

double gk_thermal_lag(double t0, double t_inf, double tau, double t)
{
    if (tau <= 0.0) {
        return t_inf;
    }
    return t_inf + (t0 - t_inf) * exp(-t / tau);
}

double gk_beam_deflection(double load, double length, double e, double inertia,
                          double x)
{
    double numer;
    if (e <= 0.0 || inertia <= 0.0 || length <= 0.0) {
        return 0.0;
    }
    numer = load * x * (length * length * length -
                        2.0 * length * x * x + x * x * x);
    return numer / (48.0 * e * inertia);
}

double gk_thermo_mech_strain(double alpha, double delta_t, double stress,
                             double e)
{
    double thermal = alpha * delta_t;
    double mech = (e > 0.0) ? stress / e : 0.0;
    return thermal + mech;
}

double gk_natural_freq(const gk_modal_mode *m)
{
    if (m == NULL || m->mass <= 0.0) {
        return 0.0;
    }
    return sqrt(m->stiffness / m->mass) / (2.0 * GK_PI);
}

double gk_mode_response(const gk_modal_mode *m, double excitation_freq,
                        double force)
{
    double wn, r, denom;
    if (m == NULL || m->stiffness <= 0.0) {
        return 0.0;
    }
    wn = gk_natural_freq(m) * 2.0 * GK_PI;
    r = excitation_freq / (wn > 0.0 ? wn : 1.0);
    denom = (1.0 - r * r) * (1.0 - r * r) +
            (2.0 * m->damping * r) * (2.0 * m->damping * r);
    if (denom <= 0.0) {
        return 0.0;
    }
    return force / (m->stiffness * sqrt(denom));
}

double gk_stability_lobe_depth(double kt, double damping, double stiffness,
                               double tooth_pass_freq)
{
    double limit;
    if (kt <= 0.0 || stiffness <= 0.0) {
        return 0.0;
    }
    limit = 2.0 * damping * stiffness / kt;
    if (tooth_pass_freq <= 0.0) {
        return limit;
    }
    return limit * (1.0 - exp(-tooth_pass_freq / 100.0));
}

int gk_chatter_detect(double vibration_amplitude, double baseline,
                      double threshold)
{
    return (vibration_amplitude > baseline * (1.0 + threshold)) ? 1 : 0;
}

double gk_servo_flex_error(double force, double stiffness, double damping,
                           double velocity)
{
    double static_err;
    if (stiffness <= 0.0) {
        return 0.0;
    }
    static_err = force / stiffness;
    return static_err + damping * velocity;
}

gk_spark_effect gk_spark_generate(double material_hardness, double rpm,
                                  double depth_of_cut)
{
    gk_spark_effect s;
    double h = clamp01(material_hardness / 60.0);
    double r = clamp01(rpm / 10000.0);
    s.intensity = clamp01(h * 0.6 + r * 0.2 + clamp01(depth_of_cut / 5.0) * 0.2);
    s.particle_count = 200.0 * s.intensity;
    s.lifetime = 0.3 + 0.7 * s.intensity;
    return s;
}

double gk_smoke_density(double coolant_effectiveness, double temperature,
                        double material_rate)
{
    double t = clamp01((temperature - 300.0) / 500.0);
    double c = clamp01(coolant_effectiveness);
    double m = clamp01(material_rate / 50.0);
    return clamp01((1.0 - c) * 0.5 + t * 0.3 + m * 0.2);
}
