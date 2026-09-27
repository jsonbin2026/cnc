#ifndef GK_MSCALE_H
#define GK_MSCALE_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_MS_NAME 64
#define GK_MS_MAX_ITEMS 64

/* ---- scale domain (789-799) ---- */

typedef enum {
    GK_MS_SCALE_SERVO = 0,     /* 789 microsecond servo */
    GK_MS_SCALE_NANO,          /* 790 nanosecond cutting */
    GK_MS_SCALE_ATOM,          /* 791 atomic */
    GK_MS_SCALE_GRAIN,         /* 792 grain */
    GK_MS_SCALE_CHIP,          /* 793 microscopic chip */
    GK_MS_SCALE_TOOL_TIP,      /* 794 tool tip */
    GK_MS_SCALE_WORKPIECE,     /* 795 workpiece */
    GK_MS_SCALE_MACHINE,       /* 796 machine */
    GK_MS_SCALE_SHOP,          /* 797 workshop */
    GK_MS_SCALE_FACTORY,       /* 798 factory */
    GK_MS_SCALE_SUPPLY,        /* 799 supply chain */
    GK_MS_SCALE_COUNT
} gk_ms_scale;

const char *gk_ms_scale_name(gk_ms_scale s);
/* characteristic length in metres for each domain */
double gk_ms_scale_length(gk_ms_scale s);
/* characteristic time in seconds for each domain */
double gk_ms_scale_time(gk_ms_scale s);

typedef struct {
    gk_ms_scale scale;
    double length_m;
    double time_s;
    double value;
} gk_ms_state;

/* ---- 789 microsecond servo simulation ---- */

typedef struct {
    double kp;
    double kv;
    double dt;             /* control period, seconds */
    double position;
    double velocity;
    double setpoint;
    double integral;
    double max_velocity;
    double max_accel;
    int saturated;
} gk_ms_servo;

void gk_ms_servo_init(gk_ms_servo *s, double kp, double kv, double dt);
gk_status gk_ms_servo_step(gk_ms_servo *s);
gk_status gk_ms_servo_run(gk_ms_servo *s, double duration);

/* ---- 790 nanosecond cutting ---- */

typedef struct {
    double vibration_hz;
    double amplitude_nm;
    double period_ns;
    double cutting_velocity;
} gk_ms_nano_cut;

void gk_ms_nano_cut_init(gk_ms_nano_cut *n, double frequency_hz,
                         double amplitude_nm);
/* effective cutting time (ns) within one vibration period at a feed speed */
double gk_ms_nano_cut_time(const gk_ms_nano_cut *n, double feed_mm_s);

/* ---- 791 atomic ---- */

typedef struct {
    int atoms;
    double lattice_constant_nm;
    double formation_energy_ev;
    double temperature_k;
} gk_ms_atom;

void gk_ms_atom_init(gk_ms_atom *a, double lattice_nm);
/* dislocation density estimate from strain and temperature */
double gk_ms_atom_dislocation(const gk_ms_atom *a, double strain);
/* thermal activation probability from Arrhenius */
double gk_ms_atom_activation(const gk_ms_atom *a);

/* ---- 792 grain ---- */

typedef struct {
    double mean_diameter_um;
    double sigma;
    int phase_count;
    int grain_count;
} gk_ms_grain;

void gk_ms_grain_init(gk_ms_grain *g, double mean_diameter_um);
gk_status gk_ms_grain_grow(gk_ms_grain *g, double time_s, double temp_c);
/* Hall-Petch strength: sigma_y = sigma0 + k / sqrt(d) */
double gk_ms_grain_hall_petch(const gk_ms_grain *g, double sigma0, double k);

/* ---- 793 microscopic chip ---- */

typedef struct {
    double thickness_um;
    double width_um;
    double curl_radius_um;
    double shear_angle_deg;
    double force_n;
} gk_ms_chip;

void gk_ms_chip_init(gk_ms_chip *c);
/* Merchant shear angle from rake angle and friction angle (degrees) */
double gk_ms_chip_shear_angle(double rake_deg, double friction_deg);
gk_status gk_ms_chip_estimate(gk_ms_chip *c, double depth_of_cut_um,
                              double feed_um, double rake_deg,
                              double friction_deg, double shear_strength);

