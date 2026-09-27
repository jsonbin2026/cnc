#include "gk/gk_iot.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static void gk__copy(char *dst, size_t len, const char *src)
{
    size_t i;
    if (dst == NULL || len == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    for (i = 0; i + 1 < len && src[i] != '\0'; ++i) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

/* ---- data bus ---- */

void gk_data_bus_init(gk_data_bus *b)
{
    if (b != NULL) {
        memset(b, 0, sizeof(*b));
    }
}

int gk_bus_tag_index(const gk_data_bus *b, const char *name)
{
    int i;
    if (b == NULL || name == NULL) {
        return -1;
    }
    for (i = 0; i < b->count; ++i) {
        if (strcmp(b->tags[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static gk_tag *gk__add_tag(gk_data_bus *b, const char *name, gk_data_type t)
{
    gk_tag *tag;
    if (b == NULL || name == NULL || b->count >= GK_IOT_MAX_TAGS) {
        return NULL;
    }
    tag = &b->tags[b->count++];
    memset(tag, 0, sizeof(*tag));
    gk__copy(tag->name, sizeof(tag->name), name);
    tag->type = t;
    tag->quality = 1;
    tag->timestamp = b->clock;
    return tag;
}

gk_status gk_bus_add_double(gk_data_bus *b, const char *name, double value)
{
    gk_tag *t = gk__add_tag(b, name, GK_DTYPE_DOUBLE);
    if (t == NULL) {
        return GK_ERR_OUT_OF_RANGE;
    }
    t->dval = value;
    return GK_OK;
}

gk_status gk_bus_add_int(gk_data_bus *b, const char *name, int value)
{
    gk_tag *t = gk__add_tag(b, name, GK_DTYPE_INT);
    if (t == NULL) {
        return GK_ERR_OUT_OF_RANGE;
    }
    t->ival = value;
    return GK_OK;
}

gk_status gk_bus_add_bool(gk_data_bus *b, const char *name, int value)
{
    gk_tag *t = gk__add_tag(b, name, GK_DTYPE_BOOL);
    if (t == NULL) {
        return GK_ERR_OUT_OF_RANGE;
    }
    t->bval = value ? 1 : 0;
    return GK_OK;
}

gk_status gk_bus_set_double(gk_data_bus *b, const char *name, double value)
{
    int i = gk_bus_tag_index(b, name);
    if (i < 0) {
        return GK_ERR_NOT_FOUND;
    }
    b->tags[i].dval = value;
    b->tags[i].timestamp = b->clock;
    return GK_OK;
}

gk_status gk_bus_set_int(gk_data_bus *b, const char *name, int value)
{
    int i = gk_bus_tag_index(b, name);
    if (i < 0) {
        return GK_ERR_NOT_FOUND;
    }
    b->tags[i].ival = value;
    b->tags[i].timestamp = b->clock;
    return GK_OK;
}

double gk_bus_get_double(const gk_data_bus *b, const char *name)
{
    int i = gk_bus_tag_index(b, name);
    if (i < 0) {
        return 0.0;
    }
    return b->tags[i].type == GK_DTYPE_DOUBLE ? b->tags[i].dval
                                              : (double)b->tags[i].ival;
}

int gk_bus_get_int(const gk_data_bus *b, const char *name)
{
    int i = gk_bus_tag_index(b, name);
    if (i < 0) {
        return 0;
    }
    return b->tags[i].ival;
}

int gk_bus_tag_count(const gk_data_bus *b)
{
    return b != NULL ? b->count : 0;
}

/* ---- OPC UA ---- */

void gk_opcua_server_init(gk_opcua_server *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->port = 4840;
    s->sampling_interval = 0.1;
}

gk_status gk_opcua_server_start(gk_opcua_server *s, int port)
{
    if (s == NULL || port <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    s->port = port;
    s->server_running = 1;
    snprintf(s->endpoint, sizeof(s->endpoint), "opc.tcp://0.0.0.0:%d", port);
    return GK_OK;
}

gk_status gk_opcua_server_stop(gk_opcua_server *s)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    s->server_running = 0;
    s->session_count = 0;
    return GK_OK;
}

void gk_opcua_client_init(gk_opcua_client *c)
{
    if (c != NULL) {
        memset(c, 0, sizeof(*c));
    }
}

gk_status gk_opcua_connect(gk_opcua_client *c, const char *url)
{
    if (c == NULL || url == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    gk__copy(c->url, sizeof(c->url), url);
    c->connected = 1;
    return GK_OK;
}

gk_status gk_opcua_read(gk_opcua_client *c, const char *node, double *out)
{
    if (c == NULL || node == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!c->connected) {
        return GK_ERR_STATE;
    }
    c->read_count++;
    *out = (double)strlen(node);
    return GK_OK;
}

gk_status gk_opcua_write(gk_opcua_client *c, const char *node, double value)
{
    (void)value;
    if (c == NULL || node == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!c->connected) {
        return GK_ERR_STATE;
    }
    c->write_count++;
    return GK_OK;
}

/* ---- MTConnect ---- */

void gk_mtconnect_init(gk_mtconnect *m)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    gk__copy(m->device, sizeof(m->device), "GK-CNC");
    gk__copy(m->agent_url, sizeof(m->agent_url), "http://localhost:5000");
    m->heartbeat = 5000;
    m->enabled = 1;
}

int gk_mtconnect_probe(const gk_mtconnect *m, char *buf, size_t len)
{
    if (m == NULL || buf == NULL) {
        return 0;
    }
    return snprintf(buf, len, "MTConnect|device=%s|agent=%s|hb=%d",
                    m->device, m->agent_url, m->heartbeat);
}

/* ---- MQTT ---- */

void gk_mqtt_init(gk_mqtt *m)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    m->port = 1883;
    m->qos = 0;
}

gk_status gk_mqtt_connect(gk_mqtt *m, const char *broker, int port)
{
    if (m == NULL || broker == NULL || port <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    gk__copy(m->broker, sizeof(m->broker), broker);
    m->port = port;
    m->connected = 1;
    return GK_OK;
}

gk_status gk_mqtt_publish(gk_mqtt *m, const char *topic, const char *payload)
{
    if (m == NULL || topic == NULL || payload == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!m->connected) {
        return GK_ERR_STATE;
    }
    gk__copy(m->topic, sizeof(m->topic), topic);
    m->published++;
    return GK_OK;
}

/* ---- Modbus ---- */

void gk_modbus_init(gk_modbus *m, int unit_id)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    m->unit_id = unit_id;
    m->baud = 9600;
}

gk_status gk_modbus_connect(gk_modbus *m)
{
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    m->connected = 1;
    return GK_OK;
}

gk_status gk_modbus_write_register(gk_modbus *m, int addr, int value)
{
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!m->connected) {
        return GK_ERR_STATE;
    }
    if (addr < 0 || addr >= 64) {
        return GK_ERR_OUT_OF_RANGE;
    }
    m->registers[addr] = value;
    return GK_OK;
}

int gk_modbus_read_register(const gk_modbus *m, int addr)
{
    if (m == NULL || addr < 0 || addr >= 64) {
        return -1;
    }
    return m->registers[addr];
}

gk_status gk_modbus_write_coil(gk_modbus *m, int addr, int value)
{
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!m->connected) {
        return GK_ERR_STATE;
    }
    if (addr < 0 || addr >= 64) {
        return GK_ERR_OUT_OF_RANGE;
    }
    m->discrete[addr] = value ? 1 : 0;
    return GK_OK;
}

int gk_modbus_read_coil(const gk_modbus *m, int addr)
{
    if (m == NULL || addr < 0 || addr >= 64) {
        return -1;
    }
    return m->discrete[addr];
}

/* ---- fieldbus ---- */

const char *gk_fieldbus_name(gk_fieldbus_kind k)
{
    switch (k) {
    case GK_FIELDBUS_PROFINET: return "PROFINET";
    case GK_FIELDBUS_ETHERCAT: return "EtherCAT";
    case GK_FIELDBUS_ETHERNET_IP: return "EtherNet/IP";
    default: return "unknown";
    }
}

gk_status gk_fieldbus_init(gk_fieldbus *f, gk_fieldbus_kind kind, int slaves)
{
    if (f == NULL || kind < 0 || kind >= GK_FIELDBUS_COUNT || slaves <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    memset(f, 0, sizeof(*f));
    f->kind = kind;
    f->slaves = slaves;
    f->cycle_time_us = (double)kind * 100.0 + 500.0;
    return GK_OK;
}

gk_status gk_fieldbus_start(gk_fieldbus *f)
{
    if (f == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    f->connected = 1;
    return GK_OK;
}

void gk_fieldbus_tick(gk_fieldbus *f, double elapsed)
{
    if (f == NULL || !f->connected || f->cycle_time_us <= 0.0) {
        return;
    }
    /* occasional frame loss in the simulator */
    if (elapsed > f->slaves * f->cycle_time_us / 1e6) {
        f->lost_frames++;
    }
}

/* ---- API services ---- */

const char *gk_api_kind_name(gk_api_kind k)
{
    switch (k) {
    case GK_API_REST: return "REST";
    case GK_API_WEBSOCKET: return "WebSocket";
    case GK_API_GRPC: return "gRPC";
    case GK_API_GRAPHQL: return "GraphQL";
    default: return "unknown";
    }
}

gk_status gk_api_init(gk_api_service *s, gk_api_kind kind, int port)
{
    if (s == NULL || kind < 0 || kind >= GK_API_COUNT || port <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    memset(s, 0, sizeof(*s));
    s->kind = kind;
    s->port = port;
    return GK_OK;
}

gk_status gk_api_start(gk_api_service *s)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    s->running = 1;
    return GK_OK;
}

gk_status gk_api_stop(gk_api_service *s)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    s->running = 0;
    return GK_OK;
}

gk_status gk_api_request(gk_api_service *s, const char *path, int valid)
{
    if (s == NULL || path == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!s->running) {
        return GK_ERR_STATE;
    }
    s->requests++;
    if (!valid) {
        s->errors++;
        return GK_ERR_PARSE;
    }
    return GK_OK;
}

int gk_api_request_count(const gk_api_service *s)
{
    return s != NULL ? s->requests : 0;
}

/* ---- digital twin ---- */

void gk_digital_twin_init(gk_digital_twin *t)
{
    if (t != NULL) {
        memset(t, 0, sizeof(*t));
    }
}

gk_status gk_twin_map_add(gk_digital_twin *t, const char *physical,
                          const char *virt, double scale, double offset)
{
    gk_twin_map *m;
    if (t == NULL || physical == NULL || virt == NULL ||
        t->count >= GK_IOT_MAX_TAGS) {
        return GK_ERR_INVALID_ARG;
    }
    m = &t->maps[t->count++];
    memset(m, 0, sizeof(*m));
    gk__copy(m->physical, sizeof(m->physical), physical);
    gk__copy(m->virtual_tag, sizeof(m->virtual_tag), virt);
    m->scale = scale;
    m->offset = offset;
    return GK_OK;
}

double gk_twin_translate(const gk_digital_twin *t, const char *physical,
                         double value)
{
    int i;
    if (t == NULL) {
        return value;
    }
    for (i = 0; i < t->count; ++i) {
        if (strcmp(t->maps[i].physical, physical) == 0) {
            return value * t->maps[i].scale + t->maps[i].offset;
        }
    }
    return value;
}

int gk_twin_sync(gk_digital_twin *t, gk_data_bus *bus, double physical_value)
{
    int i;
    int synced = 0;
    if (t == NULL || bus == NULL) {
        return 0;
    }
    for (i = 0; i < t->count; ++i) {
        double v = gk_twin_translate(t, t->maps[i].physical, physical_value);
        if (gk_bus_set_double(bus, t->maps[i].virtual_tag, v) == GK_OK) {
            synced++;
        }
    }
    t->synced = synced;
    return synced;
}

/* ---- time series DB ---- */

void gk_tsdb_init(gk_tsdb *db, const char *name)
{
    if (db == NULL) {
        return;
    }
    memset(db, 0, sizeof(*db));
    gk__copy(db->dbname, sizeof(db->dbname), name);
}

gk_status gk_tsdb_write(gk_tsdb *db, const char *tag, double time,
                        double value)
{
    gk_ts_point *p;
    if (db == NULL || tag == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (db->count >= 1024) {
        db->dropped++;
        return GK_ERR_OUT_OF_RANGE;
    }
    p = &db->points[db->count++];
    gk__copy(p->tag, sizeof(p->tag), tag);
    p->time = time;
    p->value = value;
    return GK_OK;
}

int gk_tsdb_query(const gk_tsdb *db, const char *tag, double t0, double t1,
                  double *out, int max_out)
{
    int i;
    int n = 0;
    if (db == NULL || tag == NULL || out == NULL || max_out <= 0) {
        return 0;
    }
    for (i = 0; i < db->count && n < max_out; ++i) {
        if (strcmp(db->points[i].tag, tag) == 0 &&
            db->points[i].time >= t0 && db->points[i].time <= t1) {
            out[n++] = db->points[i].value;
        }
    }
    return n;
}

double gk_tsdb_mean(const gk_tsdb *db, const char *tag)
{
    double sum = 0.0;
    int n = 0;
    int i;
    if (db == NULL || tag == NULL) {
        return 0.0;
    }
    for (i = 0; i < db->count; ++i) {
        if (strcmp(db->points[i].tag, tag) == 0) {
            sum += db->points[i].value;
            n++;
        }
    }
    return n > 0 ? sum / n : 0.0;
}

/* ---- acquisition ---- */

void gk_acquisition_init(gk_acquisition *a, gk_data_bus *bus, double interval)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    a->bus = bus;
    a->interval = interval > 0.0 ? interval : 0.1;
}

gk_status gk_acquisition_start(gk_acquisition *a, double now)
{
    if (a == NULL || a->bus == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    a->collecting = 1;
    a->started_at = now;
    return GK_OK;
}

gk_status gk_acquisition_stop(gk_acquisition *a)
{
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    a->collecting = 0;
    return GK_OK;
}

int gk_acquisition_tick(gk_acquisition *a, double dt)
{
    if (a == NULL || !a->collecting || a->interval <= 0.0) {
        return 0;
    }
    a->accumulator += dt;
    if (a->accumulator >= a->interval) {
        a->accumulator -= a->interval;
        a->samples++;
        return 1;
    }
    return 0;
}

/* ---- playback ---- */

void gk_playback_init(gk_playback *p, gk_tsdb *db)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->db = db;
    p->speed = 1.0;
}

gk_status gk_playback_start(gk_playback *p, double speed)
{
    if (p == NULL || p->db == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    p->playing = 1;
    p->cursor = 0.0;
    p->index = 0;
    if (speed > 0.0) {
        p->speed = speed;
    }
    return GK_OK;
}

gk_status gk_playback_stop(gk_playback *p)
{
    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    p->playing = 0;
    return GK_OK;
}

int gk_playback_step(gk_playback *p, double dt, gk_ts_point *out)
{
    if (p == NULL || p->db == NULL || !p->playing) {
        return 0;
    }
    p->cursor += dt * p->speed;
    if (p->index >= p->db->count) {
        p->playing = 0;
        return 0;
    }
    if (out != NULL) {
        *out = p->db->points[p->index];
    }
    p->index++;
    return 1;
}

/* ---- visualization ---- */

int gk_tsviz_render(const gk_tsdb *db, const char *tag, int width, int height,
                    char *buf, size_t len)
{
    int off = 0;
    int i;
    int total = 0;
    if (db == NULL || tag == NULL || buf == NULL || width <= 0 || height <= 0) {
        return 0;
    }
    for (i = 0; i < db->count; ++i) {
        if (strcmp(db->points[i].tag, tag) == 0) {
            total++;
        }
    }
    off += snprintf(buf + off, len - (size_t)off,
                    "TSDB %s|tag=%s|points=%d|size=%dx%d\n",
                    db->dbname, tag, total, width, height);
    if (off < 0 || (size_t)off >= len) {
        return (int)len - 1;
    }
    return off;
}

const char *gk_iot_protocol_name(int protocol_id)
{
    static const char *names[] = {
        "bus", "opcua", "mtconnect", "mqtt", "modbus-tcp", "modbus-rtu",
        "profinet", "ethercat", "ethernet-ip", "rest", "websocket", "grpc",
        "graphql", "twin", "influxdb", "tsdb", "acquisition", "playback",
        "visualization", "transport"
    };
    if (protocol_id < 0 ||
        protocol_id >= (int)(sizeof(names) / sizeof(names[0]))) {
        return "unknown";
    }
    return names[protocol_id];
}
