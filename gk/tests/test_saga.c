#include "gk_test.h"
#include "gk/gk_saga.h"

#include <math.h>
#include <string.h>

static int near(double a, double b, double tol)
{
    return fabs(a - b) <= tol;
}

static void test_reverse_program(void)
{
    gk_deduce_part part;
    gk_deduce_program prog;

    part.width = 100.0;
    part.height = 80.0;
    part.depth = 10.0;
    part.holes = 2;
    part.slot_length = 20.0;
    GK_CHECK_EQ_INT(gk_deduce_program_from_part(&part, &prog), GK_OK);
    GK_CHECK_EQ_INT(prog.operations, 5); /* face+contour + 2 holes + slot */
    GK_CHECK(prog.estimated_minutes > 0.0);
    GK_CHECK(strstr(prog.program, "M30") != NULL);

    part.slot_length = 0.0;
    part.holes = 0;
    GK_CHECK_EQ_INT(gk_deduce_program_from_part(&part, &prog), GK_OK);
    GK_CHECK_EQ_INT(prog.operations, 2);

    part.width = 0.0;
    GK_CHECK_EQ_INT(gk_deduce_program_from_part(&part, &prog),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_deduce_program_from_part(NULL, &prog),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_deduce_program_from_part(&part, NULL),
                    GK_ERR_INVALID_ARG);
}

static void test_reverse_sound(void)
{
    gk_deduce_acoustic a;

    GK_CHECK_EQ_INT(gk_deduce_state_from_sound(50.0, 10.0, &a), GK_OK);
    GK_CHECK_EQ_INT(a.state, GK_DEDUCE_SOUND_IDLE);
    GK_CHECK_EQ_INT(a.suggested_rpm, 0);
    GK_CHECK_EQ_INT(gk_deduce_state_from_sound(300.0, 60.0, &a), GK_OK);
    GK_CHECK_EQ_INT(a.state, GK_DEDUCE_SOUND_CUTTING);
    GK_CHECK_EQ_INT(gk_deduce_state_from_sound(5000.0, 70.0, &a), GK_OK);
    GK_CHECK_EQ_INT(a.state, GK_DEDUCE_SOUND_CHATTER);
    GK_CHECK_EQ_INT(gk_deduce_state_from_sound(1000.0, 90.0, &a), GK_OK);
    GK_CHECK_EQ_INT(a.state, GK_DEDUCE_SOUND_TOOL_WEAR);
    GK_CHECK_EQ_INT(gk_deduce_state_from_sound(1000.0, 50.0, &a), GK_OK);
    GK_CHECK_EQ_INT(a.state, GK_DEDUCE_SOUND_RAPID);
    GK_CHECK_EQ_INT(gk_deduce_state_from_sound(-1.0, 50.0, &a),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_deduce_state_from_sound(100.0, 50.0, NULL),
                    GK_ERR_INVALID_ARG);

    GK_CHECK_STR_EQ(gk_deduce_sound_name(GK_DEDUCE_SOUND_IDLE), "idle");
    GK_CHECK_STR_EQ(gk_deduce_sound_name(GK_DEDUCE_SOUND_TOOL_WEAR),
                    "tool-wear");
    GK_CHECK_STR_EQ(gk_deduce_sound_name(GK_DEDUCE_SOUND_COUNT), "unknown");
}

