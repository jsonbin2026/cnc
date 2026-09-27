#ifndef GK_PHYSICS_H
#define GK_PHYSICS_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_PHYS_MAX_MODES 16
#define GK_PHYS_MAX_MATERIALS 32
#define GK_PHYS_MAX_LOBE 128

/* ---- 581/582 chatter and regenerative chatter ---- */

typedef struct {
    double stiffness;     /* N/m */
    double damping;
    double mass;          /* kg */
    double natural_freq;  /* Hz */
} gk_structure;

void gk_structure_init(gk_structure *s, double mass, double stiffness,
                       double damping);
double gk_structure_natural_freq(const gk_structure *s);
double gk_structure_damping_ratio(const gk_structure *s);

/* chatter stability: maximum stable depth of cut (mm). */
typedef enum {
    GK_CHATTER_TURNING = 0,
    GK_CHATTER_MILLING
} gk_chatter_kind;

double gk_chatter_stability_limit(const gk_structure *s, double cutting_coeff,
                                  double spindle_rpm, int teeth,
                                  gk_chatter_kind kind);
/* 582 regenerative chatter growth rate (positive = unstable). */
double gk_chatter_growth_rate(const gk_structure *s, double depth,
                              double cutting_coeff, double width);

/* ---- 583 stability lobe diagram ---- */

typedef struct {
    double rpm[GK_PHYS_MAX_LOBE];
    double depth[GK_PHYS_MAX_LOBE];
    int count;
} gk_lobe;

void gk_lobe_init(gk_lobe *l);
/* generate a lobe for the given lobe number using the structure dynamics. */
int gk_lobe_generate(gk_lobe *l, const gk_structure *s, double cutting_coeff,
                     int teeth, int lobe_number);
double gk_lobe_max_depth(const gk_lobe *l);
int gk_lobe_safe_at(const gk_lobe *l, double rpm, double depth);

/* ---- 584 FEA workpiece deformation ---- */

typedef struct {
    double length;
    double width;
    double height;
    double youngs;
    double poisson;
} gk_workpiece;

void gk_workpiece_init(gk_workpiece *w, double l, double wd, double h,
                       double e, double nu);
/* cantilever deflection under force at the given overhang. */
double gk_workpiece_deflection(const gk_workpiece *w, double force,
                               double overhang);
double gk_workpiece_stiffness(const gk_workpiece *w);

/* ---- 585 thermal-mechanical coupling / 586 spindle growth / 587 screw ---- */

typedef struct {
    double alpha;       /* 1/K */
    double length;
    double temp;
    double ref_temp;
    double conductivity;
} gk_thermo;

void gk_thermo_init(gk_thermo *t, double alpha, double length,
                    double ref_temp);
double gk_thermo_expansion(const gk_thermo *t);
double gk_thermo_coupled_strain(const gk_thermo *t, double stress,
                                double youngs);
/* spindle thermal growth over time (first-order lag). */
double gk_spindle_growth(double max_growth, double tau, double time);
/* ball screw thermal deformation with preload. */
double gk_screw_deformation(double alpha, double length, double delta_temp,
                            double preload, double area, double youngs);

/* ---- 588 modal analysis ---- */

typedef struct {
    double frequencies[GK_PHYS_MAX_MODES];
    double damping[GK_PHYS_MAX_MODES];
    int count;
} gk_modal;

void gk_modal_init(gk_modal *m);
gk_status gk_modal_add_mode(gk_modal *m, double freq, double damping);
double gk_modal_frf(const gk_modal *m, double freq);

/* ---- 589 servo flexibility ---- */

typedef struct {
    double bandwidth;
    double inertia;
    double gain;
    double resonance;
} gk_axis_servo;

void gk_axis_servo_init(gk_axis_servo *s, double bandwidth, double resonance);
double gk_axis_servo_phase_lag(const gk_axis_servo *s, double freq);
int gk_axis_servo_is_stable(const gk_axis_servo *s);

/* ---- 590-595 specialized drives ---- */

typedef enum {
    GK_DRIVE_AIR_BEARING = 0,   /* 590 */
    GK_DRIVE_HYDROSTATIC,       /* 591 */
    GK_DRIVE_MAGNETIC,          /* 592 */
    GK_DRIVE_LINEAR_MOTOR,      /* 593 */
    GK_DRIVE_VOICE_COIL,        /* 594 */
    GK_DRIVE_PIEZO,             /* 595 */
    GK_DRIVE_COUNT
} gk_drive_kind;

const char *gk_drive_name(gk_drive_kind k);

