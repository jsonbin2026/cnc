#include "gk_test.h"
#include "gk/gk_real.h"

#include <math.h>

static int near(double a, double b, double tol)
{
    return fabs(a - b) <= tol;
}

/* deterministic model used by the Monte-Carlo test: a die roll 1..6 */
static double dice_model(void *ctx, gk_rng *r)
{
    (void)ctx;
    return gk_rng_uniform(r, 1.0, 6.0);
}

static void test_rng(void)
{
    gk_rng a;
    gk_rng b;
    int i;
    int same = 1;

    gk_rng_seed(&a, 12345ULL);
    gk_rng_seed(&b, 12345ULL);
    for (i = 0; i < 100; i++) {
        if (gk_rng_next(&a) != gk_rng_next(&b)) {
            same = 0;
        }
    }
    GK_CHECK(same); /* deterministic for identical seeds */

    gk_rng_seed(&a, 0ULL);
    GK_CHECK(gk_rng_next(&a) != 0ULL);

    for (i = 0; i < 1000; i++) {
        double u = gk_rng_uniform(&a, -2.0, 5.0);
        GK_CHECK(u >= -2.0 && u < 5.0);
    }

    GK_CHECK_EQ_INT(gk_rng_chance(&a, 0.0), 0);
    GK_CHECK_EQ_INT(gk_rng_chance(&a, 1.0), 1);

    /* normal distribution is roughly centred on the mean */
    {
        double sum = 0.0;
        for (i = 0; i < 5000; i++) {
            sum += gk_rng_normal(&a, 10.0, 2.0);
        }
        GK_CHECK(near(sum / 5000.0, 10.0, 0.2));
    }

    /* NULL-safety */
    gk_rng_seed(NULL, 1ULL);
    GK_CHECK_EQ_INT(gk_rng_next(NULL), 0);
    GK_CHECK(near(gk_rng_uniform(NULL, 3.0, 9.0), 3.0, 1e-12));
    GK_CHECK_EQ_INT(gk_rng_chance(NULL, 1.0), 0);
}

static void test_variation(void)
{
    gk_variation v;
    gk_rng r;
    gk_blank_variation b;
    gk_tool_batch t;
    int i;

    gk_rng_seed(&r, 7ULL);
    gk_variation_init(&v, 50.0, 1.0, 48.0, 52.0);
    for (i = 0; i < 2000; i++) {
        double s = gk_variation_sample(&v, &r);
        GK_CHECK(s >= 48.0 && s <= 52.0);
    }

    gk_blank_variation_init(&b);
    GK_CHECK(gk_variation_sample(&b.blank_length, &r) >= 99.0);
    GK_CHECK(gk_variation_sample(&b.hardness, &r) >= 170.0);

    gk_tool_batch_init(&t, 7, &r);
    GK_CHECK_EQ_INT(t.batch_id, 7);
    GK_CHECK(t.runout >= 0.0);
    GK_CHECK(t.wear_factor >= 0.5 && t.wear_factor <= 2.0);

    /* NULL-safety */
    gk_variation_init(NULL, 1.0, 1.0, 0.0, 2.0);
    GK_CHECK(near(gk_variation_sample(NULL, &r), 0.0, 1e-12));
    gk_blank_variation_init(NULL);
    gk_tool_batch_init(NULL, 0, &r);
}

static void test_environment(void)
{
    gk_environment e;
    int k;

    gk_environment_init(&e);
    GK_CHECK_EQ_INT(gk_environment_kind_count(), 5);

    /* a sine of period 24h at phase 0 is nominal at t=0,0.5*period,period */
    GK_CHECK(near(gk_environment_sample(&e, 0.0, 0), 380.0, 1e-9));
    GK_CHECK(near(gk_environment_sample(&e, 12.0, 0), 380.0, 1e-6));

    for (k = 0; k < gk_environment_kind_count(); k++) {
        double lo = gk_environment_sample(&e, 0.0, k);
        double hi = gk_environment_sample(&e, 6.0, k);
        GK_CHECK(isfinite(lo));
        GK_CHECK(isfinite(hi));
    }
    /* temperature reaches its peak at 1/4 period (6h), phase 0 */
    GK_CHECK(gk_environment_sample(&e, 6.0, 3) > 22.0);

    GK_CHECK(near(gk_environment_sample(NULL, 0.0, 0), 0.0, 1e-12));
    GK_CHECK(near(gk_environment_sample(&e, 0.0, 99), 0.0, 1e-12));
    gk_environment_init(NULL);
}

