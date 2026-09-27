#include "gk/gk_devops.h"

#include <stdio.h>
#include <stdlib.h>
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
 * Part A: diagnostics
 * =================================================================== */

void gk_dev_regression_init(gk_dev_regression *r)
{
    if (r == NULL) {
        return;
    }
    memset(r, 0, sizeof(*r));
}

int gk_dev_regression_add(gk_dev_regression *r, const char *name, int passed)
{
    if (r == NULL || r->count >= GK_DEV_MAX_ITEMS) {
        return -1;
    }
    gk__copy(r->cases[r->count].name, sizeof(r->cases[r->count].name), name);
    r->cases[r->count].passed = passed ? 1 : 0;
    r->count++;
    return r->count;
}

int gk_dev_regression_compare(gk_dev_regression *r, const int *baseline,
                              int baseline_count)
{
    int i;
    int regressions = 0;
    if (r == NULL || baseline == NULL) {
        return -1;
    }
    for (i = 0; i < r->count && i < baseline_count; i++) {
        /* a regression is a case that used to pass and now fails */
        if (baseline[i] && !r->cases[i].passed) {
            regressions++;
        }
    }
    r->regressions = regressions;
    return regressions;
}

void gk_dev_prof_init(gk_dev_profiler *p)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
}

int gk_dev_prof_begin(gk_dev_profiler *p, const char *name)
{
    int i;
    if (p == NULL || name == NULL) {
        return -1;
    }
    for (i = 0; i < p->count; i++) {
        if (gk__streq(p->sections[i].name, name)) {
            p->active = 1;
            p->start_tick = i;
            return i;
        }
    }
    if (p->count >= GK_DEV_MAX_SECTIONS) {
        return -1;
    }
    gk__copy(p->sections[p->count].name, sizeof(p->sections[p->count].name),
             name);
    p->active = 1;
    p->start_tick = p->count;
    p->count++;
    return p->start_tick;
}

gk_status gk_dev_prof_end(gk_dev_profiler *p)
{
    if (p == NULL || !p->active) {
        return GK_ERR_STATE;
    }
    p->sections[p->start_tick].calls++;
    p->active = 0;
    return GK_OK;
}

int gk_dev_prof_hottest(const gk_dev_profiler *p)
{
    int i;
    int best = -1;
    double best_total = -1.0;
    if (p == NULL) {
        return -1;
    }
    for (i = 0; i < p->count; i++) {
        if (p->sections[i].total_ms > best_total) {
            best_total = p->sections[i].total_ms;
            best = i;
        }
    }
    return best;
}

void gk_dev_memwatch_init(gk_dev_memwatch *m, size_t limit)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    m->limit = limit;
}

gk_status gk_dev_memwatch_alloc(gk_dev_memwatch *m, size_t bytes)
{
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    m->current += bytes;
    m->allocations++;
    if (m->current > m->peak) {
        m->peak = m->current;
    }
    return GK_OK;
}

void gk_dev_memwatch_free(gk_dev_memwatch *m, size_t bytes)
{
    if (m == NULL) {
        return;
    }
    if (bytes > m->current) {
        m->current = 0;
    } else {
        m->current -= bytes;
    }
}

int gk_dev_memwatch_over_limit(const gk_dev_memwatch *m)
{
    if (m == NULL || m->limit == 0) {
        return 0;
    }
    return m->current > m->limit;
}

void gk_dev_crash_init(gk_dev_crash *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
}

gk_status gk_dev_crash_report(const gk_dev_crash *c, char *out, size_t out_cap)
{
    int n;
    if (c == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap,
                 "CRASH signal=%d address=0x%lx module=%s thread=%d\n",
                 c->signal_number, c->address, c->module, c->thread_id);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

void gk_dev_crash_handler_init(gk_dev_crash_handler *h)
{
    if (h == NULL) {
        return;
    }
    memset(h, 0, sizeof(*h));
}

int gk_dev_crash_handler_install(gk_dev_crash_handler *h)
{
    if (h == NULL) {
        return 0;
    }
    h->installed = 1;
    return 1;
}

int gk_dev_crash_handler_capture(gk_dev_crash_handler *h, int signal_number,
                                 unsigned long address)
{
    if (h == NULL || !h->installed) {
        return 0;
    }
    h->captured_signal = signal_number;
    h->last.signal_number = signal_number;
    h->last.address = address;
    return 1;
}

void gk_dev_telemetry_init(gk_dev_telemetry *t)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->enabled = 1;
}