/* ---- 794 tool tip ---- */

typedef struct {
    double edge_radius_um;
    double coating_thickness_um;
    double temperature_c;
    double wear_vb_um;
} gk_ms_tool_tip;

void gk_ms_tool_tip_init(gk_ms_tool_tip *t);
/* Taylor flank-wear rate dVB/dt from temperature and cutting speed */
double gk_ms_tool_tip_wear_rate(const gk_ms_tool_tip *t, double cutting_speed);
gk_status gk_ms_tool_tip_update(gk_ms_tool_tip *t, double cutting_speed,
                                double dt);

/* ---- 795 workpiece (multiscale variant) ---- */

typedef struct {
    double length_mm;
    double width_mm;
    double height_mm;
    double youngs_modulus_gpa;
    double poisson;
} gk_ms_workpiece;

void gk_ms_workpiece_init(gk_ms_workpiece *w, double l, double wd, double h);
/* thin-wall static deflection under a cutting force at mid-span */
double gk_ms_workpiece_deflection(const gk_ms_workpiece *w, double force_n);

/* ---- 796-799 factory hierarchy ---- */

typedef struct {
    char name[GK_MS_NAME];
    int drives;
    int spindles;
    double utilization;   /* 0..1 */
    double power_kw;
} gk_ms_machine;

typedef struct {
    char name[GK_MS_NAME];
    gk_ms_machine machines[GK_MS_MAX_ITEMS];
    int machine_count;
    double oee;           /* 0..1 */
} gk_ms_shop;

void gk_ms_machine_init(gk_ms_machine *m, const char *name, int spindles);
void gk_ms_shop_init(gk_ms_shop *s, const char *name);
int gk_ms_shop_add_machine(gk_ms_shop *s, const gk_ms_machine *m);
/* overall equipment effectiveness from availability, performance, quality */
double gk_ms_shop_oee(gk_ms_shop *s, double availability, double performance,
                      double quality);

typedef struct {
    char name[GK_MS_NAME];
    gk_ms_shop shops[GK_MS_MAX_ITEMS];
    int shop_count;
    double daily_output;
} gk_ms_factory_ms;

void gk_ms_factory_init(gk_ms_factory_ms *f, const char *name);
int gk_ms_factory_add_shop(gk_ms_factory_ms *f, const gk_ms_shop *s);
/* aggregate daily output from all shops at a given rate per shop */
double gk_ms_factory_output(gk_ms_factory_ms *f);

typedef struct {
    int nodes;
    double lead_time_days;
    double inventory_days;
    double service_level;   /* 0..1 */
    double cost_per_unit;
} gk_ms_supply;

void gk_ms_supply_init(gk_ms_supply *s, int nodes);
/* bullwhip amplification factor for n echelons */
double gk_ms_supply_bullwhip(const gk_ms_supply *s, double demand_variance);
/* total landed cost from procurement, holding and stockout */
double gk_ms_supply_landed_cost(const gk_ms_supply *s, double procurement,
                                double holding_rate, double stockout_cost);

/* ---- 800 hourly thermal deformation ---- */

typedef struct {
    double coefficient;      /* thermal expansion, 1/K */
    double reference_temp_c;
    double ambient_c;
    double heating_rate_c_per_h;   /* machine warm-up rate */
    double time_constant_h;         /* exponential thermal time constant */
    double length_mm;               /* characteristic length */
    double elapsed_h;
    double temperature_c;
    double growth_um;
} gk_ms_thermal_drift;

void gk_ms_thermal_drift_init(gk_ms_thermal_drift *t, double coeff,
                              double length_mm);
/* advance the hourly thermal model by dt hours */
gk_status gk_ms_thermal_drift_update(gk_ms_thermal_drift *t, double dt_h);
/* steady-state growth (um) after settling */
double gk_ms_thermal_drift_steady(const gk_ms_thermal_drift *t);

#ifdef __cplusplus
}
#endif

#endif /* GK_MSCALE_H */
