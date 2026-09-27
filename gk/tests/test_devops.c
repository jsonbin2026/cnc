#include "gk_test.h"
#include "gk/gk_devops.h"
#include "gk/gk_a11y.h"

#include <math.h>
#include <string.h>

/* ---- diagnostics ---- */

static void test_regression(void)
{
    gk_dev_regression r;
    int baseline[3] = {1, 1, 1};
    gk_dev_regression_init(&r);
    GK_CHECK_EQ_INT(gk_dev_regression_add(&r, "a", 1), 1);
    GK_CHECK_EQ_INT(gk_dev_regression_add(&r, "b", 0), 2);
    GK_CHECK_EQ_INT(gk_dev_regression_add(&r, "c", 1), 3);
    GK_CHECK_EQ_INT(gk_dev_regression_compare(&r, baseline, 3), 1);
    GK_CHECK_EQ_INT(r.regressions, 1);
    GK_CHECK_STR_EQ(r.cases[0].name, "a");
}

static void test_profiler(void)
{
    gk_dev_profiler p;
    gk_dev_prof_init(&p);
    GK_CHECK_EQ_INT(gk_dev_prof_begin(&p, "render"), 0);
    GK_CHECK(gk_dev_prof_end(&p) == GK_OK);
    GK_CHECK(gk_dev_prof_end(&p) == GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_dev_prof_begin(&p, "render"), 0);
    GK_CHECK(gk_dev_prof_end(&p) == GK_OK);
    GK_CHECK_EQ_INT(p.sections[0].calls, 2);
    GK_CHECK_EQ_INT(gk_dev_prof_hottest(&p), 0);
}

static void test_memwatch(void)
{
    gk_dev_memwatch m;
    gk_dev_memwatch_init(&m, 100);
    GK_CHECK(!gk_dev_memwatch_over_limit(&m));
    GK_CHECK(gk_dev_memwatch_alloc(&m, 60) == GK_OK);
    GK_CHECK_EQ_INT((int)m.current, 60);
    GK_CHECK(gk_dev_memwatch_alloc(&m, 60) == GK_OK);
    GK_CHECK_EQ_INT((int)m.peak, 120);
    GK_CHECK(gk_dev_memwatch_over_limit(&m));
    gk_dev_memwatch_free(&m, 100);
    GK_CHECK(!gk_dev_memwatch_over_limit(&m));
}

static void test_crash(void)
{
    gk_dev_crash c;
    gk_dev_crash_handler h;
    char out[128];
    gk_dev_crash_init(&c);
    c.signal_number = 11;
    c.address = 0xDEAD;
    GK_CHECK(gk_dev_crash_report(&c, out, sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "signal=11") != NULL);
    gk_dev_crash_handler_init(&h);
    GK_CHECK_EQ_INT(gk_dev_crash_handler_capture(&h, 11, 1), 0);
    GK_CHECK_EQ_INT(gk_dev_crash_handler_install(&h), 1);
    GK_CHECK_EQ_INT(gk_dev_crash_handler_capture(&h, 11, 0x1234), 1);
    GK_CHECK_EQ_INT(h.last.signal_number, 11);
}

static void test_telemetry(void)
{
    gk_dev_telemetry t;
    char out[128];
    gk_dev_telemetry_init(&t);
    GK_CHECK(gk_dev_telemetry_event(&t, "start") == GK_OK);
    GK_CHECK(gk_dev_telemetry_event(&t, "start") == GK_OK);
    GK_CHECK(gk_dev_telemetry_event(&t, "stop") == GK_OK);
    GK_CHECK_EQ_INT((int)gk_dev_telemetry_count(&t, "start"), 2);
    GK_CHECK_EQ_INT((int)gk_dev_telemetry_count(&t, "none"), 0);
    GK_CHECK(gk_dev_telemetry_flush(&t, out, sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "start=2") != NULL);
    t.enabled = 0;
    GK_CHECK(gk_dev_telemetry_event(&t, "x") == GK_ERR_STATE);
}

static void test_hotreload(void)
{
    gk_dev_hotreload h;
    gk_dev_hotreload_init(&h, "mach.module");
    GK_CHECK_EQ_INT(h.generation, 1);
    GK_CHECK_EQ_INT(gk_dev_hotreload_poll(&h, 0), 0);
    GK_CHECK_EQ_INT(gk_dev_hotreload_poll(&h, 1), 2);
    GK_CHECK_EQ_INT(h.reloads, 1);
    h.auto_reload = 0;
    GK_CHECK_EQ_INT(gk_dev_hotreload_poll(&h, 1), 0);
}

/* ---- scripting / plugins / config / log ---- */

