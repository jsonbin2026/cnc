#ifndef GK_CUTD_H
#define GK_CUTD_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===================================================================
 * Batch 40: real cutting details (1061-1090)
 * Prefix: gk_cutd_
 * =================================================================== */

/* 1061 cutting entry impact */
typedef struct {
    double approach_speed;
    double material_factor;
    double impact_force;
    int registered;
} gk_cutd_entry;

void gk_cutd_entry_init(gk_cutd_entry *e, double material_factor);
gk_status gk_cutd_entry_engage(gk_cutd_entry *e, double approach_speed,
                               double engagement_area);
double gk_cutd_entry_signal(const gk_cutd_entry *e);

/* 1062 cutting exit burr / chipping */
typedef struct {
    double edge_angle_deg;
    double support_factor;
    double burr_height_um;
} gk_cutd_exit;

void gk_cutd_exit_init(gk_cutd_exit *e);
gk_status gk_cutd_exit_leave(gk_cutd_exit *e, double edge_angle_deg,
                             double support_factor);
double gk_cutd_exit_burr_height(const gk_cutd_exit *e);
int gk_cutd_exit_chips(const gk_cutd_exit *e, double threshold_um);

/* 1063 cutting force fluctuation */
typedef struct {
    double mean_n;
    double amplitude_n;
    double frequency_hz;
} gk_cutd_force_wave;

void gk_cutd_force_wave_init(gk_cutd_force_wave *w, double mean_n,
                             double amplitude_n, double frequency_hz);
double gk_cutd_force_wave_at(const gk_cutd_force_wave *w, double t);

/* 1064 cutting force jump (sudden change) */
typedef struct {
    double previous_n;
    double current_n;
    double jump_n;
    int detected;
} gk_cutd_force_jump;

void gk_cutd_force_jump_init(gk_cutd_force_jump *j);
gk_status gk_cutd_force_jump_update(gk_cutd_force_jump *j, double force_n,
                                    double threshold_n);
int gk_cutd_force_jump_detected(const gk_cutd_force_jump *j);

/* 1065 chatter */
typedef struct {
    double natural_hz;
    double excitation_hz;
    double damping;
    double amplitude;
} gk_cutd_chatter;

void gk_cutd_chatter_init(gk_cutd_chatter *c);
gk_status gk_cutd_chatter_excite(gk_cutd_chatter *c, double excitation_hz,
                                 double depth_mm);
double gk_cutd_chatter_gain(const gk_cutd_chatter *c);
int gk_cutd_chatter_unstable(const gk_cutd_chatter *c, double limit);

/* 1066 cutting noise */
typedef struct {
    double level_db;
    double base_db;
} gk_cutd_noise;

void gk_cutd_noise_init(gk_cutd_noise *n);
double gk_cutd_noise_from_speed(gk_cutd_noise *n, double cutting_speed);
double gk_cutd_noise_from_force(gk_cutd_noise *n, double force_n);

/* 1067 sparks */
typedef struct {
    double rate_per_s;
    double intensity;
    int active;
} gk_cutd_spark;

void gk_cutd_spark_init(gk_cutd_spark *s);
gk_status gk_cutd_spark_emit(gk_cutd_spark *s, double material_hardness,
                             double cutting_speed);
int gk_cutd_spark_visible(const gk_cutd_spark *s, double threshold);

/* 1068 smoke / mist */
typedef struct {
    double density;
    double coolant_effect;
} gk_cutd_smoke;

void gk_cutd_smoke_init(gk_cutd_smoke *s);
double gk_cutd_smoke_generate(gk_cutd_smoke *s, double heat_index,
                              double coolant_flow);

/* 1069 smell (peripheral output) */
typedef enum {
    GK_CUTD_SMELL_NONE = 0,
    GK_CUTD_SMELL_OIL,
    GK_CUTD_SMELL_BURN,
    GK_CUTD_SMELL_COOLANT
} gk_cutd_smell;

const char *gk_cutd_smell_name(gk_cutd_smell s);
gk_cutd_smell gk_cutd_smell_classify(double temperature_c, int coolant_used);

/* 1070 chip ejection / splash */
typedef struct {
    double velocity_m_s;
    double angle_deg;
    double range_mm;
} gk_cutd_chip_fly;

void gk_cutd_chip_fly_init(gk_cutd_chip_fly *f);
double gk_cutd_chip_fly_compute(gk_cutd_chip_fly *f, double spindle_rpm,
                                double diameter_mm);

/* 1071 chip accumulation */
typedef struct {
    double produced_g_s;
    double removed_g_s;
    double accumulated_g;
} gk_cutd_chip_pile;

void gk_cutd_chip_pile_init(gk_cutd_chip_pile *p);
gk_status gk_cutd_chip_pile_update(gk_cutd_chip_pile *p, double dt);
int gk_cutd_chip_pile_clear(gk_cutd_chip_pile *p, double capacity_g);

/* 1072 chip entanglement */
typedef struct {
    double length_mm;
    double curl_ratio;
    int tangled;
} gk_cutd_chip_tangle;

