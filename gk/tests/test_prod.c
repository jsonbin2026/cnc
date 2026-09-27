#include "gk_test.h"
#include "gk/gk_prod.h"

#include <math.h>
#include <string.h>

static void test_cell(void)
{
    gk_prod_cell c;
    int m;
    gk_prod_cell_init(&c);
    GK_CHECK_EQ_INT(gk_prod_cell_add(&c, "M1", 10.0), 0);
    GK_CHECK_EQ_INT(gk_prod_cell_add(&c, "M2", 10.0), 1);
    m = gk_prod_cell_assign(&c, 6.0);
    GK_CHECK(m == 0 || m == 1);
    m = gk_prod_cell_assign(&c, 6.0);
    GK_CHECK(m >= 0);
    GK_CHECK_EQ_INT(gk_prod_cell_assign(&c, 100.0), -1);
    GK_CHECK(gk_prod_cell_utilization(&c, 0) > 0.0);
    GK_CHECK(gk_prod_cell_utilization(&c, 0) <= 1.0);
}

static void test_line(void)
{
    gk_prod_line l;
    gk_prod_line_init(&l);
    GK_CHECK_EQ_INT(gk_prod_line_add(&l, "S1", 12.0), 0);
    GK_CHECK_EQ_INT(gk_prod_line_add(&l, "S2", 30.0), 1);
    GK_CHECK_EQ_INT(gk_prod_line_add(&l, "S3", 18.0), 2);
    GK_CHECK_EQ_INT(gk_prod_line_bottleneck(&l), 1);
    GK_CHECK(fabs(gk_prod_line_takt(&l) - 30.0) < 1e-9);
}

static void test_agv(void)
{
    gk_prod_agv a;
    gk_prod_agv_init(&a, 2.0);
    GK_CHECK(fabs(gk_prod_agv_travel_time(&a, 10.0) - 5.0) < 1e-9);
    GK_CHECK(gk_prod_agv_move(&a, 10.0) == GK_OK);
    GK_CHECK(fabs(a.position_m - 10.0) < 1e-9);
    GK_CHECK(a.battery_pct < 100.0);
    a.battery_pct = 50.0;
    GK_CHECK(gk_prod_agv_charge(&a, 100.0) == GK_OK);
    GK_CHECK(fabs(a.battery_pct - 100.0) < 1e-9);
}

static void test_robot(void)
{
    gk_prod_robot r;
    double t;
    gk_prod_robot_init(&r);
    t = gk_prod_robot_cycle_time(&r, 1.0, 1.0);
    GK_CHECK(t > 2.0);
    r.grip_s = 100.0;
    GK_CHECK(gk_prod_robot_cycle_time(&r, 1.0, 1.0) > t);
}

static void test_vision_rfid(void)
{
    double xs[4] = {0.0, 2.0, 0.0, 2.0};
    double ys[4] = {0.0, 0.0, 4.0, 4.0};
    double cx = 0.0, cy = 0.0;
    gk_prod_rfid tag;
    char out[64];
    GK_CHECK(gk_prod_vision_find(xs, ys, 4, &cx, &cy) == GK_OK);
    GK_CHECK(fabs(cx - 1.0) < 1e-9);
    GK_CHECK(fabs(cy - 2.0) < 1e-9);
    GK_CHECK(fabs(gk_prod_vision_offset(10.5, 10.0) - 0.5) < 1e-9);
    gk_prod_rfid_init(&tag, "TAG-01");
    GK_CHECK_STR_EQ(tag.tag_id, "TAG-01");
    GK_CHECK(gk_prod_rfid_read(&tag, out, sizeof(out)) == GK_ERR_STATE);
    GK_CHECK(gk_prod_rfid_write(&tag, "part=alice") == GK_OK);
    GK_CHECK(gk_prod_rfid_read(&tag, out, sizeof(out)) == GK_OK);
    GK_CHECK_STR_EQ(out, "part=alice");
}

static void test_mes_erp_wms(void)
{
    gk_prod_mes m;
    gk_prod_erp e;
    gk_prod_wms w;
    gk_prod_mes_init(&m, "WO-1");
    GK_CHECK_STR_EQ(gk_prod_mes_state_name(m.state), "planned");
    GK_CHECK(gk_prod_mes_report(&m, 1, 0) == GK_ERR_STATE);
    GK_CHECK(gk_prod_mes_advance(&m) == GK_OK);
    GK_CHECK(gk_prod_mes_advance(&m) == GK_OK);
    GK_CHECK(m.state == GK_PROD_MES_RUNNING);
    GK_CHECK(gk_prod_mes_report(&m, 8, 2) == GK_OK);
    GK_CHECK_EQ_INT(m.good, 8);
    GK_CHECK_EQ_INT(m.scrap, 2);
    gk_prod_erp_init(&e, "SO-9", 100, 30);
    GK_CHECK(fabs(gk_prod_erp_progress(&e)) < 1e-9);
    GK_CHECK(gk_prod_erp_receive(&e, 40) == GK_OK);
    GK_CHECK(fabs(gk_prod_erp_progress(&e) - 0.4) < 1e-9);
    gk_prod_wms_init(&w, "BOLT", 100);
    GK_CHECK_EQ_INT(gk_prod_wms_available(&w), 100);
    GK_CHECK(gk_prod_wms_reserve(&w, 30) == GK_OK);
    GK_CHECK_EQ_INT(gk_prod_wms_available(&w), 70);
    GK_CHECK(gk_prod_wms_reserve(&w, 80) == GK_ERR_STATE);
    GK_CHECK(gk_prod_wms_pick(&w, 20) == GK_OK);
    GK_CHECK_EQ_INT(w.on_hand, 80);
    GK_CHECK_EQ_INT(w.reserved, 10);
}

