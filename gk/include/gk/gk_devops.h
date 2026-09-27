#ifndef GK_DEVOPS_H
#define GK_DEVOPS_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_DEV_NAME 64
#define GK_DEV_TEXT 1024
#define GK_DEV_MAX_ITEMS 64
#define GK_DEV_MAX_SECTIONS 32
#define GK_DEV_MAX_STAGES 16

/* ===================================================================
 * Part A: diagnostics (851-857)
 * =================================================================== */

/* 851 regression test */
typedef struct {
    char name[GK_DEV_NAME];
    int passed;
} gk_dev_reg_case;

typedef struct {
    gk_dev_reg_case cases[GK_DEV_MAX_ITEMS];
    int count;
    int regressions;
} gk_dev_regression;

void gk_dev_regression_init(gk_dev_regression *r);
int gk_dev_regression_add(gk_dev_regression *r, const char *name, int passed);
/* compare against a baseline mask; returns number of regressions */
int gk_dev_regression_compare(gk_dev_regression *r, const int *baseline,
                              int baseline_count);

/* 852 profiler */
typedef struct {
    char name[GK_DEV_NAME];
    double total_ms;
    long calls;
} gk_dev_prof_section;

typedef struct {
    gk_dev_prof_section sections[GK_DEV_MAX_SECTIONS];
    int count;
    long start_tick;
    int active;
} gk_dev_profiler;

void gk_dev_prof_init(gk_dev_profiler *p);
int gk_dev_prof_begin(gk_dev_profiler *p, const char *name);
gk_status gk_dev_prof_end(gk_dev_profiler *p);
int gk_dev_prof_hottest(const gk_dev_profiler *p);

/* 853 memory monitoring */
typedef struct {
    size_t current;
    size_t peak;
    size_t limit;
    long allocations;
} gk_dev_memwatch;

void gk_dev_memwatch_init(gk_dev_memwatch *m, size_t limit);
gk_status gk_dev_memwatch_alloc(gk_dev_memwatch *m, size_t bytes);
void gk_dev_memwatch_free(gk_dev_memwatch *m, size_t bytes);
int gk_dev_memwatch_over_limit(const gk_dev_memwatch *m);

/* 854 crash report */
typedef struct {
    int signal_number;
    unsigned long address;
    char module[GK_DEV_NAME];
    int thread_id;
} gk_dev_crash;

void gk_dev_crash_init(gk_dev_crash *c);
gk_status gk_dev_crash_report(const gk_dev_crash *c, char *out, size_t out_cap);

/* 855 crash capture */
typedef struct {
    int installed;
    int captured_signal;
    gk_dev_crash last;
} gk_dev_crash_handler;

void gk_dev_crash_handler_init(gk_dev_crash_handler *h);
int gk_dev_crash_handler_install(gk_dev_crash_handler *h);
int gk_dev_crash_handler_capture(gk_dev_crash_handler *h, int signal_number,
                                 unsigned long address);

/* 856 telemetry */
typedef struct {
    char event[GK_DEV_NAME];
    long count;
} gk_dev_telemetry_entry;

typedef struct {
    gk_dev_telemetry_entry entries[GK_DEV_MAX_ITEMS];
    int count;
    int enabled;
} gk_dev_telemetry;

void gk_dev_telemetry_init(gk_dev_telemetry *t);
gk_status gk_dev_telemetry_event(gk_dev_telemetry *t, const char *event);
long gk_dev_telemetry_count(const gk_dev_telemetry *t, const char *event);
gk_status gk_dev_telemetry_flush(const gk_dev_telemetry *t, char *out,
                                 size_t out_cap);

/* 857 hot reload */
typedef struct {
    char module[GK_DEV_NAME];
    int generation;
    int reloads;
    int auto_reload;
} gk_dev_hotreload;

void gk_dev_hotreload_init(gk_dev_hotreload *h, const char *module);
gk_status gk_dev_hotreload_watch(gk_dev_hotreload *h, const char *module);
int gk_dev_hotreload_poll(gk_dev_hotreload *h, int changed);

/* ===================================================================
 * Part B: scripting, plugins, config, logging (858-865)
 * =================================================================== */

typedef enum {
    GK_DEV_SCRIPT_LUA = 0,   /* 858 */
    GK_DEV_SCRIPT_PYTHON     /* 859 */
} gk_dev_script_engine;

const char *gk_dev_script_name(gk_dev_script_engine e);

/* 858/859 script host: register and invoke a named function */
typedef struct {
    gk_dev_script_engine engine;
    char names[GK_DEV_MAX_ITEMS][GK_DEV_NAME];
    int count;
} gk_dev_script_host;

void gk_dev_script_host_init(gk_dev_script_host *s, gk_dev_script_engine e);
gk_status gk_dev_script_register(gk_dev_script_host *s, const char *fn);
int gk_dev_script_invoke(gk_dev_script_host *s, const char *fn);