static void test_reverse_curve_tool(void)
{
    double x[5] = {0.0, 1.0, 2.0, 3.0, 4.0};
    double y[5] = {1.0, 3.0, 5.0, 7.0, 9.0};
    gk_deduce_line line;
    gk_deduce_tool tool;

    GK_CHECK_EQ_INT(gk_deduce_params_from_curve(x, y, 5, &line), GK_OK);
    GK_CHECK(near(line.slope, 2.0, 1e-9));
    GK_CHECK(near(line.intercept, 1.0, 1e-9));
    GK_CHECK(near(line.r_squared, 1.0, 1e-9));

    /* noisy-ish but linear data still recovers slope */
    {
        double y2[5] = {1.0, 2.9, 5.1, 7.0, 9.1};
        GK_CHECK_EQ_INT(gk_deduce_params_from_curve(x, y2, 5, &line), GK_OK);
        GK_CHECK(near(line.slope, 2.0, 0.05));
        GK_CHECK(line.r_squared > 0.99);
    }
    /* vertical line is degenerate */
    {
        double x3[2] = {1.0, 1.0};
        double y3[2] = {0.0, 5.0};
        GK_CHECK_EQ_INT(gk_deduce_params_from_curve(x3, y3, 2, &line),
                        GK_ERR_OUT_OF_RANGE);
    }
    GK_CHECK_EQ_INT(gk_deduce_params_from_curve(x, y, 1, &line),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_deduce_params_from_curve(NULL, y, 5, &line),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_deduce_params_from_curve(x, y, 5, NULL),
                    GK_ERR_INVALID_ARG);

    /* scallop height h = f^2 / (8R) -> f = sqrt(8 R h) */
    GK_CHECK_EQ_INT(gk_deduce_tool_from_texture(0.01, 3.0, &tool), GK_OK);
    GK_CHECK(near(tool.feed_per_tooth, sqrt(8.0 * 3.0 * 0.01), 1e-9));
    GK_CHECK(near(tool.diameter, 6.0, 1e-9));
    GK_CHECK_EQ_INT(gk_deduce_tool_from_texture(0.0, 3.0, &tool),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_deduce_tool_from_texture(0.01, 0.0, &tool),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_deduce_tool_from_texture(0.01, 3.0, NULL),
                    GK_ERR_INVALID_ARG);
}

static void test_reverse_action_cause(void)
{
    gk_deduce_action act;
    gk_deduce_cause cause;

    GK_CHECK_EQ_INT(gk_deduce_action_from_alarm("E041", &act), GK_OK);
    GK_CHECK_EQ_INT(act.severity, 3);
    GK_CHECK_STR_EQ(act.code, "E041");
    GK_CHECK(strstr(act.action, "tool wear") != NULL);
    GK_CHECK_EQ_INT(gk_deduce_action_from_alarm("E001", &act), GK_OK);
    GK_CHECK_EQ_INT(act.severity, 2);
    GK_CHECK_EQ_INT(gk_deduce_action_from_alarm("E100", &act), GK_OK);
    GK_CHECK_EQ_INT(act.severity, 1);
    GK_CHECK_EQ_INT(gk_deduce_action_from_alarm("XYZ", &act), GK_OK);
    GK_CHECK(strstr(act.action, "manual") != NULL);
    GK_CHECK_EQ_INT(gk_deduce_action_from_alarm(NULL, &act),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_deduce_action_from_alarm("E041", NULL),
                    GK_ERR_INVALID_ARG);

    GK_CHECK_EQ_INT(gk_deduce_cause_from_scrap(0.0, 0.0, &cause), GK_OK);
    GK_CHECK_EQ_INT(cause, GK_DEDUCE_CAUSE_NONE);
    GK_CHECK_EQ_INT(gk_deduce_cause_from_scrap(0.1, 0.0, &cause), GK_OK);
    GK_CHECK_EQ_INT(cause, GK_DEDUCE_CAUSE_WRONG_TOOL);
    GK_CHECK_EQ_INT(gk_deduce_cause_from_scrap(0.1, 0.1, &cause), GK_OK);
    GK_CHECK_EQ_INT(cause, GK_DEDUCE_CAUSE_WORN_TOOL);
    GK_CHECK_EQ_INT(gk_deduce_cause_from_scrap(0.0, 0.2, &cause), GK_OK);
    GK_CHECK_EQ_INT(cause, GK_DEDUCE_CAUSE_WRONG_FEED);
    GK_CHECK_EQ_INT(gk_deduce_cause_from_scrap(0.03, 0.05, &cause), GK_OK);
    GK_CHECK_EQ_INT(cause, GK_DEDUCE_CAUSE_LOOSE_CLAMP);
    GK_CHECK_EQ_INT(gk_deduce_cause_from_scrap(0.1, 0.1, NULL),
                    GK_ERR_INVALID_ARG);

    GK_CHECK_STR_EQ(gk_deduce_cause_name(GK_DEDUCE_CAUSE_NONE), "none");
    GK_CHECK_STR_EQ(gk_deduce_cause_name(GK_DEDUCE_CAUSE_LOOSE_CLAMP),
                    "loose-clamp");
    GK_CHECK_STR_EQ(gk_deduce_cause_name(GK_DEDUCE_CAUSE_COUNT), "unknown");
}

