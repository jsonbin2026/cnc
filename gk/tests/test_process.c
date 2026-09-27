#include "gk_test.h"

#include "gk/gk_process.h"

#include <math.h>
#include <string.h>

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

static void test_stage_names(void)
{
    int i;

    for (i = 0; i < GK_STAGE_COUNT; ++i) {
        GK_CHECK(strlen(gk_stage_name((gk_stage_kind)i)) > 0);
    }
    GK_CHECK_STR_EQ(gk_stage_name(GK_STAGE_INCOMING_INSPECTION),
                    "incoming-inspection");
    GK_CHECK_STR_EQ(gk_stage_name(GK_STAGE_LIFECYCLE), "lifecycle-management");
    GK_CHECK_STR_EQ(gk_stage_status_name(GK_STAGE_FAILED), "failed");
    GK_CHECK_STR_EQ(gk_stage_status_name(GK_STAGE_SKIPPED), "skipped");
}

static void test_inspection(void)
{
    gk_inspection i;

    gk_inspection_init(&i, 50.0, 0.05);
    GK_CHECK(near(i.measured, 50.0, 1e-9));
    GK_CHECK_EQ_INT(gk_inspection_pass(&i), 1);
    i.measured = 50.04;
    GK_CHECK_EQ_INT(gk_inspection_pass(&i), 1);
    GK_CHECK(near(gk_inspection_deviation(&i), 0.04, 1e-9));
    i.measured = 50.10;
    GK_CHECK_EQ_INT(gk_inspection_pass(&i), 0);
    GK_CHECK_EQ_INT(gk_inspection_pass(NULL), 0);
    GK_CHECK_EQ_INT(gk_inspection_deviation(NULL), 0.0);
}

static void test_blank(void)
{
    gk_blank b;

    gk_blank_init(&b, 100.0, 50.0, 30.0, 2.0);
    GK_CHECK(near(gk_blank_volume(&b), 150000.0, 1e-6));
    GK_CHECK_EQ_INT(gk_blank_covers(&b, 96.0, 46.0, 26.0), 1);
    GK_CHECK_EQ_INT(gk_blank_covers(&b, 98.0, 48.0, 28.0), 0);
    GK_CHECK_EQ_INT(gk_blank_covers(NULL, 1, 1, 1), 0);
    GK_CHECK(gk_blank_volume(NULL) == 0.0);
}

static void test_setup(void)
{
    gk_setup s;

    gk_setup_init(&s);
    GK_CHECK_EQ_INT(s.aligned, 0);
    GK_CHECK_EQ_INT(gk_setup_align(&s, 1.0, 2.0, 3.0, 0.01), GK_OK);
    GK_CHECK_EQ_INT(s.aligned, 1);
    GK_CHECK(near(s.offset_x, 1.0, 1e-9));
    GK_CHECK_EQ_INT(gk_setup_within_tolerance(&s, 0.02), 1);
    GK_CHECK_EQ_INT(gk_setup_within_tolerance(&s, 0.005), 0);
    GK_CHECK_EQ_INT(gk_setup_align(&s, 0, 0, 0, -1.0), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_setup_within_tolerance(NULL, 1.0), 0);
}

static void test_tool_set(void)
{
    gk_tool_set t;

    gk_tool_set_init(&t, 7);
    GK_CHECK_EQ_INT(t.tool_id, 7);
    GK_CHECK_EQ_INT(t.measured, 0);
    GK_CHECK_EQ_INT(gk_tool_set_probe(&t, 120.5, 3.0), GK_OK);
    GK_CHECK_EQ_INT(t.measured, 1);
    GK_CHECK(near(t.length_offset, 120.5, 1e-9));
    GK_CHECK(near(t.radius_offset, 3.0, 1e-9));
    GK_CHECK_EQ_INT(gk_tool_set_probe(NULL, 0, 0), GK_ERR_INVALID_ARG);
}

