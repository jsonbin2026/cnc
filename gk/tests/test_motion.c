#include "gk_test.h"

#include "gk/gk_math.h"
#include "gk/gk_state.h"
#include "gk/gk_motion.h"
#include "gk/gk_move.h"
#include "gk/gk_voxel.h"

#include <math.h>

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

static void test_math(void)
{
    gk_vec3 a = gk_vec3_make(1.0, 2.0, 3.0);
    gk_vec3 b = gk_vec3_make(4.0, 5.0, 6.0);
    gk_vec3 c;
    gk_aabb box;

    GK_CHECK(near(gk_vec3_dot(a, b), 32.0, 1e-9));
    c = gk_vec3_cross(a, b);
    GK_CHECK(near(c.x, -3.0, 1e-9));
    GK_CHECK(near(c.y, 6.0, 1e-9));
    GK_CHECK(near(c.z, -3.0, 1e-9));
    GK_CHECK(near(gk_vec3_length(gk_vec3_make(3, 4, 0)), 5.0, 1e-9));
    GK_CHECK(near(gk_vec3_distance(a, b), sqrt(27.0), 1e-9));

    box = gk_aabb_from_points(a, b);
    GK_CHECK(gk_aabb_is_valid(&box));
    GK_CHECK(gk_aabb_contains(&box, gk_vec3_make(2, 3, 4)));
    GK_CHECK(!gk_aabb_contains(&box, gk_vec3_make(0, 3, 4)));
    box = gk_aabb_expand(box, gk_vec3_make(0, 0, 0));
    GK_CHECK(gk_aabb_contains(&box, gk_vec3_make(0, 0, 0)));

    GK_CHECK(near(gk_clamp(5.0, 0.0, 1.0), 1.0, 1e-9));
    GK_CHECK(near(gk_lerp(0.0, 10.0, 0.25), 2.5, 1e-9));
    GK_CHECK(near(gk_normalize_angle(-90.0), 270.0, 1e-9));
    GK_CHECK(near(gk_angle_diff(350.0, 10.0), -20.0, 1e-9));
}

static void test_state(void)
{
    gk_machine_state s;
    gk_state_init(&s);
    GK_CHECK_EQ_INT(s.plane, GK_PLANE_XY);
    GK_CHECK_EQ_INT(s.unit, GK_UNIT_MM);
    GK_CHECK_EQ_INT(s.distance, GK_DIST_ABSOLUTE);
    GK_CHECK_EQ_INT(gk_state_select_plane(&s, GK_PLANE_ZX), GK_OK);
    GK_CHECK_EQ_INT(s.plane, GK_PLANE_ZX);
    GK_CHECK_EQ_INT(gk_state_select_plane(&s, (gk_plane)99),
                    GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_state_set_unit(&s, GK_UNIT_INCH), GK_OK);
    GK_CHECK(near(gk_state_unit_scale(&s), 25.4, 1e-9));
    GK_CHECK_EQ_INT(gk_state_set_work_offset(&s, 54), GK_OK);
    GK_CHECK_EQ_INT(s.work_offset, 0);
    GK_CHECK_EQ_INT(gk_state_set_work_offset(&s, 53), GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_state_set_work_offset(&s, 59), GK_OK);
    GK_CHECK_EQ_INT(s.work_offset, 5);
    gk_state_set_axis(&s, GK_AXIS_X, 12.5);
    GK_CHECK(near(gk_state_get_axis(&s, GK_AXIS_X), 12.5, 1e-9));
}

static void test_linear_move(void)
{
    gk_move m;
    gk_point3 a = gk_vec3_make(0, 0, 0);
    gk_point3 b = gk_vec3_make(3, 4, 0);
    GK_CHECK_EQ_INT(gk_move_linear(&m, a, b, 100.0, GK_MOTION_LINEAR), GK_OK);
    GK_CHECK(near(gk_move_length(&m), 5.0, 1e-9));
    GK_CHECK_EQ_INT(gk_move_linear(&m, a, b, 100.0, GK_MOTION_CW),
                    GK_ERR_INVALID_ARG);
}