static void test_reverse_cost_feature(void)
{
    gk_deduce_craft_input in;
    gk_deduce_craft craft;
    double tx[4] = {0.0, 10.0, 20.0, 0.0};
    double ty[4] = {0.0, 0.0, 10.0, 0.0};
    gk_deduce_feature feat;

    in.machine_rate = 60.0;
    in.labor_rate = 40.0;
    in.material_cost = 10.0;
    in.setup_minutes = 60.0;
    in.quantity = 10;
    GK_CHECK_EQ_INT(gk_deduce_craft_from_cost(&in, 100.0, &craft), GK_OK);
    /* cycle = 2 + 60/10 = 8 min; cost = 10 + 8/60*100 = 23.33 */
    GK_CHECK(near(craft.per_unit_minutes, 8.0, 1e-9));
    GK_CHECK(near(craft.unit_cost, 10.0 + 8.0 / 60.0 * 100.0, 1e-9));
    GK_CHECK(strstr(craft.recommendation, "within budget") != NULL);

    GK_CHECK_EQ_INT(gk_deduce_craft_from_cost(&in, 5.0, &craft), GK_OK);
    GK_CHECK(strstr(craft.recommendation, "Reduce cycle") != NULL);
    in.quantity = 0;
    GK_CHECK_EQ_INT(gk_deduce_craft_from_cost(&in, 5.0, &craft),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_deduce_craft_from_cost(NULL, 5.0, &craft),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_deduce_craft_from_cost(&in, 5.0, NULL),
                    GK_ERR_INVALID_ARG);

    GK_CHECK_EQ_INT(gk_deduce_feature_from_toolpath(tx, ty, 4, &feat), GK_OK);
    GK_CHECK_EQ_INT(feat.contour_segments, 4);
    GK_CHECK(near(feat.bounding_volume, 200.0, 1e-9));
    GK_CHECK_EQ_INT(gk_deduce_feature_from_toolpath(tx, ty, 0, &feat),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_deduce_feature_from_toolpath(NULL, ty, 4, &feat),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_deduce_feature_from_toolpath(tx, ty, 4, NULL),
                    GK_ERR_INVALID_ARG);
}

static void test_story_actors(void)
{
    gk_saga_actor mentor, apprentice;

    gk_saga_actor_init(&mentor, "Master Li");
    gk_saga_actor_init(&apprentice, "Xiao Wang");
    mentor.skill = 0.9;
    mentor.level = 10;

    GK_CHECK_EQ_INT(gk_saga_mentor_teach(&mentor, &apprentice, 1.0), GK_OK);
    GK_CHECK(apprentice.skill > 0.1);
    GK_CHECK(apprentice.skill <= 0.9);
    GK_CHECK(apprentice.reputation > 0.0);
    /* apprentice cannot exceed the mentor */
    {
        int i;
        for (i = 0; i < 100; i++) {
            (void)gk_saga_mentor_teach(&mentor, &apprentice, 1.0);
        }
        GK_CHECK(apprentice.skill <= mentor.skill + 1e-9);
    }
    GK_CHECK_EQ_INT(gk_saga_mentor_teach(&mentor, &apprentice, 0.0),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_saga_mentor_teach(NULL, &apprentice, 1.0),
                    GK_ERR_INVALID_ARG);
    gk_saga_actor_init(NULL, "x");
}

static void test_factory(void)
{
    gk_saga_factory f;
    gk_saga_order o;
    int id;

    gk_saga_factory_init(&f, 1000.0);
    GK_CHECK(near(f.cash, 1000.0, 1e-9));
    o.quantity = 100;
    o.unit_price = 5.0;
    o.deadline_days = 10.0;
    o.difficulty = 2;
    id = gk_saga_factory_intake(&f, &o);
    GK_CHECK_EQ_INT(id, 1);
    GK_CHECK_EQ_INT(f.count, 1);
    GK_CHECK_EQ_INT(gk_saga_factory_fulfil(&f, 1), GK_OK);
    GK_CHECK(near(f.cash, 1000.0 + 500.0, 1e-9));
    /* cannot fulfil twice */
    GK_CHECK_EQ_INT(gk_saga_factory_fulfil(&f, 1), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_saga_factory_fulfil(&f, 99), GK_ERR_OUT_OF_RANGE);

    /* late order pays a penalty */
    o.deadline_days = 0.0;
    id = gk_saga_factory_intake(&f, &o);
    GK_CHECK_EQ_INT(id, 2);
    GK_CHECK_EQ_INT(gk_saga_factory_fulfil(&f, 2), GK_OK);
    GK_CHECK(near(f.cash, 1500.0 + 250.0, 1e-9));

    o.quantity = 0;
    GK_CHECK_EQ_INT(gk_saga_factory_intake(&f, &o), -1);
    GK_CHECK_EQ_INT(gk_saga_factory_intake(NULL, &o), -1);
    GK_CHECK_EQ_INT(gk_saga_factory_intake(&f, NULL), -1);
    gk_saga_factory_init(NULL, 0.0);
}

