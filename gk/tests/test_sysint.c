#include "gk_test.h"
#include "gk/gk_sysint.h"

#include <math.h>
#include <string.h>

static void test_loop(void)
{
    gk_sysint_loop l;
    double first;
    gk_sysint_loop_init(&l, GK_SYSINT_HIL, 0.001);
    GK_CHECK_STR_EQ(gk_sysint_mode_name(l.mode), "hil");
    GK_CHECK(gk_sysint_loop_step(&l, 5.0) == GK_ERR_STATE);
    GK_CHECK(gk_sysint_loop_connect(&l) == GK_OK);
    l.setpoint = 10.0;
    GK_CHECK(gk_sysint_loop_step(&l, 5.0) == GK_OK);
    first = l.plant_output;
    GK_CHECK(first > 0.0);
    GK_CHECK(gk_sysint_loop_step(&l, 5.0) == GK_OK);
    GK_CHECK(l.plant_output > first);
    GK_CHECK_EQ_INT(l.steps, 2);
    GK_CHECK(fabs(l.sim_time_s - 0.002) < 1e-12);
    GK_CHECK(gk_sysint_rcp_build(&l, "servo") == GK_OK);
    GK_CHECK(fabs(l.sample_time_s - 0.0005) < 1e-12);
    GK_CHECK(gk_sysint_rcp_build(&l, "nonsense") == GK_ERR_UNSUPPORTED);
}

static void test_twin(void)
{
    gk_sysint_twin t;
    gk_sysint_twin_init(&t, "X-axis");
    GK_CHECK_STR_EQ(t.tag, "X-axis");
    GK_CHECK(gk_sysint_twin_sync(&t, 10.0, 9.5) == GK_OK);
    GK_CHECK(fabs(gk_sysint_twin_error(&t) - 0.5) < 1e-9);
    GK_CHECK(gk_sysint_twin_sync(&t, 3.0, 3.0) == GK_OK);
    GK_CHECK(fabs(gk_sysint_twin_error(&t)) < 1e-9);
}

static void test_remote(void)
{
    gk_sysint_remote r;
    double x, y, z;
    int sync = 0;
    gk_sysint_remote_init(&r);
    GK_CHECK(gk_sysint_remote_write(&r, 1, 2, 3) == GK_ERR_STATE);
    GK_CHECK(gk_sysint_remote_set_interlock(&r, 0) == GK_OK);
    r.enabled = 1;
    GK_CHECK(gk_sysint_remote_write(&r, 1, 2, 3) == GK_OK);
    GK_CHECK(gk_sysint_remote_read(&r, &x, &y, &z) == GK_OK);
    GK_CHECK(fabs(z - 3.0) < 1e-9);
    GK_CHECK(gk_sysint_link(5.0, 5.1, 0.2, &sync) == GK_OK);
    GK_CHECK_EQ_INT(sync, 1);
    GK_CHECK(gk_sysint_link(5.0, 5.5, 0.2, &sync) == GK_OK);
    GK_CHECK_EQ_INT(sync, 0);
}

static void test_shadow_device(void)
{
    gk_sysint_shadow s;
    gk_sysint_link_dev dev;
    double deg;
    gk_sysint_shadow_init(&s);
    GK_CHECK(gk_sysint_shadow_compare(&s, 10.0, 10.1, 0.5) == GK_OK);
    GK_CHECK_EQ_INT(s.divergent, 0);
    GK_CHECK(gk_sysint_shadow_compare(&s, 10.0, 12.0, 0.5) == GK_OK);
    GK_CHECK_EQ_INT(s.divergent, 1);
    GK_CHECK(fabs(s.max_deviation - 2.0) < 1e-9);
    GK_CHECK(gk_sysint_device_connect(&dev, GK_SYSINT_CNC, "tcp://1.2.3.4") ==
             GK_OK);
    GK_CHECK_EQ_INT(dev.online, 1);
    GK_CHECK_STR_EQ(gk_sysint_device_name(dev.kind), "cnc");
    dev.last_value = 42.0;
    {
        double v = 0.0;
        GK_CHECK(gk_sysint_device_read(&dev, &v) == GK_OK);
        GK_CHECK(fabs(v - 42.0) < 1e-9);
    }
    GK_CHECK(gk_sysint_handwheel_read(&dev, &deg) == GK_ERR_STATE);
    GK_CHECK(gk_sysint_device_connect(&dev, GK_SYSINT_HANDWHEEL, "usb:0") ==
             GK_OK);
    dev.last_value = 1.25;
    GK_CHECK(gk_sysint_handwheel_read(&dev, &deg) == GK_OK);
    GK_CHECK(fabs(deg - 1.25) < 1e-9);
}

