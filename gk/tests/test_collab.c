#include "gk_test.h"

#include "gk/gk_collab.h"

#include <math.h>
#include <string.h>

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

static void test_session(void)
{
    gk_session s;
    int t, st1, st2;

    gk_session_init(&s);
    t = gk_session_add_user(&s, "Teacher A", GK_ROLE_TEACHER);
    st1 = gk_session_add_user(&s, "Student 1", GK_ROLE_STUDENT);
    st2 = gk_session_add_user(&s, "Student 2", GK_ROLE_STUDENT);
    GK_CHECK(t > 0 && st1 > 0 && st2 > 0);
    GK_CHECK_EQ_INT(s.count, 3);
    GK_CHECK_EQ_INT(gk_session_count_role(&s, GK_ROLE_STUDENT), 2);
    GK_CHECK_EQ_INT(gk_session_count_role(&s, GK_ROLE_TEACHER), 1);
    GK_CHECK(gk_session_user(&s, st1) != NULL);
    GK_CHECK_STR_EQ(gk_session_user(&s, st1)->name, "Student 1");
    GK_CHECK(gk_session_user(&s, 999) == NULL);
    GK_CHECK_EQ_INT(gk_session_set_online(&s, st1, 0), GK_OK);
    GK_CHECK_EQ_INT(s.users[1].online, 0);
    GK_CHECK_EQ_INT(gk_session_set_online(&s, 999, 0), GK_ERR_NOT_FOUND);
}

static void test_monitor(void)
{
    gk_monitor_board m;
    gk_monitor_frame f;

    gk_monitor_init(&m);
    memset(&f, 0, sizeof(f));
    f.user_id = 1;
    f.spindle = 3000.0;
    f.load = 40.0;
    GK_CHECK_EQ_INT(gk_monitor_update(&m, &f), GK_OK);
    f.user_id = 2;
    f.load = 80.0;
    GK_CHECK_EQ_INT(gk_monitor_update(&m, &f), GK_OK);
    GK_CHECK_EQ_INT(m.count, 2);
    f.user_id = 1;
    f.load = 95.0;
    GK_CHECK_EQ_INT(gk_monitor_update(&m, &f), GK_OK);
    GK_CHECK_EQ_INT(m.count, 2);
    GK_CHECK(near(gk_monitor_of(&m, 1)->load, 95.0, 1e-9));
    GK_CHECK_EQ_INT(gk_monitor_peak_load(&m), 1);
    GK_CHECK(gk_monitor_of(&m, 3) == NULL);
}

static void test_takeover(void)
{
    gk_takeover t;

    gk_takeover_init(&t);
    GK_CHECK_EQ_INT(gk_takeover_act(&t), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_takeover_start(&t, 1, 1), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_takeover_start(&t, 1, 2), GK_OK);
    GK_CHECK_EQ_INT(gk_takeover_start(&t, 3, 4), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_takeover_act(&t), GK_OK);
    GK_CHECK_EQ_INT(gk_takeover_act(&t), GK_OK);
    GK_CHECK_EQ_INT(t.actions, 2);
    GK_CHECK_EQ_INT(gk_takeover_stop(&t), GK_OK);
    GK_CHECK_EQ_INT(t.active, 0);
}

static void test_tasks(void)
{
    gk_task_board tb;
    int id;

    gk_task_board_init(&tb);
    id = gk_task_dispatch(&tb, "Square pocket", 2, 100.0);
    GK_CHECK(id > 0);
    GK_CHECK_EQ_INT(gk_task_dispatch(&tb, "G71 cycle", 3, 200.0), id + 1);
    GK_CHECK_EQ_INT(gk_task_open_count(&tb), 2);
    GK_CHECK_EQ_INT(gk_task_count_for_user(&tb, 2), 1);
    GK_CHECK_EQ_INT(gk_task_grade(&tb, id, 90.0), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_task_submit(&tb, id, "M30"), GK_OK);
    GK_CHECK_EQ_INT(gk_task_submit(&tb, id, "again"), GK_ERR_STATE);
    GK_CHECK_STR_EQ(tb.items[0].content, "M30");
    GK_CHECK_EQ_INT(gk_task_open_count(&tb), 1);
    GK_CHECK_EQ_INT(gk_task_grade(&tb, id, 90.0), GK_OK);
    GK_CHECK(near(tb.items[0].score, 90.0, 1e-9));
    GK_CHECK_EQ_INT(tb.items[0].status, 2);
    GK_CHECK_EQ_INT(gk_task_submit(&tb, 999, "x"), GK_ERR_NOT_FOUND);
}