static void test_arc_ijk_length(void)
{
    gk_move m;
    gk_point3 a = gk_vec3_make(10, 0, 0);
    gk_point3 b = gk_vec3_make(0, 10, 0);
    gk_point3 c = gk_vec3_make(0, 0, 0);
    /* Quarter circle CCW, radius 10 -> length pi*10/2 */
    GK_CHECK_EQ_INT(gk_move_arc_ijk(&m, a, b, c, 100.0, GK_MOTION_CCW,
                                     GK_PLANE_XY), GK_OK);
    GK_CHECK(near(m.radius, 10.0, 1e-9));
    GK_CHECK(near(gk_move_length(&m), GK_PI * 10.0 / 2.0, 1e-6));

    /* Same endpoints CW -> 3/4 circle */
    GK_CHECK_EQ_INT(gk_move_arc_ijk(&m, a, b, c, 100.0, GK_MOTION_CW,
                                     GK_PLANE_XY), GK_OK);
    GK_CHECK(near(gk_move_length(&m), 3.0 * GK_PI * 10.0 / 2.0, 1e-6));
}

static void test_arc_radius(void)
{
    gk_move m;
    gk_point3 a = gk_vec3_make(0, 0, 0);
    gk_point3 b = gk_vec3_make(10, 0, 0);
    /* Half circle with R=5 from (0,0) to (10,0). */
    GK_CHECK_EQ_INT(gk_move_arc_radius(&m, a, b, 5.0, 100.0, GK_MOTION_CCW,
                                       GK_PLANE_XY), GK_OK);
    GK_CHECK(near(m.radius, 5.0, 1e-9));
    GK_CHECK(near(gk_move_length(&m), GK_PI * 5.0, 1e-6));
    GK_CHECK(near(m.center.x, 5.0, 1e-6));
    GK_CHECK(near(m.center.y, 0.0, 1e-6));

    /* Too small R for the chord -> out of range */
    GK_CHECK_EQ_INT(gk_move_arc_radius(&m, a, b, 2.0, 100.0, GK_MOTION_CCW,
                                       GK_PLANE_XY), GK_ERR_OUT_OF_RANGE);
}

static void test_arc_interpolate(void)
{
    gk_move m;
    gk_point3 a = gk_vec3_make(10, 0, 0);
    gk_point3 b = gk_vec3_make(0, 10, 0);
    gk_point3 c = gk_vec3_make(0, 0, 0);
    gk_point3 p;
    gk_move_arc_ijk(&m, a, b, c, 100.0, GK_MOTION_CCW, GK_PLANE_XY);
    GK_CHECK_EQ_INT(gk_interpolate_arc_point(&m, 0.0, &p), GK_OK);
    GK_CHECK(near(p.x, 10.0, 1e-6));
    GK_CHECK(near(p.y, 0.0, 1e-6));
    GK_CHECK_EQ_INT(gk_interpolate_arc_point(&m, 1.0, &p), GK_OK);
    GK_CHECK(near(p.x, 0.0, 1e-6));
    GK_CHECK(near(p.y, 10.0, 1e-6));
    /* midpoint should be on the circle */
    gk_interpolate_arc_point(&m, 0.5, &p);
    GK_CHECK(near(sqrt(p.x * p.x + p.y * p.y), 10.0, 1e-6));
}

