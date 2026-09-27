#include "gk_test.h"

#include "gk/gk_action.h"

#include <math.h>

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

static void test_spindle(void)
{
    gk_spindle_axis s;
    gk_spindle_init(&s);
    GK_CHECK_EQ_INT(s.direction, 0);

    GK_CHECK_EQ_INT(gk_spindle_start(&s, 3000.0, 1), GK_OK);
    s.accel_rate = 1000.0;
    gk_spindle_update(&s, 1.0);
    GK_CHECK(near(s.current_rpm, 1000.0, 1e-9));
    GK_CHECK(!gk_spindle_is_up_to_speed(&s, 1.0));
    gk_spindle_update(&s, 10.0);
    GK_CHECK(near(s.current_rpm, 3000.0, 1e-9));
    GK_CHECK(gk_spindle_is_up_to_speed(&s, 1.0));

    GK_CHECK_EQ_INT(gk_spindle_stop(&s), GK_OK);
    s.decel_rate = 2000.0;
    gk_spindle_update(&s, 1.0);
    GK_CHECK(near(s.current_rpm, 1000.0, 1e-9));
    gk_spindle_update(&s, 10.0);
    GK_CHECK(near(s.current_rpm, 0.0, 1e-9));

    GK_CHECK_EQ_INT(gk_spindle_start(&s, 3000.0, -1), GK_OK);
    GK_CHECK_EQ_INT(s.direction, -1);
    GK_CHECK_EQ_INT(gk_spindle_start(&s, 3000.0, 0), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_spindle_start(&s, -1.0, 1), GK_ERR_INVALID_ARG);

    GK_CHECK_EQ_INT(gk_spindle_select_gear(&s, 500.0), GK_OK);
    GK_CHECK_EQ_INT(s.gear, 1);
    GK_CHECK_EQ_INT(gk_spindle_select_gear(&s, 6000.0), GK_OK);
    GK_CHECK_EQ_INT(s.gear, 3);
    GK_CHECK_EQ_INT(gk_spindle_select_gear(&s, 50000.0), GK_ERR_OUT_OF_RANGE);

    GK_CHECK_EQ_INT(gk_spindle_orient(&s, 90), GK_OK);
    GK_CHECK(s.orient && s.orient_angle == 90);
    GK_CHECK(near(s.current_rpm, 0.0, 1e-9));
    GK_CHECK_EQ_INT(gk_spindle_orient(&s, 45), GK_ERR_OUT_OF_RANGE);
}

static void test_magazine(void)
{
    gk_magazine m;
    gk_magazine_init(&m, GK_MAGAZINE_DISC, 12);
    GK_CHECK_EQ_INT(m.capacity, 12);
    GK_CHECK_STR_EQ(gk_magazine_kind_name(m.kind), "disc");
    GK_CHECK(gk_magazine_at_target(&m));

    GK_CHECK_EQ_INT(gk_magazine_select(&m, 3), GK_OK);
    GK_CHECK(!gk_magazine_at_target(&m));
    {
        int guard = 0;
        double deg = 0.0;
        while (!gk_magazine_at_target(&m) && guard < 12) {
            deg += gk_magazine_index_step(&m, 1.0);
            guard += 1;
        }
        GK_CHECK(deg > 0.0);
        GK_CHECK_EQ_INT(m.current_pocket, 3);
        GK_CHECK(gk_magazine_at_target(&m));
    }
    GK_CHECK_EQ_INT(gk_magazine_select(&m, 99), GK_ERR_OUT_OF_RANGE);

    /* chain magazine: sequential only */
    {
        gk_magazine c;
        gk_magazine_init(&c, GK_MAGAZINE_CHAIN, 20);
        GK_CHECK_STR_EQ(gk_magazine_kind_name(c.kind), "chain");
        GK_CHECK_EQ_INT(gk_magazine_select(&c, 5), GK_ERR_UNSUPPORTED);
        GK_CHECK_EQ_INT(gk_magazine_select(&c, 1), GK_OK);
    }
    GK_CHECK_STR_EQ(gk_magazine_kind_name(GK_MAGAZINE_UMBRELLA), "umbrella");
}

