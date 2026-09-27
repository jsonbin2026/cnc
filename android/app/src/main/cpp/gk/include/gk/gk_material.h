#ifndef GK_MATERIAL_H
#define GK_MATERIAL_H

#include <stddef.h>
#include "gk/gk_error.h"
#include "gk/gk_math.h"
#include "gk/gk_mem.h"
#include "gk/gk_voxel.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- coolant & chip ---- */

typedef enum {
    GK_COOLANT_OFF = 0,
    GK_COOLANT_FLOOD,
    GK_COOLANT_MIST,
    GK_COOLANT_THROUGH,
    GK_COOLANT_AIR
} gk_coolant_type;

const char *gk_coolant_name(gk_coolant_type t);

typedef enum {
    GK_CHIP_CONTINUOUS = 0,
    GK_CHIP_SEGMENTED,
    GK_CHIP_DISCONTINUOUS,
    GK_CHIP_BUILT_UP
} gk_chip_form;

const char *gk_chip_form_name(gk_chip_form f);

typedef struct {
    gk_coolant_type type;
    gk_vec3 nozzle_dir;    /* normalized jet direction */
    double pressure;       /* bar */
    double flow;           /* l/min */
} gk_coolant_jet;

gk_status gk_coolant_jet_init(gk_coolant_jet *j, gk_coolant_type type,
                              gk_vec3 dir, double pressure, double flow);
/* Effective cooling coefficient 0..1 given nozzle aim and tool position. */
double gk_coolant_effectiveness(const gk_coolant_jet *j, gk_point3 tool,
                                gk_point3 cut_point);

/* ---- height map ---- */

typedef struct {
    double *heights;
    size_t nx;
    size_t ny;
    double cell_size;
    gk_point3 origin;
    gk_allocator alloc;
} gk_height_map;

gk_status gk_height_map_init(gk_height_map *h, size_t nx, size_t ny,
                             double cell_size, double fill,
                             const gk_allocator *alloc);
void gk_height_map_destroy(gk_height_map *h);
gk_status gk_height_map_at(const gk_height_map *h, size_t ix, size_t iy,
                           double *out);
/* Cut down to z at (ix,iy) if lower than current; returns 1 if changed. */
int gk_height_map_cut(gk_height_map *h, size_t ix, size_t iy, double z);
/* Raster a line segment, applying the tool radius as a planar disc. */
gk_status gk_height_map_cut_segment(gk_height_map *h, gk_point3 from,
                                    gk_point3 to, double radius);

/* ---- CSG boolean on voxel grids ---- */

typedef enum {
    GK_CSG_UNION = 0,
    GK_CSG_INTERSECT,
    GK_CSG_DIFFERENCE
} gk_csg_op;

gk_status gk_voxel_csg(gk_voxel_grid *dst, const gk_voxel_grid *other,
                       gk_csg_op op);

/* ---- surface quality ---- */

typedef struct {
    double ra;             /* arithmetic mean roughness (um) */
    double rz;
    double scallop_height;
    double step_over;
} gk_surface_quality;

/* Theoretical scallop height for a ball tool, feed per tooth etc. */
double gk_scallop_height(double tool_radius, double step_over);
gk_status gk_surface_roughness(double feed_per_tooth, double tool_radius,
                               double step_over, double nose_radius,
                               gk_surface_quality *out);
/* Burr height estimate for a given edge exit angle. */
double gk_burr_height(double exit_angle_deg, double depth_of_cut,
                      double tool_radius);

/* ---- cut quality detection ---- */

typedef struct {
    double target;
    double actual;
    double error;          /* actual - target */
    double tolerance;
} gk_cut_check;

int gk_overcut_detect(double target, double actual, double tolerance);
int gk_undercut_detect(double target, double actual, double tolerance);
gk_cut_check gk_cut_check_eval(double target, double actual,
                               double tolerance);

/* ---- cutting force / wear ---- */

