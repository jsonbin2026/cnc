#include "gk_test.h"
#include "gk/gk_net2.h"

#include <math.h>

static void test_names(void)
{
    GK_CHECK_STR_EQ(gk_net2_name(GK_NET2_ETHERNET), "ethernet");
    GK_CHECK_STR_EQ(gk_net2_name(GK_NET2_SYNC), "data-sync");
    GK_CHECK_STR_EQ(gk_net2_name((gk_net2_kind)999), "unknown");
    GK_CHECK(gk_net2_typical_bandwidth(GK_NET2_USB) >
             gk_net2_typical_bandwidth(GK_NET2_SERIAL));
}

static void test_link(void)
{
    gk_net2_link l;
    GK_CHECK(gk_net2_connect(&l, GK_NET2_ETHERNET, "192.168.1.10") == GK_OK);
    GK_CHECK_EQ_INT(gk_net2_connected(&l), 1);
    GK_CHECK_STR_EQ(l.address, "192.168.1.10");
    GK_CHECK_EQ_INT(gk_net2_healthy(&l), 1);
    GK_CHECK(gk_net2_set_quality(&l, 5.0, 0.0) == GK_OK);
    GK_CHECK_EQ_INT(l.state, GK_NET2_UP);
    GK_CHECK(gk_net2_set_quality(&l, 300.0, 0.2) == GK_OK);
    GK_CHECK_EQ_INT(l.state, GK_NET2_DEGRADED);
    GK_CHECK_EQ_INT(gk_net2_healthy(&l), 0);
    GK_CHECK(gk_net2_set_quality(&l, 1.0, 2.0) == GK_ERR_OUT_OF_RANGE);
    GK_CHECK(gk_net2_disconnect(&l) == GK_OK);
    GK_CHECK_EQ_INT(gk_net2_connected(&l), 0);
}

static void test_send(void)
{
    gk_net2_link l;
    double sec = 0.0;
    gk_net2_connect(&l, GK_NET2_ETHERNET, "host");
    GK_CHECK(gk_net2_send(&l, 1000000, &sec) == GK_OK);
    GK_CHECK(sec > 0.0);
    gk_net2_disconnect(&l);
    GK_CHECK(gk_net2_send(&l, 1000, &sec) == GK_ERR_STATE);
}

static void test_vpn(void)
{
    gk_net2_link l;
    double before;
    gk_net2_connect(&l, GK_NET2_WIFI, "ap");
    before = l.latency_ms;
    GK_CHECK(gk_net2_vpn(&l, 1) == GK_OK);
    GK_CHECK(l.latency_ms > before);
    GK_CHECK(gk_net2_vpn(&l, 0) == GK_OK);
    GK_CHECK(fabs(l.latency_ms - before) < 1e-9);
    gk_net2_disconnect(&l);
    GK_CHECK(gk_net2_vpn(&l, 1) == GK_ERR_STATE);
}

int main(void)
{
    test_names();
    test_link();
    test_send();
    test_vpn();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