static void test_atc(void)
{
    gk_atc a;
    gk_atc_init(&a);
    GK_CHECK_EQ_INT(a.state, GK_ATC_IDLE);
    GK_CHECK_EQ_INT(gk_atc_start(&a, 7), GK_OK);
    GK_CHECK_EQ_INT(gk_atc_start(&a, 8), GK_ERR_STATE);
    /* run the full sequence */
    {
        int guard = 0;
        while (a.state != GK_ATC_DONE && a.state != GK_ATC_ERROR &&
               guard < 100) {
            gk_atc_update(&a, 0.6);
            guard += 1;
        }
    }
    GK_CHECK_EQ_INT(a.state, GK_ATC_DONE);
    GK_CHECK_EQ_INT(a.spindle_tool, 7);
    GK_CHECK_STR_EQ(gk_atc_state_name(a.state), "done");
    GK_CHECK_STR_EQ(gk_atc_state_name(GK_ATC_UNCLAMP), "unclamp");

    /* manual change */
    GK_CHECK_EQ_INT(gk_atc_manual_change(&a, 12), GK_OK);
    GK_CHECK_EQ_INT(a.spindle_tool, 12);
    GK_CHECK_EQ_INT(a.manual, 1);
}

static void test_tool_setter(void)
{
    gk_tool_setter t;
    gk_tool_setter_init(&t, GK_PROBE_CONTACT);
    GK_CHECK(t.tip_diameter > 0.0);
    {
        double m = gk_tool_setter_measure(&t, 2, 10.0);
        GK_CHECK(m < 10.0);
    }
    gk_tool_setter_init(&t, GK_PROBE_LASER);
    GK_CHECK(t.resolution < 0.001);
    GK_CHECK(near(gk_tool_setter_measure(&t, 9, 1.0), 0.0, 1e-9));
}

static void test_table(void)
{
    gk_table t;
    gk_table_init(&t);
    GK_CHECK_EQ_INT(gk_table_rotate(&t, 90.0), GK_OK);
    GK_CHECK(near(t.rotation, 90.0, 1e-9));
    gk_table_rotate(&t, 300.0);
    GK_CHECK(near(t.rotation, 30.0, 1e-9));
    gk_table_rotate(&t, -60.0);
    GK_CHECK(near(t.rotation, 330.0, 1e-9));

    GK_CHECK_EQ_INT(gk_table_tilt(&t, 45.0), GK_OK);
    GK_CHECK(near(t.tilt, 45.0, 1e-9));
    GK_CHECK_EQ_INT(gk_table_tilt(&t, 200.0), GK_ERR_OUT_OF_RANGE);
}

