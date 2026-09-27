#include "gk_test.h"
#include "gk/gk_procd.h"

#include <math.h>

static void test_route(void)
{
    gk_procd_route r;
    gk_procd_route_init(&r, "shaft");
    GK_CHECK(gk_procd_route_add(&r, "face", 10, 1) == GK_OK);
    GK_CHECK(gk_procd_route_add(&r, "center-drill", 10, 2) == GK_OK);
    GK_CHECK(gk_procd_route_add(&r, "turn-od", 20, 1) == GK_OK);
    GK_CHECK_EQ_INT(gk_procd_route_count(&r), 3);
    GK_CHECK_EQ_INT(gk_procd_route_operations(&r), 20);
    GK_CHECK(gk_procd_route_add(&r, "x", -1, 1) == GK_ERR_INVALID_ARG);
}

static void test_allowance_cutting(void)
{
    gk_procd_allowance a;
    gk_procd_cutting c;
    gk_procd_allowance_init(&a, 5.0, 0.5);
    a.semi_mm = 1.0;
    GK_CHECK(fabs(gk_procd_allowance_rough(&a) - 3.5) < 1e-9);
    GK_CHECK_EQ_INT(gk_procd_allowance_valid(&a), 1);
    a.stock_mm = 1.0;
    GK_CHECK_EQ_INT(gk_procd_allowance_valid(&a), 0);
    gk_procd_cutting_init(&c, 150.0, 300.0, 2.0);
    GK_CHECK(fabs(gk_procd_cutting_time(&c, 600.0) - 2.0) < 1e-9);
    gk_procd_cutting_init(&c, 150.0, 0.0, 2.0);
    GK_CHECK(fabs(gk_procd_cutting_time(&c, 600.0)) < 1e-12);
}

static void test_selection(void)
{
    gk_procd_choice ch;
    GK_CHECK_STR_EQ(gk_procd_selection_name(GK_PROCD_SEL_GAUGE), "gauge");
    gk_procd_choice_init(&ch, GK_PROCD_SEL_TOOL, "D10-endmill", "roughing");
    GK_CHECK_EQ_INT(gk_procd_choice_ok(&ch), 1);
    GK_CHECK_STR_EQ(ch.pick, "D10-endmill");
    gk_procd_choice_init(&ch, GK_PROCD_SEL_TOOL, "", "none");
    GK_CHECK_EQ_INT(gk_procd_choice_ok(&ch), 0);
}

static void test_quotas(void)
{
    gk_procd_time_quota tq;
    gk_procd_material_quota mq;
    gk_procd_time_quota_init(&tq, 30.0, 5.0, 10);
    GK_CHECK(fabs(gk_procd_time_quota_total(&tq) - 80.0) < 1e-9);
    GK_CHECK(fabs(gk_procd_time_quota_per_part(&tq) - 8.0) < 1e-9);
    gk_procd_material_quota_init(&mq, 2.0, 0.1);
    GK_CHECK(fabs(gk_procd_material_quota_required(&mq, 10) -
                  2.0 * 10.0 / 0.9) < 1e-9);
}

static void test_plan(void)
{
    gk_procd_plan p;
    GK_CHECK_STR_EQ(gk_procd_stage_name(GK_PROCD_STAGE_FREEZE), "freeze");
    gk_procd_plan_init(&p, "PLAN-1");
    GK_CHECK_EQ_INT(p.stage, GK_PROCD_STAGE_CARD);
    GK_CHECK_EQ_INT(gk_procd_plan_frozen(&p), 0);
    GK_CHECK(gk_procd_plan_advance(&p, GK_PROCD_STAGE_REVIEW) == GK_OK);
    GK_CHECK(gk_procd_plan_advance(&p, GK_PROCD_STAGE_CARD) == GK_ERR_STATE);
    GK_CHECK(gk_procd_plan_revise(&p, "update feed") == GK_OK);
    GK_CHECK_EQ_INT(p.revision, 2);
    GK_CHECK(gk_procd_plan_advance(&p, GK_PROCD_STAGE_FREEZE) == GK_OK);
    GK_CHECK_EQ_INT(gk_procd_plan_frozen(&p), 1);
    GK_CHECK(gk_procd_plan_close(&p) == GK_OK);
    GK_CHECK(gk_procd_plan_advance(&p, GK_PROCD_STAGE_EXPERT) == GK_ERR_STATE);
}

static void test_knowledge(void)
{
    gk_procd_knowledge k;
    const char *rec;
    gk_procd_knowledge_init(&k);
    GK_CHECK_EQ_INT(gk_procd_knowledge_count(&k), 0);
    GK_CHECK(gk_procd_knowledge_add(&k, "aluminium: use high speed") == GK_OK);
    GK_CHECK(gk_procd_knowledge_add(&k, "titanium: use low speed") == GK_OK);
    GK_CHECK_EQ_INT(gk_procd_knowledge_count(&k), 2);
    rec = gk_procd_expert_recommend(&k, "titanium");
    GK_CHECK(rec != NULL);
    GK_CHECK(gk_procd_expert_recommend(&k, "plastic") == NULL);
}

int main(void)
{
    test_route();
    test_allowance_cutting();
    test_selection();
    test_quotas();
    test_plan();
    test_knowledge();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
