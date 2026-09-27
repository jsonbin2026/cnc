#include "gk_test.h"
#include "gk/gk_mscale.h"

#include <math.h>

static int near(double a, double b, double tol)
{
    return fabs(a - b) <= tol;
}

static void test_scales(void)
{
    int i;

    for (i = 0; i < GK_MS_SCALE_COUNT; i++) {
        const char *n = gk_ms_scale_name((gk_ms_scale)i);
        double len = gk_ms_scale_length((gk_ms_scale)i);
        double t = gk_ms_scale_time((gk_ms_scale)i);
        GK_CHECK(n != NULL);
        GK_CHECK(len > 0.0);
        GK_CHECK(t > 0.0);
    }
    GK_CHECK_STR_EQ(gk_ms_scale_name(GK_MS_SCALE_ATOM), "atomic");
    GK_CHECK_STR_EQ(gk_ms_scale_name(GK_MS_SCALE_SUPPLY), "supply-chain");
    GK_CHECK_STR_EQ(gk_ms_scale_name(GK_MS_SCALE_COUNT), "unknown");

    /* characteristic lengths increase from atomic through the hierarchy;
       the servo/control domain is a time scale, not a length scale, so it
       is compared only via the atomic-to-supply-chain progression */
    GK_CHECK(gk_ms_scale_length(GK_MS_SCALE_ATOM) <
             gk_ms_scale_length(GK_MS_SCALE_GRAIN));
    GK_CHECK(gk_ms_scale_length(GK_MS_SCALE_GRAIN) <
             gk_ms_scale_length(GK_MS_SCALE_WORKPIECE));
    GK_CHECK(gk_ms_scale_length(GK_MS_SCALE_WORKPIECE) <
             gk_ms_scale_length(GK_MS_SCALE_FACTORY));
    GK_CHECK(gk_ms_scale_length(GK_MS_SCALE_FACTORY) <
             gk_ms_scale_length(GK_MS_SCALE_SUPPLY));
    /* servo control period is a microsecond */
    GK_CHECK(near(gk_ms_scale_time(GK_MS_SCALE_SERVO), 1e-6, 1e-16));
    GK_CHECK(near(gk_ms_scale_time(GK_MS_SCALE_NANO), 1e-9, 1e-19));
}

static void test_servo(void)
{
    gk_ms_servo s;

    gk_ms_servo_init(&s, 100.0, 10.0, 1e-6);
    GK_CHECK(near(s.dt, 1e-6, 1e-18));
    s.setpoint = 1.0;
    GK_CHECK_EQ_INT(gk_ms_servo_run(&s, 0.001), GK_OK);
    /* servo should converge toward the setpoint */
    GK_CHECK(s.position > 0.0);
    GK_CHECK(s.position <= 1.0 + 1e-6);

    /* convergence over a longer horizon (critically damped: kv = 2*sqrt(kp)) */
    {
        gk_ms_servo s2;
        gk_ms_servo_init(&s2, 200.0, 2.0 * sqrt(200.0), 1e-5);
        s2.setpoint = 5.0;
        (void)gk_ms_servo_run(&s2, 5.0);
        GK_CHECK(near(s2.position, 5.0, 1e-3));
    }

    GK_CHECK_EQ_INT(gk_ms_servo_step(NULL), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_ms_servo_run(&s, 0.0), GK_ERR_INVALID_ARG);
    gk_ms_servo_init(NULL, 1.0, 1.0, 1e-6);
}

