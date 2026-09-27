#include "gk_test.h"

#include "gk/gk_ui.h"

#include <math.h>
#include <string.h>

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

static void test_menus(void)
{
    gk_menu_bar b;
    gk_menu *file;
    gk_menu *view;

    GK_CHECK_EQ_INT(gk_menu_bar_init(&b), GK_OK);
    GK_CHECK_EQ_INT(b.count, GK_MENU_COUNT);
    file = gk_menu_bar_get(&b, GK_MENU_FILE);
    view = gk_menu_bar_get(&b, GK_MENU_VIEW);
    GK_CHECK(file != NULL && view != NULL);
    GK_CHECK_STR_EQ(file->title, "File");
    GK_CHECK_STR_EQ(view->title, "View");

    GK_CHECK_EQ_INT(gk_menu_add_item(file, "New", "Ctrl+N"), GK_OK);
    GK_CHECK_EQ_INT(gk_menu_add_item(file, "Open", "Ctrl+O"), GK_OK);
    GK_CHECK_EQ_INT(gk_menu_add_item(file, "New", ""), GK_ERR_ALREADY_EXISTS);
    GK_CHECK_EQ_INT(file->count, 2);
    GK_CHECK_EQ_INT(gk_menu_find(file, "Open"), 1);
    GK_CHECK_EQ_INT(gk_menu_find(file, "Save"), -1);

    GK_CHECK_EQ_INT(gk_menu_enable(file, "Open", 0), GK_OK);
    GK_CHECK(!file->items[1].enabled);
    GK_CHECK_EQ_INT(gk_menu_enable(file, "Save", 0), GK_ERR_NOT_FOUND);
    GK_CHECK(gk_menu_bar_get(&b, (gk_menu_kind)99) == NULL);
}

static void test_toolbar_status(void)
{
    gk_toolbar t;
    gk_status_bar s;
    memset(&t, 0, sizeof(t));
    memset(&s, 0, sizeof(s));

    GK_CHECK_EQ_INT(gk_toolbar_add(&t, "run", "Start cycle"), GK_OK);
    GK_CHECK_EQ_INT(gk_toolbar_add(&t, "save", "Save program"), GK_OK);
    GK_CHECK_EQ_INT(gk_toolbar_click(&t, "run"), 1);
    GK_CHECK_EQ_INT(gk_toolbar_click(&t, "run"), 0);
    GK_CHECK_EQ_INT(gk_toolbar_toggle(&t, "run"), 0);
    t.buttons[1].enabled = 0;
    GK_CHECK_EQ_INT(gk_toolbar_click(&t, "save"), -1);
    GK_CHECK_EQ_INT(gk_toolbar_click(&t, "missing"), -1);

    GK_CHECK_EQ_INT(gk_status_bar_set(&s, 0, "READY", GK_OK), GK_OK);
    GK_CHECK_EQ_INT(gk_status_bar_set(&s, -1, "AUTO", GK_OK), GK_OK);
    GK_CHECK_EQ_INT(s.count, 2);
    GK_CHECK_STR_EQ(gk_status_bar_text(&s, 0), "READY");
    GK_CHECK_STR_EQ(gk_status_bar_text(&s, 1), "AUTO");
    GK_CHECK(gk_status_bar_text(&s, 5) == NULL);
}

static void test_data_panel(void)
{
    gk_data_panel d;
    gk_data_panel_init(&d);
    d.machine[0] = 12.5;
    d.work[0] = 2.5;
    d.spindle_rpm = 3000.0;
    d.total_estimate = 100.0;

    gk_data_panel_update(&d, 25.0, 150.0);
    GK_CHECK(near(d.progress, 0.25, 1e-9));
    GK_CHECK(near(gk_data_panel_remaining(&d), 75.0, 1e-9));
    GK_CHECK_EQ_INT(gk_data_panel_progress_bar(&d, 40), 10);

    gk_data_panel_update(&d, 200.0, 150.0);
    GK_CHECK(near(d.progress, 1.0, 1e-9));
    GK_CHECK(near(gk_data_panel_remaining(&d), 0.0, 1e-9));
    GK_CHECK_EQ_INT(gk_data_panel_progress_bar(&d, 40), 40);
}