static void test_scripts(void)
{
    gk_dev_script_host s;
    gk_dev_script_host_init(&s, GK_DEV_SCRIPT_LUA);
    GK_CHECK_STR_EQ(gk_dev_script_name(s.engine), "lua");
    GK_CHECK(gk_dev_script_register(&s, "on_start") == GK_OK);
    GK_CHECK(gk_dev_script_register(&s, "on_stop") == GK_OK);
    GK_CHECK_EQ_INT(gk_dev_script_invoke(&s, "on_stop"), 1);
    GK_CHECK_EQ_INT(gk_dev_script_invoke(&s, "missing"), -1);
    GK_CHECK_STR_EQ(gk_dev_script_name(GK_DEV_SCRIPT_PYTHON), "python");
}

static gk_status plugin_ok(void)
{
    return GK_OK;
}

static void plugin_shutdown(void)
{
}

static void test_plugin(void)
{
    gk_dev_plugin p;
    memset(&p, 0, sizeof(p));
    p.abi_version = GK_DEV_PLUGIN_ABI_VERSION;
    p.name = "demo";
    p.init = plugin_ok;
    p.shutdown = plugin_shutdown;
    GK_CHECK(gk_dev_plugin_abi_compatible(&p));
    GK_CHECK(gk_dev_plugin_load(&p) == GK_OK);
    p.abi_version = 999;
    GK_CHECK(!gk_dev_plugin_abi_compatible(&p));
    GK_CHECK(gk_dev_plugin_load(&p) == GK_ERR_UNSUPPORTED);
}

static void test_dylib(void)
{
    gk_dev_dylib d;
    gk_dev_dylib_init(&d);
    GK_CHECK_EQ_INT(gk_dev_dylib_open(&d, "libfoo.so"), 0);
    GK_CHECK_EQ_INT(gk_dev_dylib_open(&d, "libbar.so"), 1);
    GK_CHECK_EQ_INT(gk_dev_dylib_open(&d, "libfoo.so"), 0);
    GK_CHECK(gk_dev_dylib_symbol(&d, "libfoo.so", "foo_init") != NULL);
    GK_CHECK(gk_dev_dylib_symbol(&d, "missing.so", "x") == NULL);
    GK_CHECK(gk_dev_dylib_close(&d, "libbar.so") == GK_OK);
    GK_CHECK(gk_dev_dylib_close(&d, "libbar.so") == GK_ERR_NOT_FOUND);
}

static void test_config(void)
{
    gk_dev_config c, back;
    char saved[256];
    gk_dev_config_init(&c);
    GK_CHECK(gk_dev_config_get(&c, "x") == NULL);
    GK_CHECK(gk_dev_config_set(&c, "theme", "dark") == GK_OK);
    GK_CHECK(gk_dev_config_set(&c, "units", "mm") == GK_OK);
    GK_CHECK_STR_EQ(gk_dev_config_get(&c, "theme"), "dark");
    GK_CHECK(gk_dev_config_set(&c, "theme", "light") == GK_OK);
    GK_CHECK_STR_EQ(gk_dev_config_get(&c, "theme"), "light");
    GK_CHECK(gk_dev_config_save(&c, saved, sizeof(saved)) == GK_OK);
    gk_dev_config_init(&back);
    GK_CHECK(gk_dev_config_load(&back, saved) == GK_OK);
    GK_CHECK_STR_EQ(gk_dev_config_get(&back, "units"), "mm");
}

static void test_devlog(void)
{
    gk_devlog_sink s;
    gk_devlog_sink_init(&s, "file", GK_DEVLOG_WARN);
    GK_CHECK(!gk_devlog_enabled(&s, GK_DEVLOG_INFO));
    GK_CHECK(gk_devlog_enabled(&s, GK_DEVLOG_ERROR));
    GK_CHECK(gk_devlog_write(&s, GK_DEVLOG_INFO, "ignored") == GK_OK);
    GK_CHECK_EQ_INT((int)s.written, 0);
    GK_CHECK(gk_devlog_write(&s, GK_DEVLOG_ERROR, "boom") == GK_OK);
    GK_CHECK_EQ_INT((int)s.written, 1);
    s.max_bytes = 4;
    GK_CHECK_EQ_INT(gk_devlog_rotate(&s), 1);
    GK_CHECK_EQ_INT((int)s.rotated, 1);
    GK_CHECK_STR_EQ(gk_devlog_level_name(GK_DEVLOG_TRACE), "TRACE");
}

/* ---- packaging / deployment ---- */

