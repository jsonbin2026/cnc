#include "gk_test.h"
#include "gk/gk_staff.h"

#include <math.h>

static void test_roles_perms(void)
{
    gk_staff_perms p;
    GK_CHECK_STR_EQ(gk_staff_role_name(GK_STAFF_OPERATOR), "operator");
    GK_CHECK(gk_staff_role_level(GK_STAFF_PROD_MANAGER) >
             gk_staff_role_level(GK_STAFF_OPERATOR));
    gk_staff_perms_for_role(GK_STAFF_PROGRAMMER, &p);
    GK_CHECK_EQ_INT(gk_staff_can(&p, "edit-program"), 1);
    GK_CHECK_EQ_INT(gk_staff_can(&p, "manage-users"), 0);
    GK_CHECK_EQ_INT(gk_staff_can(&p, "start-machine"), 1);
    gk_staff_perms_for_role(GK_STAFF_PROD_MANAGER, &p);
    GK_CHECK_EQ_INT(gk_staff_can(&p, "manage-users"), 1);
    GK_CHECK_EQ_INT(gk_staff_can(&p, "unknown"), 0);
}

static void test_member(void)
{
    gk_staff_member m;
    gk_staff_init(&m, "Alice", GK_STAFF_OPERATOR);
    GK_CHECK_STR_EQ(m.name, "Alice");
    GK_CHECK_EQ_INT(gk_staff_skill_max(&m), 0);
    GK_CHECK(gk_staff_add_skill(&m, 3) == GK_OK);
    GK_CHECK(gk_staff_add_skill(&m, 7) == GK_OK);
    GK_CHECK_EQ_INT(gk_staff_skill_max(&m), 7);
    GK_CHECK(gk_staff_add_skill(&m, 99) == GK_ERR_OUT_OF_RANGE);
}

static void test_shift_attendance(void)
{
    gk_staff_shift s;
    gk_staff_attendance a;
    gk_staff_shift_init(&s, 1);
    GK_CHECK(gk_staff_shift_assign(&s, "Bob") == GK_OK);
    GK_CHECK(gk_staff_shift_assign(&s, "Carol") == GK_OK);
    GK_CHECK_EQ_INT(gk_staff_shift_size(&s), 2);
    gk_staff_attendance_init(&a);
    GK_CHECK(gk_staff_attendance_mark(&a, 1) == GK_OK);
    GK_CHECK(gk_staff_attendance_mark(&a, 1) == GK_OK);
    GK_CHECK(gk_staff_attendance_mark(&a, 0) == GK_OK);
    GK_CHECK(fabs(gk_staff_attendance_rate(&a) - 2.0 / 3.0) < 1e-9);
}

static void test_performance_training(void)
{
    gk_staff_performance p;
    gk_staff_training t;
    gk_staff_performance_init(&p);
    p.quality_score = 1.0;
    p.efficiency = 1.0;
    p.attendance = 1.0;
    GK_CHECK(fabs(gk_staff_performance_score(&p) - 1.0) < 1e-9);
    gk_staff_training_init(&t, 20);
    GK_CHECK_EQ_INT(gk_staff_training_done(&t), 0);
    GK_CHECK(gk_staff_training_complete(&t, 15) == GK_OK);
    GK_CHECK_EQ_INT(gk_staff_training_done(&t), 0);
    GK_CHECK(gk_staff_training_complete(&t, 5) == GK_OK);
    GK_CHECK_EQ_INT(gk_staff_training_done(&t), 1);
}

static void test_matrix_dispatch(void)
{
    gk_staff_matrix m;
    int skills[3] = {3, 8, 5};
    int req[3] = {4, 6, 9};
    int assignment[3];
    gk_staff_matrix_init(&m, 3, 2);
    GK_CHECK(gk_staff_matrix_set(&m, 0, 0, 5) == GK_OK);
    GK_CHECK(gk_staff_matrix_set(&m, 1, 0, 9) == GK_OK);
    GK_CHECK(gk_staff_matrix_set(&m, 2, 1, 7) == GK_OK);
    GK_CHECK_EQ_INT(gk_staff_matrix_get(&m, 1, 0), 9);
    GK_CHECK_EQ_INT(gk_staff_matrix_row_sum(&m, 1), 9);
    GK_CHECK_EQ_INT(gk_staff_matrix_best_for(&m, 0), 1);
    GK_CHECK_EQ_INT(gk_staff_matrix_best_for(&m, 1), 2);
    GK_CHECK(gk_staff_matrix_set(&m, 9, 0, 1) == GK_ERR_OUT_OF_RANGE);
    GK_CHECK(gk_staff_dispatch(skills, 3, req, assignment) == GK_OK);
    GK_CHECK_EQ_INT(assignment[0], -1);
    GK_CHECK_EQ_INT(assignment[1], 1);
    GK_CHECK_EQ_INT(assignment[2], -1);
}

int main(void)
{
    test_roles_perms();
    test_member();
    test_shift_attendance();
    test_performance_training();
    test_matrix_dispatch();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