static void test_plan_trapezoid(void)
{
    gk_motion_config cfg;
    gk_move m;
    gk_motion_plan plan;
    double v = 0.0;

    gk_motion_config_default(&cfg);
    cfg.axes[GK_AXIS_X].max_velocity = 100.0;  /* mm/s */
    cfg.axes[GK_AXIS_Y].max_velocity = 100.0;
    cfg.axes[GK_AXIS_Z].max_velocity = 100.0;
    cfg.axes[GK_AXIS_X].max_accel = 100.0;
    cfg.axes[GK_AXIS_Y].max_accel = 100.0;
    cfg.axes[GK_AXIS_Z].max_accel = 100.0;
    cfg.rapid_feed = 6000.0; /* 100 mm/s at override 1 */

    /* Long move: 1000mm at 100mm/s, accel 100 -> cruise at 100. */
    gk_move_linear(&m, gk_vec3_make(0, 0, 0), gk_vec3_make(1000, 0, 0),
                   6000.0, GK_MOTION_LINEAR);
    GK_CHECK_EQ_INT(gk_plan_move(&cfg, &m, &plan), GK_OK);
    GK_CHECK(near(plan.cruise_velocity, 100.0, 1e-6));
    GK_CHECK(near(plan.accel_time, 1.0, 1e-6));
    GK_CHECK(plan.cruise_time > 0.0);
    GK_CHECK(near(plan.duration, 2.0 + plan.cruise_time, 1e-6));
    GK_CHECK(plan.steps >= 100);

    /* Velocity at start 0, during cruise = 100, at end 0 */
    gk_plan_velocity_at(&plan, 0.0, &v);
    GK_CHECK(near(v, 0.0, 1e-9));
    gk_plan_velocity_at(&plan, plan.duration / 2.0, &v);
    GK_CHECK(near(v, 100.0, 1e-6));
    gk_plan_velocity_at(&plan, plan.duration, &v);
    GK_CHECK(near(v, 0.0, 1e-9));

    /* Sampling: endpoints correct */
    {
        gk_interp_sample s;
        gk_plan_sample(&plan, 0, plan.steps, &s);
        GK_CHECK(near(s.position.x, 0.0, 1e-6));
        gk_plan_sample(&plan, plan.steps - 1, plan.steps, &s);
        GK_CHECK(near(s.position.x, 1000.0, 1e-6));
    }
}

static void test_plan_triangular(void)
{
    gk_motion_config cfg;
    gk_move m;
    gk_motion_plan plan;
    gk_motion_config_default(&cfg);
    cfg.axes[GK_AXIS_X].max_velocity = 100.0;
    cfg.axes[GK_AXIS_X].max_accel = 100.0;

    /* Short move: 10mm cannot reach 100mm/s -> triangular. */
    gk_move_linear(&m, gk_vec3_make(0, 0, 0), gk_vec3_make(10, 0, 0),
                   6000.0, GK_MOTION_LINEAR);
    GK_CHECK_EQ_INT(gk_plan_move(&cfg, &m, &plan), GK_OK);
    GK_CHECK(plan.cruise_velocity < 100.0);
    GK_CHECK(near(plan.cruise_time, 0.0, 1e-9));
    GK_CHECK(near(plan.cruise_velocity, sqrt(10.0 * 100.0), 1e-4));
}

static void test_plan_axis_limit(void)
{
    gk_motion_config cfg;
    gk_move m;
    double v;
    gk_motion_config_default(&cfg);
    /* Only X can move fast; Y is slow. A diagonal move must be limited. */
    cfg.axes[GK_AXIS_X].max_velocity = 1000.0;
    cfg.axes[GK_AXIS_Y].max_velocity = 10.0;
    gk_move_linear(&m, gk_vec3_make(0, 0, 0), gk_vec3_make(100, 100, 0),
                   6000.0, GK_MOTION_LINEAR);
    v = gk_axis_limited_velocity(&cfg, &m, 1000.0);
    /* total = ~141.42, y frac = 100/141.42 -> max path v = 10*141.42/100 */
    GK_CHECK(near(v, 10.0 * sqrt(2.0), 1e-6));
}

static void test_axis_range(void)
{
    gk_motion_config cfg;
    gk_motion_config_default(&cfg);
    cfg.axes[GK_AXIS_X].min_travel = -100.0;
    cfg.axes[GK_AXIS_X].max_travel = 100.0;
    GK_CHECK(gk_axis_in_range(&cfg, GK_AXIS_X, 50.0));
    GK_CHECK(!gk_axis_in_range(&cfg, GK_AXIS_X, 150.0));
    cfg.axes[GK_AXIS_X].enabled = 0;
    GK_CHECK(!gk_axis_in_range(&cfg, GK_AXIS_X, 50.0));
}