static void test_pkg(void)
{
    char out[128];
    GK_CHECK_STR_EQ(gk_dev_pkg_name(GK_DEV_PKG_MSI), "msi");
    GK_CHECK_STR_EQ(gk_dev_pkg_extension(GK_DEV_PKG_DEB), ".deb");
    GK_CHECK_STR_EQ(gk_dev_pkg_extension(GK_DEV_PKG_DMG), ".dmg");
    GK_CHECK_STR_EQ(gk_dev_pkg_extension(GK_DEV_PKG_PORTABLE), ".zip");
    GK_CHECK(gk_dev_pkg_manifest(GK_DEV_PKG_MSI, "GK", "1.0", out,
                                 sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "GK-1.0.msi") != NULL);
}

static void test_update(void)
{
    gk_dev_update u;
    gk_dev_update_init(&u, "1.2.3");
    GK_CHECK_EQ_INT(gk_dev_version_compare("1.2.3", "1.2.4"), -1);
    GK_CHECK_EQ_INT(gk_dev_version_compare("2.0.0", "1.9.9"), 1);
    GK_CHECK_EQ_INT(gk_dev_version_compare("1.0.0", "1.0.0"), 0);
    GK_CHECK(!gk_dev_update_available(&u, "1.2.2"));
    GK_CHECK(gk_dev_update_available(&u, "1.3.0"));
    GK_CHECK_STR_EQ(u.latest, "1.3.0");
}

static void test_ci(void)
{
    gk_dev_ci_pipeline p;
    gk_dev_ci_init(&p);
    GK_CHECK_EQ_INT(gk_dev_ci_add_stage(&p, "build"), 1);
    GK_CHECK_EQ_INT(gk_dev_ci_add_stage(&p, "test"), 2);
    GK_CHECK_EQ_INT(gk_dev_ci_run(&p), 0);
    GK_CHECK_EQ_INT(gk_dev_ci_stage_fail(&p, "test"), 1);
    GK_CHECK_EQ_INT(gk_dev_ci_run(&p), 1);
    GK_CHECK_EQ_INT(p.failed, 1);
}

static void test_deploy(void)
{
    char out[160];
    gk_dev_cloud c;
    gk_dev_mesh m;
    GK_CHECK(gk_dev_docker_build("gk", "debian:12", "gk-server", out,
                                 sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "FROM debian:12") != NULL);
    gk_dev_cloud_init(&c, "aws");
    GK_CHECK(gk_dev_cloud_deploy(&c, "us-east-1", 3) == GK_OK);
    GK_CHECK_EQ_INT(c.replicas, 3);
    GK_CHECK_EQ_INT(c.deployed, 1);
    gk_dev_mesh_init(&m);
    GK_CHECK_EQ_INT(gk_dev_mesh_register(&m, "auth", 8001), 0);
    GK_CHECK_EQ_INT(gk_dev_mesh_register(&m, "motion", 8002), 1);
    GK_CHECK_EQ_INT(gk_dev_mesh_healthy_count(&m), 2);
    GK_CHECK_EQ_INT(gk_dev_mesh_health(&m, "auth", 0), 1);
    GK_CHECK_EQ_INT(gk_dev_mesh_healthy_count(&m), 1);
}

/* ---- licensing ---- */

static void test_serial(void)
{
    char serial[GK_DEV_NAME];
    char bad[GK_DEV_NAME];
    gk_license l;
    GK_CHECK(gk_lic_generate_serial(GK_LIC_PROFESSIONAL, 12345, serial,
                                    sizeof(serial)) == GK_OK);
    GK_CHECK(gk_lic_verify_serial(serial, GK_LIC_PROFESSIONAL));
    GK_CHECK(!gk_lic_verify_serial(serial, GK_LIC_ENTERPRISE));
    strcpy(bad, serial);
    bad[strlen(bad) - 1] = (bad[strlen(bad) - 1] == '0') ? '1' : '0';
    GK_CHECK(!gk_lic_verify_serial(bad, GK_LIC_PROFESSIONAL));
    gk_lic_init(&l, GK_LIC_PROFESSIONAL, GK_LIC_MODE_SUBSCRIPTION);
    GK_CHECK(!gk_lic_is_valid(&l));
    GK_CHECK(gk_lic_activate_online(&l, serial) == GK_OK);
    GK_CHECK(gk_lic_is_valid(&l));
    GK_CHECK_STR_EQ(l.serial, serial);
    GK_CHECK(gk_lic_activate_online(&l, "garbage") == GK_ERR_NOT_FOUND);
}