static void test_incidents_challenges(void)
{
    gk_saga_incident inc;
    gk_saga_challenge ch;

    gk_saga_incident_init(&inc, "Tool crashed");
    GK_CHECK_STR_EQ(inc.title, "Tool crashed");
    GK_CHECK_EQ_INT(gk_saga_incident_review(&inc, "wrong offset",
                                            "verify offsets before run"),
                    GK_OK);
    GK_CHECK_EQ_INT(inc.preventable, 1);
    GK_CHECK_STR_EQ(inc.root_cause, "wrong offset");
    GK_CHECK_EQ_INT(gk_saga_incident_review(&inc, NULL, "x"),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_saga_incident_review(NULL, "a", "b"),
                    GK_ERR_INVALID_ARG);
    gk_saga_incident_init(NULL, "x");

    gk_saga_challenge_init(&ch, "High-speed machining");
    GK_CHECK_EQ_INT(gk_saga_challenge_attempt(&ch, 0.0),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_saga_challenge_attempt(&ch, 0.5), GK_OK);
    GK_CHECK_EQ_INT(ch.attempts, 1);
    GK_CHECK(ch.progress > 0.0 && ch.progress < 1.0);
    /* keep trying until solved */
    {
        int i;
        for (i = 0; i < 100 && !ch.solved; i++) {
            (void)gk_saga_challenge_attempt(&ch, 0.5);
        }
        GK_CHECK_EQ_INT(ch.solved, 1);
        GK_CHECK(near(ch.progress, 1.0, 1e-9));
    }
    GK_CHECK_EQ_INT(gk_saga_challenge_attempt(&ch, 0.5), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_saga_challenge_attempt(NULL, 0.5),
                    GK_ERR_INVALID_ARG);
    gk_saga_challenge_init(NULL, "x");
}

static void test_startup(void)
{
    gk_saga_startup s;

    gk_saga_startup_init(&s, 10000.0);
    GK_CHECK_EQ_INT(s.months_active, 0);
    GK_CHECK_EQ_INT(gk_saga_startup_simulate_month(&s, -1.0, 1.0),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_saga_startup_simulate_month(&s, 5000.0, 3000.0), GK_OK);
    GK_CHECK_EQ_INT(s.months_active, 1);
    GK_CHECK_EQ_INT(s.employees, 1);
    GK_CHECK(near(s.capital, 12000.0, 1e-9));
    /* loss-making month does not hire */
    GK_CHECK_EQ_INT(gk_saga_startup_simulate_month(&s, 1000.0, 5000.0), GK_OK);
    GK_CHECK_EQ_INT(s.employees, 1);
    GK_CHECK(near(s.capital, 8000.0, 1e-9));
    /* bankruptcy */
    GK_CHECK_EQ_INT(gk_saga_startup_simulate_month(&s, 0.0, 100000.0), GK_OK);
    GK_CHECK_EQ_INT(s.bankrupt, 1);
    GK_CHECK_EQ_INT(gk_saga_startup_simulate_month(&s, 1.0, 1.0), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_saga_startup_simulate_month(NULL, 1.0, 1.0),
                    GK_ERR_INVALID_ARG);
    gk_saga_startup_init(NULL, 0.0);
}