static void test_data_interfaces(void)
{
    char out[512];
    double vals[3] = {1.5, 2.5, 3.5};
    double errs[3] = {0.1, 0.2, 0.3};
    gk_sysint_benchmark b;
    GK_CHECK(gk_sysint_hdf5_header("force", 2, 2, out, sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "float64") != NULL);
    GK_CHECK(gk_sysint_hdf5_dataset(vals, 3, out, sizeof(out)) == GK_OK);
    GK_CHECK_STR_EQ(out, "1.5,2.5,3.5");
    GK_CHECK(gk_sysint_python_bind("gk", "run", out, sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "import gk") != NULL);
    GK_CHECK(gk_sysint_matlab_export("y", vals, 3, out, sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "y = [") != NULL);
    GK_CHECK(gk_sysint_ros_publish("/gk/state", "Float64", 3.14, out,
                                   sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "/gk/state") != NULL);
    GK_CHECK(gk_sysint_fmu_export("plant", "3.0", 2, 1, out, sizeof(out)) ==
             GK_OK);
    GK_CHECK(strstr(out, "inputs=2") != NULL);
    GK_CHECK(gk_sysint_modelica_model("Plant", "der(x)=u", out, sizeof(out)) ==
             GK_OK);
    GK_CHECK(strstr(out, "end Plant;") != NULL);
    GK_CHECK(gk_sysint_paper_figure(3, "Force vs time", out, sizeof(out)) ==
             GK_OK);
    GK_CHECK(strstr(out, "Figure 3.") != NULL);
    gk_sysint_benchmark_init(&b, "GK-Bench", 3);
    GK_CHECK(gk_sysint_benchmark_eval(&b, errs, 3) == GK_OK);
    GK_CHECK(fabs(b.baseline_error - 0.2) < 1e-9);
    GK_CHECK(gk_sysint_experiment_script("exp1", 2, out, sizeof(out)) == GK_OK);
    GK_CHECK(strstr(out, "run(1)") != NULL);
}

static void test_replay(void)
{
    gk_sysint_replay r;
    double data[4] = {10.0, 20.0, 30.0, 40.0};
    double v = 0.0;
    gk_sysint_replay_init(&r);
    GK_CHECK_EQ_INT(gk_sysint_replay_load(&r, data, 4), 4);
    GK_CHECK_EQ_INT(gk_sysint_replay_next(&r, &v), 1);
    GK_CHECK(fabs(v - 10.0) < 1e-9);
    GK_CHECK(gk_sysint_replay_seek(&r, 3) == GK_OK);
    GK_CHECK_EQ_INT(gk_sysint_replay_next(&r, &v), 4);
    GK_CHECK(fabs(v - 40.0) < 1e-9);
    GK_CHECK_EQ_INT(gk_sysint_replay_next(&r, &v), 1);
    GK_CHECK(gk_sysint_replay_seek(&r, 9) == GK_ERR_OUT_OF_RANGE);
}

static void test_crypto(void)
{
    const unsigned char key[4] = {0x11, 0x22, 0x33, 0x44};
    const unsigned char plain[6] = {1, 2, 3, 4, 5, 6};
    unsigned char cipher[6];
    unsigned char back[6];
    GK_CHECK(gk_sec_encrypt(key, 4, plain, 6, cipher) == GK_OK);
    GK_CHECK(cipher[0] != plain[0]);
    GK_CHECK(gk_sec_decrypt(key, 4, cipher, 6, back) == GK_OK);
    GK_CHECK(memcmp(plain, back, 6) == 0);
    GK_CHECK(gk_sec_encrypt(NULL, 0, plain, 6, cipher) == GK_ERR_INVALID_ARG);
}

static void test_auth_perm_audit(void)
{
    gk_sec_account a;
    gk_sec_role role;
    gk_sec_audit audit;
    gk_sec_account_init(&a, "alice", "secret");
    GK_CHECK_STR_EQ(a.user, "alice");
    GK_CHECK(gk_sec_authenticate(&a, "wrong") == GK_ERR_NOT_FOUND);
    GK_CHECK(gk_sec_authenticate(&a, "secret") == GK_OK);
    GK_CHECK(gk_sec_change_password(&a, "secret", "newpass") == GK_OK);
    GK_CHECK(gk_sec_authenticate(&a, "newpass") == GK_OK);
    GK_CHECK(gk_sec_change_password(&a, "bad", "x") == GK_ERR_NOT_FOUND);
    gk_sec_role_init(&role, "alice",
                     GK_SEC_PERM_READ | GK_SEC_PERM_WRITE);
    GK_CHECK(gk_sec_role_allows(&role, GK_SEC_PERM_READ));
    GK_CHECK(!gk_sec_role_allows(&role, GK_SEC_PERM_EXECUTE));
    gk_sec_role_init(&role, "root", GK_SEC_PERM_ADMIN);
    GK_CHECK(gk_sec_role_allows(&role, GK_SEC_PERM_EXECUTE));
    gk_sec_audit_init(&audit);
    GK_CHECK(gk_sec_audit_log(&audit, "login alice") == GK_OK);
    GK_CHECK(gk_sec_audit_contains(&audit, "login alice"));
    GK_CHECK(!gk_sec_audit_contains(&audit, "logout"));
}