static void test_operator_clamp(void)
{
    gk_operator_model o;
    gk_rng r;
    gk_clamp_force c;
    int i;

    gk_rng_seed(&r, 99ULL);
    gk_operator_model_init(&o, 0.9);
    GK_CHECK(near(o.skill, 0.9, 1e-12));
    GK_CHECK(near(o.fatigue, 0.0, 1e-12));

    for (i = 0; i < 500; i++) {
        GK_CHECK(gk_operator_setting_error(&o, &r) >= 0.0);
        GK_CHECK(gk_operator_reaction_time(&o, &r) >= 0.0);
    }
    /* clamping skill */
    gk_operator_model_init(&o, 5.0);
    GK_CHECK(near(o.skill, 1.0, 1e-12));
    gk_operator_model_init(&o, -1.0);
    GK_CHECK(near(o.skill, 0.0, 1e-12));

    gk_clamp_force_init(&c, 1000.0, 50.0, 1500.0);
    for (i = 0; i < 500; i++) {
        double f = gk_clamp_force_sample(&c, &r);
        GK_CHECK(f >= 0.0);
    }
    GK_CHECK_EQ_INT(gk_clamp_force_ok(&c, 1000.0), 1);
    GK_CHECK_EQ_INT(gk_clamp_force_ok(&c, 2000.0), 0);
    GK_CHECK_EQ_INT(gk_clamp_force_ok(&c, 0.0), 0);
    GK_CHECK_EQ_INT(gk_clamp_force_ok(NULL, 1.0), 0);

    GK_CHECK(near(gk_operator_setting_error(NULL, &r), 0.0, 1e-12));
    GK_CHECK(near(gk_operator_reaction_time(NULL, &r), 0.0, 1e-12));
    gk_operator_model_init(NULL, 0.5);
    gk_clamp_force_init(NULL, 1.0, 1.0, 1.0);
    GK_CHECK(near(gk_clamp_force_sample(NULL, &r), 0.0, 1e-12));
}

static void test_faults(void)
{
    gk_fault_model m;
    gk_rng r;
    int i;
    int seen = 0;

    gk_rng_seed(&r, 4242ULL);
    gk_fault_model_init(&m, 0.5);
    GK_CHECK(m.count >= 4);
    GK_CHECK_EQ_INT(gk_fault_model_set(&m, GK_REAL_FAULT_TOOL_BREAK, 0.1),
                    GK_OK); /* update existing */
    GK_CHECK_EQ_INT(m.count, 4);
    GK_CHECK_EQ_INT(gk_fault_model_set(&m, GK_REAL_FAULT_POWER_LOSS, 0.2),
                    GK_OK); /* add new */
    GK_CHECK_EQ_INT(m.count, 5);
    GK_CHECK_EQ_INT(gk_fault_model_set(&m, GK_REAL_FAULT_NONE, 0.1),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_fault_model_set(NULL, GK_REAL_FAULT_TOOL_BREAK, 0.1),
                    GK_ERR_INVALID_ARG);

    for (i = 0; i < 2000; i++) {
        gk_real_fault f = gk_fault_model_sample(&m, &r, 1.0);
        if (f != GK_REAL_FAULT_NONE) {
            seen++;
        }
    }
    GK_CHECK(seen > 0);
    GK_CHECK_EQ_INT(gk_fault_model_sample(&m, &r, 0.0), GK_REAL_FAULT_NONE);
    GK_CHECK_EQ_INT(gk_fault_model_sample(NULL, &r, 1.0), GK_REAL_FAULT_NONE);

    GK_CHECK_STR_EQ(gk_real_fault_name(GK_REAL_FAULT_TOOL_BREAK),
                    "tool breakage");
    GK_CHECK_STR_EQ(gk_real_fault_name(GK_REAL_FAULT_NONE), "none");
    GK_CHECK_STR_EQ(gk_real_fault_name(GK_REAL_FAULT_COUNT), "unknown");
    (void)gk_real_fault_name(GK_REAL_FAULT_POWER_LOSS);
    (void)gk_real_fault_name(GK_REAL_FAULT_SPINDLE_STALL);
    (void)gk_real_fault_name(GK_REAL_FAULT_COOLANT_LOSS);
    (void)gk_real_fault_name(GK_REAL_FAULT_CHIP_JAM);
}