typedef struct {
    gk_drive_kind kind;
    double load_capacity;
    double stiffness;
    double max_speed;
    double force;
    int enabled;
} gk_drive;

gk_status gk_drive_init(gk_drive *d, gk_drive_kind kind);
double gk_drive_stiffness(const gk_drive *d);

/* ---- 596-600 assisted machining ---- */

typedef enum {
    GK_ASSIST_ULTRASONIC = 0,   /* 596 */
    GK_ASSIST_LASER,            /* 597 */
    GK_ASSIST_CRYOGENIC,        /* 598 */
    GK_ASSIST_MQL,              /* 599 */
    GK_ASSIST_HIGH_PRESSURE,    /* 600 */
    GK_ASSIST_COUNT
} gk_assist_kind;

const char *gk_assist_name(gk_assist_kind k);

typedef struct {
    gk_assist_kind kind;
    double frequency;      /* ultrasonic Hz */
    double power;          /* laser W */
    double temperature;    /* cryogenic C */
    double flow;           /* MQL ml/h */
    double pressure;       /* high pressure bar */
    int enabled;
} gk_assist;

gk_status gk_assist_init(gk_assist *a, gk_assist_kind kind);
/* return a cutting-force or tool-life factor (>1 helps, <1 hurts) */
double gk_assist_benefit(const gk_assist *a, double baseline);

/* ---- 601-612 material library ---- */

typedef enum {
    GK_MAT_CARBON_STEEL = 0,  /* 601 */
    GK_MAT_ALLOY_STEEL,       /* 602 */
    GK_MAT_STAINLESS,         /* 603 */
    GK_MAT_ALUMINUM,          /* 604 */
    GK_MAT_COPPER,            /* 605 */
    GK_MAT_TITANIUM,          /* 606 */
    GK_MAT_SUPERALLOY,        /* 607 */
    GK_MAT_CAST_IRON,         /* 608 */
    GK_MAT_PLASTIC,           /* 609 */
    GK_MAT_COMPOSITE,         /* 610 */
    GK_MAT_CERAMIC,           /* 611 */
    GK_MAT_COUNT
} gk_material_kind;

const char *gk_material_name(gk_material_kind k);

typedef struct {
    gk_material_kind kind;
    double density;         /* kg/m3 */
    double hardness_hb;
    double youngs_gpa;
    double vc;              /* recommended cutting speed m/min */
    double feed_per_tooth;  /* mm */
    double thermal_conductivity;
} gk_material;

const gk_material *gk_material_get(gk_material_kind k);

/* ---- 612 constitutive model / 613 Johnson-Cook ---- */

typedef struct {
    double A;      /* yield stress MPa */
    double B;      /* hardening MPa */
    double n;      /* hardening exponent */
    double C;      /* strain rate sensitivity */
    double m;      /* thermal softening */
    double melting;/* K */
} gk_johnson_cook;

void gk_jc_init(gk_johnson_cook *jc, double A, double B, double n, double C,
                double m, double melting);
/* flow stress for a given strain, strain rate, temperature */
double gk_jc_flow_stress(const gk_johnson_cook *jc, double strain,
                         double strain_rate, double temperature);

/* ---- 614 hardness distribution / 615 heat treatment ---- */

typedef enum {
    GK_HEAT_ANNEALED = 0,
    GK_HEAT_NORMALIZED,
    GK_HEAT_QUENCHED,
    GK_HEAT_TEMPERED,
    GK_HEAT_COUNT
} gk_heat_treatment;

const char *gk_heat_name(gk_heat_treatment h);

double gk_hardness_at_depth(double surface_hb, double core_hb, double depth,
                            double case_depth);
int gk_heat_affects_hardness(gk_heat_treatment h);

/* ---- 616-621 recommendation ---- */

typedef struct {
    double speed;      /* rpm */
    double feed;       /* mm/min */
    double depth;      /* mm */
    double width;      /* mm */
} gk_cut_params;

typedef struct {
    gk_material_kind material;
    double tool_diameter;
    int flutes;
    double hardness;
} gk_cut_input;

gk_status gk_recommend_speed(const gk_cut_input *in, gk_cut_params *out);
gk_status gk_recommend_feed(const gk_cut_input *in, gk_cut_params *out);
gk_status gk_recommend_depth(const gk_cut_input *in, gk_cut_params *out);
int gk_recommend_tool(const gk_cut_input *in, char *buf, size_t len);
int gk_recommend_cooling(const gk_cut_input *in, char *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* GK_PHYSICS_H */
