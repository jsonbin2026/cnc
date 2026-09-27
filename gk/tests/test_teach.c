#include "gk_test.h"

#include "gk/gk_teach.h"

#include <math.h>
#include <string.h>

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

static void test_course(void)
{
    gk_course c;
    gk_lesson_step s;
    gk_progress p;

    gk_course_init(&c);
    GK_CHECK_EQ_INT(gk_course_add_lesson(&c, 1, "Basics", "{\"id\":1}"), GK_OK);
    GK_CHECK_EQ_INT(gk_course_add_lesson(&c, 2, "Advanced", "{}"), GK_OK);
    GK_CHECK_EQ_INT(c.lesson_count, 2);

    memset(&s, 0, sizeof(s));
    s.id = 1;
    strcpy(s.title, "G00");
    strcpy(s.highlight, "#btn-g00");
    strcpy(s.narration, "G00 is rapid");
    strcpy(s.subtitle, "Rapid move");
    GK_CHECK_EQ_INT(gk_course_add_step(&c, 0, &s), GK_OK);
    s.id = 2;
    strcpy(s.highlight, "#btn-g01");
    GK_CHECK_EQ_INT(gk_course_add_step(&c, 0, &s), GK_OK);

    GK_CHECK_EQ_INT(gk_course_goto(&c, 0, 0), GK_OK);
    GK_CHECK(gk_course_current_step(&c) != NULL);
    GK_CHECK_STR_EQ(gk_course_current_step(&c)->title, "G00");
    GK_CHECK_STR_EQ(gk_course_current_highlight(&c), "#btn-g00");

    /* next step within the lesson */
    GK_CHECK_EQ_INT(gk_course_next_step(&c), GK_OK);
    GK_CHECK_STR_EQ(gk_course_current_highlight(&c), "#btn-g01");
    /* next lesson */
    GK_CHECK_EQ_INT(gk_course_next_step(&c), GK_OK);
    GK_CHECK_EQ_INT(c.current_lesson, 1);
    /* at the end -> completes */
    GK_CHECK_EQ_INT(gk_course_next_step(&c), GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(c.completed, 1);

    GK_CHECK_EQ_INT(gk_course_prev_step(&c), GK_OK);
    GK_CHECK_EQ_INT(gk_course_goto(&c, 5, 0), GK_ERR_OUT_OF_RANGE);

    gk_progress_init(&p);
    GK_CHECK_EQ_INT(gk_progress_mark(&p, 0, 0, 12.0), GK_OK);
    GK_CHECK_EQ_INT(gk_progress_mark(&p, 0, 0, 3.0), GK_OK); /* no double count */
    GK_CHECK_EQ_INT(p.total_steps_done, 1);
    GK_CHECK(near(p.total_time, 15.0, 1e-9));
    GK_CHECK(gk_progress_is_done(&p, 0, 0));
    GK_CHECK(!gk_progress_is_done(&p, 0, 1));
    GK_CHECK(near(gk_progress_completion(&p, &c), 0.5, 1e-9));
}

static void test_manuals(void)
{
    GK_CHECK(gk_manual_g(0) != NULL);
    GK_CHECK_STR_EQ(gk_manual_g(0)->name, "G00");
    GK_CHECK_STR_EQ(gk_manual_g(83)->description, "Peck drilling cycle");
    GK_CHECK(gk_manual_m(6) != NULL);
    GK_CHECK_STR_EQ(gk_manual_m(6)->name, "M06");
    GK_CHECK(gk_manual_m(6)->is_mcode);
    GK_CHECK(gk_manual_g(9999) == NULL);
    GK_CHECK(gk_manual_count(0) > 0);
    GK_CHECK(gk_manual_count(1) > 0);
    GK_CHECK(gk_manual_lookup(90, 0) != NULL);
    GK_CHECK(gk_manual_lookup(90, 1) == NULL);
}

static void test_shortcuts_score(void)
{
    gk_shortcut_set sc;
    gk_score s;
    gk_shortcut_set_init(&sc);
    GK_CHECK_EQ_INT(gk_shortcut_add(&sc, "Ctrl+G", "goto"), GK_OK);
    GK_CHECK_EQ_INT(gk_shortcut_add(&sc, "Space", "run"), GK_OK);
    GK_CHECK_EQ_INT(gk_shortcut_use(&sc, "Space"), GK_OK);
    GK_CHECK_EQ_INT(gk_shortcut_use(&sc, "Space"), GK_OK);
    GK_CHECK_EQ_INT(gk_shortcut_use(&sc, "Ctrl+G"), GK_OK);
    GK_CHECK_EQ_INT(gk_shortcut_total_uses(&sc), 3);
    GK_CHECK_EQ_INT(gk_shortcut_use(&sc, "Ctrl+Z"), GK_ERR_NOT_FOUND);

    gk_score_init(&s);
    s.correctness = 1.0;
    s.speed = 1.0;
    s.efficiency = 1.0;
    GK_CHECK(near(gk_score_compute(&s), 100.0, 1e-9));
    s.penalties = 20.0;
    GK_CHECK(near(gk_score_compute(&s), 80.0, 1e-9));
}

static void test_wrong_log_grade(void)
{
    gk_wrong_log l;
    gk_score s;
    int answers[4] = {1, 0, 1, 1};
    int keys[4] = {1, 1, 1, 0};
    char out[512];

    gk_wrong_log_init(&l);
    gk_wrong_log_add(&l, 10, 2, 3, "G-codes");
    gk_wrong_log_add(&l, 10, 2, 3, "G-codes"); /* repeat */
    GK_CHECK_EQ_INT(l.count, 1);
    GK_CHECK_EQ_INT(l.items[0].attempts, 2);
    GK_CHECK_EQ_INT(gk_wrong_log_repeat_count(&l), 1);

    GK_CHECK_EQ_INT(gk_auto_grade(answers, keys, 4), 2);

    gk_score_init(&s);
    s.correctness = 0.5;
    s.score = 50.0;
    GK_CHECK(gk_transcript_export(&s, &l, out, sizeof(out)) > 0);
    GK_CHECK(strstr(out, "score=50.0") != NULL);
    GK_CHECK(strstr(out, "wrong q10") != NULL);
}

static void test_exam(void)
{
    gk_exam e;
    gk_exam_init(&e);
    GK_CHECK(!gk_exam_time_up(&e));
    gk_exam_start(&e, 60.0);
    GK_CHECK(e.exam_mode);
    e.elapsed = 61.0;
    GK_CHECK(gk_exam_time_up(&e));
}

static void test_leaderboard(void)
{
    gk_leaderboard b;
    gk_leaderboard_init(&b);
    GK_CHECK_EQ_INT(gk_leaderboard_submit(&b, "alice", 80), GK_OK);
    GK_CHECK_EQ_INT(gk_leaderboard_submit(&b, "bob", 95), GK_OK);
    GK_CHECK_EQ_INT(gk_leaderboard_submit(&b, "carol", 70), GK_OK);
    GK_CHECK_EQ_INT(b.count, 3);
    GK_CHECK_STR_EQ(b.entries[0].name, "bob");
    GK_CHECK_STR_EQ(b.entries[1].name, "alice");
    GK_CHECK_EQ_INT(gk_leaderboard_rank_of(&b, "alice"), 2);
    GK_CHECK_EQ_INT(gk_leaderboard_rank_of(&b, "dave"), -1);
}

static void test_points_badges(void)
{
    gk_player_points p;
    gk_badge_set bs;
    gk_player_points_init(&p);
    GK_CHECK_EQ_INT(p.level, 1);
    gk_player_points_add(&p, 150);
    GK_CHECK_EQ_INT(p.points, 150);
    GK_CHECK_EQ_INT(p.level, 2);
    GK_CHECK_EQ_INT(p.streak, 1);
    gk_player_points_add(&p, -50);
    GK_CHECK_EQ_INT(p.streak, 0);
    GK_CHECK_EQ_INT(gk_player_level_for_points(0), 1);
    GK_CHECK_EQ_INT(gk_player_level_for_points(999), 10);

    gk_badge_set_init(&bs);
    gk_badge_add(&bs, "first", "First Steps", "Complete first lesson", 1);
    gk_badge_add(&bs, "expert", "Expert", "Score 90", 90);
    GK_CHECK_EQ_INT(gk_badge_evaluate(&bs, 50.0), 1);
    GK_CHECK_EQ_INT(gk_badge_evaluate(&bs, 95.0), 2);
    GK_CHECK_EQ_INT(gk_badge_evaluate(&bs, 95.0), 2);
}

static void test_certificate(void)
{
    gk_certificate cert;
    gk_certificate_build(&cert, "Alice", "CNC Basics", 85, 2026, 9, 27);
    GK_CHECK(cert.valid);
    GK_CHECK_STR_EQ(cert.holder, "Alice");
    {
        char buf[256];
        gk_certificate_text(&cert, buf, sizeof(buf));
        GK_CHECK(strstr(buf, "Alice") != NULL);
        GK_CHECK(strstr(buf, "VALID") != NULL);
    }
    gk_certificate_build(&cert, "Bob", "CNC Basics", 50, 2026, 1, 1);
    GK_CHECK(!cert.valid);
}

static void test_challenge(void)
{
    gk_challenge c;
    GK_CHECK_STR_EQ(gk_challenge_name(GK_CHALLENGE_TIMED), "timed");
    GK_CHECK_STR_EQ(gk_challenge_name((gk_challenge_kind)99), "unknown");

    GK_CHECK_EQ_INT(gk_challenge_init(&c, GK_CHALLENGE_TIMED, 100.0, 5.0),
                    GK_OK);
    gk_challenge_update(&c, 100.0, 3.0);
    GK_CHECK(gk_challenge_passed(&c));
    gk_challenge_update(&c, 100.0, 5.0);
    GK_CHECK(!gk_challenge_passed(&c)); /* exceeded time */

    GK_CHECK_EQ_INT(gk_challenge_init(&c, GK_CHALLENGE_PRECISION, 0.05, 0.0),
                    GK_OK);
    gk_challenge_update(&c, 0.03, 1.0);
    GK_CHECK(gk_challenge_passed(&c));
    gk_challenge_update(&c, 0.08, 1.0);
    GK_CHECK(!gk_challenge_passed(&c));
}

static void test_dialog(void)
{
    gk_dialog d;
    gk_dialog_init(&d);
    gk_dialog_add(&d, 1, "Master", "Welcome!");
    gk_dialog_add(&d, 2, "Apprentice", "Thanks.");
    gk_dialog_add(&d, 3, "Master", "Let us begin.");
    d.nodes[0].next_id = 2;
    d.nodes[0].branch_a = 2;
    d.nodes[0].branch_b = 3;

    GK_CHECK(gk_dialog_current(&d) != NULL);
    GK_CHECK_STR_EQ(gk_dialog_current(&d)->speaker, "Master");
    GK_CHECK_EQ_INT(gk_dialog_advance(&d), GK_OK);
    GK_CHECK_STR_EQ(gk_dialog_current(&d)->text, "Thanks.");

    d.current = 0;
    GK_CHECK_EQ_INT(gk_dialog_choose(&d, 1), GK_OK);
    GK_CHECK_STR_EQ(gk_dialog_current(&d)->text, "Let us begin.");

    d.current = 2; /* node 3 has no next */
    GK_CHECK_EQ_INT(gk_dialog_advance(&d), GK_ERR_OUT_OF_RANGE);
}

static void test_training(void)
{
    gk_train_session s;
    GK_CHECK_STR_EQ(gk_train_mode_name(GK_TRAIN_REVERSE), "reverse");
    gk_train_session_init(&s, GK_TRAIN_BLIND);
    GK_CHECK(s.ui_hidden && s.program_hidden);
    gk_train_session_error(&s);
    GK_CHECK_EQ_INT(s.errors, 1);

    gk_train_session_init(&s, GK_TRAIN_FORWARD);
    GK_CHECK(!s.ui_hidden && !s.program_hidden);
}

static void test_replay(void)
{
    gk_replay r;
    double axes[6] = {1, 2, 3, 0, 0, 0};
    const gk_replay_frame *f;
    gk_replay_init(&r);
    GK_CHECK_EQ_INT(gk_replay_push(&r, 0.1, axes, 0), GK_OK);
    axes[0] = 5.0;
    GK_CHECK_EQ_INT(gk_replay_push(&r, 0.2, axes, 1), GK_OK);
    GK_CHECK_EQ_INT(r.count, 2);

    GK_CHECK_EQ_INT(gk_replay_start_replay(&r), GK_OK);
    f = gk_replay_next(&r);
    GK_CHECK(f != NULL);
    GK_CHECK(near(f->axis[0], 1.0, 1e-9));
    f = gk_replay_next(&r);
    GK_CHECK(near(f->axis[0], 5.0, 1e-9));
    GK_CHECK(gk_replay_next(&r) == NULL);
    GK_CHECK(gk_replay_at_end(&r));
}

static void test_cognitive(void)
{
    gk_heatmap h;
    gk_heatmap_init(&h);
    GK_CHECK_EQ_INT(gk_heatmap_add_region(&h, "editor"), GK_OK);
    GK_CHECK_EQ_INT(gk_heatmap_add_region(&h, "editor"), GK_ERR_ALREADY_EXISTS);
    gk_heatmap_observe(&h, "editor", 3.0);
    gk_heatmap_observe(&h, "toolbar", 1.0);
    GK_CHECK(near(gk_heatmap_weight(&h, "editor"), 0.75, 1e-9));
    GK_CHECK_STR_EQ(gk_heatmap_hottest(&h), "editor");

    {
        gk_gaze a = {0, 0, 0};
        gk_gaze b = {0.001, 0.0, 0.1};
        gk_gaze c = {50, 50, 0.2};
        GK_CHECK(gk_gaze_is_fixation(&a, &b, 10.0));
        GK_CHECK(!gk_gaze_is_fixation(&b, &c, 10.0));
        GK_CHECK(gk_gaze_velocity(&b, &c) > 0.0);
    }

    GK_CHECK(near(gk_cognitive_load(5.0, 10.0), 0.5, 1e-9));
    GK_CHECK_STR_EQ(gk_cognitive_load_level(0.5), "optimal");
    GK_CHECK_STR_EQ(gk_cognitive_load_level(2.0), "overload");

    GK_CHECK(near(gk_memory_retention(10.0, 0.0), 1.0, 1e-9));
    GK_CHECK(gk_memory_retention(10.0, 10.0) < 0.4);

    GK_CHECK(near(gk_next_review_interval(10.0, 3), 18.0, 1e-9));
    GK_CHECK(near(gk_next_review_interval(10.0, 0), 5.0, 1e-9));
    GK_CHECK_EQ_INT(gk_micro_session_count(100, 15), 7);
    GK_CHECK_EQ_INT(gk_micro_session_count(100, 0), 0);

    GK_CHECK(gk_flow_detect(10.0, 10.0));
    GK_CHECK(!gk_flow_detect(10.0, 30.0));

    {
        gk_metacog m = {0.9, 0.9};
        GK_CHECK(near(gk_metacog_calibration(&m), 1.0, 1e-9));
        m.accuracy = 0.5;
        GK_CHECK(near(gk_metacog_calibration(&m), 0.6, 1e-9));
    }
    GK_CHECK_STR_EQ(gk_error_attribution(0.8, 0.2), "internal");
    GK_CHECK_STR_EQ(gk_error_attribution(0.2, 0.8), "external");
    GK_CHECK(near(gk_transfer_gain(10.0, 15.0), 0.5, 1e-9));

    {
        double times[3] = {0.5, 0.6, 0.4};
        int results[4] = {1, 1, 0, 1};
        GK_CHECK(near(gk_reaction_mean(times, 3), 0.5, 1e-9));
        GK_CHECK(gk_reaction_stddev(times, 3) > 0.0);
        GK_CHECK(near(gk_error_rate(results, 4), 0.25, 1e-9));
        GK_CHECK(near(gk_engagement_index(8.0, 2.0, 0.0), 0.8, 1e-9));
    }
}

int main(void)
{
    test_course();
    test_manuals();
    test_shortcuts_score();
    test_wrong_log_grade();
    test_exam();
    test_leaderboard();
    test_points_badges();
    test_certificate();
    test_challenge();
    test_dialog();
    test_training();
    test_replay();
    test_cognitive();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