static void test_tailstock_steady(void)
{
    gk_tailstock ts;
    gk_tailstock_init(&ts);
    GK_CHECK_EQ_INT(gk_tailstock_move(&ts, 100.0), GK_OK);
    GK_CHECK(near(ts.position, 100.0, 1e-9));
    GK_CHECK_EQ_INT(gk_tailstock_clamp(&ts, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_tailstock_move(&ts, 50.0), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_tailstock_clamp(&ts, 0), GK_OK);
    GK_CHECK_EQ_INT(gk_tailstock_move(&ts, 50.0), GK_OK);
    GK_CHECK_EQ_INT(gk_tailstock_move(&ts, 9999.0), GK_ERR_OUT_OF_RANGE);

    {
        gk_steady_rest r;
        gk_steady_rest_init(&r);
        GK_CHECK(!r.engaged);
        GK_CHECK_EQ_INT(gk_steady_rest_engage(&r, 40.0), GK_OK);
        GK_CHECK(r.engaged && near(r.open, 40.0, 1e-9));
        GK_CHECK_EQ_INT(gk_steady_rest_engage(&r, 0.0), GK_ERR_INVALID_ARG);
        GK_CHECK_EQ_INT(gk_steady_rest_release(&r), GK_OK);
        GK_CHECK(!r.engaged);
    }
}

static void test_aux(void)
{
    gk_chip_conveyor c;
    gk_chip_conveyor_init(&c);
    gk_chip_conveyor_set(&c, 60.0);
    GK_CHECK(near(gk_chip_conveyor_update(&c, 2.0), 2.0, 1e-9));

    {
        gk_auto_door d;
        gk_auto_door_init(&d);
        gk_auto_door_set(&d, 1);
        gk_auto_door_update(&d, 0.5);
        GK_CHECK(near(d.position, 0.5, 1e-9));
        gk_auto_door_update(&d, 1.0);
        GK_CHECK(near(d.position, 1.0, 1e-9));
        gk_auto_door_set(&d, 0);
        gk_auto_door_update(&d, 1.0);
        GK_CHECK(near(d.position, 0.0, 1e-9));
    }

    {
        gk_auto_fixture f;
        gk_auto_fixture_init(&f);
        GK_CHECK_EQ_INT(gk_auto_fixture_set(&f, 1, 5000.0), GK_OK);
        GK_CHECK(f.clamped && near(f.clamp_force, 5000.0, 1e-9));
        GK_CHECK(near(f.stroke, 1.0, 1e-9));
        gk_auto_fixture_set(&f, 0, 0.0);
        GK_CHECK(!f.clamped && near(f.clamp_force, 0.0, 1e-9));
        GK_CHECK_EQ_INT(gk_auto_fixture_set(&f, 1, -1.0), GK_ERR_INVALID_ARG);
    }

    {
        gk_coolant_valve v;
        gk_coolant_valve_init(&v);
        gk_coolant_valve_set(&v, 1, 12.0);
        GK_CHECK(v.on && near(v.flow, 12.0, 1e-9));
        gk_coolant_valve_set(&v, 0, 12.0);
        GK_CHECK(!v.on && near(v.flow, 0.0, 1e-9));
    }

    {
        gk_air_blast a;
        gk_air_blast_init(&a);
        gk_air_blast_set(&a, 1, 6.0);
        GK_CHECK(a.on && near(a.pressure, 6.0, 1e-9));
        gk_air_blast_set(&a, 0, 6.0);
        GK_CHECK(near(a.pressure, 0.0, 1e-9));
    }

    {
        gk_work_light l;
        gk_work_light_init(&l);
        gk_work_light_set(&l, 1, 2.0);
        GK_CHECK(l.on && near(l.intensity, 1.0, 1e-9));
        gk_work_light_set(&l, 0, 0.5);
        GK_CHECK(!l.on && near(l.intensity, 0.0, 1e-9));
    }
}

static void test_beacon_buzzer(void)
{
    gk_beacon b;
    gk_beacon_init(&b);
    GK_CHECK_EQ_INT(b.color, GK_BEACON_OFF);
    gk_beacon_set(&b, GK_BEACON_RED, 1);
    GK_CHECK_EQ_INT(b.color, GK_BEACON_RED);
    GK_CHECK(b.blink);
    GK_CHECK_STR_EQ(gk_beacon_color_name(GK_BEACON_GREEN), "green");
    GK_CHECK_STR_EQ(gk_beacon_color_name((gk_beacon_color)99), "unknown");

    {
        gk_buzzer z;
        gk_buzzer_init(&z);
        GK_CHECK(near(z.frequency, 1000.0, 1e-9));
        gk_buzzer_set(&z, 1, 2000.0, 1.5);
        GK_CHECK(z.on && near(z.frequency, 2000.0, 1e-9));
        GK_CHECK(near(z.volume, 1.0, 1e-9));
    }
}

int main(void)
{
    test_spindle();
    test_magazine();
    test_atc();
    test_tool_setter();
    test_table();
    test_tailstock_steady();
    test_aux();
    test_beacon_buzzer();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
