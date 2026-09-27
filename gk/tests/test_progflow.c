#include "gk_test.h"
#include "gk/gk_progflow.h"

#include <math.h>

static void test_stages(void)
{
    gk_progflow p;
    GK_CHECK_STR_EQ(gk_progflow_stage_name(GK_PROGFLOW_DRAWING),
                    "drawing-analysis");
    GK_CHECK_STR_EQ(gk_progflow_stage_name(GK_PROGFLOW_RESTORE), "restore");
    GK_CHECK_EQ_INT(gk_progflow_stage_index(GK_PROGFLOW_CAM), 3);
    GK_CHECK_EQ_INT(gk_progflow_stage_index((gk_progflow_stage)99), -1);
    gk_progflow_init(&p, "O1000");
    GK_CHECK_STR_EQ(p.name, "O1000");
    GK_CHECK_EQ_INT(p.revision, 1);
    GK_CHECK(fabs(gk_progflow_progress(&p)) < 1e-12);
}

static void test_flags(void)
{
    gk_progflow p;
    gk_progflow_init(&p, "O1000");
    GK_CHECK_EQ_INT(gk_progflow_flag(&p, "simulated"), 0);
    GK_CHECK(gk_progflow_set_flag(&p, "simulated", 1) == GK_OK);
    GK_CHECK_EQ_INT(gk_progflow_flag(&p, "simulated"), 1);
    GK_CHECK(gk_progflow_set_flag(&p, "bogus", 1) == GK_ERR_NOT_FOUND);
    GK_CHECK(gk_progflow_set_flag(&p, "permission-ok", 1) == GK_OK);
    GK_CHECK_EQ_INT(gk_progflow_flag(&p, "permission-ok"), 1);
}

static void test_flow(void)
{
    gk_progflow p;
    gk_progflow_init(&p, "O1000");
    /* cannot first-cut before simulation */
    GK_CHECK(gk_progflow_advance(&p, GK_PROGFLOW_FIRST_CUT) == GK_ERR_STATE);
    GK_CHECK(gk_progflow_advance(&p, GK_PROGFLOW_SIMULATION) == GK_OK);
    GK_CHECK_EQ_INT(p.simulated, 1);
    GK_CHECK(gk_progflow_advance(&p, GK_PROGFLOW_VERIFY) == GK_OK);
    GK_CHECK(gk_progflow_advance(&p, GK_PROGFLOW_FIRST_CUT) == GK_OK);
    /* archive requires freeze */
    GK_CHECK(gk_progflow_advance(&p, GK_PROGFLOW_ARCHIVE) == GK_ERR_STATE);
    GK_CHECK(gk_progflow_advance(&p, GK_PROGFLOW_FREEZE) == GK_OK);
    GK_CHECK_EQ_INT(p.frozen, 1);
    GK_CHECK(gk_progflow_advance(&p, GK_PROGFLOW_ARCHIVE) == GK_OK);
    GK_CHECK_EQ_INT(p.archived, 1);
    /* frozen blocks most advances */
    GK_CHECK(gk_progflow_advance(&p, GK_PROGFLOW_BACKUP) == GK_ERR_STATE);
    GK_CHECK(fabs(gk_progflow_progress(&p) - 10.0 / 14.0) < 1e-9);
}

static void test_sim_first_article(void)
{
    gk_progflow_sim s;
    gk_progflow_first_article f;
    gk_progflow_sim_init(&s);
    GK_CHECK_EQ_INT(gk_progflow_sim_clean(&s), 1);
    s.collisions = 1;
    GK_CHECK_EQ_INT(gk_progflow_sim_clean(&s), 0);
    gk_progflow_first_article_init(&f, 10.0, 0.05, 10.02);
    GK_CHECK_EQ_INT(gk_progflow_first_article_ok(&f), 1);
    f.measured = 10.2;
    GK_CHECK_EQ_INT(gk_progflow_first_article_ok(&f), 0);
}

static void test_backup_restore(void)
{
    gk_progflow p;
    gk_progflow_backup b;
    gk_progflow_init(&p, "O1000");
    p.revision = 5;
    p.frozen = 1;
    gk_progflow_backup_init(&b, "/backup/O1000.nc", 4);
    GK_CHECK_STR_EQ(b.path, "/backup/O1000.nc");
    GK_CHECK(gk_progflow_restore(&p, &b) == GK_OK);
    GK_CHECK_EQ_INT(p.revision, 4);
    GK_CHECK_EQ_INT(p.frozen, 0);
    GK_CHECK_EQ_INT(p.current, GK_PROGFLOW_RESTORE);
    gk_progflow_backup_init(&b, "", 4);
    GK_CHECK(gk_progflow_restore(&p, &b) == GK_ERR_STATE);
}

int main(void)
{
    test_stages();
    test_flags();
    test_flow();
    test_sim_first_article();
    test_backup_restore();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
