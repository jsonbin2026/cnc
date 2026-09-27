#include "gk_test.h"

#include "gk/gk_macro.h"

#include <math.h>
#include <string.h>

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

static void test_var_space(void)
{
    gk_macro_vars v;
    double out = 0.0;
    gk_macro_vars_init(&v);

    GK_CHECK(gk_macro_index_is_valid(1));
    GK_CHECK(gk_macro_index_is_valid(33));
    GK_CHECK(gk_macro_index_is_valid(100));
    GK_CHECK(gk_macro_index_is_valid(199));
    GK_CHECK(gk_macro_index_is_valid(1000));
    GK_CHECK(!gk_macro_index_is_valid(0));
    GK_CHECK(!gk_macro_index_is_valid(34));
    GK_CHECK(!gk_macro_index_is_valid(99));
    GK_CHECK(!gk_macro_index_is_valid(200));
    GK_CHECK(!gk_macro_index_is_valid(1064));

    GK_CHECK_EQ_INT(gk_macro_set(&v, 1, 42.5), GK_OK);
    GK_CHECK_EQ_INT(gk_macro_get(&v, 1, &out), GK_OK);
    GK_CHECK(near(out, 42.5, 1e-12));

    GK_CHECK_EQ_INT(gk_macro_set(&v, 150, -3.0), GK_OK);
    GK_CHECK_EQ_INT(gk_macro_get(&v, 150, &out), GK_OK);
    GK_CHECK(near(out, -3.0, 1e-12));

    GK_CHECK_EQ_INT(gk_macro_set(&v, 1000, 7.0), GK_OK);
    GK_CHECK_EQ_INT(gk_macro_get(&v, 1000, &out), GK_OK);
    GK_CHECK(near(out, 7.0, 1e-12));

    /* local and common spaces are disjoint */
    GK_CHECK_EQ_INT(gk_macro_get(&v, 100, &out), GK_OK);
    GK_CHECK(near(out, 0.0, 1e-12));

    GK_CHECK_EQ_INT(gk_macro_set(&v, 34, 1.0), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_macro_get(&v, 34, &out), GK_ERR_INVALID_ARG);
    GK_CHECK_EQ_INT(gk_macro_set(NULL, 1, 1.0), GK_ERR_INVALID_ARG);
}

static void test_arith(void)
{
    gk_macro_vars v;
    double out = 0.0;
    gk_macro_vars_init(&v);

    GK_CHECK_EQ_INT(gk_macro_eval(&v, "1+2*3", &out), GK_OK);
    GK_CHECK(near(out, 7.0, 1e-12));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "(1+2)*3", &out), GK_OK);
    GK_CHECK(near(out, 9.0, 1e-12));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "[1+2]*3", &out), GK_OK);
    GK_CHECK(near(out, 9.0, 1e-12));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "10/4", &out), GK_OK);
    GK_CHECK(near(out, 2.5, 1e-12));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "-5+2", &out), GK_OK);
    GK_CHECK(near(out, -3.0, 1e-12));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "10 MOD 3", &out), GK_OK);
    GK_CHECK(near(out, 1.0, 1e-12));

    /* errors */
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "1/0", &out), GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "1+", &out), GK_ERR_PARSE);
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "(1+2", &out), GK_ERR_PARSE);
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "1 2", &out), GK_ERR_PARSE);
    GK_CHECK_EQ_INT(gk_macro_eval(&v, NULL, &out), GK_ERR_INVALID_ARG);
}

