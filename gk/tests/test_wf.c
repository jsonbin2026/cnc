#include "gk_test.h"
#include "gk/gk_wf.h"

#include <math.h>
#include <string.h>

static void test_procedure(void)
{
    gk_wf_procedure p;
    GK_CHECK(gk_wf_init(&p, "power-on") == GK_OK);
    GK_CHECK_STR_EQ(p.name, "power-on");
    GK_CHECK(gk_wf_add_step(&p, "inspect") == GK_OK);
    GK_CHECK(gk_wf_add_step(&p, "home") == GK_OK);
    GK_CHECK_EQ_INT(p.step_count, 2);
    GK_CHECK(gk_wf_start(&p) == GK_OK);
    GK_CHECK(fabs(gk_wf_progress(&p)) < 1e-12);
    GK_CHECK(gk_wf_complete_step(&p) == GK_OK);
    GK_CHECK(fabs(gk_wf_progress(&p) - 0.5) < 1e-9);
    GK_CHECK_EQ_INT(gk_wf_done(&p), 0);
    GK_CHECK(gk_wf_complete_step(&p) == GK_OK);
    GK_CHECK_EQ_INT(gk_wf_done(&p), 1);
    GK_CHECK(gk_wf_complete_step(&p) == GK_ERR_STATE);
}

static void test_abort_estop(void)
{
    gk_wf_procedure p;
    gk_wf_init(&p, "batch");
    gk_wf_add_step(&p, "stage");
    gk_wf_add_step(&p, "run");
    gk_wf_start(&p);
    GK_CHECK(gk_wf_abort(&p) == GK_OK);
    GK_CHECK_EQ_INT(gk_wf_done(&p), 0);
    gk_wf_init(&p, "e");
    gk_wf_add_step(&p, "a");
    gk_wf_start(&p);
    GK_CHECK(gk_wf_estop(&p) == GK_OK);
    GK_CHECK_EQ_INT(p.aborted, 1);
}

static void test_templates(void)
{
    gk_wf_procedure p;
    GK_CHECK(gk_wf_load_template(&p, 1206) == GK_OK);
    GK_CHECK_EQ_INT(p.step_count, 6);
    GK_CHECK_STR_EQ(p.name, "power-on");
    GK_CHECK(gk_wf_load_template(&p, 1213) == GK_OK);
    GK_CHECK_STR_EQ(p.steps[3].name, "swap tool");
    GK_CHECK(gk_wf_load_template(&p, 9999) == GK_ERR_NOT_FOUND);
}

static void test_alarm_handover(void)
{
    gk_wf_alarm_action a;
    char out[128];
    gk_wf_alarm_action_init(&a, 41, "reduce load");
    GK_CHECK_EQ_INT(a.code, 41);
    GK_CHECK(gk_wf_alarm_resolve(&a) == GK_OK);
    GK_CHECK_EQ_INT(a.resolved, 1);
    gk_wf_alarm_action_init(&a, 1, "");
    GK_CHECK(gk_wf_alarm_resolve(&a) == GK_ERR_STATE);
    GK_CHECK(gk_wf_handover(out, sizeof(out), "Alice", "Bob", "stable") ==
             GK_OK);
    GK_CHECK(strstr(out, "Alice->Bob") != NULL);
}

static void test_processes(void)
{
    int passed = 0, total = 0;
    GK_CHECK_STR_EQ(gk_wf_process_name(GK_WF_PROC_5S), "5S");
    GK_CHECK(gk_wf_process_run(GK_WF_PROC_SAFETY, &passed, &total) == GK_OK);
    GK_CHECK_EQ_INT(total, 12);
    GK_CHECK_EQ_INT(passed, 12);
    GK_CHECK(gk_wf_process_run(GK_WF_PROC_CLEANING, &passed, &total) == GK_OK);
    GK_CHECK_EQ_INT(total, 6);
}

static void test_checklists(void)
{
    gk_wf_checklist c;
    GK_CHECK_STR_EQ(gk_wf_checklist_name(GK_WF_CHK_MONTHLY), "monthly");
    GK_CHECK_EQ_INT(gk_wf_checklist_item_count(GK_WF_CHK_YEARLY), 30);
    GK_CHECK(gk_wf_checklist_init(&c, GK_WF_CHK_DAILY) == GK_OK);
    GK_CHECK_EQ_INT(c.item_count, 10);
    GK_CHECK_EQ_INT(gk_wf_checklist_all(&c), 0);
    GK_CHECK_EQ_INT(gk_wf_checklist_score(&c), 0);
    GK_CHECK(gk_wf_checklist_check(&c, 0, 1) == GK_OK);
    GK_CHECK_EQ_INT(gk_wf_checklist_score(&c), 10);
    GK_CHECK(gk_wf_checklist_check(&c, 99, 1) == GK_ERR_OUT_OF_RANGE);
    {
        int i;
        for (i = 0; i < c.item_count; i++) {
            GK_CHECK(gk_wf_checklist_check(&c, i, 1) == GK_OK);
        }
    }
    GK_CHECK_EQ_INT(gk_wf_checklist_all(&c), 1);
    GK_CHECK_EQ_INT(gk_wf_checklist_score(&c), 100);
}

int main(void)
{
    test_procedure();
    test_abort_estop();
    test_templates();
    test_alarm_handover();
    test_processes();
    test_checklists();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