static void test_lockout(void)
{
    gk_sec_account a;
    gk_sec_account_init(&a, "bob", "pw");
    GK_CHECK(gk_sec_authenticate(&a, "x") == GK_ERR_NOT_FOUND);
    GK_CHECK(gk_sec_authenticate(&a, "x") == GK_ERR_NOT_FOUND);
    GK_CHECK(gk_sec_authenticate(&a, "x") == GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(a.locked, 1);
    GK_CHECK(gk_sec_authenticate(&a, "pw") == GK_ERR_STATE);
}

static void test_policy_compliance_gdpr(void)
{
    gk_sec_policy p;
    gk_sec_compliance c;
    char out[256];
    char record[64];
    gk_sec_policy_init(&p, "v2");
    GK_CHECK(gk_sec_policy_accept(&p, "v1") == GK_ERR_NOT_FOUND);
    GK_CHECK(gk_sec_policy_accept(&p, "v2") == GK_OK);
    GK_CHECK_EQ_INT(p.accepted, 1);
    gk_sec_compliance_init(&c);
    GK_CHECK_EQ_INT(gk_sec_compliance_add(&c, "GDPR"), 1);
    GK_CHECK_EQ_INT(gk_sec_compliance_add(&c, "ISO27001"), 2);
    GK_CHECK(!gk_sec_compliance_all_pass(&c));
    GK_CHECK(gk_sec_compliance_set(&c, "GDPR", 1) == GK_OK);
    GK_CHECK(gk_sec_compliance_set(&c, "ISO27001", 1) == GK_OK);
    GK_CHECK(gk_sec_compliance_all_pass(&c));
    GK_CHECK(gk_sec_compliance_set(&c, "SOC2", 1) == GK_ERR_NOT_FOUND);
    GK_CHECK(gk_sec_gdpr_export("alice", "score=90", out, sizeof(out)) ==
             GK_OK);
    GK_CHECK(strstr(out, "SUBJECT:alice") != NULL);
    strcpy(record, "user=alice;score=90");
    GK_CHECK(gk_sec_gdpr_erase(record, "alice") == GK_OK);
    GK_CHECK(strstr(record, "alice") == NULL);
    GK_CHECK(strstr(record, "*****") != NULL);
}

static void test_network_tamper_sign(void)
{
    gk_sec_network n;
    gk_sec_tamper t;
    gk_sec_key k;
    unsigned long sig;
    gk_sec_network_init(&n);
    GK_CHECK(gk_sec_network_enable_tls(&n, "AES-256-GCM", 256) == GK_OK);
    GK_CHECK_EQ_INT(n.tls_enabled, 1);
    GK_CHECK(gk_sec_network_enable_tls(&n, "weak", 64) == GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_sec_network_port_allowed(&n, 443), 1);
    n.allowed_ports[0] = 443;
    n.port_count = 1;
    GK_CHECK_EQ_INT(gk_sec_network_port_allowed(&n, 443), 1);
    GK_CHECK_EQ_INT(gk_sec_network_port_allowed(&n, 8080), 0);
    gk_sec_tamper_init(&t);
    (void)gk_sec_tamper_append(&t, "rec1");
    (void)gk_sec_tamper_append(&t, "rec2");
    GK_CHECK_EQ_INT(t.count, 2);
    GK_CHECK(gk_sec_tamper_verify(&t, 0, "rec1"));
    GK_CHECK(gk_sec_tamper_verify(&t, 1, "rec2"));
    GK_CHECK(!gk_sec_tamper_verify(&t, 1, "tampered"));
    gk_sec_key_init(&k, 3233UL, 17UL);
    sig = gk_sec_sign(&k, "hello");
    GK_CHECK(sig != 0);
    GK_CHECK(gk_sec_verify(&k, "hello", sig));
    GK_CHECK(!gk_sec_verify(&k, "hello!", sig));
}

int main(void)
{
    test_loop();
    test_twin();
    test_remote();
    test_shadow_device();
    test_data_interfaces();
    test_replay();
    test_crypto();
    test_auth_perm_audit();
    test_lockout();
    test_policy_compliance_gdpr();
    test_network_tamper_sign();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