static void test_ui_view_theme_lang(void)
{
    gk_ui ui;
    gk_ui_init(&ui);
    GK_CHECK_EQ_INT(ui.theme, GK_THEME_DARK);
    GK_CHECK_EQ_INT(ui.lang, GK_LANG_EN);

    GK_CHECK(gk_ui_add_widget(&ui, "toolbar", 0, 0, 800, 40) == 0);
    GK_CHECK(gk_ui_add_widget(&ui, "panel", 0, 40, 200, 400) == 1);
    GK_CHECK(gk_ui_add_widget(&ui, "toolbar", 0, 0, 1, 1) == -1);
    GK_CHECK(gk_ui_find(&ui, "panel") != NULL);
    GK_CHECK(gk_ui_find(&ui, "nope") == NULL);

    gk_ui_set_visible(&ui, "panel", 0);
    GK_CHECK(!gk_ui_find(&ui, "panel")->visible);
    gk_ui_set_visible(&ui, "nope", 0);

    gk_ui_switch_view(&ui, GK_VIEW_SIDE);
    GK_CHECK_EQ_INT(ui.view, GK_VIEW_SIDE);
    GK_CHECK_STR_EQ(gk_view_mode_name(ui.view), "side");
    gk_ui_switch_view(&ui, (gk_view_mode)99);
    GK_CHECK_EQ_INT(ui.view, GK_VIEW_SIDE);

    gk_ui_toggle_fullscreen(&ui);
    GK_CHECK(ui.fullscreen);
    gk_ui_toggle_fullscreen(&ui);
    GK_CHECK(!ui.fullscreen);

    gk_ui_set_theme(&ui, GK_THEME_LIGHT);
    GK_CHECK_EQ_INT(ui.theme, GK_THEME_LIGHT);
    gk_ui_set_lang(&ui, GK_LANG_ZH);
    GK_CHECK_STR_EQ(gk_ui_tr(ui.lang, "menu.file"), "文件");
    GK_CHECK_STR_EQ(gk_ui_tr(GK_LANG_EN, "menu.file"), "File");
    GK_CHECK_STR_EQ(gk_ui_tr(GK_LANG_EN, "unknown.key"), "unknown.key");
}

static void test_panels(void)
{
    GK_CHECK_STR_EQ(gk_panel_style_name(GK_PANEL_FANUC), "FANUC");
    GK_CHECK_STR_EQ(gk_panel_style_name(GK_PANEL_HAAS), "HAAS");
    GK_CHECK_STR_EQ(gk_panel_softkey(GK_PANEL_FANUC, 0), "POS");
    GK_CHECK_STR_EQ(gk_panel_softkey(GK_PANEL_SIEMENS, 0), "MACHINE");
    GK_CHECK_STR_EQ(gk_panel_softkey(GK_PANEL_MITSUBISHI, 2), "EDIT");
    GK_CHECK_STR_EQ(gk_panel_softkey(GK_PANEL_HAAS, 1), "POSIT");
    GK_CHECK_STR_EQ(gk_panel_softkey(GK_PANEL_FANUC, 99), "");
    GK_CHECK_STR_EQ(gk_panel_softkey((gk_panel_style)99, 0), "");
}

static void test_keyboard(void)
{
    gk_keyboard k;
    gk_keyboard_init(&k);
    GK_CHECK(gk_keyboard_is_digit("7"));
    GK_CHECK(!gk_keyboard_is_digit("77"));
    GK_CHECK(!gk_keyboard_is_digit("X"));
    GK_CHECK(gk_keyboard_is_letter("X"));
    GK_CHECK(!gk_keyboard_is_letter("XY"));

    GK_CHECK_EQ_INT(gk_keyboard_press(&k, "7"), GK_OK);
    GK_CHECK_EQ_INT(k.keys[0].kind, GK_KEY_DIGIT);
    GK_CHECK_EQ_INT(gk_keyboard_press(&k, "X"), GK_OK);
    GK_CHECK_EQ_INT(k.keys[1].kind, GK_KEY_LETTER);
    GK_CHECK_EQ_INT(gk_keyboard_press(&k, "F5"), GK_OK);
    GK_CHECK_EQ_INT(k.keys[2].kind, GK_KEY_FUNC);
    GK_CHECK_EQ_INT(gk_keyboard_press(&k, "POS"), GK_OK);
    GK_CHECK_EQ_INT(k.keys[3].kind, GK_KEY_SOFT);
    GK_CHECK_STR_EQ(k.buffer, "7XF5POS");
    GK_CHECK_EQ_INT(k.len, 7);
    gk_keyboard_clear(&k);
    GK_CHECK_STR_EQ(k.buffer, "");
    GK_CHECK_EQ_INT(gk_keyboard_press(&k, ""), GK_ERR_INVALID_ARG);
}

