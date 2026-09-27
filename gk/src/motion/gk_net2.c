#include "gk/gk_net2.h"

#include <math.h>
#include <string.h>

static void gk__net2_copy(char *dst, size_t cap, const char *src)
{
    size_t n;
    if (dst == NULL || cap == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    n = strlen(src);
    if (n >= cap) {
        n = cap - 1;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
}

const char *gk_net2_name(gk_net2_kind k)
{
    switch (k) {
    case GK_NET2_ETHERNET: return "ethernet";
    case GK_NET2_SERIAL: return "serial";
    case GK_NET2_USB: return "usb";
    case GK_NET2_WIRELESS: return "wireless";
    case GK_NET2_BLUETOOTH: return "bluetooth";
    case GK_NET2_WIFI: return "wifi";
    case GK_NET2_CELLULAR: return "4g5g";
    case GK_NET2_IND_ETHERNET: return "industrial-ethernet";
    case GK_NET2_FIELDBUS: return "fieldbus";
    case GK_NET2_DNC: return "dnc";
    case GK_NET2_MES: return "mes";
    case GK_NET2_ERP: return "erp";
    case GK_NET2_CLOUD: return "cloud";
    case GK_NET2_EDGE: return "edge";
    case GK_NET2_REMOTE_DIAG: return "remote-diagnosis";
    case GK_NET2_REMOTE_UPDATE: return "remote-update";
    case GK_NET2_REMOTE_MONITOR: return "remote-monitor";
    case GK_NET2_REMOTE_OP: return "remote-operation";
    case GK_NET2_SYNC: return "data-sync";
    default: return "unknown";
    }
}

double gk_net2_typical_bandwidth(gk_net2_kind k)
{
    switch (k) {
    case GK_NET2_SERIAL: return 0.115;
    case GK_NET2_USB: return 480.0;
    case GK_NET2_BLUETOOTH: return 3.0;
    case GK_NET2_WIFI: return 300.0;
    case GK_NET2_CELLULAR: return 100.0;
    case GK_NET2_FIELDBUS: return 12.0;
    case GK_NET2_ETHERNET:
    case GK_NET2_IND_ETHERNET:
    case GK_NET2_DNC:
    case GK_NET2_MES:
    case GK_NET2_ERP:
    case GK_NET2_CLOUD:
    case GK_NET2_EDGE:
    case GK_NET2_REMOTE_DIAG:
    case GK_NET2_REMOTE_UPDATE:
    case GK_NET2_REMOTE_MONITOR:
    case GK_NET2_REMOTE_OP:
    case GK_NET2_SYNC:
    case GK_NET2_WIRELESS:
        return 1000.0;
    default:
        return 0.0;
    }
}

const char *gk_net2_state_name(gk_net2_state s)
{
    switch (s) {
    case GK_NET2_DOWN: return "down";
    case GK_NET2_UP: return "up";
    case GK_NET2_DEGRADED: return "degraded";
    default: return "unknown";
    }
}

gk_status gk_net2_connect(gk_net2_link *l, gk_net2_kind k, const char *address)
{
    if (l == NULL || address == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    memset(l, 0, sizeof(*l));
    l->kind = k;
    l->state = GK_NET2_UP;
    l->latency_ms = 1.0;
    gk__net2_copy(l->address, sizeof(l->address), address);
    return GK_OK;
}

gk_status gk_net2_disconnect(gk_net2_link *l)
{
    if (l == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    l->state = GK_NET2_DOWN;
    return GK_OK;
}

int gk_net2_connected(const gk_net2_link *l)
{
    if (l == NULL) {
        return 0;
    }
    return l->state != GK_NET2_DOWN;
}

gk_status gk_net2_set_quality(gk_net2_link *l, double latency_ms,
                              double loss)
{
    if (l == NULL || latency_ms < 0.0 || loss < 0.0 || loss > 1.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    l->latency_ms = latency_ms;
    l->packet_loss = loss;
    if (l->state != GK_NET2_DOWN) {
        if (loss > 0.1 || latency_ms > 200.0) {
            l->state = GK_NET2_DEGRADED;
        } else {
            l->state = GK_NET2_UP;
        }
    }
    return GK_OK;
}

int gk_net2_healthy(const gk_net2_link *l)
{
    if (l == NULL) {
        return 0;
    }
    return l->state == GK_NET2_UP && l->packet_loss <= 0.01 &&
           l->latency_ms <= 50.0;
}

gk_status gk_net2_send(gk_net2_link *l, size_t bytes, double *seconds_out)
{
    double bw_mbps, bits, eff;
    if (l == NULL || seconds_out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (l->state == GK_NET2_DOWN) {
        return GK_ERR_STATE;
    }
    bw_mbps = gk_net2_typical_bandwidth(l->kind);
    if (bw_mbps <= 0.0) {
        return GK_ERR_UNSUPPORTED;
    }
    /* apply loss as effective goodput reduction */
    eff = bw_mbps * (1.0 - l->packet_loss);
    if (eff <= 0.0) {
        return GK_ERR_STATE;
    }
    bits = (double)bytes * 8.0;
    *seconds_out = bits / (eff * 1e6) + l->latency_ms / 1000.0;
    return GK_OK;
}

gk_status gk_net2_vpn(gk_net2_link *l, int enabled)
{
    if (l == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (l->state == GK_NET2_DOWN) {
        return GK_ERR_STATE;
    }
    /* VPN adds fixed overhead latency */
    l->latency_ms += enabled ? 5.0 : -5.0;
    if (l->latency_ms < 0.0) {
        l->latency_ms = 0.0;
    }
    return GK_OK;
}