static void test_branch_plot(void)
{
    gk_saga_plot p;
    int start, good, bad;

    gk_saga_plot_init(&p);
    start = gk_saga_plot_add(&p, "You get an order", 2, 3, 0);
    good = gk_saga_plot_add(&p, "You succeed", 0, 0, 1);
    bad = gk_saga_plot_add(&p, "You fail", 0, 0, 1);
    GK_CHECK_EQ_INT(start, 1);
    GK_CHECK_EQ_INT(good, 2);
    GK_CHECK_EQ_INT(bad, 3);

    GK_CHECK_EQ_INT(gk_saga_plot_start(&p, start), GK_OK);
    GK_CHECK(gk_saga_plot_current(&p) != NULL);
    GK_CHECK_STR_EQ(gk_saga_plot_current(&p)->text, "You get an order");
    GK_CHECK_EQ_INT(gk_saga_plot_choose(&p, 0), GK_OK);
    GK_CHECK_EQ_INT(p.current, 2);
    GK_CHECK_EQ_INT(p.decisions, 1);
    GK_CHECK(gk_saga_plot_current(&p)->terminal);
    /* terminal node cannot branch further */
    GK_CHECK_EQ_INT(gk_saga_plot_choose(&p, 0), GK_ERR_STATE);

    /* take branch b to the failure node */
    GK_CHECK_EQ_INT(gk_saga_plot_start(&p, start), GK_OK);
    GK_CHECK_EQ_INT(gk_saga_plot_choose(&p, 1), GK_OK);
    GK_CHECK_EQ_INT(p.current, 3);

    GK_CHECK_EQ_INT(gk_saga_plot_start(&p, 99), GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_saga_plot_start(NULL, 1), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_saga_plot_choose(NULL, 0), GK_ERR_INVALID_ARG);
    GK_CHECK(gk_saga_plot_current(NULL) == NULL);

    /* choosing before start is an error */
    {
        gk_saga_plot fresh;
        gk_saga_plot_init(&fresh);
        (void)gk_saga_plot_add(&fresh, "x", 1, 1, 1);
        GK_CHECK_EQ_INT(gk_saga_plot_choose(&fresh, 0), GK_ERR_STATE);
        GK_CHECK(gk_saga_plot_current(&fresh) == NULL);
    }
    gk_saga_plot_init(NULL);
}

static void test_npc_immersion(void)
{
    gk_saga_npc_script s;
    gk_saga_immersion im;

    gk_saga_npc_script_init(&s);
    GK_CHECK_EQ_INT(gk_saga_npc_add(&s, "Foreman", "Ready to start?", 0), 1);
    GK_CHECK_EQ_INT(gk_saga_npc_add(&s, "Foreman", "You messed up!", 1), 2);
    GK_CHECK_EQ_INT(gk_saga_npc_add(&s, "Foreman", "Great work!", 2), 3);
    GK_CHECK_EQ_INT(s.count, 3);
    GK_CHECK_EQ_INT(gk_saga_npc_add(NULL, "a", "b", 0), -1);
    GK_CHECK_EQ_INT(gk_saga_npc_add(&s, "a", NULL, 0), -1);

    GK_CHECK_STR_EQ(gk_saga_npc_greet(&s, 2), "Great work!");
    GK_CHECK_STR_EQ(gk_saga_npc_greet(&s, 1), "You messed up!");
    /* unknown mood falls back to the first line */
    GK_CHECK_STR_EQ(gk_saga_npc_greet(&s, 99), "Ready to start?");
    GK_CHECK(gk_saga_npc_greet(NULL, 0) == NULL);
    gk_saga_npc_script_init(NULL);

    gk_saga_immersion_init(&im);
    GK_CHECK_EQ_INT(im.view, GK_SAGA_VIEW_FIRST_PERSON);
    GK_CHECK(near(im.head_height, 1.7, 1e-9));
    GK_CHECK_EQ_INT(im.hands_visible, 1);
    GK_CHECK(gk_saga_immersion_score(&im) > 0.5);
    im.voice_enabled = 1;
    GK_CHECK(near(gk_saga_immersion_score(&im), 1.0, 1e-9));
    GK_CHECK_EQ_INT(gk_saga_immersion_set_view(&im, GK_SAGA_VIEW_THIRD_PERSON),
                    GK_OK);
    GK_CHECK(gk_saga_immersion_score(&im) < 1.0);
    im.fov_deg = 30.0;
    GK_CHECK(gk_saga_immersion_score(&im) <= 0.8);
    GK_CHECK_EQ_INT(gk_saga_immersion_set_view(&im, (gk_saga_view)9),
                    GK_ERR_INVALID_ARG);
    GK_CHECK(near(gk_saga_immersion_score(NULL), 0.0, 1e-9));
    gk_saga_immersion_init(NULL);
}