static void test_nano_atom(void)
{
    gk_ms_nano_cut n;
    gk_ms_atom a;

    gk_ms_nano_cut_init(&n, 20e3, 50.0); /* 20 kHz, 50 nm */
    GK_CHECK(near(n.period_ns, 1e9 / 20e3, 1e-6));
    GK_CHECK(near(n.vibration_hz, 20e3, 1e-9));
    /* faster feed -> longer effective contact time (until saturation) */
    {
        double t1 = gk_ms_nano_cut_time(&n, 0.05);
        double t2 = gk_ms_nano_cut_time(&n, 0.30);
        GK_CHECK(t1 >= 0.0);
        GK_CHECK(t2 > t1);
        GK_CHECK(t2 <= n.period_ns + 1e-9);
    }
    GK_CHECK(near(gk_ms_nano_cut_time(&n, 0.0), 0.0, 1e-12));
    GK_CHECK(near(gk_ms_nano_cut_time(NULL, 1.0), 0.0, 1e-12));

    gk_ms_atom_init(&a, 0.361);
    GK_CHECK(near(a.lattice_constant_nm, 0.361, 1e-9));
    GK_CHECK(gk_ms_atom_dislocation(&a, 0.01) > 0.0);
    GK_CHECK(near(gk_ms_atom_dislocation(&a, -1.0), 0.0, 1e-12));
    /* activation probability rises with temperature (lower energy barrier) */
    {
        double cold, hot;
        a.temperature_k = 300.0;
        cold = gk_ms_atom_activation(&a);
        a.temperature_k = 600.0;
        hot = gk_ms_atom_activation(&a);
        GK_CHECK(hot > cold);
        GK_CHECK(cold > 0.0 && cold < 1.0);
    }
    a.temperature_k = 0.0;
    GK_CHECK(near(gk_ms_atom_activation(&a), 0.0, 1e-12));
    GK_CHECK(near(gk_ms_atom_activation(NULL), 0.0, 1e-12));
    GK_CHECK(near(gk_ms_atom_dislocation(NULL, 1.0), 0.0, 1e-12));
    gk_ms_atom_init(NULL, 0.3);
}

static void test_grain_chip(void)
{
    gk_ms_grain g;
    gk_ms_chip c;

    gk_ms_grain_init(&g, 10.0);
    GK_CHECK(near(g.mean_diameter_um, 10.0, 1e-9));
    /* higher temperature causes more growth */
    {
        gk_ms_grain grow;
        gk_ms_grain_init(&grow, 10.0);
        GK_CHECK_EQ_INT(gk_ms_grain_grow(&g, 100.0, 500.0), GK_OK);
        GK_CHECK_EQ_INT(gk_ms_grain_grow(&grow, 100.0, 1000.0), GK_OK);
        GK_CHECK(grow.mean_diameter_um > g.mean_diameter_um);
    }
    GK_CHECK(g.grain_count > 0);
    GK_CHECK_EQ_INT(gk_ms_grain_grow(&g, -1.0, 500.0), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_ms_grain_grow(NULL, 1.0, 500.0), GK_ERR_INVALID_ARG);
    gk_ms_grain_init(NULL, 10.0);

    /* Hall-Petch: finer grain -> higher strength */
    {
        gk_ms_grain fine, coarse;
        double sf, sc;
        gk_ms_grain_init(&fine, 1.0);
        gk_ms_grain_init(&coarse, 100.0);
        sf = gk_ms_grain_hall_petch(&fine, 200.0, 300.0);
        sc = gk_ms_grain_hall_petch(&coarse, 200.0, 300.0);
        GK_CHECK(sf > sc);
        GK_CHECK(near(gk_ms_grain_hall_petch(NULL, 1.0, 1.0), 0.0, 1e-12));
    }

    /* Merchant shear angle: 45 degrees for zero rake and friction */
    GK_CHECK(near(gk_ms_chip_shear_angle(0.0, 0.0), 45.0, 1e-9));
    GK_CHECK(near(gk_ms_chip_shear_angle(10.0, 10.0), 45.0, 1e-9));

    gk_ms_chip_init(&c);
    GK_CHECK_EQ_INT(gk_ms_chip_estimate(&c, 100.0, 50.0, 5.0, 20.0, 600.0),
                    GK_OK);
    GK_CHECK(near(c.width_um, 100.0, 1e-9));
    GK_CHECK(near(c.thickness_um, 50.0, 1e-9));
    GK_CHECK(c.force_n > 0.0);
    GK_CHECK(c.shear_angle_deg > 0.0 && c.shear_angle_deg < 90.0);
    /* an obtuse Merchant angle is rejected */
    GK_CHECK_EQ_INT(gk_ms_chip_estimate(&c, 100.0, 50.0, -10.0, 100.0, 600.0),
                    GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_ms_chip_estimate(&c, 0.0, 50.0, 5.0, 20.0, 600.0),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_ms_chip_estimate(NULL, 1.0, 1.0, 0.0, 0.0, 1.0),
                    GK_ERR_INVALID_ARG);
    gk_ms_chip_init(NULL);
}

