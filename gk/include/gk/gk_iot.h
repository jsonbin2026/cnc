#ifndef GK_IOT_H
#define GK_IOT_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_IOT_MAX_TAGS 128
#define GK_IOT_TAG_NAME 64
#define GK_IOT_BUF 4096

/* ---- data bus (496) ---- */

typedef enum {
    GK_DTYPE_INT = 0,
    GK_DTYPE_DOUBLE,
    GK_DTYPE_BOOL,
    GK_DTYPE_STRING
} gk_data_type;

typedef struct {
    char name[GK_IOT_TAG_NAME];
    gk_data_type type;
    double dval;
    int ival;
    int bval;
    char sval[GK_IOT_TAG_NAME];
    double timestamp;
    int quality;
} gk_tag;

typedef struct {
    gk_tag tags[GK_IOT_MAX_TAGS];
    int count;
    double clock;
} gk_data_bus;

void gk_data_bus_init(gk_data_bus *b);
int gk_bus_tag_index(const gk_data_bus *b, const char *name);
gk_status gk_bus_add_double(gk_data_bus *b, const char *name, double value);
gk_status gk_bus_add_int(gk_data_bus *b, const char *name, int value);
gk_status gk_bus_add_bool(gk_data_bus *b, const char *name, int value);
gk_status gk_bus_set_double(gk_data_bus *b, const char *name, double value);
gk_status gk_bus_set_int(gk_data_bus *b, const char *name, int value);
double gk_bus_get_double(const gk_data_bus *b, const char *name);
int gk_bus_get_int(const gk_data_bus *b, const char *name);
int gk_bus_tag_count(const gk_data_bus *b);

/* ---- 497/498 OPC UA ---- */

typedef struct {
    int server_running;
    int port;
    char endpoint[128];
    int session_count;
    int subscription_count;
    double sampling_interval;
} gk_opcua_server;

typedef struct {
    char url[128];
    int connected;
    int read_count;
    int write_count;
} gk_opcua_client;

void gk_opcua_server_init(gk_opcua_server *s);
gk_status gk_opcua_server_start(gk_opcua_server *s, int port);
gk_status gk_opcua_server_stop(gk_opcua_server *s);
void gk_opcua_client_init(gk_opcua_client *c);
gk_status gk_opcua_connect(gk_opcua_client *c, const char *url);
gk_status gk_opcua_read(gk_opcua_client *c, const char *node, double *out);
gk_status gk_opcua_write(gk_opcua_client *c, const char *node, double value);

/* ---- 499 MTConnect ---- */

typedef struct {
    int enabled;
    char device[64];
    char agent_url[128];
    int heartbeat;
} gk_mtconnect;

void gk_mtconnect_init(gk_mtconnect *m);
int gk_mtconnect_probe(const gk_mtconnect *m, char *buf, size_t len);

/* ---- 500 MQTT ---- */

typedef struct {
    int connected;
    char broker[128];
    int port;
    char topic[128];
    int qos;
    int published;
} gk_mqtt;

void gk_mqtt_init(gk_mqtt *m);
gk_status gk_mqtt_connect(gk_mqtt *m, const char *broker, int port);
gk_status gk_mqtt_publish(gk_mqtt *m, const char *topic, const char *payload);

/* ---- 501/502 Modbus ---- */

typedef struct {
    int unit_id;
    int connected;
    int registers[64];
    int discrete[64];
    int is_rtu;
    int baud;
} gk_modbus;

void gk_modbus_init(gk_modbus *m, int unit_id);
gk_status gk_modbus_connect(gk_modbus *m);
gk_status gk_modbus_write_register(gk_modbus *m, int addr, int value);
int gk_modbus_read_register(const gk_modbus *m, int addr);
gk_status gk_modbus_write_coil(gk_modbus *m, int addr, int value);
int gk_modbus_read_coil(const gk_modbus *m, int addr);

/* ---- 503-505 fieldbus ---- */

typedef enum {
    GK_FIELDBUS_PROFINET = 0,   /* 503 */
    GK_FIELDBUS_ETHERCAT,       /* 504 */
    GK_FIELDBUS_ETHERNET_IP,    /* 505 */
    GK_FIELDBUS_COUNT
} gk_fieldbus_kind;

