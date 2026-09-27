#ifndef GK_SYSINT_H
#define GK_SYSINT_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_SYSINT_NAME 64
#define GK_SYSINT_TEXT 4096
#define GK_SYSINT_MAX_ITEMS 64
#define GK_SYSINT_MAX_AUDIT 256

/* ===================================================================
 * Part A: HIL / SIL / digital twin / real interfaces (919-930)
 * =================================================================== */

typedef enum {
    GK_SYSINT_HIL = 0,   /* 919 */
    GK_SYSINT_SIL,       /* 920 */
    GK_SYSINT_RCP        /* 921 */
} gk_sysint_mode;

const char *gk_sysint_mode_name(gk_sysint_mode m);

typedef struct {
    gk_sysint_mode mode;
    double sample_time_s;
    double sim_time_s;
    double plant_output;
    double control_output;
    double setpoint;
    int steps;
    int connected;
} gk_sysint_loop;

void gk_sysint_loop_init(gk_sysint_loop *l, gk_sysint_mode m,
                         double sample_time_s);
gk_status gk_sysint_loop_connect(gk_sysint_loop *l);
/* run one closed-loop step: proportional controller driving a first-order plant */
gk_status gk_sysint_loop_step(gk_sysint_loop *l, double gain);
/* 921 RCP: rebuild the control model for a target */
gk_status gk_sysint_rcp_build(gk_sysint_loop *l, const char *target);

/* 922 digital twin sync */
typedef struct {
    char tag[GK_SYSINT_NAME];
    double physical;
    double virtual_value;
    double last_error;
} gk_sysint_twin;

void gk_sysint_twin_init(gk_sysint_twin *t, const char *tag);
gk_status gk_sysint_twin_sync(gk_sysint_twin *t, double physical,
                              double virtual_value);
double gk_sysint_twin_error(const gk_sysint_twin *t);

/* 923 remote monitoring / 924 remote operation */
typedef struct {
    double x, y, z;
    double feed_override;
    int enabled;
    int interlocked;
} gk_sysint_remote;

void gk_sysint_remote_init(gk_sysint_remote *r);
gk_status gk_sysint_remote_read(const gk_sysint_remote *r, double *x,
                                double *y, double *z);
/* remote write is refused while the safety interlock is engaged */
gk_status gk_sysint_remote_write(gk_sysint_remote *r, double x, double y,
                                 double z);
gk_status gk_sysint_remote_set_interlock(gk_sysint_remote *r, int engaged);

/* 925 virtual-physical linkage */
gk_status gk_sysint_link(double virtual_value, double physical_value,
                         double tolerance, int *in_sync);

/* 926 shadow mode */
typedef struct {
    double virtual_output;
    double real_output;
    double max_deviation;
    int divergent;
} gk_sysint_shadow;

void gk_sysint_shadow_init(gk_sysint_shadow *s);
gk_status gk_sysint_shadow_compare(gk_sysint_shadow *s, double virtual_out,
                                   double real_out, double tolerance);

/* 927-930 real hardware interfaces (registry-backed) */
typedef enum {
    GK_SYSINT_CNC = 0,      /* 927 */
    GK_SYSINT_PLC,          /* 928 */
    GK_SYSINT_SERVO,        /* 929 */
    GK_SYSINT_HANDWHEEL     /* 930 */
} gk_sysint_device;

const char *gk_sysint_device_name(gk_sysint_device d);

typedef struct {
    gk_sysint_device kind;
    char endpoint[GK_SYSINT_NAME];
    int online;
    double last_value;
} gk_sysint_link_dev;

gk_status gk_sysint_device_connect(gk_sysint_link_dev *dev,
                                   gk_sysint_device kind,
                                   const char *endpoint);
gk_status gk_sysint_device_read(gk_sysint_link_dev *dev, double *value);
gk_status gk_sysint_handwheel_read(gk_sysint_link_dev *dev,
                                   double *delta_deg);

/* ===================================================================
 * Part B: scientific data interfaces (931-940)
 * =================================================================== */

/* 931 HDF5 export */
gk_status gk_sysint_hdf5_header(const char *dataset, int rows, int cols,
                                char *out, size_t out_cap);
gk_status gk_sysint_hdf5_dataset(const double *values, int count, char *out,
                                 size_t out_cap);

/* 932 Python API binding descriptor */
gk_status gk_sysint_python_bind(const char *module, const char *function,
                                char *out, size_t out_cap);

/* 933 MATLAB interface */
gk_status gk_sysint_matlab_export(const char *var, const double *values,
                                  int count, char *out, size_t out_cap);

/* 934 ROS interface */
gk_status gk_sysint_ros_publish(const char *topic, const char *type,
                                double value, char *out, size_t out_cap);

/* 935 FMU/FMI */
gk_status gk_sysint_fmu_export(const char *model, const char *version,
                               int inputs, int outputs, char *out,
                               size_t out_cap);

/* 936 Modelica */
gk_status gk_sysint_modelica_model(const char *model, const char *equation,
                                   char *out, size_t out_cap);

/* 937 paper mode: render a figure caption block */
gk_status gk_sysint_paper_figure(int figure_number, const char *caption,
                                 char *out, size_t out_cap);

/* 938 benchmark dataset */
typedef struct {
    char name[GK_SYSINT_NAME];
    int samples;
    double baseline_error;
} gk_sysint_benchmark;