static void test_mpg(void)
{
    gk_mpg m;
    gk_mpg_init(&m);
    GK_CHECK_EQ_INT(m.multiplier, 1);
    GK_CHECK_EQ_INT(m.axis, -1);
    GK_CHECK(near(gk_mpg_step(&m), 0.001, 1e-12));

    GK_CHECK_EQ_INT(gk_mpg_set_multiplier(&m, 10), GK_OK);
    GK_CHECK(near(gk_mpg_step(&m), 0.01, 1e-12));
    GK_CHECK_EQ_INT(gk_mpg_set_multiplier(&m, 100), GK_OK);
    GK_CHECK(near(gk_mpg_step(&m), 0.1, 1e-12));
    GK_CHECK_EQ_INT(gk_mpg_set_multiplier(&m, 5), GK_ERR_INVALID_ARG);

    GK_CHECK_EQ_INT(gk_mpg_select_axis(&m, 2), GK_OK);
    GK_CHECK_EQ_INT(m.axis, 2);
    GK_CHECK_EQ_INT(gk_mpg_select_axis(&m, 9), GK_ERR_OUT_OF_RANGE);
}

static void test_overrides_modes(void)
{
    gk_overrides o;
    gk_mode_switch s;
    gk_overrides_init(&o);
    GK_CHECK(near(o.feed_override, 1.0, 1e-9));
    GK_CHECK_EQ_INT(gk_overrides_set(&o, 0, 0.5), GK_OK);
    GK_CHECK(near(o.feed_override, 0.5, 1e-9));
    GK_CHECK_EQ_INT(gk_overrides_set(&o, 2, 3.0), GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_overrides_set(&o, 9, 1.0), GK_ERR_INVALID_ARG);

    gk_mode_switch_init(&s);
    GK_CHECK_EQ_INT(s.mode, GK_MODE_EDIT);
    GK_CHECK(!gk_mode_switch_is_auto(&s));
    GK_CHECK_EQ_INT(gk_mode_switch_set(&s, GK_MODE_AUTO), GK_OK);
    GK_CHECK(gk_mode_switch_is_auto(&s));
    GK_CHECK_STR_EQ(gk_op_mode_name(GK_MODE_DNC), "dnc");
    GK_CHECK_STR_EQ(gk_op_mode_name(GK_MODE_HANDLE), "handle");
    GK_CHECK_EQ_INT(gk_mode_switch_set(&s, (gk_op_mode)99), GK_ERR_INVALID_ARG);
}

static void test_panel_buttons(void)
{
    gk_panel_buttons b;
    gk_panel_buttons_init(&b);
    GK_CHECK_EQ_INT(b.key_switch, 0);

    GK_CHECK_EQ_INT(gk_panel_press(&b, GK_BTN_CYCLE_START), GK_OK);
    GK_CHECK(b.cycle_start);
    GK_CHECK_EQ_INT(gk_panel_press(&b, GK_BTN_ESTOP), GK_OK);
    GK_CHECK(b.estop);
    GK_CHECK(!b.cycle_start);
    GK_CHECK_EQ_INT(gk_panel_press(&b, GK_BTN_CYCLE_START), GK_ERR_STATE);

    GK_CHECK_EQ_INT(gk_panel_press(&b, GK_BTN_KEY), GK_OK);
    GK_CHECK_EQ_INT(b.key_switch, 1);
    GK_CHECK_EQ_INT(gk_panel_press(&b, GK_BTN_COOLANT), GK_OK);
    GK_CHECK(b.coolant);
    GK_CHECK_EQ_INT(gk_panel_press(&b, GK_BTN_SPINDLE), GK_OK);
    GK_CHECK(b.spindle);
    GK_CHECK_EQ_INT(gk_panel_press(&b, GK_BTN_LIGHT), GK_OK);
    GK_CHECK(b.light);
    GK_CHECK_EQ_INT(gk_panel_press(&b, 99), GK_ERR_INVALID_ARG);
}