static void test_tool_tip_workpiece(void)
{
    gk_ms_tool_tip t;
    gk_ms_workpiece w;

    gk_ms_tool_tip_init(&t);
    GK_CHECK(near(t.edge_radius_um, 10.0, 1e-9));
    /* more speed -> faster wear */
    {
        double r1 = gk_ms_tool_tip_wear_rate(&t, 100.0);
        double r2 = gk_ms_tool_tip_wear_rate(&t, 300.0);
        GK_CHECK(r2 > r1);
        GK_CHECK(r1 > 0.0);
    }
    /* hotter edge -> faster wear */
    {
        double cold, hot;
        t.temperature_c = 25.0;
        cold = gk_ms_tool_tip_wear_rate(&t, 200.0);
        t.temperature_c = 500.0;
        hot = gk_ms_tool_tip_wear_rate(&t, 200.0);
        GK_CHECK(hot > cold);
        t.temperature_c = 25.0;
    }
    GK_CHECK(near(gk_ms_tool_tip_wear_rate(&t, -1.0), 0.0, 1e-12));
    GK_CHECK(near(gk_ms_tool_tip_wear_rate(NULL, 100.0), 0.0, 1e-12));

    GK_CHECK_EQ_INT(gk_ms_tool_tip_update(&t, 200.0, 1.0), GK_OK);
    GK_CHECK(t.wear_vb_um > 0.0);
    GK_CHECK(t.temperature_c > 25.0);
    GK_CHECK_EQ_INT(gk_ms_tool_tip_update(&t, 200.0, 0.0),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_ms_tool_tip_update(NULL, 200.0, 1.0),
                    GK_ERR_INVALID_ARG);
    gk_ms_tool_tip_init(NULL);

    gk_ms_workpiece_init(&w, 100.0, 50.0, 20.0);
    GK_CHECK(near(w.youngs_modulus_gpa, 200.0, 1e-9));
    {
        double d1 = gk_ms_workpiece_deflection(&w, 100.0);
        double d2 = gk_ms_workpiece_deflection(&w, 200.0);
        GK_CHECK(d1 > 0.0);
        GK_CHECK(near(d2, d1 * 2.0, 1e-9)); /* linear in force */
    }
    GK_CHECK(near(gk_ms_workpiece_deflection(&w, -1.0), 0.0, 1e-12));
    GK_CHECK(near(gk_ms_workpiece_deflection(NULL, 1.0), 0.0, 1e-12));
    gk_ms_workpiece_init(NULL, 1.0, 1.0, 1.0);
}

