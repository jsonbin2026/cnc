#include "gk/gk_scen.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const char *gk_scen_name(gk_scen_kind k)
{
    switch (k) {
    case GK_SCEN_AL_HS: return "aluminium-high-speed";
    case GK_SCEN_STEEL_HEAVY: return "steel-heavy";
    case GK_SCEN_STAINLESS: return "stainless";
    case GK_SCEN_TITANIUM: return "titanium";
    case GK_SCEN_SUPERALLOY: return "superalloy";
    case GK_SCEN_HARDENED: return "hardened-steel";
    case GK_SCEN_CAST_IRON: return "cast-iron";
    case GK_SCEN_COPPER: return "copper-electrode";
    case GK_SCEN_GRAPHITE: return "graphite";
    case GK_SCEN_COMPOSITE: return "composite";
    case GK_SCEN_PLASTIC: return "plastic";
    case GK_SCEN_CERAMIC: return "ceramic";
    case GK_SCEN_THIN_WALL: return "thin-wall";
    case GK_SCEN_DEEP_CAVITY: return "deep-cavity";
    case GK_SCEN_DEEP_HOLE: return "deep-hole";
    case GK_SCEN_MICRO_HOLE: return "micro-hole";
    case GK_SCEN_MIRROR: return "mirror-finish";
    case GK_SCEN_HIGH_PREC: return "high-precision";
    case GK_SCEN_HSM: return "hsm";
    case GK_SCEN_HARD_CUT: return "hard-cutting";
    case GK_SCEN_DRY: return "dry-cutting";
    case GK_SCEN_WET: return "wet-cutting";
    case GK_SCEN_MQL: return "mql";
    case GK_SCEN_CRYO: return "cryogenic";
    case GK_SCEN_HP_COOLANT: return "high-pressure-coolant";
    case GK_SCEN_THROUGH_COOL: return "through-spindle";
    case GK_SCEN_ULTRASONIC: return "ultrasonic";
    case GK_SCEN_LASER: return "laser-assisted";
    default: return "unknown";
    }
}

