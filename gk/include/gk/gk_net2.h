#ifndef GK_NET2_H
#define GK_NET2_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_NET2_ADDR 64
#define GK_NET2_NAME 48

/* ===================================================================
 * Batch 49: real networking and communication (1312-1330)
 * Prefix: gk_net2_
 * =================================================================== */

typedef enum {
    GK_NET2_ETHERNET = 0,  /* 1312 */
    GK_NET2_SERIAL,        /* 1313 */
    GK_NET2_USB,           /* 1314 */
    GK_NET2_WIRELESS,      /* 1315 */
    GK_NET2_BLUETOOTH,     /* 1316 */
    GK_NET2_WIFI,          /* 1317 */
    GK_NET2_CELLULAR,      /* 1318 4G/5G */
    GK_NET2_IND_ETHERNET,  /* 1319 */
    GK_NET2_FIELDBUS,      /* 1320 */
    GK_NET2_DNC,           /* 1321 */
    GK_NET2_MES,           /* 1322 */
    GK_NET2_ERP,           /* 1323 */
    GK_NET2_CLOUD,         /* 1324 */
    GK_NET2_EDGE,          /* 1325 */
    GK_NET2_REMOTE_DIAG,   /* 1326 */
    GK_NET2_REMOTE_UPDATE, /* 1327 */
    GK_NET2_REMOTE_MONITOR,/* 1328 */
    GK_NET2_REMOTE_OP,     /* 1329 */
    GK_NET2_SYNC          /* 1330 */
} gk_net2_kind;

const char *gk_net2_name(gk_net2_kind k);
double gk_net2_typical_bandwidth(gk_net2_kind k); /* Mbps */

typedef enum {
    GK_NET2_DOWN = 0,
    GK_NET2_UP,
    GK_NET2_DEGRADED
} gk_net2_state;

const char *gk_net2_state_name(gk_net2_state s);

typedef struct {
    gk_net2_kind kind;
    gk_net2_state state;
    char address[GK_NET2_ADDR];
    double latency_ms;
    double packet_loss;
} gk_net2_link;

gk_status gk_net2_connect(gk_net2_link *l, gk_net2_kind k, const char *address);
gk_status gk_net2_disconnect(gk_net2_link *l);
int gk_net2_connected(const gk_net2_link *l);
gk_status gk_net2_set_quality(gk_net2_link *l, double latency_ms,
                              double loss);
int gk_net2_healthy(const gk_net2_link *l);

/* payload transfer with quantization/limits */
gk_status gk_net2_send(gk_net2_link *l, size_t bytes, double *seconds_out);
gk_status gk_net2_vpn(gk_net2_link *l, int enabled);

#ifdef __cplusplus
}
#endif

#endif /* GK_NET2_H */