/* 860 plugin ABI */
#define GK_DEV_PLUGIN_ABI_VERSION 3
typedef struct {
    int abi_version;
    const char *name;
    gk_status (*init)(void);
    void (*shutdown)(void);
} gk_dev_plugin;

int gk_dev_plugin_abi_compatible(const gk_dev_plugin *p);
gk_status gk_dev_plugin_load(gk_dev_plugin *p);

/* 861 dynamic library loading (registry-backed mock) */
typedef struct {
    char paths[GK_DEV_MAX_ITEMS][GK_DEV_NAME];
    void *handles[GK_DEV_MAX_ITEMS];
    int count;
} gk_dev_dylib;

void gk_dev_dylib_init(gk_dev_dylib *d);
int gk_dev_dylib_open(gk_dev_dylib *d, const char *path);
void *gk_dev_dylib_symbol(gk_dev_dylib *d, const char *path, const char *sym);
gk_status gk_dev_dylib_close(gk_dev_dylib *d, const char *path);

/* 862 config persistence */
typedef struct {
    char keys[GK_DEV_MAX_ITEMS][GK_DEV_NAME];
    char values[GK_DEV_MAX_ITEMS][GK_DEV_NAME];
    int count;
} gk_dev_config;

void gk_dev_config_init(gk_dev_config *c);
gk_status gk_dev_config_set(gk_dev_config *c, const char *key,
                            const char *value);
const char *gk_dev_config_get(const gk_dev_config *c, const char *key);
gk_status gk_dev_config_save(const gk_dev_config *c, char *out, size_t out_cap);
gk_status gk_dev_config_load(gk_dev_config *c, const char *text);

/* 863 logging system */
typedef enum {
    GK_DEVLOG_TRACE = 0,
    GK_DEVLOG_DEBUG,
    GK_DEVLOG_INFO,
    GK_DEVLOG_WARN,
    GK_DEVLOG_ERROR
} gk_devlog_level;

const char *gk_devlog_level_name(gk_devlog_level l);

typedef struct {
    char target[GK_DEV_NAME];
    gk_devlog_level min_level;
    unsigned long written;
    unsigned long rotated;
    size_t max_bytes;
    size_t bytes;
} gk_devlog_sink;

void gk_devlog_sink_init(gk_devlog_sink *s, const char *target,
                         gk_devlog_level min_level);
int gk_devlog_enabled(const gk_devlog_sink *s, gk_devlog_level l);
gk_status gk_devlog_write(gk_devlog_sink *s, gk_devlog_level l,
                          const char *message);
/* 865 rotation: returns 1 when a rotation happened */
int gk_devlog_rotate(gk_devlog_sink *s);

/* ===================================================================
 * Part C: packaging & deployment (866-874)
 * =================================================================== */

typedef enum {
    GK_DEV_PKG_MSI = 0,     /* 866 */
    GK_DEV_PKG_DEB,         /* 867 */
    GK_DEV_PKG_DMG,         /* 868 */
    GK_DEV_PKG_PORTABLE,    /* 869 */
    GK_DEV_PKG_COUNT
} gk_dev_pkg_format;

const char *gk_dev_pkg_name(gk_dev_pkg_format f);
const char *gk_dev_pkg_extension(gk_dev_pkg_format f);
/* render a package manifest */
gk_status gk_dev_pkg_manifest(gk_dev_pkg_format f, const char *product,
                              const char *version, char *out, size_t out_cap);

/* 870 auto update */
typedef struct {
    char current[GK_DEV_NAME];
    char latest[GK_DEV_NAME];
    int mandatory;
} gk_dev_update;

void gk_dev_update_init(gk_dev_update *u, const char *current);
/* semantic version compare: -1, 0, 1 */
int gk_dev_version_compare(const char *a, const char *b);
int gk_dev_update_available(gk_dev_update *u, const char *latest);

/* 871 CI/CD pipeline */
typedef struct {
    char name[GK_DEV_NAME];
    int failed;
    double seconds;
} gk_dev_ci_stage;

typedef struct {
    gk_dev_ci_stage stages[GK_DEV_MAX_STAGES];
    int count;
    int failed;
} gk_dev_ci_pipeline;

void gk_dev_ci_init(gk_dev_ci_pipeline *p);
int gk_dev_ci_add_stage(gk_dev_ci_pipeline *p, const char *name);
int gk_dev_ci_run(gk_dev_ci_pipeline *p);
int gk_dev_ci_stage_fail(gk_dev_ci_pipeline *p, const char *name);

/* 872 docker image */
gk_status gk_dev_docker_build(const char *image, const char *base,
                              const char *cmd, char *out, size_t out_cap);