static void test_events(void)
{
    gk_event_model m;
    gk_rng r;
    int ids[8];
    int n;

    gk_rng_seed(&r, 31337ULL);
    gk_event_model_init(&m);
    GK_CHECK_EQ_INT(gk_event_model_add(&m, "chip jam", 1.0, 0.2), 1);
    GK_CHECK_EQ_INT(gk_event_model_add(&m, "power dip", 1.0, 0.5), 2);
    GK_CHECK_EQ_INT(gk_event_model_add(NULL, "x", 1.0, 0.0), -1);
    GK_CHECK_EQ_INT(gk_event_model_add(&m, NULL, 1.0, 0.0), -1);
    GK_CHECK_EQ_INT(m.count, 2);

    n = gk_event_model_sample(&m, &r, 100.0, ids, 8);
    GK_CHECK(n >= 0);
    GK_CHECK_EQ_INT(gk_event_model_sample(NULL, &r, 1.0, ids, 8), 0);

    /* exhausted model */
    while (m.count < GK_REAL_MAX_EVENTS) {
        char name[16];
        int id = gk_event_model_add(&m, "evt", 0.001, 0.0);
        if (id < 0) {
            break;
        }
        (void)name;
    }
    GK_CHECK_EQ_INT(gk_event_model_add(&m, "overflow", 0.1, 0.0), -1);
}

static void test_noise_stats(void)
{
    gk_measure_noise mn;
    gk_rng r;
    gk_stats s;
    double vals[2000];
    int i;

    gk_rng_seed(&r, 2718ULL);
    gk_measure_noise_init(&mn, 0.01, 0.02, 0.001);
    for (i = 0; i < 1000; i++) {
        double v = gk_measure_noise_apply(&mn, 10.0, &r);
        /* quantised to 0.001 */
        double q = v / 0.001;
        GK_CHECK(near(q, floor(q + 0.5), 1e-6));
    }
    GK_CHECK(near(gk_measure_noise_apply(NULL, 3.5, &r), 3.5, 1e-12));
    gk_measure_noise_init(NULL, 1.0, 0.0, 0.0);

    /* stats */
    gk_stats_init(&s);
    for (i = 0; i < 100; i++) {
        GK_CHECK_EQ_INT(gk_stats_add(&s, (double)i), GK_OK);
    }
    GK_CHECK_EQ_INT(s.samples, 100);
    GK_CHECK(near(s.minimum, 0.0, 1e-12));
    GK_CHECK(near(s.maximum, 99.0, 1e-12));
    GK_CHECK_EQ_INT(gk_stats_finalize(&s), GK_OK);
    GK_CHECK(near(s.mean, 49.5, 1e-9));
    GK_CHECK_EQ_INT(gk_stats_add(NULL, 1.0), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_stats_finalize(NULL), GK_ERR_INVALID_ARG);

    {
        gk_stats empty;
        gk_stats_init(&empty);
        GK_CHECK_EQ_INT(gk_stats_finalize(&empty), GK_ERR_STATE);
    }

    /* Monte-Carlo with a die model: mean ~ 3.5 */
    gk_stats mc;
    int n = gk_monte_carlo(dice_model, NULL, &r, 4000, vals, 2000, &mc);
    GK_CHECK(n > 0);
    GK_CHECK_EQ_INT(mc.samples, 4000);
    GK_CHECK(near(mc.mean, 3.5, 0.2));
    GK_CHECK(mc.p05 >= mc.minimum - 1e-9);
    GK_CHECK(mc.p05 <= mc.p50);
    GK_CHECK(mc.p50 <= mc.p95);
    GK_CHECK(mc.p95 <= mc.maximum + 1e-9);
    GK_CHECK(mc.minimum >= 1.0 && mc.maximum <= 6.0);

    /* no value buffer: statistics still computed */
    {
        gk_stats mc2;
        int n2 = gk_monte_carlo(dice_model, NULL, &r, 100, NULL, 0, &mc2);
        GK_CHECK_EQ_INT(n2, 100);
        GK_CHECK_EQ_INT(mc2.samples, 100);
        GK_CHECK(mc2.p05 == mc2.minimum);
        GK_CHECK(mc2.p95 == mc2.maximum);
    }

    GK_CHECK_EQ_INT(gk_monte_carlo(NULL, NULL, &r, 10, vals, 10, &mc), 0);
    GK_CHECK_EQ_INT(gk_monte_carlo(dice_model, NULL, &r, 0, vals, 10, &mc), 0);
    /* sample count is capped at GK_REAL_MAX_SAMPLES */
    GK_CHECK_EQ_INT(gk_monte_carlo(dice_model, NULL, &r, 100000, NULL, 0, &mc),
                    GK_REAL_MAX_SAMPLES);
}