static void test_functions(void)
{
    gk_macro_vars v;
    double out = 0.0;
    double pi = 3.14159265358979323846;
    gk_macro_vars_init(&v);

    GK_CHECK_EQ_INT(gk_macro_eval(&v, "ABS[-3.5]", &out), GK_OK);
    GK_CHECK(near(out, 3.5, 1e-12));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "SQRT[16]", &out), GK_OK);
    GK_CHECK(near(out, 4.0, 1e-12));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "SIN[30]", &out), GK_OK);
    GK_CHECK(near(out, 0.5, 1e-9));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "COS[60]", &out), GK_OK);
    GK_CHECK(near(out, 0.5, 1e-9));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "TAN[45]", &out), GK_OK);
    GK_CHECK(near(out, 1.0, 1e-9));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "ASIN[1]", &out), GK_OK);
    GK_CHECK(near(out, 90.0, 1e-9));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "ACOS[0]", &out), GK_OK);
    GK_CHECK(near(out, 90.0, 1e-9));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "ATAN[1]", &out), GK_OK);
    GK_CHECK(near(out, 45.0, 1e-9));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "ATAN[1]", &out), GK_OK);
    GK_CHECK(near(out, 45.0, 1e-9));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "ROUND[2.6]", &out), GK_OK);
    GK_CHECK(near(out, 3.0, 1e-12));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "FIX[2.9]", &out), GK_OK);
    GK_CHECK(near(out, 2.0, 1e-12));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "FUP[2.1]", &out), GK_OK);
    GK_CHECK(near(out, 3.0, 1e-12));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "LN[1]", &out), GK_OK);
    GK_CHECK(near(out, 0.0, 1e-12));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "EXP[0]", &out), GK_OK);
    GK_CHECK(near(out, 1.0, 1e-12));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "PI", &out), GK_OK);
    GK_CHECK(near(out, pi, 1e-9));

    /* function errors */
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "SQRT[-1]", &out), GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "LN[0]", &out), GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "FOO[1]", &out), GK_ERR_NOT_FOUND);
}

static void test_var_expr(void)
{
    gk_macro_vars v;
    double out = 0.0;
    gk_macro_vars_init(&v);
    gk_macro_set(&v, 1, 10.0);
    gk_macro_set(&v, 2, 4.0);
    gk_macro_set(&v, 100, 100.0);

    GK_CHECK_EQ_INT(gk_macro_eval(&v, "#1+#2", &out), GK_OK);
    GK_CHECK(near(out, 14.0, 1e-12));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "#1*#2", &out), GK_OK);
    GK_CHECK(near(out, 40.0, 1e-12));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "SQRT[#1+#2]", &out), GK_OK);
    GK_CHECK(near(out, sqrt(14.0), 1e-12));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "#[1+1]", &out), GK_OK);
    GK_CHECK(near(out, 4.0, 1e-12));
    GK_CHECK_EQ_INT(gk_macro_eval(&v, "#200", &out), GK_ERR_OUT_OF_RANGE);
}

static void test_assign(void)
{
    gk_macro_vars v;
    double out = 0.0;
    gk_macro_vars_init(&v);

    GK_CHECK_EQ_INT(gk_macro_assign(&v, "#1=5+5"), GK_OK);
    GK_CHECK_EQ_INT(gk_macro_get(&v, 1, &out), GK_OK);
    GK_CHECK(near(out, 10.0, 1e-12));

    GK_CHECK_EQ_INT(gk_macro_assign(&v, "#2= #1*2"), GK_OK);
    gk_macro_get(&v, 2, &out);
    GK_CHECK(near(out, 20.0, 1e-12));

    GK_CHECK_EQ_INT(gk_macro_assign(&v, "#3=#1+#2"), GK_OK);
    gk_macro_get(&v, 3, &out);
    GK_CHECK(near(out, 30.0, 1e-12));

    GK_CHECK_EQ_INT(gk_macro_assign(&v, "5"), GK_ERR_PARSE);
    GK_CHECK_EQ_INT(gk_macro_assign(&v, "#1+2"), GK_ERR_PARSE);
    GK_CHECK_EQ_INT(gk_macro_assign(&v, "#x=1"), GK_ERR_PARSE);
    GK_CHECK_EQ_INT(gk_macro_assign(NULL, "#1=1"), GK_ERR_INVALID_ARG);
}

static void test_func_registry(void)
{
    int n = gk_macro_func_count();
    GK_CHECK(n >= 13);
    GK_CHECK_STR_EQ(gk_macro_func_name(0), "ABS");
    GK_CHECK(gk_macro_func_name(n) == NULL);
    GK_CHECK(gk_macro_func_name(-1) == NULL);
}

int main(void)
{
    test_var_space();
    test_arith();
    test_functions();
    test_var_expr();
    test_assign();
    test_func_registry();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