typedef struct {
    double kc;             /* specific cutting force N/mm^2 */
    double mc;             /* exponent */
    double feed_per_tooth; /* mm */
    double depth_of_cut;   /* mm */
    double width_of_cut;   /* mm */
    double diameter;       /* mm */
    int flutes;
} gk_cut_force_params;

typedef struct {
    double tangential;
    double radial;
    double axial;
    double power_kw;
    double torque;
} gk_cut_force;

gk_status gk_cut_force_model(const gk_cut_force_params *p, double spindle_rpm,
                             gk_cut_force *out);

typedef struct {
    double initial_wear;   /* mm */
    double rate;           /* mm per unit cutting time */
    double max_wear;       /* wear land threshold */
} gk_wear_model;

double gk_tool_wear(const gk_wear_model *m, double cutting_time,
                    double speed_factor);
/* Taylor tool life: V*T^n = C. Returns life in minutes. */
double gk_taylor_life(double cutting_speed, double c, double n);
int gk_tool_life_alarm(double wear, const gk_wear_model *m);
int gk_tool_break_detect(double measured_force, double nominal_force,
                         double threshold_ratio);

/* Compute chip form for given material/conditions. */
gk_chip_form gk_chip_form_for(double ductility, double speed, double feed);
double gk_chip_curl_radius(double depth_of_cut, double rake_angle_deg);
int gk_chip_breaks(double curl_radius, double thickness, double max_curvature);
double gk_bue_tendency(double speed, double temperature, double ductility);
/* Cutting-edge hone radius effect on effective rake (radians). */
double gk_edge_radius_effect(double hone_radius, double depth_of_cut,
                             double rake_deg);
/* Material hardness value at a work coordinate (gradient model). */
double gk_hardness_at(double base_hardness, gk_point3 p, gk_point3 center,
                      double scale);

/* ---- thermal / dynamic ---- */

typedef struct {
    double alpha;          /* thermal expansion 1/K */
    double delta_t;        /* temperature rise K */
    double length;         /* affected length mm */
} gk_thermal_model;

double gk_thermal_expansion(const gk_thermal_model *m);
/* Steady-state spindle axial growth (mm). */
double gk_spindle_thermal_growth(double rise, double length, double alpha);
/* Ball-screw thermal growth (mm). */
double gk_ballscrew_thermal_growth(double rise, double length, double alpha);
/* First-order thermal lag: T(t) = Tinf + (T0-Tinf) exp(-t/tau). */
double gk_thermal_lag(double t0, double t_inf, double tau, double t);

/* FEA-style workpiece deflection under a point load (simply-supported beam). */
double gk_beam_deflection(double load, double length, double e, double inertia,
                          double x);
/* Thermo-mechanical coupled strain. */
double gk_thermo_mech_strain(double alpha, double delta_t, double stress,
                             double e);

typedef struct {
    double frequency;      /* Hz */
    double damping;        /* damping ratio */
    double stiffness;
    double mass;
} gk_modal_mode;

double gk_natural_freq(const gk_modal_mode *m);
double gk_mode_response(const gk_modal_mode *m, double excitation_freq,
                        double force);

/* Regenerative chatter stability: critical depth of cut (mm). */
double gk_stability_lobe_depth(double kt, double damping, double stiffness,
                               double tooth_pass_freq);
int gk_chatter_detect(double vibration_amplitude, double baseline,
                      double threshold);

/* Servo compliance positional error (mm) under cutting force. */
double gk_servo_flex_error(double force, double stiffness, double damping,
                           double velocity);

/* ---- sparks / smoke ---- */

typedef struct {
    double intensity;      /* 0..1 */
    double particle_count;
    double lifetime;
} gk_spark_effect;

gk_spark_effect gk_spark_generate(double material_hardness, double rpm,
                                  double depth_of_cut);
double gk_smoke_density(double coolant_effectiveness, double temperature,
                        double material_rate);

#ifdef __cplusplus
}
#endif

#endif
