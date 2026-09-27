#include "gk_test.h"

#include "gk/gk_safety.h"

#include <math.h>
#include <string.h>

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

static void test_procedure(void)
{
    gk_safety_procedure p;

    gk_safety_procedure_init(&p, "Manual Procedure");
    GK_CHECK_STR_EQ(p.title, "Manual Procedure");
    GK_CHECK_EQ_INT(gk_safety_step_add(&p, "Wear PPE", 1), 1);
    GK_CHECK_EQ_INT(gk_safety_step_add(&p, "Check guards", 1), 2);
    GK_CHECK_EQ_INT(gk_safety_step_add(&p, "Warm up", 0), 3);
    GK_CHECK_EQ_INT(p.count, 3);
    GK_CHECK_EQ_INT(gk_safety_procedure_complete(&p), 0);
    GK_CHECK_EQ_INT(gk_safety_remaining(&p), 3);
    GK_CHECK_EQ_INT(gk_safety_step_done(&p, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_safety_step_done(&p, 999), GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_safety_procedure_complete(&p), 0);
    GK_CHECK_EQ_INT(gk_safety_next(&p), GK_OK);
    GK_CHECK_EQ_INT(gk_safety_next(&p), GK_OK);
    GK_CHECK_EQ_INT(gk_safety_remaining(&p), 1);
    GK_CHECK_EQ_INT(gk_safety_procedure_complete(&p), 1);
    GK_CHECK_EQ_INT(gk_safety_next(&p), GK_OK);
    GK_CHECK_EQ_INT(gk_safety_remaining(&p), 0);
    GK_CHECK_EQ_INT(gk_safety_procedure_complete(&p), 1);
    GK_CHECK_EQ_INT(gk_safety_next(&p), GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_safety_next(NULL), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_safety_remaining(NULL), 0);
}

static void test_power_sequences(void)
{
    gk_safety_procedure on, off;

    GK_CHECK_EQ_INT(gk_safety_build_power_on(&on), GK_OK);
    GK_CHECK(on.count >= 5);
    GK_CHECK_STR_EQ(on.title, "Power-On Sequence");
    /* all mandatory done in order -> complete */
    while (gk_safety_next(&on) == GK_OK) {
    }
    GK_CHECK_EQ_INT(gk_safety_procedure_complete(&on), 1);

    GK_CHECK_EQ_INT(gk_safety_build_power_off(&off), GK_OK);
    GK_CHECK(off.count >= 4);
    GK_CHECK_STR_EQ(off.title, "Power-Off Sequence");
    /* skipping a mandatory step leaves it incomplete */
    gk_safety_step_done(&off, 1);
    gk_safety_step_done(&off, 3);
    gk_safety_step_done(&off, 4);
    GK_CHECK_EQ_INT(gk_safety_procedure_complete(&off), 0);
    gk_safety_step_done(&off, 2);
    gk_safety_step_done(&off, 5);
    GK_CHECK_EQ_INT(gk_safety_procedure_complete(&off), 1);
    GK_CHECK_EQ_INT(gk_safety_build_power_on(NULL), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_safety_build_power_off(NULL), GK_ERR_INVALID_ARG);
}

static void test_ppe(void)
{
    gk_ppe_state s;
    int i;

    for (i = 0; i < GK_PPE_COUNT; ++i) {
        GK_CHECK(strlen(gk_ppe_name((gk_ppe_kind)i)) > 0);
        GK_CHECK(strlen(gk_ppe_requirement((gk_ppe_kind)i)) > 0);
    }
    GK_CHECK_STR_EQ(gk_ppe_name(GK_PPE_GOGGLES), "safety-goggles");
    GK_CHECK_STR_EQ(gk_ppe_name(GK_PPE_SHOES), "safety-shoes");

    gk_ppe_state_init(&s);
    GK_CHECK_EQ_INT(gk_ppe_compliant(&s), 0);
    GK_CHECK_EQ_INT(gk_ppe_missing_count(&s), GK_PPE_COUNT);
    GK_CHECK_EQ_INT(gk_ppe_wear(&s, GK_PPE_GOGGLES, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_ppe_wear(&s, GK_PPE_COVERALL, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_ppe_wear(&s, GK_PPE_SHOES, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_ppe_wear(&s, GK_PPE_GLOVES, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_ppe_wear(&s, GK_PPE_HEARING, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_ppe_compliant(&s), 1);
    GK_CHECK_EQ_INT(gk_ppe_missing_count(&s), 0);
    GK_CHECK_EQ_INT(gk_ppe_wear(&s, GK_PPE_GOGGLES, 0), GK_OK);
    GK_CHECK_EQ_INT(gk_ppe_compliant(&s), 0);
    GK_CHECK_EQ_INT(gk_ppe_wear(&s, (gk_ppe_kind)99, 1), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_ppe_compliant(NULL), 0);
}

static void test_hazard(void)
{
    gk_hazard_map m;

    gk_hazard_map_init(&m);
    GK_CHECK_EQ_INT(gk_hazard_add(&m, 0, 0, 0, 100.0, 2, "chuck"), 1);
    GK_CHECK_EQ_INT(gk_hazard_add(&m, 500, 0, 0, 50.0, 3, "high-volt"), 2);
    GK_CHECK_EQ_INT(m.count, 2);
    GK_CHECK_EQ_INT(gk_hazard_query(&m, 10, 0, 0), 1);
    GK_CHECK_EQ_INT(gk_hazard_query(&m, 500, 0, 0), 2);
    /* point in both zones -> higher severity wins */
    GK_CHECK_EQ_INT(gk_hazard_query(&m, 60, 0, 0), 1);
    GK_CHECK_EQ_INT(gk_hazard_query(&m, 9000, 0, 0), 0);
    GK_CHECK_EQ_INT(gk_hazard_add(&m, 0, 0, 0, -1.0, 1, "bad"), -1);
    GK_CHECK_EQ_INT(gk_hazard_query(NULL, 0, 0, 0), 0);

    GK_CHECK_EQ_INT(gk_rotating_warning(0, 10), 0);
    GK_CHECK_EQ_INT(gk_rotating_warning(1000, 10), 1);
    GK_CHECK_EQ_INT(gk_rotating_warning(10000, 100), 2);
    GK_CHECK_EQ_INT(gk_rotating_warning(20000, 100), 3);
}

static void test_estop(void)
{
    gk_estop_layout l;
    const gk_estop_location *s;
    gk_estop_drill d;

    gk_estop_layout_init(&l);
    GK_CHECK_EQ_INT(gk_estop_add(&l, 0, 0, 0, "front"), 1);
    GK_CHECK_EQ_INT(gk_estop_add(&l, 1000, 0, 0, "back"), 2);
    GK_CHECK_EQ_INT(l.count, 2);
    s = gk_estop_nearest(&l, 10, 0, 0);
    GK_CHECK(s != NULL);
    GK_CHECK_STR_EQ(s->label, "front");
    s = gk_estop_nearest(&l, 900, 0, 0);
    GK_CHECK_STR_EQ(s->label, "back");
    GK_CHECK(gk_estop_nearest(NULL, 0, 0, 0) == NULL);

    gk_estop_drill_init(&d, 1.0);
    GK_CHECK_EQ_INT(gk_estop_drill_press(&d, 0.5), GK_OK);
    GK_CHECK_EQ_INT(d.pressed, 1);
    GK_CHECK_EQ_INT(d.passed, 1);
    GK_CHECK_EQ_INT(gk_estop_drill_press(&d, 1.5), GK_OK);
    GK_CHECK_EQ_INT(d.passed, 0);
    GK_CHECK_EQ_INT(gk_estop_drill_press(&d, -1.0), GK_ERR_INVALID_ARG);
}

static void test_loto(void)
{
    gk_loto_board b;

    gk_loto_board_init(&b);
    GK_CHECK_EQ_INT(gk_loto_is_locked(&b, "MCC-PANEL"), 0);
    GK_CHECK_EQ_INT(gk_loto_apply(&b, "MCC-PANEL", "alice"), 1);
    GK_CHECK_EQ_INT(gk_loto_is_locked(&b, "MCC-PANEL"), 1);
    /* wrong owner cannot remove */
    GK_CHECK_EQ_INT(gk_loto_remove(&b, "MCC-PANEL", "bob"), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_loto_is_locked(&b, "MCC-PANEL"), 1);
    GK_CHECK_EQ_INT(gk_loto_remove(&b, "MCC-PANEL", "alice"), GK_OK);
    GK_CHECK_EQ_INT(gk_loto_is_locked(&b, "MCC-PANEL"), 0);
    GK_CHECK_EQ_INT(gk_loto_remove(&b, "none", "x"), GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_loto_remove(&b, NULL, "x"), GK_ERR_INVALID_ARG);
}

static void test_5s(void)
{
    gk_5s_board b;
    int i;

    for (i = 0; i < GK_5S_COUNT; ++i) {
        GK_CHECK(strlen(gk_5s_name((gk_5s_pillar)i)) > 0);
    }
    GK_CHECK_STR_EQ(gk_5s_name(GK_5S_SORT), "sort");
    GK_CHECK_STR_EQ(gk_5s_name(GK_5S_SUSTAIN), "sustain");

    gk_5s_board_init(&b);
    GK_CHECK_EQ_INT(gk_5s_area_add(&b, "Zone A"), 1);
    GK_CHECK_EQ_INT(gk_5s_area_add(&b, "Zone B"), 2);
    GK_CHECK(near(gk_5s_area_score(&b, "Zone A"), 0.0, 1e-9));
    for (i = 0; i < GK_5S_COUNT; ++i) {
        GK_CHECK_EQ_INT(gk_5s_set_score(&b, "Zone A", (gk_5s_pillar)i, 80),
                        GK_OK);
        GK_CHECK_EQ_INT(gk_5s_set_score(&b, "Zone B", (gk_5s_pillar)i, 60),
                        GK_OK);
    }
    GK_CHECK(near(gk_5s_area_score(&b, "Zone A"), 80.0, 1e-9));
    GK_CHECK(near(gk_5s_area_score(&b, "Zone B"), 60.0, 1e-9));
    GK_CHECK(near(gk_5s_overall(&b), 70.0, 1e-9));
    GK_CHECK_EQ_INT(gk_5s_set_score(&b, "none", GK_5S_SORT, 50),
                    GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_5s_set_score(&b, "Zone A", GK_5S_SORT, 200),
                    GK_ERR_OUT_OF_RANGE);
    GK_CHECK(near(gk_5s_area_score(&b, "none"), 0.0, 1e-9));
    GK_CHECK(near(gk_5s_overall(NULL), 0.0, 1e-9));
}

static void test_incidents(void)
{
    gk_incident_library l;
    const gk_incident_case *c;
    char buf[512];

    gk_incident_library_init(&l);
    GK_CHECK_EQ_INT(gk_incident_add(&l, "Chuck key left in", "forgot key",
                                    "always remove chuck key", 4.0), 1);
    GK_CHECK_EQ_INT(gk_incident_add(&l, "Hand cut on chip", "no gloves",
                                    "wear gloves when clearing chips", 2.0), 2);
    GK_CHECK_EQ_INT(l.count, 2);
    c = gk_incident_find(&l, "Chuck key left in");
    GK_CHECK(c != NULL);
    GK_CHECK_STR_EQ(c->cause, "forgot key");
    GK_CHECK(gk_incident_find(&l, "none") == NULL);
    GK_CHECK(gk_incident_replay(c, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "INCIDENT|Chuck key left in") != NULL);
    GK_CHECK(strstr(buf, "severity=4.0") != NULL);
    GK_CHECK_EQ_INT(gk_incident_replay(NULL, buf, sizeof(buf)), 0);
}

static void test_violations_score(void)
{
    gk_violation_book b;
    gk_safety_score s;

    gk_violation_book_init(&b);
    GK_CHECK_EQ_INT(gk_violation_total(&b, 1), 0);
    GK_CHECK_EQ_INT(gk_violation_add(&b, 1, 5), GK_OK);
    GK_CHECK_EQ_INT(gk_violation_add(&b, 1, 3), GK_OK);
    GK_CHECK_EQ_INT(gk_violation_add(&b, 2, 2), GK_OK);
    GK_CHECK_EQ_INT(gk_violation_total(&b, 1), 8);
    GK_CHECK_EQ_INT(gk_violation_total(&b, 2), 2);
    GK_CHECK_EQ_INT(gk_violation_disqualified(&b, 1, 10), 0);
    GK_CHECK_EQ_INT(gk_violation_add(&b, 1, 5), GK_OK);
    GK_CHECK_EQ_INT(gk_violation_total(&b, 1), 13);
    GK_CHECK_EQ_INT(gk_violation_disqualified(&b, 1, 10), 1);
    GK_CHECK_EQ_INT(gk_violation_add(&b, 1, -1), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_violation_total(NULL, 1), 0);

    gk_safety_score_init(&s);
    GK_CHECK(near(gk_safety_score_value(&s), 0.0, 1e-9));
    gk_safety_score_add(&s, 1);
    gk_safety_score_add(&s, 1);
    gk_safety_score_add(&s, 0);
    gk_safety_score_add(&s, 1);
    GK_CHECK(near(gk_safety_score_value(&s), 0.75, 1e-9));
    GK_CHECK_EQ_INT(gk_safety_score_pass(&s, 0.7), 1);
    GK_CHECK_EQ_INT(gk_safety_score_pass(&s, 0.8), 0);
    GK_CHECK_EQ_INT(gk_safety_score_add(NULL, 1), GK_ERR_INVALID_ARG);
}

static void test_cert(void)
{
    gk_safety_cert c;
    char buf[256];

    gk_safety_cert_init(&c, "Alice", "CNC Safety L1");
    GK_CHECK_STR_EQ(c.holder, "Alice");
    GK_CHECK_STR_EQ(c.course, "CNC Safety L1");
    GK_CHECK_EQ_INT(gk_safety_cert_issue(&c, 85.0, 1000.0, 80.0), GK_OK);
    GK_CHECK_EQ_INT(c.valid, 1);
    GK_CHECK(gk_safety_cert_text(&c, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "SAFETY CERT|Alice|CNC Safety L1|85.0|valid") != NULL);
    GK_CHECK_EQ_INT(gk_safety_cert_issue(&c, 70.0, 1000.0, 80.0),
                    GK_ERR_STATE);
    GK_CHECK_EQ_INT(c.valid, 0);
    GK_CHECK_EQ_INT(gk_safety_cert_issue(NULL, 1.0, 1.0, 0.0),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_safety_cert_text(NULL, buf, sizeof(buf)), 0);
}

static void test_emergency_leakage(void)
{
    gk_emergency_plan p;
    gk_leakage_protector l;

    gk_emergency_plan_init(&p);
    GK_CHECK_EQ_INT(gk_emergency_add(&p, "Fire drill", 120.0), 1);
    GK_CHECK_EQ_INT(gk_emergency_add(&p, "Earthquake drill", 60.0), 2);
    GK_CHECK_EQ_INT(gk_emergency_all_pass(&p), 0);
    GK_CHECK_EQ_INT(gk_emergency_complete(&p, 1, 90.0), GK_OK);
    GK_CHECK_EQ_INT(gk_emergency_complete(&p, 2, 45.0), GK_OK);
    GK_CHECK_EQ_INT(gk_emergency_all_pass(&p), 1);
    GK_CHECK_EQ_INT(gk_emergency_complete(&p, 2, 90.0), GK_OK);
    GK_CHECK_EQ_INT(gk_emergency_all_pass(&p), 0);
    GK_CHECK_EQ_INT(gk_emergency_complete(&p, 9, 1.0), GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_emergency_complete(&p, 1, -1.0), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_emergency_add(&p, "x", 0.0), -1);

    gk_leakage_init(&l, 30.0, 100.0);
    GK_CHECK_EQ_INT(gk_leakage_test(&l, 10.0, 0.0), GK_OK);
    GK_CHECK_EQ_INT(l.tripped, 0);
    GK_CHECK_EQ_INT(gk_leakage_pass(&l), 0);
    GK_CHECK_EQ_INT(gk_leakage_test(&l, 35.0, 80.0), GK_OK);
    GK_CHECK_EQ_INT(l.tripped, 1);
    GK_CHECK_EQ_INT(gk_leakage_pass(&l), 1);
    GK_CHECK_EQ_INT(gk_leakage_test(&l, 35.0, 150.0), GK_OK);
    GK_CHECK_EQ_INT(gk_leakage_pass(&l), 0);
    GK_CHECK_EQ_INT(gk_leakage_test(&l, -1.0, 0.0), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_leakage_pass(NULL), 0);
}

int main(void)
{
    test_procedure();
    test_power_sequences();
    test_ppe();
    test_hazard();
    test_estop();
    test_loto();
    test_5s();
    test_incidents();
    test_violations_score();
    test_cert();
    test_emergency_leakage();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