gk_status gk_dev_telemetry_event(gk_dev_telemetry *t, const char *event)
{
    int i;
    if (t == NULL || event == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!t->enabled) {
        return GK_ERR_STATE;
    }
    for (i = 0; i < t->count; i++) {
        if (gk__streq(t->entries[i].event, event)) {
            t->entries[i].count++;
            return GK_OK;
        }
    }
    if (t->count >= GK_DEV_MAX_ITEMS) {
        return GK_ERR_OVERFLOW;
    }
    gk__copy(t->entries[t->count].event, sizeof(t->entries[t->count].event),
             event);
    t->entries[t->count].count = 1;
    t->count++;
    return GK_OK;
}

long gk_dev_telemetry_count(const gk_dev_telemetry *t, const char *event)
{
    int i;
    if (t == NULL || event == NULL) {
        return 0;
    }
    for (i = 0; i < t->count; i++) {
        if (gk__streq(t->entries[i].event, event)) {
            return t->entries[i].count;
        }
    }
    return 0;
}

gk_status gk_dev_telemetry_flush(const gk_dev_telemetry *t, char *out,
                                 size_t out_cap)
{
    int i;
    int off = 0;
    if (t == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    out[0] = '\0';
    for (i = 0; i < t->count; i++) {
        int n = snprintf(out + off, out_cap - (size_t)off, "%s=%ld\n",
                         t->entries[i].event, t->entries[i].count);
        if (n < 0 || (size_t)n >= out_cap - (size_t)off) {
            return GK_ERR_OUT_OF_RANGE;
        }
        off += n;
    }
    return GK_OK;
}

void gk_dev_hotreload_init(gk_dev_hotreload *h, const char *module)
{
    if (h == NULL) {
        return;
    }
    memset(h, 0, sizeof(*h));
    gk__copy(h->module, sizeof(h->module), module);
    h->generation = 1;
    h->auto_reload = 1;
}

gk_status gk_dev_hotreload_watch(gk_dev_hotreload *h, const char *module)
{
    if (h == NULL || module == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    gk__copy(h->module, sizeof(h->module), module);
    return GK_OK;
}

int gk_dev_hotreload_poll(gk_dev_hotreload *h, int changed)
{
    if (h == NULL) {
        return 0;
    }
    if (changed && h->auto_reload) {
        h->generation++;
        h->reloads++;
        return h->generation;
    }
    return 0;
}

/* ===================================================================
 * Part B: scripting, plugins, config, logging
 * =================================================================== */

const char *gk_dev_script_name(gk_dev_script_engine e)
{
    switch (e) {
    case GK_DEV_SCRIPT_LUA: return "lua";
    case GK_DEV_SCRIPT_PYTHON: return "python";
    default: return "unknown";
    }
}

void gk_dev_script_host_init(gk_dev_script_host *s, gk_dev_script_engine e)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->engine = e;
}

gk_status gk_dev_script_register(gk_dev_script_host *s, const char *fn)
{
    if (s == NULL || fn == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (s->count >= GK_DEV_MAX_ITEMS) {
        return GK_ERR_OVERFLOW;
    }
    gk__copy(s->names[s->count], sizeof(s->names[s->count]), fn);
    s->count++;
    return GK_OK;
}

int gk_dev_script_invoke(gk_dev_script_host *s, const char *fn)
{
    int i;
    if (s == NULL || fn == NULL) {
        return -1;
    }
    for (i = 0; i < s->count; i++) {
        if (gk__streq(s->names[i], fn)) {
            return i;
        }
    }
    return -1;
}

static gk_status gk__plugin_noop_init(void)
{
    return GK_OK;
}

static void gk__plugin_noop_shutdown(void)
{
}

int gk_dev_plugin_abi_compatible(const gk_dev_plugin *p)
{
    if (p == NULL) {
        return 0;
    }
    return p->abi_version == GK_DEV_PLUGIN_ABI_VERSION;
}

gk_status gk_dev_plugin_load(gk_dev_plugin *p)
{
    if (p == NULL || p->init == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!gk_dev_plugin_abi_compatible(p)) {
        return GK_ERR_UNSUPPORTED;
    }
    if (p->init() != GK_OK) {
        return GK_ERR_STATE;
    }
    if (p->shutdown == NULL) {
        p->shutdown = gk__plugin_noop_shutdown;
    }
    (void)gk__plugin_noop_init;
    return GK_OK;
}

void gk_dev_dylib_init(gk_dev_dylib *d)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
}

static int gk__dylib_find(const gk_dev_dylib *d, const char *path)
{
    int i;
    if (d == NULL || path == NULL) {
        return -1;
    }
    for (i = 0; i < d->count; i++) {
        if (gk__streq(d->paths[i], path)) {
            return i;
        }
    }
    return -1;
}

int gk_dev_dylib_open(gk_dev_dylib *d, const char *path)
{
    int idx;
    if (d == NULL || path == NULL) {
        return -1;
    }
    idx = gk__dylib_find(d, path);
    if (idx >= 0) {
        return idx;
    }
    if (d->count >= GK_DEV_MAX_ITEMS) {
        return -1;
    }
    gk__copy(d->paths[d->count], sizeof(d->paths[d->count]), path);
    d->handles[d->count] = (void *)(size_t)(d->count + 1);
    d->count++;
    return d->count - 1;
}

void *gk_dev_dylib_symbol(gk_dev_dylib *d, const char *path, const char *sym)
{
    int idx = gk__dylib_find(d, path);
    if (idx < 0 || sym == NULL) {
        return NULL;
    }
    /* mock: the handle doubles as the symbol address */
    return d->handles[idx];
}

gk_status gk_dev_dylib_close(gk_dev_dylib *d, const char *path)
{
    int idx = gk__dylib_find(d, path);
    if (idx < 0) {
        return GK_ERR_NOT_FOUND;
    }
    d->handles[idx] = NULL;
    d->paths[idx][0] = '\0';
    return GK_OK;
}

void gk_dev_config_init(gk_dev_config *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
}

static int gk__config_find(const gk_dev_config *c, const char *key)
{
    int i;
    if (c == NULL || key == NULL) {
        return -1;
    }
    for (i = 0; i < c->count; i++) {
        if (gk__streq(c->keys[i], key)) {
            return i;
        }
    }
    return -1;
}

gk_status gk_dev_config_set(gk_dev_config *c, const char *key,
                            const char *value)
{
    int idx;
    if (c == NULL || key == NULL || value == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    idx = gk__config_find(c, key);
    if (idx < 0) {
        if (c->count >= GK_DEV_MAX_ITEMS) {
            return GK_ERR_OVERFLOW;
        }
        idx = c->count;
        gk__copy(c->keys[idx], sizeof(c->keys[idx]), key);
        c->count++;
    }
    gk__copy(c->values[idx], sizeof(c->values[idx]), value);
    return GK_OK;
}

const char *gk_dev_config_get(const gk_dev_config *c, const char *key)
{
    int idx = gk__config_find(c, key);
    if (idx < 0) {
        return NULL;
    }
    return c->values[idx];
}

gk_status gk_dev_config_save(const gk_dev_config *c, char *out, size_t out_cap)
{
    int i;
    int off = 0;
    if (c == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    out[0] = '\0';
    for (i = 0; i < c->count; i++) {
        int n = snprintf(out + off, out_cap - (size_t)off, "%s=%s\n",
                         c->keys[i], c->values[i]);
        if (n < 0 || (size_t)n >= out_cap - (size_t)off) {
            return GK_ERR_OUT_OF_RANGE;
        }
        off += n;
    }
    return GK_OK;
}

gk_status gk_dev_config_load(gk_dev_config *c, const char *text)
{
    const char *p;
    if (c == NULL || text == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    p = text;
    while (*p != '\0') {
        const char *nl = strchr(p, '\n');
        const char *eq = strchr(p, '=');
        size_t line_len = (nl != NULL) ? (size_t)(nl - p) : strlen(p);
        if (eq != NULL && (nl == NULL || eq < nl)) {
            char key[GK_DEV_NAME];
            char val[GK_DEV_NAME];
            size_t klen = (size_t)(eq - p);
            size_t vlen = line_len - klen - 1;
            if (klen >= sizeof(key)) {
                klen = sizeof(key) - 1;
            }
            if (vlen >= sizeof(val)) {
                vlen = sizeof(val) - 1;
            }
            memcpy(key, p, klen);
            key[klen] = '\0';
            memcpy(val, eq + 1, vlen);
            val[vlen] = '\0';
            (void)gk_dev_config_set(c, key, val);
        }
        if (nl == NULL) {
            break;
        }
        p = nl + 1;
    }
    return GK_OK;
}

const char *gk_devlog_level_name(gk_devlog_level l)
{
    switch (l) {
    case GK_DEVLOG_TRACE: return "TRACE";
    case GK_DEVLOG_DEBUG: return "DEBUG";
    case GK_DEVLOG_INFO: return "INFO";
    case GK_DEVLOG_WARN: return "WARN";
    case GK_DEVLOG_ERROR: return "ERROR";
    default: return "UNKNOWN";
    }
}

void gk_devlog_sink_init(gk_devlog_sink *s, const char *target,
                         gk_devlog_level min_level)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    gk__copy(s->target, sizeof(s->target), target);
    s->min_level = min_level;
    s->max_bytes = 1024;
}

int gk_devlog_enabled(const gk_devlog_sink *s, gk_devlog_level l)
{
    if (s == NULL) {
        return 0;
    }
    return l >= s->min_level;
}

gk_status gk_devlog_write(gk_devlog_sink *s, gk_devlog_level l,
                          const char *message)
{
    size_t len;
    if (s == NULL || message == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!gk_devlog_enabled(s, l)) {
        return GK_OK;
    }
    len = strlen(message) + 2;
    s->bytes += len;
    s->written++;
    (void)gk_devlog_rotate(s);
    return GK_OK;
}

int gk_devlog_rotate(gk_devlog_sink *s)
{
    if (s == NULL) {
        return 0;
    }
    if (s->max_bytes > 0 && s->bytes >= s->max_bytes) {
        s->bytes = 0;
        s->rotated++;
        return 1;
    }
    return 0;
}

/* ===================================================================
 * Part C: packaging & deployment
 * =================================================================== */

const char *gk_dev_pkg_name(gk_dev_pkg_format f)
{
    switch (f) {
    case GK_DEV_PKG_MSI: return "msi";
    case GK_DEV_PKG_DEB: return "deb";
    case GK_DEV_PKG_DMG: return "dmg";
    case GK_DEV_PKG_PORTABLE: return "portable";
    default: return "unknown";
    }
}

const char *gk_dev_pkg_extension(gk_dev_pkg_format f)
{
    switch (f) {
    case GK_DEV_PKG_MSI: return ".msi";
    case GK_DEV_PKG_DEB: return ".deb";
    case GK_DEV_PKG_DMG: return ".dmg";
    case GK_DEV_PKG_PORTABLE: return ".zip";
    default: return "";
    }
}

gk_status gk_dev_pkg_manifest(gk_dev_pkg_format f, const char *product,
                              const char *version, char *out, size_t out_cap)
{
    int n;
    if (out == NULL || out_cap == 0 || product == NULL || version == NULL ||
        f >= GK_DEV_PKG_COUNT) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "package=%s-%s%s format=%s\n", product, version,
                 gk_dev_pkg_extension(f), gk_dev_pkg_name(f));
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

void gk_dev_update_init(gk_dev_update *u, const char *current)
{
    if (u == NULL) {
        return;
    }
    memset(u, 0, sizeof(*u));
    gk__copy(u->current, sizeof(u->current), current);
}

int gk_dev_version_compare(const char *a, const char *b)
{
    const char *ra = a;
    const char *rb = b;
    if (a == NULL || b == NULL) {
        return 0;
    }
    for (;;) {
        int va = 0, vb = 0;
        int da = 0, db = 0;
        while (*ra >= '0' && *ra <= '9') {
            va = va * 10 + (*ra - '0');
            ra++;
            da++;
        }
        while (*rb >= '0' && *rb <= '9') {
            vb = vb * 10 + (*rb - '0');
            rb++;
            db++;
        }
        if (va != vb) {
            return va < vb ? -1 : 1;
        }
        if (da == 0 && db == 0) {
            break;
        }
        if (*ra == '.') {
            ra++;
        }
        if (*rb == '.') {
            rb++;
        }
    }
    return 0;
}

int gk_dev_update_available(gk_dev_update *u, const char *latest)
{
    if (u == NULL || latest == NULL) {
        return 0;
    }
    gk__copy(u->latest, sizeof(u->latest), latest);
    return gk_dev_version_compare(latest, u->current) > 0;
}

void gk_dev_ci_init(gk_dev_ci_pipeline *p)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
}

int gk_dev_ci_add_stage(gk_dev_ci_pipeline *p, const char *name)
{
    if (p == NULL || name == NULL || p->count >= GK_DEV_MAX_STAGES) {
        return -1;
    }
    gk__copy(p->stages[p->count].name, sizeof(p->stages[p->count].name), name);
    p->count++;
    return p->count;
}

int gk_dev_ci_stage_fail(gk_dev_ci_pipeline *p, const char *name)
{
    int i;
    if (p == NULL || name == NULL) {
        return 0;
    }
    for (i = 0; i < p->count; i++) {
        if (gk__streq(p->stages[i].name, name)) {
            p->stages[i].failed = 1;
            return 1;
        }
    }
    return 0;
}

int gk_dev_ci_run(gk_dev_ci_pipeline *p)
{
    int i;
    int failed = 0;
    if (p == NULL) {
        return 0;
    }
    p->failed = 0;
    for (i = 0; i < p->count; i++) {
        if (!p->stages[i].failed) {
            p->stages[i].seconds = 1.0;
        }
    }
    for (i = 0; i < p->count; i++) {
        if (p->stages[i].failed) {
            failed++;
        }
    }
    p->failed = failed;
    return failed;
}

gk_status gk_dev_docker_build(const char *image, const char *base,
                              const char *cmd, char *out, size_t out_cap)
{
    int n;
    if (image == NULL || base == NULL || cmd == NULL || out == NULL ||
        out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "FROM %s\nCOPY . /app\nCMD [\"%s\"]\n", base,
                 cmd);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    (void)image;
    return GK_OK;
}

void gk_dev_cloud_init(gk_dev_cloud *c, const char *provider)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    gk__copy(c->provider, sizeof(c->provider), provider);
}

gk_status gk_dev_cloud_deploy(gk_dev_cloud *c, const char *region,
                              int replicas)
{
    if (c == NULL || region == NULL || replicas <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    gk__copy(c->region, sizeof(c->region), region);
    c->replicas = replicas;
    c->deployed = 1;
    return GK_OK;
}

void gk_dev_mesh_init(gk_dev_mesh *m)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
}

static int gk__mesh_find(const gk_dev_mesh *m, const char *name)
{
    int i;
    if (m == NULL || name == NULL) {
        return -1;
    }
    for (i = 0; i < m->count; i++) {
        if (gk__streq(m->services[i].name, name)) {
            return i;
        }
    }
    return -1;
}

int gk_dev_mesh_register(gk_dev_mesh *m, const char *name, int port)
{
    int idx;
    if (m == NULL || name == NULL || port <= 0) {
        return -1;
    }
    idx = gk__mesh_find(m, name);
    if (idx >= 0) {
        m->services[idx].port = port;
        return idx;
    }
    if (m->count >= GK_DEV_MAX_ITEMS) {
        return -1;
    }
    gk__copy(m->services[m->count].name, sizeof(m->services[m->count].name),
             name);
    m->services[m->count].port = port;
    m->services[m->count].healthy = 1;
    m->count++;
    return m->count - 1;
}

int gk_dev_mesh_health(gk_dev_mesh *m, const char *name, int healthy)
{
    int idx = gk__mesh_find(m, name);
    if (idx < 0) {
        return -1;
    }
    m->services[idx].healthy = healthy ? 1 : 0;
    return 1;
}

int gk_dev_mesh_healthy_count(const gk_dev_mesh *m)
{
    int i;
    int n = 0;
    if (m == NULL) {
        return 0;
    }
    for (i = 0; i < m->count; i++) {
        if (m->services[i].healthy) {
            n++;
        }
    }
    return n;
}

/* ===================================================================
 * Part D: licensing & editions
 * =================================================================== */

const char *gk_lic_edition_name(gk_lic_edition e)
{
    switch (e) {
    case GK_LIC_FREE: return "free";
    case GK_LIC_PROFESSIONAL: return "professional";
    case GK_LIC_EDUCATION: return "education";
    case GK_LIC_ENTERPRISE: return "enterprise";
    default: return "unknown";
    }
}

int gk_lic_edition_tier(gk_lic_edition e)
{
    switch (e) {
    case GK_LIC_FREE: return 0;
    case GK_LIC_EDUCATION: return 1;
    case GK_LIC_PROFESSIONAL: return 2;
    case GK_LIC_ENTERPRISE: return 3;
    default: return -1;
    }
}

static unsigned short gk__lic_checksum(const char *s)
{
    unsigned short sum = 0x1234;
    size_t i;
    if (s == NULL) {
        return 0;
    }
    for (i = 0; s[i] != '\0'; i++) {
        sum = (unsigned short)((sum << 1) ^ (unsigned char)s[i]);
    }
    return sum;
}

gk_status gk_lic_generate_serial(gk_lic_edition e, unsigned int seed,
                                 char *out, size_t out_cap)
{
    unsigned int mix;
    char body[GK_DEV_NAME];
    unsigned short chk;
    int n;
    if (out == NULL || out_cap == 0 || e >= GK_LIC_EDITION_COUNT) {
        return GK_ERR_INVALID_ARG;
    }
    mix = (seed * 2654435761u) ^ ((unsigned int)e << 24);
    n = snprintf(body, sizeof(body), "%s-%08X", gk_lic_edition_name(e), mix);
    if (n < 0 || (size_t)n >= sizeof(body)) {
        return GK_ERR_OUT_OF_RANGE;
    }
    chk = gk__lic_checksum(body);
    n = snprintf(out, out_cap, "%s-%04X", body, chk);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

int gk_lic_verify_serial(const char *serial, gk_lic_edition e)
{
    const char *chk_dash;
    char body[GK_DEV_NAME];
    const char *body_dash;
    unsigned int given = 0;
    unsigned short expected;
    size_t blen;
    if (serial == NULL || e >= GK_LIC_EDITION_COUNT) {
        return 0;
    }
    chk_dash = strrchr(serial, '-');
    if (chk_dash == NULL || strlen(chk_dash + 1) != 4) {
        return 0;
    }
    blen = (size_t)(chk_dash - serial);
    if (blen == 0 || blen >= sizeof(body)) {
        return 0;
    }
    memcpy(body, serial, blen);
    body[blen] = '\0';
    body_dash = strrchr(body, '-');
    if (body_dash == NULL) {
        return 0;
    }
    /* check the edition name prefix */
    {
        size_t plen = (size_t)(body_dash - body);
        size_t elen = strlen(gk_lic_edition_name(e));
        if (plen != elen || strncmp(body, gk_lic_edition_name(e), plen) != 0) {
            return 0;
        }
    }
    if (sscanf(chk_dash + 1, "%4X", &given) != 1) {
        return 0;
    }
    expected = gk__lic_checksum(body);
    return given == (unsigned int)expected;
}

void gk_lic_set_serial(gk_license *l, const char *serial)
{
    if (l == NULL) {
        return;
    }
    gk__copy(l->serial, sizeof(l->serial), serial);
}

void gk_lic_init(gk_license *l, gk_lic_edition e, gk_lic_mode m)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
    l->edition = e;
    l->mode = m;
    l->state = GK_LIC_STATE_INACTIVE;
    l->seats = 1;
    if (m == GK_LIC_MODE_TRIAL) {
        l->state = GK_LIC_STATE_ACTIVE;
        l->expiry_day = 30;
    }
}

gk_status gk_lic_activate_online(gk_license *l, const char *activation_key)
{
    if (l == NULL || activation_key == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!gk_lic_verify_serial(activation_key, l->edition)) {
        return GK_ERR_NOT_FOUND;
    }
    gk_lic_set_serial(l, activation_key);
    l->state = GK_LIC_STATE_ACTIVE;
    l->offline = 0;
    return GK_OK;
}

gk_status gk_lic_offline_request(const char *serial, char *out, size_t cap)
{
    int n;
    if (serial == NULL || out == NULL || cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, cap, "REQ:%s", serial);
    if (n < 0 || (size_t)n >= cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

gk_status gk_lic_activate_offline(gk_license *l, const char *request,
                                  const char *response)
{
    if (l == NULL || request == NULL || response == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (strncmp(response, "RESP:", 5) != 0) {
        return GK_ERR_PARSE;
    }
    if (strstr(request, l->serial) == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    l->state = GK_LIC_STATE_ACTIVE;
    l->offline = 1;
    return GK_OK;
}

int gk_lic_is_valid(const gk_license *l)
{
    if (l == NULL) {
        return 0;
    }
    if (l->state != GK_LIC_STATE_ACTIVE) {
        return 0;
    }
    if (l->expiry_day > 0 && l->today >= l->expiry_day) {
        return 0;
    }
    return 1;
}

gk_status gk_lic_track_usage(gk_license *l, double hours)
{
    if (l == NULL || hours < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    /* accumulating usage advances the accounting day in 24h increments */
    l->today += (long)(hours / 24.0);
    return GK_OK;
}

long gk_lic_days_to_renewal(const gk_license *l)
{
    if (l == NULL || l->expiry_day <= 0) {
        return -1;
    }
    return l->expiry_day - l->today;
}

int gk_lic_feature_allowed(const gk_license *l, int feature_tier)
{
    if (l == NULL || !gk_lic_is_valid(l)) {
        return feature_tier == 0;
    }
    return feature_tier <= gk_lic_edition_tier(l->edition);
}

int gk_lic_seat_available(const gk_license *l)
{
    if (l == NULL) {
        return 0;
    }
    return l->used_seats < l->seats;
}

int gk_lic_time_expired(const gk_license *l)
{
    if (l == NULL) {
        return 1;
    }
    if (l->expiry_day <= 0) {
        return 0;
    }
    return l->today >= l->expiry_day;
}

gk_status gk_lic_watermark(const gk_license *l, char *out, size_t out_cap)
{
    int n;
    const char *text;
    if (l == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    if (!gk_lic_is_valid(l) || l->mode == GK_LIC_MODE_TRIAL) {
        text = "EVALUATION COPY - NOT FOR PRODUCTION";
    } else {
        text = "";
    }
    n = snprintf(out, out_cap, "%s", text);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

int gk_lic_integrity_ok(const unsigned char *data, size_t len,
                        unsigned long checksum)
{
    unsigned long sum = 2166136261u;
    size_t i;
    if (data == NULL && len > 0) {
        return 0;
    }
    for (i = 0; i < len; i++) {
        sum = (sum ^ data[i]) * 16777619u;
    }
    return sum == checksum;
}

void gk_lic_dongle_init(gk_lic_dongle *d, unsigned long expected_id)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    d->expected_id = expected_id;
}

gk_status gk_lic_dongle_plug(gk_lic_dongle *d, unsigned long dongle_id)
{
    if (d == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    d->present = 1;
    d->dongle_id = dongle_id;
    return GK_OK;
}

int gk_lic_dongle_valid(const gk_lic_dongle *d)
{
    if (d == NULL || !d->present) {
        return 0;
    }
    return d->dongle_id == d->expected_id;
}

void gk_lic_server_init(gk_lic_server *s, int max_seats)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->max_seats = max_seats;
}

gk_status gk_lic_server_checkout(gk_lic_server *s)
{
    if (s == NULL || !s->online) {
        return GK_ERR_STATE;
    }
    if (s->checked_out >= s->max_seats) {
        return GK_ERR_OVERFLOW;
    }
    s->checked_out++;
    return GK_OK;
}

gk_status gk_lic_server_checkin(gk_lic_server *s)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (s->checked_out <= 0) {
        return GK_ERR_STATE;
    }
    s->checked_out--;
    return GK_OK;
}