void gk_cutd_chip_tangle_init(gk_cutd_chip_tangle *t);
gk_status gk_cutd_chip_tangle_grow(gk_cutd_chip_tangle *t, double feed_mm);
int gk_cutd_chip_tangle_check(const gk_cutd_chip_tangle *t, double limit_mm);

/* 1073 chip breaking */
typedef struct {
    double critical_depth_mm;
    double feed_mm_rev;
    int broken;
} gk_cutd_chip_break;

void gk_cutd_chip_break_init(gk_cutd_chip_break *c);
int gk_cutd_chip_break_check(gk_cutd_chip_break *c, double depth_mm,
                             double feed_mm_rev);

/* 1074 chip colour (temperature) */
gk_status gk_cutd_chip_colour(double temperature_c, char *out, size_t out_cap);

/* 1075-1078 tool wear progression */
typedef enum {
    GK_CUTD_WEAR_NORMAL = 0,
    GK_CUTD_WEAR_RAPID,
    GK_CUTD_WEAR_CHIP,
    GK_CUTD_WEAR_BREAK
} gk_cutd_wear_kind;

const char *gk_cutd_wear_name(gk_cutd_wear_kind k);

typedef struct {
    double wear_mm;
    double normal_rate;
    double time_min;
    int broken;
} gk_cutd_tool_wear;

void gk_cutd_tool_wear_init(gk_cutd_tool_wear *w, double normal_rate);
gk_status gk_cutd_tool_wear_advance(gk_cutd_tool_wear *w, double dt_min,
                                    double load_factor);
gk_status gk_cutd_tool_wear_shock(gk_cutd_tool_wear *w, double shock_mm);
int gk_cutd_tool_wear_break_check(gk_cutd_tool_wear *w, double limit_mm);
int gk_cutd_tool_wear_chipped(const gk_cutd_tool_wear *w, double jump_mm);

/* 1079 tool red heat */
typedef struct {
    double temperature_c;
    double critical_c;
} gk_cutd_redheat;

void gk_cutd_redheat_init(gk_cutd_redheat *r, double critical_c);
double gk_cutd_redheat_update(gk_cutd_redheat *r, double heat_input,
                              double cooling, double dt);
int gk_cutd_redheat_critical(const gk_cutd_redheat *r);

/* 1080 surface pattern / feed marks */
typedef struct {
    double feed_mm_rev;
    double tool_radius_mm;
    double pitch_mm;
} gk_cutd_pattern;

void gk_cutd_pattern_init(gk_cutd_pattern *p);
double gk_cutd_pattern_pitch(gk_cutd_pattern *p, double feed_mm_rev);
double gk_cutd_pattern_ra(const gk_cutd_pattern *p);

/* 1081-1084 surface defects */
typedef enum {
    GK_CUTD_DEFECT_NONE = 0,
    GK_CUTD_DEFECT_BURN,      /* 1081 */
    GK_CUTD_DEFECT_SCRATCH,   /* 1082 */
    GK_CUTD_DEFECT_CHATTER,   /* 1083 */
    GK_CUTD_DEFECT_BURR       /* 1084 */
} gk_cutd_defect;

const char *gk_cutd_defect_name(gk_cutd_defect d);

typedef struct {
    gk_cutd_defect defect;
    double severity;
    int detected;
} gk_cutd_surface;

void gk_cutd_surface_init(gk_cutd_surface *s);
gk_status gk_cutd_surface_detect(gk_cutd_surface *s, double temperature_c,
                                 double vibration, double burr_um,
                                 double scratch_um);

/* 1085 dimensional drift */
typedef struct {
    double nominal_mm;
    double drift_mm;
    double drift_rate;
} gk_cutd_drift;

void gk_cutd_drift_init(gk_cutd_drift *d, double nominal_mm);
double gk_cutd_drift_update(gk_cutd_drift *d, double dt_min,
                            double wear_source);
double gk_cutd_drift_deviation(const gk_cutd_drift *d);

/* 1086-1087 thermal expansion / contraction */
typedef struct {
    double length_mm;
    double alpha_per_c;
    double temp_delta_c;
} gk_cutd_thermal;

void gk_cutd_thermal_init(gk_cutd_thermal *t, double length_mm,
                          double alpha_per_c);
double gk_cutd_thermal_expansion(gk_cutd_thermal *t, double temp_delta_c);
double gk_cutd_thermal_contraction(gk_cutd_thermal *t, double temp_delta_c);

/* 1088 residual stress distortion */
typedef struct {
    double stress_mpa;
    double modulus_mpa;
    double length_mm;
} gk_cutd_stress;

void gk_cutd_stress_init(gk_cutd_stress *s, double modulus_mpa,
                         double length_mm);
double gk_cutd_stress_distortion(gk_cutd_stress *s, double stress_mpa);

/* 1089-1090 tool deflection */
typedef struct {
    double force_n;
    double stiffness_n_mm;
    double deflection_mm;
    double recovered_mm;
} gk_cutd_deflect;

void gk_cutd_deflect_init(gk_cutd_deflect *d, double stiffness_n_mm);
double gk_cutd_deflect_apply(gk_cutd_deflect *d, double force_n);
double gk_cutd_deflect_release(gk_cutd_deflect *d, double recovery_ratio);

#ifdef __cplusplus
}
#endif

#endif /* GK_CUTD_H */