static void test_offline(void)
{
    char serial[GK_DEV_NAME];
    char req[GK_DEV_NAME];
    char resp[256];
    gk_license l;
    (void)gk_lic_generate_serial(GK_LIC_ENTERPRISE, 777, serial,
                                 sizeof(serial));
    gk_lic_init(&l, GK_LIC_ENTERPRISE, GK_LIC_MODE_PERPETUAL);
    gk_lic_set_serial(&l, serial);
    GK_CHECK(gk_lic_offline_request(serial, req, sizeof(req)) == GK_OK);
    snprintf(resp, sizeof(resp), "RESP:%s", serial);
    GK_CHECK(gk_lic_activate_offline(&l, req, resp) == GK_OK);
    GK_CHECK_EQ_INT(l.offline, 1);
    GK_CHECK(gk_lic_is_valid(&l));
    GK_CHECK(gk_lic_activate_offline(&l, req, "NOPE") == GK_ERR_PARSE);
}

static void test_licensing_features(void)
{
    gk_license l;
    char wm[64];
    GK_CHECK_STR_EQ(gk_lic_edition_name(GK_LIC_FREE), "free");
    GK_CHECK(gk_lic_edition_tier(GK_LIC_ENTERPRISE) >
             gk_lic_edition_tier(GK_LIC_PROFESSIONAL));
    gk_lic_init(&l, GK_LIC_PROFESSIONAL, GK_LIC_MODE_SUBSCRIPTION);
    l.state = GK_LIC_STATE_ACTIVE;
    GK_CHECK(gk_lic_feature_allowed(&l, 0));
    GK_CHECK(gk_lic_feature_allowed(&l, 2));
    GK_CHECK(!gk_lic_feature_allowed(&l, 3));
    GK_CHECK(gk_lic_seat_available(&l));
    GK_CHECK(gk_lic_watermark(&l, wm, sizeof(wm)) == GK_OK);
    GK_CHECK_STR_EQ(wm, "");
    gk_lic_init(&l, GK_LIC_FREE, GK_LIC_MODE_TRIAL);
    GK_CHECK(gk_lic_is_valid(&l));
    GK_CHECK(gk_lic_watermark(&l, wm, sizeof(wm)) == GK_OK);
    GK_CHECK(strstr(wm, "EVALUATION") != NULL);
    l.today = 31;
    GK_CHECK(gk_lic_time_expired(&l));
    l.expiry_day = 100;
    l.today = 95;
    GK_CHECK_EQ_INT((int)gk_lic_days_to_renewal(&l), 5);
    l.used_seats = 1;
    l.seats = 1;
    GK_CHECK(!gk_lic_seat_available(&l));
    GK_CHECK(gk_lic_track_usage(&l, 48.0) == GK_OK);
    GK_CHECK_EQ_INT((int)l.today, 97);
}

static void test_integrity_dongle_server(void)
{
    unsigned char data[3] = {1, 2, 3};
    unsigned long sum = 2166136261u;
    gk_lic_dongle d;
    gk_lic_server s;
    size_t i;
    for (i = 0; i < 3; i++) {
        sum = (sum ^ data[i]) * 16777619u;
    }
    GK_CHECK(gk_lic_integrity_ok(data, 3, sum));
    GK_CHECK(!gk_lic_integrity_ok(data, 3, sum ^ 1u));
    gk_lic_dongle_init(&d, 42);
    GK_CHECK(!gk_lic_dongle_valid(&d));
    GK_CHECK(gk_lic_dongle_plug(&d, 42) == GK_OK);
    GK_CHECK(gk_lic_dongle_valid(&d));
    gk_lic_dongle_init(&d, 42);
    (void)gk_lic_dongle_plug(&d, 7);
    GK_CHECK(!gk_lic_dongle_valid(&d));
    gk_lic_server_init(&s, 2);
    GK_CHECK(gk_lic_server_checkout(&s) == GK_ERR_STATE);
    s.online = 1;
    GK_CHECK(gk_lic_server_checkout(&s) == GK_OK);
    GK_CHECK(gk_lic_server_checkout(&s) == GK_OK);
    GK_CHECK(gk_lic_server_checkout(&s) == GK_ERR_OVERFLOW);
    GK_CHECK(gk_lic_server_checkin(&s) == GK_OK);
    GK_CHECK_EQ_INT(s.checked_out, 1);
}

/* ---- accessibility ---- */