/* 873 cloud deploy */
typedef struct {
    char provider[GK_DEV_NAME];
    char region[GK_DEV_NAME];
    char instance[GK_DEV_NAME];
    int replicas;
    int deployed;
} gk_dev_cloud;

void gk_dev_cloud_init(gk_dev_cloud *c, const char *provider);
gk_status gk_dev_cloud_deploy(gk_dev_cloud *c, const char *region,
                              int replicas);

/* 874 microservices */
typedef struct {
    char name[GK_DEV_NAME];
    int port;
    int healthy;
} gk_dev_service;

typedef struct {
    gk_dev_service services[GK_DEV_MAX_ITEMS];
    int count;
} gk_dev_mesh;

void gk_dev_mesh_init(gk_dev_mesh *m);
int gk_dev_mesh_register(gk_dev_mesh *m, const char *name, int port);
int gk_dev_mesh_health(gk_dev_mesh *m, const char *name, int healthy);
int gk_dev_mesh_healthy_count(const gk_dev_mesh *m);

/* ===================================================================
 * Part D: licensing & editions (875-893)
 * =================================================================== */

typedef enum {
    GK_LIC_FREE = 0,        /* 875 */
    GK_LIC_PROFESSIONAL,    /* 876 */
    GK_LIC_EDUCATION,       /* 877 */
    GK_LIC_ENTERPRISE,      /* 878 */
    GK_LIC_EDITION_COUNT
} gk_lic_edition;

const char *gk_lic_edition_name(gk_lic_edition e);
/* highest unlockable feature tier; free<edu<pro<enterprise */
int gk_lic_edition_tier(gk_lic_edition e);

typedef enum {
    GK_LIC_MODE_NONE = 0,
    GK_LIC_MODE_SUBSCRIPTION,   /* 884 */
    GK_LIC_MODE_PERPETUAL,      /* 885 */
    GK_LIC_MODE_VOLUME,         /* 886 */
    GK_LIC_MODE_TRIAL           /* 887 */
} gk_lic_mode;

typedef enum {
    GK_LIC_STATE_INACTIVE = 0,
    GK_LIC_STATE_ACTIVE,
    GK_LIC_STATE_EXPIRED,
    GK_LIC_STATE_REVOKED
} gk_lic_state;

typedef struct {
    gk_lic_edition edition;
    gk_lic_mode mode;
    gk_lic_state state;
    char serial[GK_DEV_NAME];
    long issued_day;
    long expiry_day;      /* 0 = never */
    int seats;
    int used_seats;
    int offline;          /* 881 */
    long today;
} gk_license;

/* 879 serial number */
gk_status gk_lic_generate_serial(gk_lic_edition e, unsigned int seed,
                                 char *out, size_t out_cap);
int gk_lic_verify_serial(const char *serial, gk_lic_edition e);
void gk_lic_set_serial(gk_license *l, const char *serial);

/* 880 online activation */
gk_status gk_lic_activate_online(gk_license *l, const char *activation_key);
/* 881 offline activation */
gk_status gk_lic_offline_request(const char *serial, char *out, size_t cap);
gk_status gk_lic_activate_offline(gk_license *l, const char *request,
                                  const char *response);

void gk_lic_init(gk_license *l, gk_lic_edition e, gk_lic_mode m);
int gk_lic_is_valid(const gk_license *l);

/* 882 usage statistics */
gk_status gk_lic_track_usage(gk_license *l, double hours);
/* 883 renewal reminder: days until expiry, -1 if perpetual/expired */
long gk_lic_days_to_renewal(const gk_license *l);
/* 888 feature limits */
int gk_lic_feature_allowed(const gk_license *l, int feature_tier);
int gk_lic_seat_available(const gk_license *l);
/* 889 time limit */
int gk_lic_time_expired(const gk_license *l);
/* 890 watermark */
gk_status gk_lic_watermark(const gk_license *l, char *out, size_t out_cap);
/* 891 anti-crack / integrity */
int gk_lic_integrity_ok(const unsigned char *data, size_t len,
                        unsigned long checksum);
/* 892 dongle */
typedef struct {
    int present;
    unsigned long dongle_id;
    unsigned long expected_id;
} gk_lic_dongle;

void gk_lic_dongle_init(gk_lic_dongle *d, unsigned long expected_id);
gk_status gk_lic_dongle_plug(gk_lic_dongle *d, unsigned long dongle_id);
int gk_lic_dongle_valid(const gk_lic_dongle *d);
/* 893 license server */
typedef struct {
    int online;
    int max_seats;
    int checked_out;
} gk_lic_server;

void gk_lic_server_init(gk_lic_server *s, int max_seats);
gk_status gk_lic_server_checkout(gk_lic_server *s);
gk_status gk_lic_server_checkin(gk_lic_server *s);

#ifdef __cplusplus
}
#endif

#endif /* GK_DEVOPS_H */
