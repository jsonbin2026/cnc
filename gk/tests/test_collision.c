#include "gk_test.h"

#include "gk/gk_collision.h"

static gk_aabb box(double x0, double y0, double z0, double x1, double y1,
                   double z1)
{
    gk_aabb b;
    b.min = gk_vec3_make(x0, y0, z0);
    b.max = gk_vec3_make(x1, y1, z1);
    return b;
}

static void test_names(void)
{
    GK_CHECK_STR_EQ(gk_body_kind_name(GK_BODY_TOOL), "tool");
    GK_CHECK_STR_EQ(gk_body_kind_name(GK_BODY_WORKPIECE), "workpiece");
    GK_CHECK_STR_EQ(gk_body_kind_name((gk_body_kind)99), "unknown");
    GK_CHECK_STR_EQ(gk_collision_type_name(GK_COLLISION_TOOL_WORKPIECE),
                    "tool-workpiece");
    GK_CHECK_STR_EQ(gk_collision_type_name((gk_collision_type)99), "unknown");
}

static void test_pair(void)
{
    gk_collision_body a, b;
    gk_collision_event ev;
    a.kind = GK_BODY_TOOL;
    a.enabled = 1;
    a.box = box(0, 0, 0, 2, 2, 5);
    b.kind = GK_BODY_WORKPIECE;
    b.enabled = 1;
    b.box = box(1, 1, 4, 10, 10, 10);

    GK_CHECK(gk_collision_test_pair(&a, &b, &ev));
    GK_CHECK_EQ_INT(ev.type, GK_COLLISION_TOOL_WORKPIECE);
    GK_CHECK(ev.penetration > 0.0);
    GK_CHECK(ev.severity > 0.0 && ev.severity <= 1.0);

    /* disjoint */
    b.box = box(40, 40, 40, 50, 50, 50);
    GK_CHECK(!gk_collision_test_pair(&a, &b, &ev));

    /* disabled */
    a.enabled = 0;
    b.box = box(1, 1, 4, 10, 10, 10);
    GK_CHECK(!gk_collision_test_pair(&a, &b, &ev));
}

static void test_classify(void)
{
    gk_collision_body a, b;
    gk_collision_event ev;
    a.enabled = 1;
    b.enabled = 1;
    a.box = b.box = box(0, 0, 0, 1, 1, 1);

    a.kind = GK_BODY_SPINDLE;
    b.kind = GK_BODY_WORKPIECE;
    gk_collision_test_pair(&a, &b, &ev);
    GK_CHECK_EQ_INT(ev.type, GK_COLLISION_SPINDLE_WORKPIECE);

    a.kind = GK_BODY_HOLDER;
    gk_collision_test_pair(&a, &b, &ev);
    GK_CHECK_EQ_INT(ev.type, GK_COLLISION_HOLDER_WORKPIECE);

    a.kind = GK_BODY_TABLE;
    b.kind = GK_BODY_SPINDLE;
    gk_collision_test_pair(&a, &b, &ev);
    GK_CHECK_EQ_INT(ev.type, GK_COLLISION_TABLE_SPINDLE);

    a.kind = GK_BODY_TOOL;
    b.kind = GK_BODY_FIXTURE;
    gk_collision_test_pair(&a, &b, &ev);
    GK_CHECK_EQ_INT(ev.type, GK_COLLISION_TOOL_FIXTURE);

    a.kind = GK_BODY_TOOL;
    b.kind = GK_BODY_TOOL;
    gk_collision_test_pair(&a, &b, &ev);
    GK_CHECK_EQ_INT(ev.type, GK_COLLISION_CRASH);
}

static void test_world(void)
{
    gk_collision_world w;
    gk_collision_event events[8];
    size_t n;
    gk_collision_world_init(&w);
    GK_CHECK_EQ_INT(gk_collision_world_add(&w, GK_BODY_TOOL,
                                           box(0, 0, 0, 1, 1, 10), "tool"),
                    GK_OK);
    GK_CHECK_EQ_INT(gk_collision_world_add(&w, GK_BODY_WORKPIECE,
                                           box(5, 5, 5, 10, 10, 10), "wp"),
                    GK_OK);
    GK_CHECK_EQ_INT(w.body_count, 2);
    GK_CHECK_STR_EQ(w.bodies[0].name, "tool");

    n = gk_collision_check(&w, events, 8);
    GK_CHECK_EQ_INT(n, 0);

    /* move tool into the workpiece */
    GK_CHECK_EQ_INT(gk_collision_world_move(&w, 0, box(4, 4, 4, 6, 6, 10)),
                    GK_OK);
    n = gk_collision_check(&w, events, 8);
    GK_CHECK_EQ_INT(n, 1);
    GK_CHECK_EQ_INT(events[0].type, GK_COLLISION_TOOL_WORKPIECE);
    GK_CHECK_EQ_INT(events[0].a_index, 0);
    GK_CHECK_EQ_INT(events[0].b_index, 1);

    GK_CHECK(gk_collision_alarm(&events[0], 0.1));
    GK_CHECK(!gk_collision_alarm(&events[0], 0.99) ||
             events[0].severity >= 0.99);
    GK_CHECK(gk_crash_damage(events, 1) > 0.0);

    GK_CHECK_EQ_INT(gk_collision_world_move(&w, 5, box(0, 0, 0, 1, 1, 1)),
                    GK_ERR_OUT_OF_RANGE);
}

static void test_world_overflow(void)
{
    gk_collision_world w;
    int i;
    gk_collision_world_init(&w);
    for (i = 0; i < GK_COLLISION_MAX_BODIES; ++i) {
        GK_CHECK_EQ_INT(gk_collision_world_add(&w, GK_BODY_TOOL,
                                               box(0, 0, 0, 1, 1, 1), NULL),
                        GK_OK);
    }
    GK_CHECK_EQ_INT(gk_collision_world_add(&w, GK_BODY_TOOL,
                                           box(0, 0, 0, 1, 1, 1), NULL),
                    GK_ERR_OVERFLOW);
}

int main(void)
{
    test_names();
    test_pair();
    test_classify();
    test_world();
    test_world_overflow();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
