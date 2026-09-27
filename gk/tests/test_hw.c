#include "gk_test.h"
#include "gk/gk_hw.h"

#include <math.h>
#include <string.h>

static void test_buzzer_estop(void)
{
    gk_hw_buzzer b;
    gk_hw_estop e;
    gk_hw_buzzer_init(&b);
    GK_CHECK(gk_hw_buzzer_beep(&b, 3000.0, 1.0) == GK_OK);
    GK_CHECK_EQ_INT(b.active, 1);
    GK_CHECK(fabs(gk_hw_buzzer_remaining(&b, 0.4) - 0.6) < 1e-9);
    GK_CHECK(fabs(gk_hw_buzzer_remaining(&b, 2.0)) < 1e-9);
    GK_CHECK(gk_hw_buzzer_beep(&b, 0.0, 1.0) == GK_ERR_INVALID_ARG);
    gk_hw_estop_init(&e);
    GK_CHECK_EQ_INT(gk_hw_estop_engaged(&e), 0);
    GK_CHECK(gk_hw_estop_press(&e, 10.0) == GK_OK);
    GK_CHECK_EQ_INT(gk_hw_estop_engaged(&e), 1);
    GK_CHECK(gk_hw_estop_release(&e, 0) == GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_hw_estop_engaged(&e), 1);
    GK_CHECK(gk_hw_estop_release(&e, 1) == GK_OK);
    GK_CHECK_EQ_INT(gk_hw_estop_engaged(&e), 0);
}

static void test_handle_window_wiper(void)
{
    gk_hw_handle h;
    gk_hw_window w;
    gk_hw_wiper wp;
    gk_hw_handle_init(&h, 120.0);
    GK_CHECK(gk_hw_handle_pull(&h, 45.0) == GK_OK);
    GK_CHECK_EQ_INT(h.open, 1);
    GK_CHECK(gk_hw_handle_pull(&h, 120.0) == GK_ERR_OUT_OF_RANGE);
    gk_hw_window_init(&w, 500.0, 400.0);
    GK_CHECK(fabs(gk_hw_window_view_area(&w) - 200000.0) < 1e-6);
    gk_hw_wiper_init(&wp);
    GK_CHECK(gk_hw_wiper_stroke(&wp) == GK_ERR_STATE);
    GK_CHECK(gk_hw_wiper_start(&wp) == GK_OK);
    GK_CHECK(gk_hw_wiper_stroke(&wp) == GK_OK);
    GK_CHECK_EQ_INT((int)wp.strokes, 1);
    GK_CHECK(gk_hw_wiper_stop(&wp) == GK_OK);
}

static void test_locks(void)
{
    gk_hw_lock l;
    gk_hw_door_sensor ds;
    gk_hw_safety_switch sw;
    gk_hw_lock_init(&l, "K123");
    GK_CHECK(gk_hw_lock_engage(&l) == GK_OK);
    GK_CHECK_EQ_INT(l.locked, 1);
    GK_CHECK(gk_hw_lock_disengage(&l, "WRONG") == GK_ERR_UNSUPPORTED);
    GK_CHECK_EQ_INT(l.locked, 1);
    GK_CHECK(gk_hw_lock_disengage(&l, "K123") == GK_OK);
    gk_hw_door_sensor_init(&ds);
    GK_CHECK(gk_hw_door_sensor_update(&ds, 0) == GK_OK);
    GK_CHECK_EQ_INT(ds.triggered, 1);
    gk_hw_safety_switch_init(&sw);
    GK_CHECK(gk_hw_safety_switch_set(&sw, 1, 1) == GK_OK);
    GK_CHECK_EQ_INT(gk_hw_safety_switch_consistent(&sw), 1);
    GK_CHECK_EQ_INT(gk_hw_safety_switch_ok(&sw), 1);
    GK_CHECK(gk_hw_safety_switch_set(&sw, 1, 0) == GK_OK);
    GK_CHECK_EQ_INT(gk_hw_safety_switch_consistent(&sw), 0);
    GK_CHECK_EQ_INT(gk_hw_safety_switch_ok(&sw), 0);
}