const char *gk_fieldbus_name(gk_fieldbus_kind k);

typedef struct {
    gk_fieldbus_kind kind;
    int connected;
    int slaves;
    double cycle_time_us;
    int lost_frames;
} gk_fieldbus;

gk_status gk_fieldbus_init(gk_fieldbus *f, gk_fieldbus_kind kind, int slaves);
gk_status gk_fieldbus_start(gk_fieldbus *f);
void gk_fieldbus_tick(gk_fieldbus *f, double elapsed);

/* ---- 506 REST / 507 WebSocket / 508 gRPC / 509 GraphQL ---- */

typedef enum {
    GK_API_REST = 0,
    GK_API_WEBSOCKET,
    GK_API_GRPC,
    GK_API_GRAPHQL,
    GK_API_COUNT
} gk_api_kind;

const char *gk_api_kind_name(gk_api_kind k);

typedef struct {
    gk_api_kind kind;
    int running;
    int port;
    int requests;
    int errors;
} gk_api_service;

gk_status gk_api_init(gk_api_service *s, gk_api_kind kind, int port);
gk_status gk_api_start(gk_api_service *s);
gk_status gk_api_stop(gk_api_service *s);
/* record an incoming request with a path and method correctness. */
gk_status gk_api_request(gk_api_service *s, const char *path, int valid);
int gk_api_request_count(const gk_api_service *s);

/* ---- 510 digital twin mapping ---- */

typedef struct {
    char physical[GK_IOT_TAG_NAME];
    char virtual_tag[GK_IOT_TAG_NAME];
    double scale;
    double offset;
} gk_twin_map;

typedef struct {
    gk_twin_map maps[GK_IOT_MAX_TAGS];
    int count;
    int synced;
} gk_digital_twin;

void gk_digital_twin_init(gk_digital_twin *t);
gk_status gk_twin_map_add(gk_digital_twin *t, const char *physical,
                          const char *virt, double scale, double offset);
double gk_twin_translate(const gk_digital_twin *t, const char *physical,
                         double value);
int gk_twin_sync(gk_digital_twin *t, gk_data_bus *bus, double physical_value);

/* ---- 511/512 time series DB (InfluxDB-like) ---- */

typedef struct {
    char tag[GK_IOT_TAG_NAME];
    double time;
    double value;
} gk_ts_point;

typedef struct {
    gk_ts_point points[1024];
    int count;
    char dbname[64];
    int dropped;
} gk_tsdb;

void gk_tsdb_init(gk_tsdb *db, const char *name);
gk_status gk_tsdb_write(gk_tsdb *db, const char *tag, double time,
                        double value);
int gk_tsdb_query(const gk_tsdb *db, const char *tag, double t0, double t1,
                  double *out, int max_out);
double gk_tsdb_mean(const gk_tsdb *db, const char *tag);

/* ---- 513 acquisition / 514 playback / 515 visualization ---- */

typedef struct {
    gk_data_bus *bus;
    int collecting;
    double interval;
    double accumulator;
    int samples;
    double started_at;
} gk_acquisition;

void gk_acquisition_init(gk_acquisition *a, gk_data_bus *bus, double interval);
gk_status gk_acquisition_start(gk_acquisition *a, double now);
gk_status gk_acquisition_stop(gk_acquisition *a);
int gk_acquisition_tick(gk_acquisition *a, double dt);

typedef struct {
    gk_tsdb *db;
    int playing;
    double cursor;
    double speed;
    int index;
} gk_playback;

void gk_playback_init(gk_playback *p, gk_tsdb *db);
gk_status gk_playback_start(gk_playback *p, double speed);
gk_status gk_playback_stop(gk_playback *p);
/* returns 1 when a point was emitted */
int gk_playback_step(gk_playback *p, double dt, gk_ts_point *out);

/* render collected data as a text chart */
int gk_tsviz_render(const gk_tsdb *db, const char *tag, int width, int height,
                    char *buf, size_t len);

/* ---- transport helpers shared by the protocol modules ---- */

const char *gk_iot_protocol_name(int protocol_id);

#ifdef __cplusplus
}
#endif

#endif /* GK_IOT_H */