static void test_rank(void)
{
    gk_class_rank r;

    gk_class_rank_init(&r);
    GK_CHECK_EQ_INT(gk_class_rank_submit(&r, "Ann", 80.0), GK_OK);
    GK_CHECK_EQ_INT(gk_class_rank_submit(&r, "Bob", 95.0), GK_OK);
    GK_CHECK_EQ_INT(gk_class_rank_submit(&r, "Cid", 70.0), GK_OK);
    GK_CHECK_STR_EQ(r.entries[0].name, "Bob");
    GK_CHECK_STR_EQ(r.entries[2].name, "Cid");
    GK_CHECK_EQ_INT(gk_class_rank_of(&r, "Cid"), 3);
    GK_CHECK_EQ_INT(gk_class_rank_of(&r, "Zoe"), -1);
    GK_CHECK_EQ_INT(gk_class_rank_submit(&r, "Cid", 99.0), GK_OK);
    GK_CHECK_EQ_INT(r.count, 3);
    GK_CHECK_STR_EQ(r.entries[0].name, "Cid");
}

static void test_network(void)
{
    gk_network n;

    gk_network_init(&n, GK_NET_RING);
    GK_CHECK_EQ_INT(gk_network_add_machine(&n, 10), GK_OK);
    GK_CHECK_EQ_INT(gk_network_add_machine(&n, 20), GK_OK);
    GK_CHECK_EQ_INT(gk_network_add_machine(&n, 30), GK_OK);
    GK_CHECK_EQ_INT(gk_network_link(&n, 10, 20), GK_OK);
    GK_CHECK_EQ_INT(gk_network_link(&n, 20, 30), GK_OK);
    GK_CHECK_EQ_INT(gk_network_link(&n, 10, 99), GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_network_links(&n), 2);
    GK_CHECK_EQ_INT(gk_network_add_machine(&n, 40), GK_OK);
    (void)n;
}

static void test_chat(void)
{
    gk_chat c;

    gk_chat_init(&c);
    GK_CHECK_EQ_INT(gk_chat_send(&c, 1, "hello"), GK_OK);
    GK_CHECK_EQ_INT(gk_chat_send(&c, 2, "world"), GK_OK);
    GK_CHECK_EQ_INT(gk_chat_count(&c), 2);
    GK_CHECK_EQ_INT(c.messages[0].user_id, 1);
    GK_CHECK_STR_EQ(c.messages[1].text, "world");
    GK_CHECK_EQ_INT(gk_chat_allowed(&c, 100.0, 1.0), 1);
    c.messages[1].time = 10.0;
    GK_CHECK_EQ_INT(gk_chat_allowed(&c, 10.5, 1.0), 0);
    GK_CHECK_EQ_INT(gk_chat_allowed(&c, 11.5, 1.0), 1);
}

static void test_broadcast(void)
{
    gk_broadcast b;

    gk_broadcast_init(&b);
    GK_CHECK_EQ_INT(gk_broadcast_start(&b, 1, 2), GK_OK);
    GK_CHECK_EQ_INT(gk_broadcast_start(&b, 2, 2), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_broadcast_join(&b, 1), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_broadcast_join(&b, 2), GK_OK);
    GK_CHECK_EQ_INT(gk_broadcast_join(&b, 2), GK_ERR_ALREADY_EXISTS);
    GK_CHECK_EQ_INT(gk_broadcast_join(&b, 3), GK_OK);
    GK_CHECK_EQ_INT(b.viewer_count, 2);
    GK_CHECK_EQ_INT(gk_broadcast_stop(&b), GK_OK);
    GK_CHECK_EQ_INT(gk_broadcast_join(&b, 4), GK_ERR_STATE);
}