static void test_a11y_color_font(void)
{
    unsigned char r = 200, g = 100, b = 50;
    gk_a11y_contrast c;
    gk_a11y_font f;
    unsigned char r0 = r, g0 = g, b0 = b;
    GK_CHECK_STR_EQ(gk_a11y_colorblind_name(GK_A11Y_CB_NONE), "none");
    gk_a11y_colorblind_map(GK_A11Y_CB_NONE, &r, &g, &b);
    GK_CHECK_EQ_INT(r, r0);
    gk_a11y_colorblind_map(GK_A11Y_CB_DEUTERANOPIA, &r, &g, &b);
    GK_CHECK(r != r0 || g != g0 || b != b0);
    gk_a11y_contrast_init(&c);
    GK_CHECK_EQ_INT(c.contrast, 100);
    GK_CHECK(gk_a11y_contrast_set(&c, 200) == GK_OK);
    GK_CHECK(gk_a11y_contrast_luminance(&c, 0.6) > 0.6);
    GK_CHECK(gk_a11y_contrast_set(&c, 50) == GK_ERR_OUT_OF_RANGE);
    gk_a11y_font_init(&f);
    GK_CHECK_EQ_INT(gk_a11y_font_size(&f, 12), 12);
    GK_CHECK(gk_a11y_font_set_scale(&f, 2.0) == GK_OK);
    GK_CHECK_EQ_INT(gk_a11y_font_size(&f, 12), 24);
    GK_CHECK(gk_a11y_font_set_scale(&f, 100.0) == GK_ERR_OUT_OF_RANGE);
}

static void test_a11y_voice_onehand(void)
{
    gk_a11y_voice v;
    gk_a11y_onehand o;
    double x, y;
    gk_a11y_voice_init(&v);
    GK_CHECK(gk_a11y_voice_add(&v, "start", "cmd.start") == GK_OK);
    GK_CHECK(gk_a11y_voice_add(&v, "stop", "cmd.stop") == GK_OK);
    GK_CHECK_STR_EQ(gk_a11y_voice_match(&v, "stop"), "cmd.stop");
    GK_CHECK(gk_a11y_voice_match(&v, "unknown") == NULL);
    gk_a11y_onehand_init(&o);
    GK_CHECK(gk_a11y_onehand_enable(&o, 0) == GK_OK);
    GK_CHECK_EQ_INT(o.mirrored, 0);
    GK_CHECK(gk_a11y_onehand_remap(&o, 1.0, 0.0, &x, &y) == GK_OK);
    GK_CHECK(x > 0.5 && x <= 1.0);
    GK_CHECK(y < 0.5 && y >= 0.0);
    GK_CHECK(gk_a11y_onehand_enable(&o, 1) == GK_OK);
    GK_CHECK_EQ_INT(o.mirrored, 1);
}

static void test_a11y_subtitles_slowmo(void)
{
    gk_a11y_subtitles s;
    gk_a11y_slowmo m;
    gk_a11y_subtitles_init(&s);
    GK_CHECK(gk_a11y_subtitles_add(&s, "hello", 0.0, 1.0) == GK_OK);
    GK_CHECK(gk_a11y_subtitles_add(&s, "world", 1.0, 2.0) == GK_OK);
    GK_CHECK(gk_a11y_subtitles_add(&s, "bad", 1.0, 1.0) ==
             GK_ERR_INVALID_ARG);
    GK_CHECK_STR_EQ(gk_a11y_subtitles_at(&s, 0.5), "hello");
    GK_CHECK_STR_EQ(gk_a11y_subtitles_at(&s, 1.5), "world");
    GK_CHECK(gk_a11y_subtitles_at(&s, 3.0) == NULL);
    s.enabled = 0;
    GK_CHECK(gk_a11y_subtitles_at(&s, 0.5) == NULL);
    gk_a11y_slowmo_init(&m);
    GK_CHECK(gk_a11y_font_size(NULL, 10) == 10);
    GK_CHECK(fabs(gk_a11y_slowmo_apply(&m, 2.0) - 2.0) < 1e-9);
    GK_CHECK(gk_a11y_slowmo_set(&m, 0.25) == GK_OK);
    GK_CHECK_EQ_INT(m.enabled, 1);
    GK_CHECK(fabs(gk_a11y_slowmo_apply(&m, 4.0) - 1.0) < 1e-9);
    GK_CHECK(gk_a11y_slowmo_set(&m, 2.0) == GK_ERR_OUT_OF_RANGE);
}

int main(void)
{
    test_regression();
    test_profiler();
    test_memwatch();
    test_crash();
    test_telemetry();
    test_hotreload();
    test_scripts();
    test_plugin();
    test_dylib();
    test_config();
    test_devlog();
    test_pkg();
    test_update();
    test_ci();
    test_deploy();
    test_serial();
    test_offline();
    test_licensing_features();
    test_integrity_dongle_server();
    test_a11y_color_font();
    test_a11y_voice_onehand();
    test_a11y_subtitles_slowmo();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
