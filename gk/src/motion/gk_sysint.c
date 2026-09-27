#include "gk/gk_sysint.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static void gk__copy(char *dst, size_t cap, const char *src)
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

static int gk__streq(const char *a, const char *b)
{
    return a != NULL && b != NULL && strcmp(a, b) == 0;
}

/* ===================================================================
 * Part A: HIL / SIL / twin / real interfaces
 * =================================================================== */

const char *gk_sysint_mode_name(gk_sysint_mode m)
{
    switch (m) {
    case GK_SYSINT_HIL: return "hil";
    case GK_SYSINT_SIL: return "sil";
    case GK_SYSINT_RCP: return "rcp";
    default: return "unknown";
    }
}

void gk_sysint_loop_init(gk_sysint_loop *l, gk_sysint_mode m,
                         double sample_time_s)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
    l->mode = m;
    l->sample_time_s = (sample_time_s > 0.0) ? sample_time_s : 0.001;
}

gk_status gk_sysint_loop_connect(gk_sysint_loop *l)
{
    if (l == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    l->connected = 1;
    return GK_OK;
}

gk_status gk_sysint_loop_step(gk_sysint_loop *l, double gain)
{
    double error;
    double plant_rate;
    if (l == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!l->connected) {
        return GK_ERR_STATE;
    }
    error = l->setpoint - l->plant_output;
    l->control_output = gain * error;
    /* first-order plant: tau = 0.1 s */
    plant_rate = (l->control_output - l->plant_output) / 0.1;
    l->plant_output += plant_rate * l->sample_time_s;
    l->sim_time_s += l->sample_time_s;
    l->steps++;
    return GK_OK;
}

gk_status gk_sysint_rcp_build(gk_sysint_loop *l, const char *target)
{
    if (l == NULL || target == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (gk__streq(target, "serve") || gk__streq(target, "servo")) {
        l->sample_time_s = 0.0005;
    } else if (gk__streq(target, "thermal")) {
        l->sample_time_s = 0.05;
    } else {
        return GK_ERR_UNSUPPORTED;
    }
    return GK_OK;
}

void gk_sysint_twin_init(gk_sysint_twin *t, const char *tag)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    gk__copy(t->tag, sizeof(t->tag), tag);
}

gk_status gk_sysint_twin_sync(gk_sysint_twin *t, double physical,
                              double virtual_value)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    t->physical = physical;
    t->virtual_value = virtual_value;
    t->last_error = physical - virtual_value;
    return GK_OK;
}

double gk_sysint_twin_error(const gk_sysint_twin *t)
{
    if (t == NULL) {
        return 0.0;
    }
    return t->last_error;
}

void gk_sysint_remote_init(gk_sysint_remote *r)
{
    if (r == NULL) {
        return;
    }
    memset(r, 0, sizeof(*r));
    r->feed_override = 1.0;
    r->interlocked = 1;
}