static void test_asrs(void)
{
    gk_prod_asrs a;
    gk_prod_asrs_init(&a, 2);
    GK_CHECK(gk_prod_asrs_retrieve(&a) == GK_ERR_STATE);
    GK_CHECK(gk_prod_asrs_store(&a) == GK_OK);
    GK_CHECK(gk_prod_asrs_store(&a) == GK_OK);
    GK_CHECK(gk_prod_asrs_store(&a) == GK_ERR_STATE);
    GK_CHECK(gk_prod_asrs_retrieve(&a) == GK_OK);
    GK_CHECK_EQ_INT(a.stored, 1);
    GK_CHECK(fabs(gk_prod_asrs_cycle_time(&a) - 40.0) < 1e-9);
}

static void test_schedule(void)
{
    gk_prod_job jobs[3];
    int order[3];
    memset(jobs, 0, sizeof(jobs));
    jobs[0].priority = 1;
    jobs[0].due_day = 10;
    jobs[1].priority = 5;
    jobs[1].due_day = 20;
    jobs[2].priority = 5;
    jobs[2].due_day = 5;
    GK_CHECK(gk_prod_schedule(jobs, 3, order) == GK_OK);
    GK_CHECK_EQ_INT(order[0], 2);
    GK_CHECK_EQ_INT(order[1], 1);
    GK_CHECK_EQ_INT(order[2], 0);
}

static void test_process_order(void)
{
    gk_prod_operation ops[3];
    int order[3];
    memset(ops, 0, sizeof(ops));
    strcpy(ops[0].op, "A");
    ops[0].predecessor = -1;
    strcpy(ops[1].op, "B");
    ops[1].predecessor = 0;
    strcpy(ops[2].op, "C");
    ops[2].predecessor = 1;
    GK_CHECK(gk_prod_optimize_process(ops, 3, order) == GK_OK);
    GK_CHECK_EQ_INT(order[0], 0);
    GK_CHECK_EQ_INT(order[1], 1);
    GK_CHECK_EQ_INT(order[2], 2);
    ops[0].predecessor = 2;
    GK_CHECK(gk_prod_optimize_process(ops, 3, order) == GK_ERR_STATE);
}

static void test_path(void)
{
    double xs[4] = {0.0, 10.0, 10.0, 0.0};
    double ys[4] = {0.0, 0.0, 10.0, 10.0};
    int order[4];
    double len = 0.0;
    GK_CHECK(gk_prod_optimize_path(xs, ys, 4, order, &len) == GK_OK);
    GK_CHECK_EQ_INT(order[0], 0);
    GK_CHECK_EQ_INT(order[1], 1);
    GK_CHECK_EQ_INT(order[2], 2);
    GK_CHECK_EQ_INT(order[3], 3);
    GK_CHECK(fabs(len - 30.0) < 1e-9);
}

static double bowl(double x, void *ctx)
{
    (void)ctx;
    return (x - 3.0) * (x - 3.0);
}

static double peak(double x, void *ctx)
{
    (void)ctx;
    return -(x - 7.0) * (x - 7.0) + 10.0;
}

static void test_optimizers(void)
{
    gk_prod_result r;
    GK_CHECK(gk_prod_optimize(0.0, 10.0, 101, bowl, NULL,
                              GK_PROD_MINIMISE, &r) == GK_OK);
    GK_CHECK(fabs(r.x - 3.0) < 0.2);
    GK_CHECK_EQ_INT(r.evaluations, 101);
    GK_CHECK(gk_prod_optimize_params(peak, NULL, 0.0, 10.0, 101, &r) ==
             GK_OK);
    GK_CHECK(fabs(r.x - 7.0) < 0.2);
    GK_CHECK(gk_prod_optimize_cost(bowl, NULL, 0.0, 10.0, 101, &r) == GK_OK);
    GK_CHECK(fabs(r.x - 3.0) < 0.2);
    GK_CHECK(gk_prod_optimize_energy(bowl, NULL, 0.0, 10.0, 101, &r) ==
             GK_OK);
    GK_CHECK(gk_prod_optimize_quality(peak, NULL, 0.0, 10.0, 101, &r) ==
             GK_OK);
    GK_CHECK(gk_prod_optimize_efficiency(peak, NULL, 0.0, 10.0, 101, &r) ==
             GK_OK);
    GK_CHECK(gk_prod_optimize_tech(peak, NULL, 0.0, 10.0, 101, &r) == GK_OK);
    GK_CHECK(gk_prod_optimize_comprehensive(peak, NULL, 0.0, 10.0, 101, &r) ==
             GK_OK);
    GK_CHECK(fabs(r.x - 7.0) < 0.2);
    GK_CHECK(gk_prod_optimize(0.0, 10.0, 0, bowl, NULL, GK_PROD_MINIMISE,
                              &r) == GK_ERR_INVALID_ARG);
}

int main(void)
{
    test_cell();
    test_line();
    test_agv();
    test_robot();
    test_vision_rfid();
    test_mes_erp_wms();
    test_asrs();
    test_schedule();
    test_process_order();
    test_path();
    test_optimizers();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