static void test_servo_first_order(void)
{
    gk_servo s;
    gk_servo_params p;
    double out;
    p.time_constant = 0.1;
    p.natural_freq = 0.0;
    p.damping = 0.0;
    p.backlash = 0.0;
    p.pitch_error = 0.0;
    p.following_error = 0.0;
    gk_servo_init(&s, &p, 0);
    /* Step toward 1.0 with dt=tau -> reaches 0.5 */
    out = gk_servo_step(&s, 1.0, 0.1);
    GK_CHECK(near(out, 0.5, 1e-9));
    /* Converges toward command */
    {
        int i;
        for (i = 0; i < 200; ++i) {
            out = gk_servo_step(&s, 1.0, 0.1);
        }
    }
    GK_CHECK(near(out, 1.0, 1e-3));
    GK_CHECK(near(gk_servo_following_error(&s, 1.0), 0.0, 1e-3));
}

static void test_servo_second_order(void)
{
    gk_servo s;
    gk_servo_params p;
    int i;
    double out = 0.0;
    p.time_constant = 0.0;
    p.natural_freq = 50.0;
    p.damping = 0.7;
    p.backlash = 0.0;
    p.pitch_error = 0.0;
    p.following_error = 0.0;
    gk_servo_init(&s, &p, 1);
    for (i = 0; i < 1000; ++i) {
        out = gk_servo_step(&s, 1.0, 0.0005);
    }
    GK_CHECK(near(out, 1.0, 1e-2));
}

static void test_servo_backlash(void)
{
    gk_servo s;
    gk_servo_params p;
    p.time_constant = 0.0;
    p.natural_freq = 0.0;
    p.damping = 0.0;
    p.backlash = 0.02;
    p.pitch_error = 0.0;
    p.following_error = 0.0;
    gk_servo_init(&s, &p, 0);
    s.position = 10.0;
    gk_servo_compensate_backlash(&s, -1.0);
    GK_CHECK(near(s.position, 10.0, 1e-9)); /* first move, no compensation */
    gk_servo_compensate_backlash(&s, 1.0);
    GK_CHECK(near(s.position, 10.02, 1e-9));
}

static void test_voxel(void)
{
    gk_voxel_grid g;
    size_t removed = 0;
    size_t present;
    GK_CHECK_EQ_INT(gk_voxel_init(&g, 10, 10, 10, 1.0,
                                  gk_vec3_make(0, 0, 0), NULL), GK_OK);
    GK_CHECK_EQ_INT(gk_voxel_count(&g), 1000);
    present = gk_voxel_present_count(&g);
    GK_CHECK_EQ_INT(present, 1000);

    /* Cut a point at the center of voxel (5,5,5) with radius 0.4. */
    GK_CHECK_EQ_INT(gk_voxel_cut_point(&g, gk_vec3_make(5.5, 5.5, 5.5),
                                       0.4, &removed), GK_OK);
    GK_CHECK_EQ_INT(removed, 1);
    GK_CHECK_EQ_INT(gk_voxel_get(&g, 5, 5, 5), 0);
    GK_CHECK_EQ_INT(gk_voxel_present_count(&g), 999);

    /* Out-of-range set */
    GK_CHECK_EQ_INT(gk_voxel_set(&g, 100, 100, 100, 0),
                    GK_ERR_OUT_OF_RANGE);
    gk_voxel_destroy(&g);
}

static void test_voxel_segment(void)
{
    gk_voxel_grid g;
    size_t removed = 0;
    gk_voxel_init(&g, 20, 20, 20, 1.0, gk_vec3_make(0, 0, 0), NULL);
    /* Cut a channel along X at y=z=10.5 with radius 0.4 */
    gk_voxel_cut_segment(&g, gk_vec3_make(0.5, 10.5, 10.5),
                         gk_vec3_make(19.5, 10.5, 10.5), 0.4, &removed);
    GK_CHECK(removed >= 19);
    GK_CHECK_EQ_INT(gk_voxel_get(&g, 10, 10, 10), 0);
    GK_CHECK_EQ_INT(gk_voxel_get(&g, 10, 0, 0), 1);
    gk_voxel_destroy(&g);
}

int main(void)
{
    test_math();
    test_state();
    test_linear_move();
    test_arc_ijk_length();
    test_arc_radius();
    test_arc_interpolate();
    test_plan_trapezoid();
    test_plan_triangular();
    test_plan_axis_limit();
    test_axis_range();
    test_servo_first_order();
    test_servo_second_order();
    test_servo_backlash();
    test_voxel();
    test_voxel_segment();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