gk_status gk_scen_recommend(gk_scen_kind k, gk_scen_params *out)
{
    gk_scen_params p;
    if (out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    memset(&p, 0, sizeof(p));
    p.teeth = 4;
    p.is_wet = 1;
    switch (k) {
    case GK_SCEN_AL_HS:
        p.surface_speed = 800.0; p.feed_per_tooth = 0.15;
        p.depth_of_cut = 5.0; p.coolant_flow = 20.0; p.teeth = 3;
        break;
    case GK_SCEN_STEEL_HEAVY:
        p.surface_speed = 120.0; p.feed_per_tooth = 0.20;
        p.depth_of_cut = 8.0; p.coolant_flow = 40.0; p.teeth = 5;
        break;
    case GK_SCEN_STAINLESS:
        p.surface_speed = 90.0; p.feed_per_tooth = 0.10;
        p.depth_of_cut = 2.0; p.coolant_flow = 35.0; p.teeth = 4;
        break;
    case GK_SCEN_TITANIUM:
        p.surface_speed = 50.0; p.feed_per_tooth = 0.08;
        p.depth_of_cut = 1.5; p.coolant_flow = 50.0; p.teeth = 4;
        break;
    case GK_SCEN_SUPERALLOY:
        p.surface_speed = 40.0; p.feed_per_tooth = 0.06;
        p.depth_of_cut = 1.0; p.coolant_flow = 55.0; p.teeth = 4;
        break;
    case GK_SCEN_HARDENED:
        p.surface_speed = 80.0; p.feed_per_tooth = 0.05;
        p.depth_of_cut = 0.5; p.coolant_flow = 0.0; p.is_wet = 0; p.teeth = 6;
        break;
    case GK_SCEN_CAST_IRON:
        p.surface_speed = 150.0; p.feed_per_tooth = 0.12;
        p.depth_of_cut = 3.0; p.coolant_flow = 0.0; p.is_wet = 0; p.teeth = 4;
        break;
    case GK_SCEN_COPPER:
        p.surface_speed = 250.0; p.feed_per_tooth = 0.10;
        p.depth_of_cut = 1.0; p.coolant_flow = 15.0; p.teeth = 2;
        break;
    case GK_SCEN_GRAPHITE:
        p.surface_speed = 300.0; p.feed_per_tooth = 0.10;
        p.depth_of_cut = 2.0; p.coolant_flow = 0.0; p.is_wet = 0; p.teeth = 2;
        break;
    case GK_SCEN_COMPOSITE:
        p.surface_speed = 200.0; p.feed_per_tooth = 0.08;
        p.depth_of_cut = 1.5; p.coolant_flow = 0.0; p.is_wet = 0; p.teeth = 4;
        break;
    case GK_SCEN_PLASTIC:
        p.surface_speed = 400.0; p.feed_per_tooth = 0.15;
        p.depth_of_cut = 3.0; p.coolant_flow = 10.0; p.teeth = 2;
        break;
    case GK_SCEN_CERAMIC:
        p.surface_speed = 60.0; p.feed_per_tooth = 0.02;
        p.depth_of_cut = 0.3; p.coolant_flow = 0.0; p.is_wet = 0; p.teeth = 6;
        break;
    case GK_SCEN_THIN_WALL:
        p.surface_speed = 200.0; p.feed_per_tooth = 0.05;
        p.depth_of_cut = 0.5; p.coolant_flow = 20.0; p.teeth = 4;
        break;
    case GK_SCEN_DEEP_CAVITY:
        p.surface_speed = 150.0; p.feed_per_tooth = 0.04;
        p.depth_of_cut = 0.3; p.coolant_flow = 30.0; p.teeth = 4;
        break;
    case GK_SCEN_DEEP_HOLE:
        p.surface_speed = 80.0; p.feed_per_tooth = 0.05;
        p.depth_of_cut = 1.0; p.coolant_flow = 45.0; p.teeth = 2;
        break;
    case GK_SCEN_MICRO_HOLE:
        p.surface_speed = 40.0; p.feed_per_tooth = 0.005;
        p.depth_of_cut = 0.1; p.coolant_flow = 5.0; p.teeth = 2;
        break;
    case GK_SCEN_MIRROR:
        p.surface_speed = 500.0; p.feed_per_tooth = 0.02;
        p.depth_of_cut = 0.05; p.coolant_flow = 25.0; p.teeth = 2;
        break;
    case GK_SCEN_HIGH_PREC:
        p.surface_speed = 300.0; p.feed_per_tooth = 0.03;
        p.depth_of_cut = 0.2; p.coolant_flow = 20.0; p.teeth = 4;
        break;
    case GK_SCEN_HSM:
        p.surface_speed = 1000.0; p.feed_per_tooth = 0.10;
        p.depth_of_cut = 1.0; p.coolant_flow = 25.0; p.teeth = 4;
        break;
    case GK_SCEN_HARD_CUT:
        p.surface_speed = 150.0; p.feed_per_tooth = 0.03;
        p.depth_of_cut = 0.3; p.coolant_flow = 0.0; p.is_wet = 0; p.teeth = 6;
        break;
    case GK_SCEN_DRY:
        p.surface_speed = 150.0; p.feed_per_tooth = 0.10;
        p.depth_of_cut = 2.0; p.coolant_flow = 0.0; p.is_wet = 0; p.teeth = 4;
        break;
    case GK_SCEN_WET:
        p.surface_speed = 150.0; p.feed_per_tooth = 0.10;
        p.depth_of_cut = 2.0; p.coolant_flow = 40.0; p.teeth = 4;
        break;
    case GK_SCEN_MQL:
        p.surface_speed = 200.0; p.feed_per_tooth = 0.09;
        p.depth_of_cut = 2.0; p.coolant_flow = 0.05; p.teeth = 4;
        break;
    case GK_SCEN_CRYO:
        p.surface_speed = 100.0; p.feed_per_tooth = 0.08;
        p.depth_of_cut = 1.5; p.coolant_flow = 8.0; p.teeth = 4;
        break;
    case GK_SCEN_HP_COOLANT:
        p.surface_speed = 120.0; p.feed_per_tooth = 0.10;
        p.depth_of_cut = 2.0; p.coolant_flow = 60.0; p.teeth = 4;
        break;
    case GK_SCEN_THROUGH_COOL:
        p.surface_speed = 100.0; p.feed_per_tooth = 0.08;
        p.depth_of_cut = 3.0; p.coolant_flow = 45.0; p.teeth = 4;
        break;
    case GK_SCEN_ULTRASONIC:
        p.surface_speed = 60.0; p.feed_per_tooth = 0.03;
        p.depth_of_cut = 0.5; p.coolant_flow = 20.0; p.teeth = 2;
        break;
    case GK_SCEN_LASER:
        p.surface_speed = 200.0; p.feed_per_tooth = 0.05;
        p.depth_of_cut = 1.0; p.coolant_flow = 10.0; p.teeth = 4;
        break;
    default:
        return GK_ERR_OUT_OF_RANGE;
    }
    *out = p;
    return GK_OK;
}

double gk_scen_rpm(const gk_scen_params *p, double diameter_mm)
{
    if (p == NULL || diameter_mm <= 0.0) {
        return 0.0;
    }
    return p->surface_speed * 1000.0 / (M_PI * diameter_mm);
}

double gk_scen_feed_rate(const gk_scen_params *p, double diameter_mm)
{
    double n;
    if (p == NULL) {
        return 0.0;
    }
    n = gk_scen_rpm(p, diameter_mm);
    return n * p->feed_per_tooth * (double)p->teeth;
}

double gk_scen_mrr(const gk_scen_params *p, double diameter_mm, double ae,
                   double ap)
{
    double f;
    if (p == NULL) {
        return 0.0;
    }
    f = gk_scen_feed_rate(p, diameter_mm);
    /* mm^3/min */
    return f * ae * ap;
}

gk_status gk_scen_set_coolant(gk_scen_params *p, gk_scen_kind mode,
                              double flow)
{
    if (p == NULL || flow < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    switch (mode) {
    case GK_SCEN_DRY:
        p->is_wet = 0;
        p->coolant_flow = 0.0;
        break;
    case GK_SCEN_MQL:
        p->is_wet = 1;
        p->coolant_flow = flow > 0.0 ? flow : 0.05;
        break;
    default:
        p->is_wet = 1;
        p->coolant_flow = flow;
        break;
    }
    return GK_OK;
}

int gk_scen_needs_coolant(gk_scen_kind k)
{
    switch (k) {
    case GK_SCEN_DRY:
    case GK_SCEN_GRAPHITE:
    case GK_SCEN_CAST_IRON:
    case GK_SCEN_COMPOSITE:
    case GK_SCEN_HARDENED:
        return 0;
    default:
        return 1;
    }
}

int gk_scen_difficulty(gk_scen_kind k)
{
    switch (k) {
    case GK_SCEN_SUPERALLOY:
    case GK_SCEN_TITANIUM:
    case GK_SCEN_MICRO_HOLE:
    case GK_SCEN_MIRROR:
        return 5;
    case GK_SCEN_STAINLESS:
    case GK_SCEN_HARDENED:
    case GK_SCEN_CERAMIC:
    case GK_SCEN_DEEP_HOLE:
    case GK_SCEN_HARD_CUT:
        return 4;
    case GK_SCEN_COMPOSITE:
    case GK_SCEN_DEEP_CAVITY:
    case GK_SCEN_THIN_WALL:
    case GK_SCEN_HIGH_PREC:
    case GK_SCEN_ULTRASONIC:
    case GK_SCEN_LASER:
    case GK_SCEN_CRYO:
        return 3;
    case GK_SCEN_STEEL_HEAVY:
    case GK_SCEN_COPPER:
    case GK_SCEN_GRAPHITE:
        return 2;
    default:
        return 1;
    }
}

double gk_scen_expected_ra(gk_scen_kind k)
{
    switch (k) {
    case GK_SCEN_MIRROR: return 0.05;
    case GK_SCEN_HIGH_PREC: return 0.2;
    case GK_SCEN_MICRO_HOLE: return 0.4;
    case GK_SCEN_THIN_WALL: return 0.8;
    case GK_SCEN_AL_HS: return 0.8;
    case GK_SCEN_STAINLESS: return 1.2;
    case GK_SCEN_HARDENED: return 0.6;
    case GK_SCEN_TITANIUM: return 1.6;
    case GK_SCEN_STEEL_HEAVY: return 3.2;
    case GK_SCEN_CAST_IRON: return 2.5;
    default: return 1.6;
    }
}