static void test_hierarchy(void)
{
    gk_ms_machine m;
    gk_ms_shop shop;
    gk_ms_factory_ms fac;
    gk_ms_supply sup;

    gk_ms_machine_init(&m, "VMC-01", 2);
    GK_CHECK_STR_EQ(m.name, "VMC-01");
    GK_CHECK_EQ_INT(m.spindles, 2);
    GK_CHECK(m.drives >= 2);
    GK_CHECK(m.power_kw > 0.0);
    gk_ms_machine_init(NULL, "x", 1);

    gk_ms_shop_init(&shop, "Shop A");
    m.utilization = 0.8;
    GK_CHECK_EQ_INT(gk_ms_shop_add_machine(&shop, &m), 1);
    GK_CHECK_EQ_INT(gk_ms_shop_add_machine(&shop, &m), 2);
    GK_CHECK_EQ_INT(shop.machine_count, 2);
    GK_CHECK_EQ_INT(gk_ms_shop_add_machine(NULL, &m), -1);
    GK_CHECK_EQ_INT(gk_ms_shop_add_machine(&shop, NULL), -1);
    GK_CHECK(near(gk_ms_shop_oee(&shop, 0.9, 0.9, 0.9), 0.729, 1e-9));
    {
        gk_ms_shop clamp_shop;
        gk_ms_shop_init(&clamp_shop, "Clamp");
        GK_CHECK(near(gk_ms_shop_oee(&clamp_shop, 2.0, 2.0, 2.0), 1.0, 1e-9));
    }
    GK_CHECK(near(gk_ms_shop_oee(NULL, 1.0, 1.0, 1.0), 0.0, 1e-12));
    gk_ms_shop_init(NULL, "x");

    gk_ms_factory_init(&fac, "Plant 1");
    GK_CHECK_EQ_INT(gk_ms_factory_add_shop(NULL, &shop), -1);
    GK_CHECK_EQ_INT(gk_ms_factory_add_shop(&fac, NULL), -1);
    /* the factory stores a copy, so pin the OEE before adding the shop */
    (void)gk_ms_shop_oee(&shop, 0.9, 0.9, 0.9);
    GK_CHECK_EQ_INT(gk_ms_factory_add_shop(&fac, &shop), 1);
    GK_CHECK_EQ_INT(fac.shop_count, 1);
    GK_CHECK(gk_ms_factory_output(&fac) > 0.0);
    GK_CHECK(near(gk_ms_factory_output(&fac), 0.8 * 2 * 0.729 * 10.0, 1e-9));
    GK_CHECK(near(gk_ms_factory_output(NULL), 0.0, 1e-12));
    gk_ms_factory_init(NULL, "x");

    gk_ms_supply_init(&sup, 3);
    GK_CHECK_EQ_INT(sup.nodes, 3);
    GK_CHECK(gk_ms_supply_bullwhip(&sup, 1.0) > 1.0);
    /* more echelons -> more bullwhip */
    {
        gk_ms_supply s2;
        gk_ms_supply_init(&s2, 1);
        GK_CHECK(gk_ms_supply_bullwhip(&sup, 1.0) >
                 gk_ms_supply_bullwhip(&s2, 1.0));
    }
    GK_CHECK(near(gk_ms_supply_bullwhip(&sup, -1.0), 0.0, 1e-12));
    GK_CHECK(gk_ms_supply_landed_cost(&sup, 10.0, 0.2, 100.0) > 10.0);
    /* a perfect service level removes the stockout penalty */
    {
        gk_ms_supply perfect;
        gk_ms_supply_init(&perfect, 3);
        perfect.service_level = 1.0;
        GK_CHECK(near(gk_ms_supply_landed_cost(&perfect, 10.0, 0.2, 100.0),
                      10.0 + perfect.cost_per_unit * 0.2 * 14.0 / 365.0,
                      1e-9));
    }
    GK_CHECK(near(gk_ms_supply_landed_cost(NULL, 1.0, 1.0, 1.0), 0.0, 1e-12));
    gk_ms_supply_init(NULL, 1);
}

static void test_thermal_drift(void)
{
    gk_ms_thermal_drift t;

    gk_ms_thermal_drift_init(&t, 12e-6, 1000.0);
    GK_CHECK(near(t.temperature_c, 20.0, 1e-9));
    GK_CHECK_EQ_INT(gk_ms_thermal_drift_update(&t, 0.0), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_ms_thermal_drift_update(&t, 1.0), GK_OK);
    GK_CHECK(t.temperature_c > 20.0);
    GK_CHECK(t.growth_um > 0.0);
    GK_CHECK(near(t.elapsed_h, 1.0, 1e-9));

    /* converges to the steady state */
    {
        int i;
        for (i = 0; i < 200; i++) {
            (void)gk_ms_thermal_drift_update(&t, 1.0);
        }
        GK_CHECK(near(t.growth_um, gk_ms_thermal_drift_steady(&t), 1e-3));
    }

    /* the steady-state growth is alpha * L * dT */
    GK_CHECK(near(gk_ms_thermal_drift_steady(&t),
                  12e-6 * 1000.0 * 1e3 * (3.0 * 2.0), 1e-6));
    GK_CHECK(near(gk_ms_thermal_drift_steady(NULL), 0.0, 1e-12));
    GK_CHECK_EQ_INT(gk_ms_thermal_drift_update(NULL, 1.0),
                    GK_ERR_INVALID_ARG);
    gk_ms_thermal_drift_init(NULL, 1e-6, 1.0);
}

int main(void)
{
    test_scales();
    test_servo();
    test_nano_atom();
    test_grain_chip();
    test_tool_tip_workpiece();
    test_hierarchy();
    test_thermal_drift();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