static void test_structure(void)
{
    gk_hw_panel p;
    gk_hw_base b;
    gk_hw_feet f;
    gk_hw_pad pad;
    gk_hw_level_bolt bolt;
    gk_hw_lifting_eye eye;
    gk_hw_fork_pocket fp;
    gk_hw_plate_bracket br;
    gk_hw_panel_init(&p, GK_HW_PANEL_SIDE, 1000.0, 2000.0, 2.0);
    GK_CHECK_STR_EQ(gk_hw_panel_name(p.kind), "side-panel");
    GK_CHECK(gk_hw_panel_mass(&p, 2700.0) > 10.0);
    GK_CHECK(gk_hw_panel_attach(&p, 0) == GK_OK);
    GK_CHECK_EQ_INT(p.attached, 0);
    gk_hw_base_init(&b, 3000.0, 2000.0, 300.0);
    GK_CHECK(fabs(gk_hw_base_footprint(&b) - 6000000.0) < 1e-3);
    gk_hw_feet_init(&f, 4);
    GK_CHECK_EQ_INT(gk_hw_feet_can_support(&f, 2000.0), 1);
    GK_CHECK_EQ_INT(gk_hw_feet_can_support(&f, 5000.0), 0);
    gk_hw_pad_init(&pad, 1000.0, 0.05);
    GK_CHECK(pad.natural_hz > 0.0);
    GK_CHECK(gk_hw_pad_transmissibility(&pad, 10.0) > 0.0);
    gk_hw_level_bolt_init(&bolt, 1.5);
    GK_CHECK(fabs(gk_hw_level_bolt_turn(&bolt, 4.0) - 6.0) < 1e-9);
    gk_hw_lifting_eye_init(&eye, 16.0, 1000.0);
    GK_CHECK_EQ_INT(gk_hw_lifting_eye_ok(&eye, 1500.0), 1);
    GK_CHECK_EQ_INT(gk_hw_lifting_eye_ok(&eye, 3000.0), 0);
    gk_hw_fork_pocket_init(&fp, 200.0, 60.0, 1200.0);
    GK_CHECK_EQ_INT(gk_hw_fork_pocket_accepts(&fp, 150.0, 50.0), 1);
    GK_CHECK_EQ_INT(gk_hw_fork_pocket_accepts(&fp, 250.0, 50.0), 0);
    gk_hw_plate_bracket_init(&br, 200.0, 100.0);
    GK_CHECK(gk_hw_plate_bracket_tilt(&br, 45.0) == GK_OK);
    GK_CHECK(gk_hw_plate_bracket_tilt(&br, 120.0) == GK_ERR_OUT_OF_RANGE);
}

static void test_spindle_drawbar(void)
{
    gk_hw_spindle s;
    gk_hw_drawbar d;
    gk_hw_spindle_init(&s);
    GK_CHECK(gk_hw_spindle_start(&s, 8000.0, 3.0) == GK_OK);
    GK_CHECK_EQ_INT(s.running, 1);
    GK_CHECK(gk_hw_spindle_start_jitter(&s) > 0.0);
    s.speed_rpm = 6000.0;
    GK_CHECK(gk_hw_spindle_coast_distance(&s, 5.0) > 0.0);
    GK_CHECK(gk_hw_spindle_shift(&s, 2) == GK_OK);
    GK_CHECK(gk_hw_spindle_shift(&s, 9) == GK_ERR_OUT_OF_RANGE);
    GK_CHECK(gk_hw_spindle_orient(&s) == GK_OK);
    GK_CHECK_EQ_INT(s.running, 0);
    GK_CHECK(gk_hw_spindle_air_blast(&s) == GK_OK);
    GK_CHECK(gk_hw_spindle_stop(&s) == GK_OK);
    gk_hw_drawbar_init(&d, 12000.0);
    GK_CHECK(gk_hw_drawbar_clamp(&d) == GK_OK);
    GK_CHECK_EQ_INT(d.clamped, 1);
    GK_CHECK(gk_hw_drawbar_release(&d) == GK_OK);
    GK_CHECK_EQ_INT(d.clamped, 0);
}

static void test_atc_magazine(void)
{
    gk_hw_atc a;
    gk_hw_magazine m;
    gk_hw_atc_init(&a);
    GK_CHECK_STR_EQ(gk_hw_atc_state_name(a.state), "idle");
    GK_CHECK(gk_hw_atc_step(&a) == GK_ERR_STATE);
    GK_CHECK(gk_hw_atc_request(&a, 5) == GK_OK);
    GK_CHECK(gk_hw_atc_step(&a) == GK_OK);
    GK_CHECK(a.state == GK_HW_ATC_GRIP);
    GK_CHECK(gk_hw_atc_step(&a) == GK_OK);
    GK_CHECK(gk_hw_atc_step(&a) == GK_OK);
    GK_CHECK(gk_hw_atc_step(&a) == GK_OK);
    GK_CHECK(a.state == GK_HW_ATC_DONE);
    GK_CHECK(gk_hw_atc_confirm(&a) == GK_OK);
    GK_CHECK_EQ_INT(a.tool_current, 5);
    gk_hw_magazine_init(&m, 16);
    GK_CHECK_EQ_INT(gk_hw_magazine_count(&m), 16);
    GK_CHECK(gk_hw_magazine_index(&m, 5) == GK_OK);
    GK_CHECK_EQ_INT(m.position, 5);
    GK_CHECK(gk_hw_magazine_index(&m, 99) == GK_ERR_OUT_OF_RANGE);
    GK_CHECK(gk_hw_magazine_lock(&m) == GK_OK);
    GK_CHECK(gk_hw_magazine_index(&m, 2) == GK_ERR_STATE);
}