static void test_first_article(void)
{
    gk_first_article fa;

    gk_first_article_init(&fa);
    GK_CHECK_EQ_INT(gk_first_article_evaluate(&fa, 10.0, 0.1, 10.05),
                    GK_ERR_STATE);
    fa.first_cut_done = 1;
    GK_CHECK_EQ_INT(gk_first_article_evaluate(&fa, 10.0, 0.1, 10.05), GK_OK);
    GK_CHECK_EQ_INT(fa.approved, 1);
    GK_CHECK_EQ_INT(gk_first_article_evaluate(&fa, 10.0, 0.1, 10.5), GK_OK);
    GK_CHECK_EQ_INT(fa.approved, 0);
    GK_CHECK_EQ_INT(gk_first_article_evaluate(NULL, 0, 0, 0),
                    GK_ERR_INVALID_ARG);
}

static void test_batch(void)
{
    gk_batch b;

    gk_batch_init(&b, 3);
    GK_CHECK_EQ_INT(gk_batch_remaining(&b), 3);
    GK_CHECK_EQ_INT(gk_batch_record(&b, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_batch_record(&b, 0), GK_OK);
    GK_CHECK_EQ_INT(gk_batch_record(&b, 1), GK_OK);
    GK_CHECK_EQ_INT(b.produced, 3);
    GK_CHECK_EQ_INT(b.good, 2);
    GK_CHECK_EQ_INT(b.scrap, 1);
    GK_CHECK(near(gk_batch_yield(&b), 2.0 / 3.0, 1e-9));
    GK_CHECK_EQ_INT(gk_batch_record(&b, 1), GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_batch_remaining(&b), 0);
    gk_batch z;
    gk_batch_init(&z, 5);
    GK_CHECK(gk_batch_yield(&z) == 0.0);
    GK_CHECK_EQ_INT(gk_batch_record(NULL, 1), GK_ERR_INVALID_ARG);
}

static void test_tool_life(void)
{
    gk_proc_tool t;

    gk_proc_tool_init(&t, 5, 60.0);
    GK_CHECK_EQ_INT(gk_proc_tool_expired(&t), 0);
    GK_CHECK_EQ_INT(gk_proc_tool_use(&t, 30.0), GK_OK);
    GK_CHECK_EQ_INT(gk_proc_tool_expired(&t), 0);
    GK_CHECK_EQ_INT(gk_proc_tool_use(&t, 30.0), GK_OK);
    GK_CHECK_EQ_INT(gk_proc_tool_expired(&t), 1);
    GK_CHECK_EQ_INT(t.needs_change, 1);
    GK_CHECK_EQ_INT(gk_proc_tool_use(&t, -1.0), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_proc_tool_expired(NULL), 0);
}

static void test_finish(void)
{
    gk_finish f;

    gk_finish_init(&f);
    GK_CHECK_EQ_INT(gk_finish_ok(&f, 5.0, 0.1), 0);
    GK_CHECK_EQ_INT(gk_finish_clean(&f, 2.0), GK_OK);
    GK_CHECK_EQ_INT(gk_finish_deburr(&f, 0.2), GK_OK);
    GK_CHECK_EQ_INT(gk_finish_ok(&f, 5.0, 0.1), 1);
    GK_CHECK_EQ_INT(gk_finish_ok(&f, 1.0, 0.1), 0);
    GK_CHECK_EQ_INT(gk_finish_ok(&f, 5.0, 0.3), 0);
    GK_CHECK_EQ_INT(gk_finish_clean(&f, -1.0), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_finish_deburr(&f, -1.0), GK_ERR_INVALID_ARG);
}

static void test_package(void)
{
    gk_package p;

    gk_package_init(&p);
    GK_CHECK_EQ_INT(p.sealed, 0);
    GK_CHECK_EQ_INT(gk_package_seal(&p, "PKG-0001", 1.5), GK_OK);
    GK_CHECK_EQ_INT(p.sealed, 1);
    GK_CHECK_EQ_INT(p.labeled, 1);
    GK_CHECK_STR_EQ(p.package_id, "PKG-0001");
    GK_CHECK(near(p.weight_kg, 1.5, 1e-9));
    GK_CHECK_EQ_INT(gk_package_seal(NULL, "x", 1.0), GK_ERR_INVALID_ARG);
}

static void test_disposition(void)
{
    double total = -1.0;

    GK_CHECK_EQ_INT(gk_disposition_decide(0.02, 0.05, 0.10),
                    GK_DISPOSITION_PASS);
    GK_CHECK_EQ_INT(gk_disposition_decide(-0.08, 0.05, 0.10),
                    GK_DISPOSITION_REWORK);
    GK_CHECK_EQ_INT(gk_disposition_decide(0.30, 0.05, 0.10),
                    GK_DISPOSITION_SCRAP);
    GK_CHECK_EQ_INT(gk_disposition_decide(0.10, 0.05, 0.0),
                    GK_DISPOSITION_SCRAP);
    GK_CHECK_STR_EQ(gk_disposition_name(GK_DISPOSITION_REWORK), "rework");
    GK_CHECK_EQ_INT(gk_rework_schedule(4, 2.5, &total), 4);
    GK_CHECK(near(total, 10.0, 1e-9));
    GK_CHECK_EQ_INT(gk_rework_schedule(-1, 2.5, &total), 0);
    GK_CHECK_EQ_INT(gk_rework_schedule(4, 2.5, NULL), 4);
}

static void test_warehouse(void)
{
    gk_warehouse w;

    gk_warehouse_init(&w);
    GK_CHECK_EQ_INT(gk_warehouse_store(&w, "A-01-03", 25, 12.5), GK_OK);
    GK_CHECK_STR_EQ(w.location, "A-01-03");
    GK_CHECK_EQ_INT(w.quantity, 25);
    GK_CHECK(near(w.total_weight, 12.5, 1e-9));
    GK_CHECK_EQ_INT(gk_warehouse_store(NULL, "x", 1, 1.0),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_warehouse_store(&w, "x", -1, 1.0),
                    GK_ERR_INVALID_ARG);
}

static void test_trace(void)
{
    gk_trace_log l;
    const gk_trace_record *r;

    gk_trace_log_init(&l);
    GK_CHECK_EQ_INT(gk_trace_add(&l, "SN-001", "Flange", "B2026", 1000.0,
                                 "alice") > 0, 1);
    GK_CHECK_EQ_INT(gk_trace_add(&l, "SN-002", "Flange", "B2026", 1001.0,
                                 "bob") > 0, 1);
    GK_CHECK_EQ_INT(gk_trace_add(&l, "SN-003", "Shaft", "B2027", 1002.0,
                                 "alice") > 0, 1);
    r = gk_trace_find(&l, "SN-002");
    GK_CHECK(r != NULL);
    GK_CHECK_STR_EQ(r->part, "Flange");
    GK_CHECK_STR_EQ(r->operator_name, "bob");
    GK_CHECK(near(r->timestamp, 1001.0, 1e-9));
    GK_CHECK(gk_trace_find(&l, "SN-999") == NULL);
    GK_CHECK_EQ_INT(gk_trace_count_batch(&l, "B2026"), 2);
    GK_CHECK_EQ_INT(gk_trace_count_batch(&l, "B2027"), 1);
    GK_CHECK_EQ_INT(gk_trace_count_batch(&l, "none"), 0);
    GK_CHECK_EQ_INT(gk_trace_add(NULL, "x", "y", "z", 0, "o"), -1);
}

static void test_qr(void)
{
    gk_trace_record in, out;
    char buf[256];
    int n;

    memset(&in, 0, sizeof(in));
    strcpy(in.serial, "SN-0100");
    strcpy(in.part, "Bracket");
    strcpy(in.batch, "B77");
    in.timestamp = 2000.0;

    n = gk_qr_encode(&in, buf, sizeof(buf));
    GK_CHECK(n > 0);
    GK_CHECK(strncmp(buf, "GK|Bracket|B77|SN-0100|", 23) == 0);
    GK_CHECK_EQ_INT(gk_qr_decode(buf, &out), GK_OK);
    GK_CHECK_STR_EQ(out.serial, "SN-0100");
    GK_CHECK_STR_EQ(out.part, "Bracket");
    GK_CHECK_STR_EQ(out.batch, "B77");
    GK_CHECK(near(out.timestamp, 2000.0, 1e-9));

    buf[strlen(buf) - 1] = '9';
    GK_CHECK_EQ_INT(gk_qr_decode(buf, &out), GK_ERR_PARSE);
    GK_CHECK_EQ_INT(gk_qr_decode("garbage", &out), GK_ERR_PARSE);
    GK_CHECK_EQ_INT(gk_qr_decode(NULL, &out), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_qr_encode(NULL, buf, sizeof(buf)), 0);
    GK_CHECK_EQ_INT(gk_qr_checksum(NULL), 0);
}

static void test_lifecycle(void)
{
    gk_lifecycle l;

    gk_lifecycle_init(&l, "TOOL-A", 3);
    GK_CHECK_EQ_INT(l.remaining_uses, 3);
    GK_CHECK_EQ_INT(gk_lifecycle_should_retire(&l), 0);
    GK_CHECK_EQ_INT(gk_lifecycle_consume(&l, 1.5), GK_OK);
    GK_CHECK_EQ_INT(gk_lifecycle_consume(&l, 1.5), GK_OK);
    GK_CHECK_EQ_INT(gk_lifecycle_consume(&l, 1.5), GK_OK);
    GK_CHECK_EQ_INT(l.remaining_uses, 0);
    GK_CHECK(near(l.accumulated_hours, 4.5, 1e-9));
    GK_CHECK_EQ_INT(gk_lifecycle_should_retire(&l), 1);
    GK_CHECK_EQ_INT(gk_lifecycle_consume(&l, 1.0), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_lifecycle_consume(&l, -1.0), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_lifecycle_should_retire(NULL), 1);
}

static void test_process(void)
{
    gk_proc_pipeline p;
    const gk_process_step *s;

    gk_proc_pipeline_init(&p);
    GK_CHECK_EQ_INT(gk_proc_pipeline_is_complete(&p), 0);
    GK_CHECK_EQ_INT(gk_proc_pipeline_add(&p, GK_STAGE_INCOMING_INSPECTION), GK_OK);
    GK_CHECK_EQ_INT(gk_proc_pipeline_add(&p, GK_STAGE_SETUP_ALIGN), GK_OK);
    GK_CHECK_EQ_INT(gk_proc_pipeline_add(&p, GK_STAGE_BATCH_MACHINING), GK_OK);
    GK_CHECK_EQ_INT(p.count, 3);
    GK_CHECK_EQ_INT(gk_proc_pipeline_add(&p, (gk_stage_kind)99),
                    GK_ERR_INVALID_ARG);

    s = gk_proc_pipeline_current(&p);
    GK_CHECK(s != NULL && s->stage == GK_STAGE_INCOMING_INSPECTION);
    GK_CHECK_STR_EQ(gk_stage_status_name(s->status), "pending");
    GK_CHECK_EQ_INT(gk_proc_pipeline_advance(&p), GK_OK);
    GK_CHECK_EQ_INT(gk_proc_pipeline_done_count(&p), 1);
    s = gk_proc_pipeline_current(&p);
    GK_CHECK(s != NULL && s->stage == GK_STAGE_SETUP_ALIGN);
    GK_CHECK_EQ_INT(gk_proc_pipeline_fail(&p, "clamp slip"), GK_OK);
    GK_CHECK_STR_EQ(p.steps[1].status == GK_STAGE_FAILED ?
                    gk_stage_status_name(p.steps[1].status) : "x", "failed");
    GK_CHECK_STR_EQ(p.steps[1].note, "clamp slip");
    GK_CHECK_EQ_INT(gk_proc_pipeline_is_complete(&p), 0);
    GK_CHECK_EQ_INT(gk_proc_pipeline_complete(&p), GK_OK);
    GK_CHECK_EQ_INT(gk_proc_pipeline_is_complete(&p), 1);
    GK_CHECK(gk_proc_pipeline_current(&p) == NULL);
    GK_CHECK_EQ_INT(gk_proc_pipeline_advance(&p), GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_proc_pipeline_advance(NULL), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_proc_pipeline_fail(NULL, "x"), GK_ERR_STATE);
}

int main(void)
{
    test_stage_names();
    test_inspection();
    test_blank();
    test_setup();
    test_tool_set();
    test_first_article();
    test_batch();
    test_tool_life();
    test_finish();
    test_package();
    test_disposition();
    test_warehouse();
    test_trace();
    test_qr();
    test_lifecycle();
    test_process();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
