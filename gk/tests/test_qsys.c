#include "gk_test.h"
#include "gk/gk_qsys.h"

#include <math.h>
#include <string.h>

static void test_standards_docs(void)
{
    gk_qsys_doc d;
    GK_CHECK_STR_EQ(gk_qsys_standard_name(GK_QSYS_ISO9001), "ISO9001");
    GK_CHECK_EQ_INT(gk_qsys_standard_clauses(GK_QSYS_AS9100), 15);
    GK_CHECK_STR_EQ(gk_qsys_doc_name(GK_QSYS_DOC_INSPECTION), "inspection-spec");
    gk_qsys_doc_init(&d, "QA-001", GK_QSYS_DOC_MANUAL);
    GK_CHECK_EQ_INT(d.revision, 1);
    GK_CHECK_EQ_INT(d.approved, 0);
    GK_CHECK(gk_qsys_doc_approve(&d) == GK_OK);
    GK_CHECK_EQ_INT(d.approved, 1);
    GK_CHECK(gk_qsys_doc_revise(&d) == GK_OK);
    GK_CHECK_EQ_INT(d.revision, 2);
    GK_CHECK_EQ_INT(d.approved, 0);
}

static void test_audits(void)
{
    gk_qsys_audit a;
    GK_CHECK_STR_EQ(gk_qsys_audit_name(GK_QSYS_AUDIT_INTERNAL), "internal-audit");
    gk_qsys_audit_init(&a, GK_QSYS_AUDIT_INTERNAL);
    GK_CHECK(fabs(a.score - 100.0) < 1e-9);
    GK_CHECK(gk_qsys_audit_finding(&a, 0) == GK_OK);
    GK_CHECK(fabs(a.score - 98.0) < 1e-9);
    GK_CHECK_EQ_INT(gk_qsys_audit_passed(&a, 90.0), 1);
    GK_CHECK(gk_qsys_audit_finding(&a, 1) == GK_OK);
    GK_CHECK_EQ_INT(a.major, 1);
    GK_CHECK_EQ_INT(gk_qsys_audit_passed(&a, 90.0), 0);
}

static void test_actions(void)
{
    gk_qsys_action a;
    GK_CHECK_STR_EQ(gk_qsys_action_name(GK_QSYS_ACTION_PREVENTIVE), "preventive");
    gk_qsys_action_init(&a, GK_QSYS_ACTION_CORRECTIVE, "fix root cause", 2);
    GK_CHECK(gk_qsys_action_close(&a) == GK_ERR_STATE);
    GK_CHECK(gk_qsys_action_step(&a) == GK_OK);
    GK_CHECK(gk_qsys_action_step(&a) == GK_OK);
    GK_CHECK(gk_qsys_action_step(&a) == GK_ERR_STATE);
    GK_CHECK(gk_qsys_action_close(&a) == GK_OK);
    GK_CHECK_EQ_INT(gk_qsys_action_complete(&a), 1);
}

static void test_8d_5why(void)
{
    gk_qsys_8d r;
    gk_qsys_5why w;
    int i;
    gk_qsys_8d_init(&r, 1);
    for (i = 0; i < 8; i++) {
        GK_CHECK(gk_qsys_8d_advance(&r) == GK_OK);
    }
    GK_CHECK_EQ_INT(gk_qsys_8d_complete(&r), 1);
    GK_CHECK(gk_qsys_8d_advance(&r) == GK_ERR_STATE);
    gk_qsys_5why_init(&w);
    GK_CHECK(gk_qsys_5why_add(&w, "why1") == GK_OK);
    GK_CHECK(gk_qsys_5why_add(&w, "why2") == GK_OK);
    GK_CHECK_STR_EQ(gk_qsys_5why_root(&w), "why2");
    GK_CHECK_EQ_INT(w.depth, 2);
}

static void test_fishbone_fmea_spc(void)
{
    gk_qsys_fishbone f;
    gk_qsys_fmea_item it;
    gk_qsys_spc s;
    GK_CHECK_STR_EQ(gk_qsys_bone_name(GK_QSYS_BONE_MACHINE), "machine");
    gk_qsys_fishbone_init(&f);
    GK_CHECK(gk_qsys_fishbone_add(&f, GK_QSYS_BONE_MAN) == GK_OK);
    GK_CHECK(gk_qsys_fishbone_add(&f, GK_QSYS_BONE_MACHINE) == GK_OK);
    GK_CHECK(gk_qsys_fishbone_add(&f, GK_QSYS_BONE_MACHINE) == GK_OK);
    GK_CHECK_EQ_INT(gk_qsys_fishbone_total(&f), 3);
    GK_CHECK_EQ_INT(gk_qsys_fishbone_main(&f), GK_QSYS_BONE_MACHINE);
    gk_qsys_fmea_init(&it, 8, 5, 3);
    GK_CHECK_EQ_INT(gk_qsys_rpn(&it), 120);
    GK_CHECK_EQ_INT(gk_qsys_fmea_critical(&it, 100), 1);
    gk_qsys_fmea_init(&it, 9, 1, 1);
    GK_CHECK_EQ_INT(gk_qsys_fmea_critical(&it, 100), 1);
    gk_qsys_spc_init(&s, 10.0, 0.0);
    GK_CHECK(gk_qsys_spc_add(&s, 5.0) == GK_OK);
    GK_CHECK(gk_qsys_spc_add(&s, 6.0) == GK_OK);
    GK_CHECK(gk_qsys_spc_add(&s, 4.0) == GK_OK);
    GK_CHECK(fabs(gk_qsys_spc_mean(&s) - 5.0) < 1e-9);
    GK_CHECK(gk_qsys_spc_stddev(&s) > 0.0);
    GK_CHECK(gk_qsys_spc_cpk(&s) > 0.0);
}

int main(void)
{
    test_standards_docs();
    test_audits();
    test_actions();
    test_8d_5why();
    test_fishbone_fmea_spc();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