static void test_quest_achievement_title(void)
{
    gk_saga_quest_log ql;
    gk_saga_achievements ach;
    gk_saga_titles titles;

    gk_saga_quest_log_init(&ql);
    GK_CHECK_EQ_INT(gk_saga_quest_add(&ql, "Make 10 parts", 10.0, 100), 1);
    GK_CHECK_EQ_INT(gk_saga_quest_add(&ql, "Zero scrap", 1.0, 50), 2);
    GK_CHECK_EQ_INT(gk_saga_quest_add(&ql, "bad", 0.0, 1), -1);
    /* progress before accept fails */
    GK_CHECK_EQ_INT(gk_saga_quest_progress(&ql, 1, 5.0), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_saga_quest_accept(&ql, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_saga_quest_accept(&ql, 1), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_saga_quest_progress(&ql, 1, 4.0), GK_OK);
    GK_CHECK_EQ_INT(ql.quests[0].state, GK_SAGA_QUEST_ACTIVE);
    GK_CHECK_EQ_INT(gk_saga_quest_progress(&ql, 1, 6.0), GK_OK);
    GK_CHECK_EQ_INT(ql.quests[0].state, GK_SAGA_QUEST_COMPLETE);
    GK_CHECK_EQ_INT(ql.completed, 1);
    GK_CHECK_EQ_INT(ql.total_points, 100);
    GK_CHECK(near(ql.quests[0].progress, 10.0, 1e-9));
    /* complete quest cannot take more progress */
    GK_CHECK_EQ_INT(gk_saga_quest_progress(&ql, 1, 1.0), GK_ERR_STATE);
    /* direct completion of the second quest */
    GK_CHECK_EQ_INT(gk_saga_quest_complete(&ql, 2), GK_OK);
    GK_CHECK_EQ_INT(ql.completed, 2);
    GK_CHECK_EQ_INT(ql.total_points, 150);
    GK_CHECK_EQ_INT(gk_saga_quest_complete(&ql, 2), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_saga_quest_accept(&ql, 99), GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_saga_quest_progress(&ql, 1, -1.0), GK_ERR_STATE);
    gk_saga_quest_log_init(NULL);

    gk_saga_achievements_init(&ach);
    GK_CHECK_EQ_INT(gk_saga_achievement_add(&ach, "first", "First Part",
                                            "Machined one part", 10), 1);
    GK_CHECK_EQ_INT(gk_saga_achievement_add(&ach, "pro", "Pro",
                                            "Machined 100 parts", 50), 2);
    GK_CHECK_EQ_INT(gk_saga_achievement_is_unlocked(&ach, "first"), 0);
    GK_CHECK_EQ_INT(gk_saga_achievement_unlock(&ach, "first"), GK_OK);
    GK_CHECK_EQ_INT(ach.total_points, 10);
    GK_CHECK_EQ_INT(gk_saga_achievement_unlock(&ach, "first"), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_saga_achievement_unlock(&ach, "nope"),
                    GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_saga_achievement_is_unlocked(&ach, "pro"), 0);
    GK_CHECK_EQ_INT(gk_saga_achievement_unlock(NULL, "first"),
                    GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_saga_achievement_is_unlocked(NULL, "first"), 0);
    gk_saga_achievements_init(NULL);

    gk_saga_titles_init(&titles);
    GK_CHECK_EQ_INT(gk_saga_title_add(&titles, "novice", "Novice", 0.0), 1);
    GK_CHECK_EQ_INT(gk_saga_title_add(&titles, "skilled", "Skilled", 100.0), 2);
    GK_CHECK_EQ_INT(gk_saga_title_add(&titles, "master", "Master", 500.0), 3);
    GK_CHECK_STR_EQ(gk_saga_title_evaluate(&titles, 0.0), "Novice");
    GK_CHECK_STR_EQ(gk_saga_title_evaluate(&titles, 150.0), "Skilled");
    GK_CHECK_STR_EQ(gk_saga_title_evaluate(&titles, 1000.0), "Master");
    GK_CHECK_STR_EQ(titles.current, "Master");
    /* below all thresholds */
    {
        gk_saga_titles t2;
        gk_saga_titles_init(&t2);
        GK_CHECK_EQ_INT(gk_saga_title_add(&t2, "x", "X", 10.0), 1);
        GK_CHECK(gk_saga_title_evaluate(&t2, 5.0) == NULL);
    }
    GK_CHECK(gk_saga_title_evaluate(NULL, 1.0) == NULL);
    GK_CHECK_EQ_INT(gk_saga_title_add(NULL, "a", "A", 0.0), -1);
    GK_CHECK_EQ_INT(gk_saga_title_add(&titles, NULL, "A", 0.0), -1);
    gk_saga_titles_init(NULL);
}

int main(void)
{
    test_reverse_program();
    test_reverse_sound();
    test_reverse_curve_tool();
    test_reverse_action_cause();
    test_reverse_cost_feature();
    test_story_actors();
    test_factory();
    test_incidents_challenges();
    test_startup();
    test_branch_plot();
    test_npc_immersion();
    test_quest_achievement_title();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