static void test_setter_clamp(void)
{
    gk_hw_setter st;
    gk_hw_clamp c;
    gk_hw_tailstock t;
    gk_hw_steady sr;
    gk_hw_setter_init(&st);
    GK_CHECK(gk_hw_setter_contact(&st, 99.95) == GK_OK);
    GK_CHECK_EQ_INT(st.signalled, 1);
    GK_CHECK(fabs(gk_hw_setter_offset(&st, 100.0) + 0.05) < 1e-9);
    GK_CHECK(gk_hw_setter_retract(&st) == GK_OK);
    gk_hw_clamp_init(&c, 5000.0);
    GK_CHECK(gk_hw_clamp_close(&c, 6000.0) == GK_OK);
    GK_CHECK_EQ_INT(gk_hw_clamp_ok(&c), 1);
    GK_CHECK(gk_hw_clamp_close(&c, 4000.0) == GK_OK);
    GK_CHECK_EQ_INT(gk_hw_clamp_ok(&c), 0);
    GK_CHECK(gk_hw_clamp_open(&c) == GK_OK);
    gk_hw_tailstock_init(&t);
    GK_CHECK(gk_hw_tailstock_advance(&t, 20.0, 3000.0) == GK_OK);
    GK_CHECK_EQ_INT(t.engaged, 1);
    GK_CHECK(gk_hw_tailstock_retract(&t) == GK_OK);
    GK_CHECK_EQ_INT(t.engaged, 0);
    gk_hw_steady_init(&sr);
    GK_CHECK_EQ_INT(sr.jaws, 3);
    GK_CHECK(gk_hw_steady_clamp(&sr, 2000.0) == GK_OK);
    GK_CHECK_EQ_INT(sr.clamped, 1);
    GK_CHECK(gk_hw_steady_release(&sr) == GK_OK);
    GK_CHECK_EQ_INT(sr.clamped, 0);
}

static void test_conveyor_coolant(void)
{
    gk_hw_conveyor c;
    gk_hw_coolant cl;
    double removed = 0.0;
    gk_hw_conveyor_init(&c);
    GK_CHECK_STR_EQ(gk_hw_chip_state_name(c.state), "stopped");
    GK_CHECK(gk_hw_conveyor_start(&c, 3.0) == GK_OK);
    GK_CHECK(c.state == GK_HW_CHIP_FORWARD);
    GK_CHECK(gk_hw_conveyor_reverse(&c) == GK_OK);
    GK_CHECK(c.state == GK_HW_CHIP_REVERSE);
    GK_CHECK(gk_hw_conveyor_stop(&c) == GK_OK);
    gk_hw_coolant_init(&cl);
    GK_CHECK_EQ_INT(cl.running, 0);
    GK_CHECK(gk_hw_coolant_start(&cl) == GK_OK);
    GK_CHECK(cl.flow_lpm > 0.0);
    GK_CHECK(gk_hw_coolant_set_flow(&cl, 30.0) == GK_OK);
    GK_CHECK(fabs(cl.flow_lpm - 30.0) < 1e-9);
    GK_CHECK(gk_hw_coolant_stop(&cl) == GK_OK);
    GK_CHECK(gk_hw_air_gun_blow(0.6, 2.0, &removed) == GK_OK);
    GK_CHECK(removed > 0.0);
    GK_CHECK(gk_hw_air_gun_blow(0.6, 2.0, NULL) == GK_ERR_INVALID_ARG);
}

static void test_door_rotary_indexer(void)
{
    gk_hw_auto_door d;
    gk_hw_rotary r;
    gk_hw_indexer ix;
    gk_hw_auto_door_init(&d);
    GK_CHECK(gk_hw_auto_door_open(&d, 100.0) == GK_OK);
    GK_CHECK(fabs(d.open_pct - 100.0) < 1e-9);
    GK_CHECK(gk_hw_auto_door_open(&d, 150.0) == GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_hw_auto_door_stop_on_obstacle(&d, 40.0), 1);
    GK_CHECK(gk_hw_auto_door_close(&d, 0.0) == GK_ERR_STATE);
    gk_hw_rotary_init(&r, 100.0);
    GK_CHECK(gk_hw_rotary_rotate(&r, 45.0) == GK_ERR_STATE);
    GK_CHECK(gk_hw_rotary_unlock(&r) == GK_OK);
    GK_CHECK(gk_hw_rotary_rotate(&r, 45.0) == GK_OK);
    GK_CHECK(fabs(r.angle_deg - 45.0) < 1e-9);
    GK_CHECK(gk_hw_rotary_rotate(&r, 400.0) == GK_OK);
    GK_CHECK(r.angle_deg < 360.0);
    GK_CHECK(gk_hw_rotary_lock(&r) == GK_OK);
    gk_hw_indexer_init(&ix, 8);
    GK_CHECK_STR_EQ(gk_hw_indexer_name(), "indexing-table");
    GK_CHECK(gk_hw_indexer_index(&ix, 3) == GK_OK);
    GK_CHECK_EQ_INT(ix.position, 3);
    GK_CHECK(gk_hw_indexer_index(&ix, 9) == GK_ERR_OUT_OF_RANGE);
    GK_CHECK(gk_hw_indexer_lock(&ix) == GK_OK);
}

int main(void)
{
    test_buzzer_estop();
    test_handle_window_wiper();
    test_locks();
    test_structure();
    test_spindle_drawbar();
    test_atc_magazine();
    test_setter_clamp();
    test_conveyor_coolant();
    test_door_rotary_indexer();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