void gk_sysint_benchmark_init(gk_sysint_benchmark *b, const char *name,
                              int samples);
gk_status gk_sysint_benchmark_eval(gk_sysint_benchmark *b,
                                   const double *errors, int count);

/* 939 experiment script */
gk_status gk_sysint_experiment_script(const char *name, int repeats,
                                      char *out, size_t out_cap);

/* 940 data replay */
typedef struct {
    double samples[GK_SYSINT_MAX_ITEMS];
    int count;
    int cursor;
    double speed;
} gk_sysint_replay;

void gk_sysint_replay_init(gk_sysint_replay *r);
int gk_sysint_replay_load(gk_sysint_replay *r, const double *data, int count);
int gk_sysint_replay_next(gk_sysint_replay *r, double *value);
gk_status gk_sysint_replay_seek(gk_sysint_replay *r, int index);

/* ===================================================================
 * Part C: security & compliance (941-950)
 * =================================================================== */

/* 941 encryption (repeating-key XOR stream, sym_encrypt/sym_decrypt pair) */
gk_status gk_sec_encrypt(const unsigned char *key, size_t key_len,
                         const unsigned char *plain, size_t len,
                         unsigned char *out);
gk_status gk_sec_decrypt(const unsigned char *key, size_t key_len,
                         const unsigned char *cipher, size_t len,
                         unsigned char *out);

/* 942 user authentication */
typedef struct {
    char user[GK_SYSINT_NAME];
    unsigned long password_hash;
    int locked;
    int failed_attempts;
} gk_sec_account;

void gk_sec_account_init(gk_sec_account *a, const char *user,
                         const char *password);
unsigned long gk_sec_hash_password(const char *password, unsigned long salt);
gk_status gk_sec_authenticate(gk_sec_account *a, const char *password);
gk_status gk_sec_change_password(gk_sec_account *a, const char *old_password,
                                 const char *new_password);

/* 943 permission management */
typedef enum {
    GK_SEC_PERM_READ = 1u << 0,
    GK_SEC_PERM_WRITE = 1u << 1,
    GK_SEC_PERM_EXECUTE = 1u << 2,
    GK_SEC_PERM_ADMIN = 1u << 3
} gk_sec_perm;

typedef struct {
    char user[GK_SYSINT_NAME];
    unsigned int perms;
} gk_sec_role;

void gk_sec_role_init(gk_sec_role *r, const char *user, unsigned int perms);
int gk_sec_role_allows(const gk_sec_role *r, unsigned int perm);

/* 944 audit log */
typedef struct {
    char events[GK_SYSINT_MAX_AUDIT][GK_SYSINT_NAME];
    int count;
} gk_sec_audit;

void gk_sec_audit_init(gk_sec_audit *a);
gk_status gk_sec_audit_log(gk_sec_audit *a, const char *event);
int gk_sec_audit_contains(const gk_sec_audit *a, const char *event);

/* 945 privacy policy */
typedef struct {
    char version[GK_SYSINT_NAME];
    char text[GK_SYSINT_TEXT];
    int accepted;
} gk_sec_policy;

void gk_sec_policy_init(gk_sec_policy *p, const char *version);
gk_status gk_sec_policy_accept(gk_sec_policy *p, const char *version);

/* 946 compliance check */
typedef struct {
    char rules[GK_SYSINT_MAX_ITEMS][GK_SYSINT_NAME];
    int passed[GK_SYSINT_MAX_ITEMS];
    int count;
} gk_sec_compliance;

void gk_sec_compliance_init(gk_sec_compliance *c);
int gk_sec_compliance_add(gk_sec_compliance *c, const char *rule);
gk_status gk_sec_compliance_set(gk_sec_compliance *c, const char *rule,
                                int passed);
int gk_sec_compliance_all_pass(const gk_sec_compliance *c);

/* 947 GDPR data subject rights */
gk_status gk_sec_gdpr_export(const char *user, const char *data, char *out,
                             size_t out_cap);
gk_status gk_sec_gdpr_erase(char *record, const char *user);

/* 948 network security */
typedef struct {
    int tls_enabled;
    char cipher[GK_SYSINT_NAME];
    int min_key_bits;
    int allowed_ports[GK_SYSINT_MAX_ITEMS];
    int port_count;
} gk_sec_network;

void gk_sec_network_init(gk_sec_network *n);
gk_status gk_sec_network_enable_tls(gk_sec_network *n, const char *cipher,
                                    int min_key_bits);
int gk_sec_network_port_allowed(const gk_sec_network *n, int port);

/* 949 tamper-proof hash chain */
typedef struct {
    unsigned long chain[GK_SYSINT_MAX_ITEMS];
    int count;
} gk_sec_tamper;

void gk_sec_tamper_init(gk_sec_tamper *t);
unsigned long gk_sec_tamper_append(gk_sec_tamper *t, const char *record);
int gk_sec_tamper_verify(const gk_sec_tamper *t, int index,
                         const char *record);

/* 950 digital signature */
typedef struct {
    unsigned long modulus;
    unsigned long exponent;
} gk_sec_key;

void gk_sec_key_init(gk_sec_key *k, unsigned long modulus,
                     unsigned long exponent);
unsigned long gk_sec_sign(const gk_sec_key *k, const char *message);
int gk_sec_verify(const gk_sec_key *k, const char *message,
                  unsigned long signature);

#ifdef __cplusplus
}
#endif

#endif /* GK_SYSINT_H */