static void test_groups(void)
{
    gk_group_set g;
    int a, b;

    gk_group_set_init(&g);
    a = gk_group_create(&g, "Team A");
    b = gk_group_create(&g, "Team B");
    GK_CHECK(a > 0 && b > 0);
    GK_CHECK_EQ_INT(gk_group_join(&g, a, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_group_join(&g, a, 2), GK_OK);
    GK_CHECK_EQ_INT(gk_group_join(&g, b, 3), GK_OK);
    GK_CHECK_EQ_INT(gk_group_size(&g, a), 2);
    GK_CHECK_EQ_INT(gk_group_size(&g, b), 1);
    GK_CHECK_EQ_INT(gk_group_join(&g, 99, 4), GK_ERR_NOT_FOUND);
}

static void test_competition(void)
{
    gk_competition c;

    gk_competition_init(&c);
    GK_CHECK_EQ_INT(gk_competition_start(&c, "Speed race", 0.0, 60.0), GK_OK);
    GK_CHECK_EQ_INT(gk_competition_record(&c, 1, 42.0), GK_OK);
    GK_CHECK_EQ_INT(gk_competition_record(&c, 2, 38.0), GK_OK);
    GK_CHECK_EQ_INT(c.winner, 2);
    GK_CHECK(near(c.best_time, 38.0, 1e-9));
    GK_CHECK_EQ_INT(gk_competition_is_over(&c, 30.0), 0);
    GK_CHECK_EQ_INT(gk_competition_is_over(&c, 61.0), 1);
}

static void test_classroom(void)
{
    gk_classroom c;

    gk_classroom_init(&c, "CNC 101");
    GK_CHECK_EQ_INT(gk_classroom_add_student(&c, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_classroom_add_student(&c, 2), GK_OK);
    GK_CHECK_EQ_INT(gk_classroom_add_student(&c, 1), GK_ERR_ALREADY_EXISTS);
    GK_CHECK_EQ_INT(gk_classroom_size(&c), 2);
    GK_CHECK_EQ_INT(gk_classroom_has_student(&c, 2), 1);
    GK_CHECK_EQ_INT(gk_classroom_remove_student(&c, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_classroom_size(&c), 1);
    GK_CHECK_EQ_INT(gk_classroom_remove_student(&c, 1), GK_ERR_NOT_FOUND);
}

static void test_progress(void)
{
    gk_progress_tracker p;

    gk_progress_tracker_init(&p);
    GK_CHECK_EQ_INT(gk_course_assign_add(&p, "G-code basics", 1), GK_OK);
    GK_CHECK_EQ_INT(gk_course_assign_add(&p, "G-code basics", 2), GK_OK);
    GK_CHECK_EQ_INT(gk_course_progress(&p, "G-code basics", 1, 0.5), GK_OK);
    GK_CHECK_EQ_INT(gk_course_progress(&p, "G-code basics", 2, 1.0), GK_OK);
    GK_CHECK(near(gk_course_avg_progress(&p, "G-code basics"), 0.75, 1e-9));
    GK_CHECK_EQ_INT(p.items[1].completed, 1);
    GK_CHECK_EQ_INT(p.items[0].completed, 0);
    GK_CHECK_EQ_INT(gk_course_progress(&p, "unknown", 1, 0.5),
                    GK_ERR_NOT_FOUND);
}

static void test_messages(void)
{
    gk_message_box m;

    gk_message_box_init(&m);
    GK_CHECK_EQ_INT(gk_message_send(&m, "teacher", "parent", "Great work", 1.0),
                    GK_OK);
    GK_CHECK_EQ_INT(gk_message_send(&m, "teacher", "parent", "See you", 2.0),
                    GK_OK);
    GK_CHECK_EQ_INT(gk_message_send(&m, "school", "parent", "Notice", 3.0),
                    GK_OK);
    GK_CHECK_EQ_INT(gk_message_count_to(&m, "parent"), 3);
    GK_CHECK_EQ_INT(gk_message_count_to(&m, "other"), 0);
    GK_CHECK_STR_EQ(m.messages[1].message, "See you");
}

static void test_course_editor(void)
{
    gk_course_editor e;
    const gk_course_slide *s;
    int a, b;

    gk_course_editor_init(&e);
    a = gk_course_add_slide(&e, "Intro", "Welcome");
    b = gk_course_add_slide(&e, "Setup", "Clamp the part");
    GK_CHECK(a > 0 && b > 0);
    GK_CHECK_EQ_INT(e.slides[0].order, 0);
    GK_CHECK_EQ_INT(e.slides[1].order, 1);
    s = gk_course_next(&e, 0);
    GK_CHECK(s != NULL && s->id == b);
    GK_CHECK(gk_course_next(&e, 1) == NULL);
    GK_CHECK_EQ_INT(gk_course_move_slide(&e, b, 5), GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_course_move_slide(&e, b, 0), GK_OK);
    GK_CHECK_EQ_INT(gk_course_move_slide(&e, 999, 0), GK_ERR_NOT_FOUND);
}

static void test_scene(void)
{
    gk_scene s;
    int id;

    gk_scene_init(&s);
    GK_CHECK_STR_EQ(s.environment, "workshop");
    id = gk_scene_add(&s, "Vise", 1.0, 2.0, 3.0);
    GK_CHECK(id > 0);
    GK_CHECK_STR_EQ(s.objects[0].name, "Vise");
    GK_CHECK_EQ_INT(s.objects[0].visible, 1);
    GK_CHECK_EQ_INT(gk_scene_transform(&s, id, 5.0, 6.0, 7.0, 90.0), GK_OK);
    GK_CHECK(near(s.objects[0].x, 5.0, 1e-9));
    GK_CHECK(near(s.objects[0].rot, 90.0, 1e-9));
    GK_CHECK_EQ_INT(gk_scene_transform(&s, 999, 0, 0, 0, 0),
                    GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_scene_add(&s, "Tool", 0, 0, 0), id + 1);
    GK_CHECK_EQ_INT(gk_scene_remove(&s, id), GK_OK);
    GK_CHECK_EQ_INT(s.count, 1);
    GK_CHECK_STR_EQ(s.objects[0].name, "Tool");
}

static void test_questions(void)
{
    gk_question_bank b;
    int ids[8];
    int n;

    gk_question_bank_init(&b);
    GK_CHECK_EQ_INT(gk_question_add(&b, "G00 means?", GK_Q_SINGLE, "rapid",
                                    10.0, 1) > 0, 1);
    GK_CHECK_EQ_INT(gk_question_add(&b, "List G-codes", GK_Q_MULTI, "ABC",
                                    20.0, 2) > 0, 1);
    GK_CHECK_EQ_INT(gk_question_add(&b, "Spindle start is M__", GK_Q_FILL,
                                    "M03", 15.0, 2) > 0, 1);
    GK_CHECK_EQ_INT(b.count, 3);
    n = gk_exam_generate(&b, 2, 5.0, ids, 8);
    GK_CHECK_EQ_INT(n, 2);
    GK_CHECK_EQ_INT(ids[0], 1);
    n = gk_exam_generate(&b, 8, 40.0, ids, 8);
    GK_CHECK_EQ_INT(n, 3);
}

static void test_library(void)
{
    gk_library l;
    const gk_lib_item *it;

    gk_library_init(&l, "tools");
    GK_CHECK_STR_EQ(l.domain, "tools");
    GK_CHECK_EQ_INT(gk_library_add(&l, "End mill 6mm", "SANDVIK", 6.0, "mm") > 0,
                    1);
    GK_CHECK_EQ_INT(gk_library_add(&l, "Drill 5mm", "OSG", 5.0, "mm") > 0, 1);
    GK_CHECK_EQ_INT(gk_library_count(&l), 2);
    it = gk_library_find(&l, "Drill 5mm");
    GK_CHECK(it != NULL);
    GK_CHECK_STR_EQ(it->vendor, "OSG");
    GK_CHECK(near(it->value, 5.0, 1e-9));
    GK_CHECK(gk_library_find(&l, "none") == NULL);
}

static void test_updater(void)
{
    gk_updater u;
    char buf[32];

    gk_updater_init(&u, 1, 2, 3);
    GK_CHECK_EQ_INT(u.update_available, 0);
    GK_CHECK_EQ_INT(gk_updater_check(&u, 1, 2, 4, 12.5), GK_OK);
    GK_CHECK_EQ_INT(u.update_available, 1);
    GK_CHECK(near(u.size_mb, 12.5, 1e-9));
    GK_CHECK_EQ_INT(gk_updater_install(&u), 1);
    GK_CHECK_EQ_INT(u.current.patch, 4);
    GK_CHECK_EQ_INT(u.update_available, 0);
    GK_CHECK_EQ_INT(gk_updater_install(&u), 0);
    GK_CHECK(gk_updater_version_string(&u, buf, sizeof(buf)) != NULL);
    GK_CHECK_STR_EQ(buf, "1.2.4");
    GK_CHECK_EQ_INT(gk_updater_check(&u, 1, 2, 0, 0.0), GK_OK);
    GK_CHECK_EQ_INT(u.update_available, 0);
}

static void test_distribution(void)
{
    gk_distribution d;

    gk_distribution_init(&d, 4, 100.0);
    GK_CHECK_EQ_INT(gk_distribution_push(&d, 10), GK_OK);
    GK_CHECK_EQ_INT(d.distributed, 40);
    GK_CHECK_EQ_INT(gk_distribution_push(&d, -1), GK_ERR_INVALID_ARG);
}

static void test_history(void)
{
    gk_content_history h;
    const gk_content_version *v;

    gk_content_history_init(&h);
    GK_CHECK_EQ_INT(gk_content_commit(&h, "initial", 1), 1);
    GK_CHECK_EQ_INT(gk_content_commit(&h, "added slides", 2), 2);
    GK_CHECK_EQ_INT(h.count, 2);
    v = gk_content_version_at(&h, 2);
    GK_CHECK(v != NULL);
    GK_CHECK_STR_EQ(v->note, "added slides");
    GK_CHECK_EQ_INT(v->author_id, 2);
    GK_CHECK(gk_content_version_at(&h, 9) == NULL);
}

static void test_ugc(void)
{
    gk_ugc_store s;
    int id;

    gk_ugc_store_init(&s);
    id = gk_ugc_publish(&s, 7, "My first part", "gcode");
    GK_CHECK(id > 0);
    GK_CHECK_STR_EQ(s.items[0].title, "My first part");
    GK_CHECK_EQ_INT(s.items[0].published, 1);
    GK_CHECK_EQ_INT(gk_ugc_rate(&s, id, 4.0), GK_OK);
    GK_CHECK_EQ_INT(gk_ugc_rate(&s, id, 5.0), GK_OK);
    GK_CHECK(near(gk_ugc_rating(&s, id), 4.5, 1e-9));
    GK_CHECK_EQ_INT(gk_ugc_rate(&s, id, 9.0), GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_ugc_rate(&s, 99, 3.0), GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_ugc_count_by_author(&s, 7), 1);
    GK_CHECK_EQ_INT(gk_ugc_count_by_author(&s, 8), 0);
}

int main(void)
{
    test_session();
    test_monitor();
    test_takeover();
    test_tasks();
    test_rank();
    test_network();
    test_chat();
    test_broadcast();
    test_groups();
    test_competition();
    test_classroom();
    test_progress();
    test_messages();
    test_course_editor();
    test_scene();
    test_questions();
    test_library();
    test_updater();
    test_distribution();
    test_history();
    test_ugc();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