static void test_diag_pages(void)
{
    gk_alarm_page ap;
    gk_param_page pp;
    gk_diag_page dp;
    memset(&ap, 0, sizeof(ap));
    memset(&pp, 0, sizeof(pp));
    memset(&dp, 0, sizeof(dp));

    GK_CHECK_EQ_INT(gk_alarm_page_add(&ap, 41, 3, "OT X+"), GK_OK);
    GK_CHECK_EQ_INT(ap.count, 1);
    GK_CHECK_STR_EQ(ap.rows[0].message, "OT X+");

    GK_CHECK_EQ_INT(gk_param_page_add(&pp, 1825, "Spindle max", 8000, "rpm"),
                    GK_OK);
    GK_CHECK_EQ_INT(gk_param_page_set(&pp, 1825, 10000), GK_OK);
    GK_CHECK(near(pp.rows[0].value, 10000.0, 1e-9));
    GK_CHECK_EQ_INT(gk_param_page_set(&pp, 9999, 1.0), GK_ERR_NOT_FOUND);

    GK_CHECK_EQ_INT(gk_diag_page_add(&dp, 100, "servo current", "A"), GK_OK);
    GK_CHECK_EQ_INT(gk_diag_page_set(&dp, 100, 4.2), GK_OK);
    GK_CHECK(near(dp.rows[0].value, 4.2, 1e-9));
    GK_CHECK_EQ_INT(gk_diag_page_set(&dp, 5, 1.0), GK_ERR_NOT_FOUND);
}

static void test_waveform(void)
{
    gk_waveform w;
    int i;
    gk_waveform_init(&w);
    for (i = 0; i < 10; ++i) {
        gk_waveform_push(&w, (double)i);
    }
    GK_CHECK_EQ_INT(w.count, 10);
    GK_CHECK(near(gk_waveform_at(&w, 0), 0.0, 1e-9));
    GK_CHECK(near(gk_waveform_at(&w, 9), 9.0, 1e-9));
    GK_CHECK(near(gk_waveform_peak(&w), 9.0, 1e-9));
    gk_waveform_push(&w, -20.0);
    GK_CHECK(near(gk_waveform_peak(&w), 20.0, 1e-9));
    GK_CHECK(near(gk_waveform_at(&w, -1), 0.0, 1e-9));
    GK_CHECK(near(gk_waveform_at(&w, 99), 0.0, 1e-9));
}

static void test_ladder_plc(void)
{
    gk_ladder l;
    gk_plc_view p;
    memset(&l, 0, sizeof(l));
    gk_ladder_add(&l, 1, "coolant on");
    GK_CHECK_EQ_INT(gk_ladder_add(&l, 1, "dup"), GK_ERR_ALREADY_EXISTS);
    GK_CHECK_EQ_INT(gk_ladder_set_input(&l, 1, 0, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_ladder_set_input(&l, 1, 1, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_ladder_evaluate(&l, 1), 1);
    GK_CHECK_EQ_INT(gk_ladder_set_input(&l, 1, 1, 0), GK_OK);
    GK_CHECK_EQ_INT(gk_ladder_evaluate(&l, 1), 0);
    GK_CHECK_EQ_INT(gk_ladder_set_input(&l, 9, 0, 1), GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_ladder_set_input(&l, 1, 99, 1), GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_ladder_evaluate(&l, 9), 0);

    gk_plc_view_init(&p);
    GK_CHECK(p.run);
    gk_plc_scan(&p, 2.5);
    GK_CHECK_EQ_INT(p.scan_count, 1);
    GK_CHECK(near(p.scan_time_ms, 2.5, 1e-9));
    gk_plc_set_io(&p, 3, 1);
    GK_CHECK_EQ_INT(gk_plc_read_io(&p, 3), 1);
    GK_CHECK_EQ_INT(gk_plc_read_io(&p, 999), -1);
    p.run = 0;
    gk_plc_scan(&p, 1.0);
    GK_CHECK_EQ_INT(p.scan_count, 1);
}

int main(void)
{
    test_menus();
    test_toolbar_status();
    test_data_panel();
    test_ui_view_theme_lang();
    test_panels();
    test_keyboard();
    test_mpg();
    test_overrides_modes();
    test_panel_buttons();
    test_diag_pages();
    test_waveform();
    test_ladder_plc();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