gk_status gk_sysint_remote_read(const gk_sysint_remote *r, double *x,
                                double *y, double *z)
{
    if (r == NULL || x == NULL || y == NULL || z == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    *x = r->x;
    *y = r->y;
    *z = r->z;
    return GK_OK;
}

gk_status gk_sysint_remote_write(gk_sysint_remote *r, double x, double y,
                                 double z)
{
    if (r == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (r->interlocked) {
        return GK_ERR_STATE;
    }
    if (!r->enabled) {
        return GK_ERR_STATE;
    }
    r->x = x;
    r->y = y;
    r->z = z;
    return GK_OK;
}

gk_status gk_sysint_remote_set_interlock(gk_sysint_remote *r, int engaged)
{
    if (r == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    r->interlocked = engaged ? 1 : 0;
    return GK_OK;
}

gk_status gk_sysint_link(double virtual_value, double physical_value,
                         double tolerance, int *in_sync)
{
    if (in_sync == NULL || tolerance < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    *in_sync = fabs(virtual_value - physical_value) <= tolerance;
    return GK_OK;
}

void gk_sysint_shadow_init(gk_sysint_shadow *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
}

gk_status gk_sysint_shadow_compare(gk_sysint_shadow *s, double virtual_out,
                                   double real_out, double tolerance)
{
    double dev;
    if (s == NULL || tolerance < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    s->virtual_output = virtual_out;
    s->real_output = real_out;
    dev = fabs(virtual_out - real_out);
    if (dev > s->max_deviation) {
        s->max_deviation = dev;
    }
    s->divergent = dev > tolerance;
    return GK_OK;
}

const char *gk_sysint_device_name(gk_sysint_device d)
{
    switch (d) {
    case GK_SYSINT_CNC: return "cnc";
    case GK_SYSINT_PLC: return "plc";
    case GK_SYSINT_SERVO: return "servo";
    case GK_SYSINT_HANDWHEEL: return "handwheel";
    default: return "unknown";
    }
}

gk_status gk_sysint_device_connect(gk_sysint_link_dev *dev,
                                   gk_sysint_device kind,
                                   const char *endpoint)
{
    if (dev == NULL || endpoint == NULL || endpoint[0] == '\0') {
        return GK_ERR_INVALID_ARG;
    }
    memset(dev, 0, sizeof(*dev));
    dev->kind = kind;
    gk__copy(dev->endpoint, sizeof(dev->endpoint), endpoint);
    dev->online = 1;
    return GK_OK;
}

gk_status gk_sysint_device_read(gk_sysint_link_dev *dev, double *value)
{
    if (dev == NULL || value == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!dev->online) {
        return GK_ERR_STATE;
    }
    *value = dev->last_value;
    return GK_OK;
}

gk_status gk_sysint_handwheel_read(gk_sysint_link_dev *dev, double *delta_deg)
{
    if (dev == NULL || delta_deg == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (dev->kind != GK_SYSINT_HANDWHEEL || !dev->online) {
        return GK_ERR_STATE;
    }
    *delta_deg = dev->last_value;
    return GK_OK;
}

/* ===================================================================
 * Part B: scientific data interfaces
 * =================================================================== */

gk_status gk_sysint_hdf5_header(const char *dataset, int rows, int cols,
                                char *out, size_t out_cap)
{
    int n;
    if (dataset == NULL || out == NULL || out_cap == 0 || rows <= 0 ||
        cols <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "HDF5 \"%s\" dims=[%d,%d] float64\n", dataset,
                 rows, cols);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

gk_status gk_sysint_hdf5_dataset(const double *values, int count, char *out,
                                 size_t out_cap)
{
    int i;
    int off = 0;
    if (values == NULL || out == NULL || out_cap == 0 || count <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    out[0] = '\0';
    for (i = 0; i < count; i++) {
        int n = snprintf(out + off, out_cap - (size_t)off, "%s%.6g",
                         (i == 0) ? "" : ",", values[i]);
        if (n < 0 || (size_t)n >= out_cap - (size_t)off) {
            return GK_ERR_OUT_OF_RANGE;
        }
        off += n;
    }
    return GK_OK;
}

gk_status gk_sysint_python_bind(const char *module, const char *function,
                                char *out, size_t out_cap)
{
    int n;
    if (module == NULL || function == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "import %s\n%s.%s(args)\n", module, module,
                 function);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

gk_status gk_sysint_matlab_export(const char *var, const double *values,
                                  int count, char *out, size_t out_cap)
{
    int i;
    int off = 0;
    if (var == NULL || values == NULL || out == NULL || out_cap == 0 ||
        count <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    {
        int n = snprintf(out, out_cap, "%s = [", var);
        if (n < 0 || (size_t)n >= out_cap) {
            return GK_ERR_OUT_OF_RANGE;
        }
        off = n;
    }
    for (i = 0; i < count; i++) {
        int n = snprintf(out + off, out_cap - (size_t)off, "%s%.6g",
                         (i == 0) ? "" : " ", values[i]);
        if (n < 0 || (size_t)n >= out_cap - (size_t)off) {
            return GK_ERR_OUT_OF_RANGE;
        }
        off += n;
    }
    if ((size_t)off + 3 >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    out[off++] = ']';
    out[off++] = ';';
    out[off] = '\0';
    return GK_OK;
}

gk_status gk_sysint_ros_publish(const char *topic, const char *type,
                                double value, char *out, size_t out_cap)
{
    int n;
    if (topic == NULL || type == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "publish %s type=%s data=%.4f\n", topic, type,
                 value);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

gk_status gk_sysint_fmu_export(const char *model, const char *version,
                               int inputs, int outputs, char *out,
                               size_t out_cap)
{
    int n;
    if (model == NULL || version == NULL || out == NULL || out_cap == 0 ||
        inputs < 0 || outputs < 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap,
                 "FMU model=%s fmiVersion=%s inputs=%d outputs=%d\n", model,
                 version, inputs, outputs);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

gk_status gk_sysint_modelica_model(const char *model, const char *equation,
                                   char *out, size_t out_cap)
{
    int n;
    if (model == NULL || equation == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "model %s\n  %s\nend %s;\n", model, equation,
                 model);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

gk_status gk_sysint_paper_figure(int figure_number, const char *caption,
                                 char *out, size_t out_cap)
{
    int n;
    if (caption == NULL || out == NULL || out_cap == 0 || figure_number <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "Figure %d. %s\n", figure_number, caption);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

void gk_sysint_benchmark_init(gk_sysint_benchmark *b, const char *name,
                              int samples)
{
    if (b == NULL) {
        return;
    }
    memset(b, 0, sizeof(*b));
    gk__copy(b->name, sizeof(b->name), name);
    b->samples = samples;
}

gk_status gk_sysint_benchmark_eval(gk_sysint_benchmark *b,
                                   const double *errors, int count)
{
    int i;
    double sum = 0.0;
    if (b == NULL || errors == NULL || count <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < count; i++) {
        sum += fabs(errors[i]);
    }
    b->baseline_error = sum / (double)count;
    return GK_OK;
}

gk_status gk_sysint_experiment_script(const char *name, int repeats, char *out,
                                      size_t out_cap)
{
    int i;
    int off = 0;
    if (name == NULL || out == NULL || out_cap == 0 || repeats <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    {
        int n = snprintf(out, out_cap, "# experiment %s\n", name);
        if (n < 0 || (size_t)n >= out_cap) {
            return GK_ERR_OUT_OF_RANGE;
        }
        off = n;
    }
    for (i = 0; i < repeats; i++) {
        int n = snprintf(out + off, out_cap - (size_t)off, "run(%d)\n", i);
        if (n < 0 || (size_t)n >= out_cap - (size_t)off) {
            return GK_ERR_OUT_OF_RANGE;
        }
        off += n;
    }
    return GK_OK;
}

void gk_sysint_replay_init(gk_sysint_replay *r)
{
    if (r == NULL) {
        return;
    }
    memset(r, 0, sizeof(*r));
    r->speed = 1.0;
}

int gk_sysint_replay_load(gk_sysint_replay *r, const double *data, int count)
{
    if (r == NULL || data == NULL || count < 0 ||
        count > GK_SYSINT_MAX_ITEMS) {
        return -1;
    }
    memcpy(r->samples, data, sizeof(double) * (size_t)count);
    r->count = count;
    r->cursor = 0;
    return count;
}

int gk_sysint_replay_next(gk_sysint_replay *r, double *value)
{
    if (r == NULL || value == NULL || r->count == 0) {
        return -1;
    }
    if (r->cursor >= r->count) {
        r->cursor = 0;
    }
    *value = r->samples[r->cursor];
    r->cursor++;
    return r->cursor;
}

gk_status gk_sysint_replay_seek(gk_sysint_replay *r, int index)
{
    if (r == NULL || index < 0 || index >= r->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    r->cursor = index;
    return GK_OK;
}

/* ===================================================================
 * Part C: security & compliance
 * =================================================================== */

gk_status gk_sec_encrypt(const unsigned char *key, size_t key_len,
                         const unsigned char *plain, size_t len,
                         unsigned char *out)
{
    size_t i;
    if (key == NULL || key_len == 0 || (plain == NULL && len > 0) ||
        (out == NULL && len > 0)) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < len; i++) {
        out[i] = (unsigned char)(plain[i] ^ key[i % key_len]);
    }
    return GK_OK;
}

gk_status gk_sec_decrypt(const unsigned char *key, size_t key_len,
                         const unsigned char *cipher, size_t len,
                         unsigned char *out)
{
    return gk_sec_encrypt(key, key_len, cipher, len, out);
}

unsigned long gk_sec_hash_password(const char *password, unsigned long salt)
{
    unsigned long h = 5381UL ^ salt;
    size_t i;
    if (password == NULL) {
        return 0;
    }
    for (i = 0; password[i] != '\0'; i++) {
        h = ((h << 5) + h) ^ (unsigned char)password[i];
    }
    return h;
}

void gk_sec_account_init(gk_sec_account *a, const char *user,
                         const char *password)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    gk__copy(a->user, sizeof(a->user), user);
    a->password_hash = gk_sec_hash_password(password, 0x5EED);
}

gk_status gk_sec_authenticate(gk_sec_account *a, const char *password)
{
    unsigned long h;
    if (a == NULL || password == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (a->locked) {
        return GK_ERR_STATE;
    }
    h = gk_sec_hash_password(password, 0x5EED);
    if (h == a->password_hash) {
        a->failed_attempts = 0;
        return GK_OK;
    }
    a->failed_attempts++;
    if (a->failed_attempts >= 3) {
        a->locked = 1;
    }
    return GK_ERR_NOT_FOUND;
}

gk_status gk_sec_change_password(gk_sec_account *a, const char *old_password,
                                 const char *new_password)
{
    if (a == NULL || old_password == NULL || new_password == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (gk_sec_hash_password(old_password, 0x5EED) != a->password_hash) {
        return GK_ERR_NOT_FOUND;
    }
    a->password_hash = gk_sec_hash_password(new_password, 0x5EED);
    return GK_OK;
}

void gk_sec_role_init(gk_sec_role *r, const char *user, unsigned int perms)
{
    if (r == NULL) {
        return;
    }
    memset(r, 0, sizeof(*r));
    gk__copy(r->user, sizeof(r->user), user);
    r->perms = perms;
}

int gk_sec_role_allows(const gk_sec_role *r, unsigned int perm)
{
    if (r == NULL) {
        return 0;
    }
    if ((r->perms & GK_SEC_PERM_ADMIN) != 0) {
        return 1;
    }
    return (r->perms & perm) == perm && perm != 0;
}

void gk_sec_audit_init(gk_sec_audit *a)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
}

gk_status gk_sec_audit_log(gk_sec_audit *a, const char *event)
{
    if (a == NULL || event == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (a->count >= GK_SYSINT_MAX_AUDIT) {
        return GK_ERR_OVERFLOW;
    }
    gk__copy(a->events[a->count], sizeof(a->events[a->count]), event);
    a->count++;
    return GK_OK;
}

int gk_sec_audit_contains(const gk_sec_audit *a, const char *event)
{
    int i;
    if (a == NULL || event == NULL) {
        return 0;
    }
    for (i = 0; i < a->count; i++) {
        if (gk__streq(a->events[i], event)) {
            return 1;
        }
    }
    return 0;
}

void gk_sec_policy_init(gk_sec_policy *p, const char *version)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    gk__copy(p->version, sizeof(p->version), version);
    gk__copy(p->text, sizeof(p->text),
             "This software processes learner data for training purposes.");
}

gk_status gk_sec_policy_accept(gk_sec_policy *p, const char *version)
{
    if (p == NULL || version == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!gk__streq(p->version, version)) {
        return GK_ERR_NOT_FOUND;
    }
    p->accepted = 1;
    return GK_OK;
}

void gk_sec_compliance_init(gk_sec_compliance *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
}

int gk_sec_compliance_add(gk_sec_compliance *c, const char *rule)
{
    if (c == NULL || rule == NULL || c->count >= GK_SYSINT_MAX_ITEMS) {
        return -1;
    }
    gk__copy(c->rules[c->count], sizeof(c->rules[c->count]), rule);
    c->passed[c->count] = 0;
    c->count++;
    return c->count;
}

gk_status gk_sec_compliance_set(gk_sec_compliance *c, const char *rule,
                                int passed)
{
    int i;
    if (c == NULL || rule == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < c->count; i++) {
        if (gk__streq(c->rules[i], rule)) {
            c->passed[i] = passed ? 1 : 0;
            return GK_OK;
        }
    }
    return GK_ERR_NOT_FOUND;
}

int gk_sec_compliance_all_pass(const gk_sec_compliance *c)
{
    int i;
    if (c == NULL || c->count == 0) {
        return 1;
    }
    for (i = 0; i < c->count; i++) {
        if (!c->passed[i]) {
            return 0;
        }
    }
    return 1;
}

gk_status gk_sec_gdpr_export(const char *user, const char *data, char *out,
                             size_t out_cap)
{
    int n;
    if (user == NULL || data == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "SUBJECT:%s\nDATA:%s\n", user, data);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

gk_status gk_sec_gdpr_erase(char *record, const char *user)
{
    char *p;
    if (record == NULL || user == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    p = strstr(record, user);
    if (p == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    memset(p, '*', strlen(user));
    return GK_OK;
}

void gk_sec_network_init(gk_sec_network *n)
{
    if (n == NULL) {
        return;
    }
    memset(n, 0, sizeof(*n));
    n->min_key_bits = 128;
}

gk_status gk_sec_network_enable_tls(gk_sec_network *n, const char *cipher,
                                    int min_key_bits)
{
    if (n == NULL || cipher == NULL || min_key_bits < 128) {
        return GK_ERR_INVALID_ARG;
    }
    gk__copy(n->cipher, sizeof(n->cipher), cipher);
    n->min_key_bits = min_key_bits;
    n->tls_enabled = 1;
    return GK_OK;
}

int gk_sec_network_port_allowed(const gk_sec_network *n, int port)
{
    int i;
    if (n == NULL || n->port_count == 0) {
        return 1;   /* no allow-list configured means all ports permitted */
    }
    for (i = 0; i < n->port_count; i++) {
        if (n->allowed_ports[i] == port) {
            return 1;
        }
    }
    return 0;
}

void gk_sec_tamper_init(gk_sec_tamper *t)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
}

unsigned long gk_sec_tamper_append(gk_sec_tamper *t, const char *record)
{
    unsigned long prev;
    unsigned long h;
    size_t i;
    if (t == NULL || record == NULL || t->count >= GK_SYSINT_MAX_ITEMS) {
        return 0;
    }
    prev = (t->count == 0) ? 0xC0FFEEUL : t->chain[t->count - 1];
    h = prev;
    for (i = 0; record[i] != '\0'; i++) {
        h ^= (unsigned char)record[i];
        h *= 1099511628211UL;
        h ^= prev;
    }
    t->chain[t->count] = h;
    t->count++;
    return h;
}

int gk_sec_tamper_verify(const gk_sec_tamper *t, int index,
                         const char *record)
{
    unsigned long prev;
    unsigned long h;
    size_t i;
    if (t == NULL || record == NULL || index < 0 || index >= t->count) {
        return 0;
    }
    prev = (index == 0) ? 0xC0FFEEUL : t->chain[index - 1];
    h = prev;
    for (i = 0; record[i] != '\0'; i++) {
        h ^= (unsigned char)record[i];
        h *= 1099511628211UL;
        h ^= prev;
    }
    return h == t->chain[index];
}

void gk_sec_key_init(gk_sec_key *k, unsigned long modulus,
                     unsigned long exponent)
{
    if (k == NULL) {
        return;
    }
    k->modulus = modulus;
    k->exponent = exponent;
}

unsigned long gk_sec_sign(const gk_sec_key *k, const char *message)
{
    unsigned long h = 1469598103934665603UL;
    unsigned long e;
    size_t i;
    if (k == NULL || message == NULL || k->modulus == 0) {
        return 0;
    }
    for (i = 0; message[i] != '\0'; i++) {
        h ^= (unsigned char)message[i];
        h *= 1099511628211UL;
    }
    h %= k->modulus;
    /* modular exponentiation by squaring */
    {
        unsigned long result = 1;
        unsigned long base = h;
        unsigned long exp = k->exponent;
        while (exp > 0) {
            if (exp & 1UL) {
                result = (result * base) % k->modulus;
            }
            base = (base * base) % k->modulus;
            exp >>= 1;
        }
        e = result;
    }
    return e;
}

int gk_sec_verify(const gk_sec_key *k, const char *message,
                  unsigned long signature)
{
    if (k == NULL || message == NULL) {
        return 0;
    }
    return gk_sec_sign(k, message) == signature;
}