static void test_disruptions(void)
{
    gk_disruption_log l;
    int i;

    gk_disruption_log_init(&l);
    GK_CHECK_EQ_INT(l.count, 0);

    GK_CHECK_EQ_INT(gk_disruption_add(&l, GK_DR_FIRE, 1.0), GK_OK);
    GK_CHECK_EQ_INT(gk_disruption_add(&l, GK_DR_TOOL_CHANGE_BROKEN, 0.5),
                    GK_OK);
    GK_CHECK_EQ_INT(l.count, 2);
    GK_CHECK(near(l.events[0].cost_impact, 50000.0, 1e-6));
    GK_CHECK(near(l.events[0].downtime_hours, 24.0, 1e-6));
    GK_CHECK(near(l.events[1].cost_impact, 75.0, 1e-6));

    /* severity clamping */
    GK_CHECK_EQ_INT(gk_disruption_add(&l, GK_DR_FIRE, 5.0), GK_OK);
    GK_CHECK(near(l.events[2].severity, 1.0, 1e-12));
    GK_CHECK_EQ_INT(gk_disruption_add(&l, GK_DR_FIRE, -3.0), GK_OK);
    GK_CHECK(near(l.events[3].severity, 0.0, 1e-12));

    GK_CHECK(gk_disruption_total_cost(&l) > 0.0);
    GK_CHECK(gk_disruption_total_downtime(&l) > 0.0);

    GK_CHECK_EQ_INT(gk_disruption_handle(&l, 0), GK_OK);
    GK_CHECK_EQ_INT(l.events[0].handled, 1);
    GK_CHECK_EQ_INT(gk_disruption_handle(&l, 0), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_disruption_handle(&l, -1), GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_disruption_handle(&l, 999), GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_disruption_add(NULL, GK_DR_FIRE, 1.0),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_disruption_add(&l, GK_DR_COUNT, 1.0),
                    GK_ERR_INVALID_ARG);

    GK_CHECK_STR_EQ(gk_disruption_name(GK_DR_POWER_RESUME),
                    "power-loss resume");
    GK_CHECK_STR_EQ(gk_disruption_name(GK_DR_FIRE), "fire");
    GK_CHECK_STR_EQ(gk_disruption_name(GK_DR_COUNT), "unknown");
    for (i = 0; i < GK_DR_COUNT; i++) {
        const char *n = gk_disruption_name((gk_disruption_kind)i);
        GK_CHECK(n != NULL);
    }

    gk_disruption_log_init(NULL);
    GK_CHECK(near(gk_disruption_total_cost(NULL), 0.0, 1e-12));
    GK_CHECK(near(gk_disruption_total_downtime(NULL), 0.0, 1e-12));
}

static void test_power_resume(void)
{
    gk_disruption_log l;
    double last[3] = {10.0, 20.0, 30.0};
    double safe[3] = {0.0, 0.0, 0.0};

    gk_disruption_log_init(&l);
    GK_CHECK_EQ_INT(gk_power_resume(&l, last, safe, 3), 3);
    GK_CHECK_EQ_INT(l.count, 1);
    GK_CHECK_EQ_INT(l.events[0].kind, GK_DR_POWER_RESUME);

    GK_CHECK_EQ_INT(gk_power_resume(&l, last, last, 3), 0);
    GK_CHECK_EQ_INT(gk_power_resume(NULL, last, safe, 2), 2);
    GK_CHECK_EQ_INT(gk_power_resume(&l, NULL, safe, 3), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_power_resume(&l, last, NULL, 3), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_power_resume(&l, last, safe, 0), GK_ERR_INVALID_ARG);
}

static void test_realism(void)
{
    gk_realism r;
    int i;
    int total = 0;

    gk_realism_init(&r, 2024ULL);
    GK_CHECK_EQ_INT(r.disruptions.count, 0);
    GK_CHECK(near(r.operator_model.skill, 0.8, 1e-12));

    for (i = 0; i < 5000; i++) {
        total += gk_realism_step(&r, (double)i * 0.01, 0.01);
    }
    GK_CHECK(r.disruptions.count > 0);
    GK_CHECK(total > 0);

    GK_CHECK_EQ_INT(gk_realism_step(&r, 1.0, 0.0), 0);
    GK_CHECK_EQ_INT(gk_realism_step(NULL, 1.0, 0.1), 0);

    gk_realism_init(NULL, 1ULL);
}

int main(void)
{
    test_rng();
    test_variation();
    test_environment();
    test_operator_clamp();
    test_faults();
    test_events();
    test_noise_stats();
    test_disruptions();
    test_power_resume();
    test_realism();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
