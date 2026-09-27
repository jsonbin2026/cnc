#include "gk/gk_file.h"

#include <stdarg.h>
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

static int gk__append(char *buf, size_t len, int off, const char *s)
{
    size_t l;
    if (buf == NULL || s == NULL || off < 0) {
        return off;
    }
    l = strlen(s);
    if ((size_t)off + l >= len) {
        l = len > (size_t)off ? len - (size_t)off - 1 : 0;
    }
    memcpy(buf + off, s, l);
    off += (int)l;
    buf[off] = '\0';
    return off;
}

static int gk__append_fmt(char *buf, size_t len, int off, const char *fmt, ...)
{
    char tmp[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    return gk__append(buf, len, off, tmp);
}

void gk_file_manager_init(gk_file_manager *m)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    m->active = -1;
}

static void gk__push_recent(gk_file_manager *m, const char *path)
{
    int i;
    if (m == NULL || path == NULL) {
        return;
    }
    for (i = 0; i < m->recent_count; ++i) {
        if (strcmp(m->recent_path[i], path) == 0) {
            int k;
            for (k = i; k > 0; --k) {
                gk__copy(m->recent_path[k], sizeof(m->recent_path[k]),
                         m->recent_path[k - 1]);
            }
            gk__copy(m->recent_path[0], sizeof(m->recent_path[0]), path);
            return;
        }
    }
    if (m->recent_count >= GK_FILE_MAX_RECENT) {
        m->recent_count = GK_FILE_MAX_RECENT - 1;
    }
    for (i = m->recent_count; i > 0; --i) {
        gk__copy(m->recent_path[i], sizeof(m->recent_path[i]),
                 m->recent_path[i - 1]);
    }
    gk__copy(m->recent_path[0], sizeof(m->recent_path[0]), path);
    m->recent_count++;
}

gk_status gk_file_new(gk_file_manager *m, const char *name)
{
    gk_nc_file *f;
    if (m == NULL || name == NULL || m->count >= GK_FILE_GROUP) {
        return GK_ERR_INVALID_ARG;
    }
    f = &m->files[m->count];
    memset(f, 0, sizeof(*f));
    gk__copy(f->name, sizeof(f->name), name);
    f->number = m->count;
    m->count++;
    m->active = m->count - 1;
    return GK_OK;
}

gk_status gk_file_open(gk_file_manager *m, const char *path)
{
    const char *base;
    if (m == NULL || path == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (m->count >= GK_FILE_GROUP) {
        return GK_ERR_OUT_OF_RANGE;
    }
    base = strrchr(path, '/');
    base = base != NULL ? base + 1 : path;
    {
        gk_nc_file *f = &m->files[m->count];
        memset(f, 0, sizeof(*f));
        gk__copy(f->name, sizeof(f->name), base);
        gk__copy(f->path, sizeof(f->path), path);
        f->number = m->count;
        m->count++;
        m->active = m->count - 1;
    }
    gk__push_recent(m, path);
    return GK_OK;
}

gk_status gk_file_close(gk_file_manager *m, int index)
{
    int i;
    if (m == NULL || index < 0 || index >= m->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    for (i = index; i + 1 < m->count; ++i) {
        m->files[i] = m->files[i + 1];
    }
    m->count--;
    if (m->active >= m->count) {
        m->active = m->count - 1;
    }
    return GK_OK;
}

static gk_nc_file *gk__active(gk_file_manager *m)
{
    if (m == NULL || m->active < 0 || m->active >= m->count) {
        return NULL;
    }
    return &m->files[m->active];
}

gk_status gk_file_save(gk_file_manager *m, const char *content)
{
    gk_nc_file *f = gk__active(m);
    if (f == NULL || content == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    gk__copy(f->content, sizeof(f->content), content);
    f->size = (int)strlen(f->content);
    f->dirty = 0;
    return GK_OK;
}

gk_status gk_file_save_as(gk_file_manager *m, const char *path)
{
    gk_nc_file *f = gk__active(m);
    const char *base;
    if (f == NULL || path == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    gk__copy(f->path, sizeof(f->path), path);
    base = strrchr(path, '/');
    base = base != NULL ? base + 1 : path;
    gk__copy(f->name, sizeof(f->name), base);
    gk__push_recent(m, path);
    return GK_OK;
}

int gk_file_recent_count(const gk_file_manager *m)
{
    return m != NULL ? m->recent_count : 0;
}

const char *gk_file_recent(const gk_file_manager *m, int i)
{
    if (m == NULL || i < 0 || i >= m->recent_count) {
        return NULL;
    }
    return m->recent_path[i];
}

gk_status gk_file_set_onumber(gk_file_manager *m, int index, int number)
{
    if (m == NULL || index < 0 || index >= m->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    if (number < 0) {
        return GK_ERR_INVALID_ARG;
    }
    m->files[index].number = number;
    return GK_OK;
}

int gk_file_find_by_onumber(const gk_file_manager *m, int number)
{
    int i;
    if (m == NULL) {
        return -1;
    }
    for (i = 0; i < m->count; ++i) {
        if (m->files[i].number == number) {
            return i;
        }
    }
    return -1;
}

/* ---------------- export ---------------- */

void gk_toolpath_init(gk_toolpath *tp)
{
    if (tp != NULL) {
        memset(tp, 0, sizeof(*tp));
    }
}

gk_status gk_toolpath_add(gk_toolpath *tp, double x, double y, double z,
                          int tool)
{
    gk_toolpath_point *p;
    if (tp == NULL || tp->count >= 512) {
        return GK_ERR_OUT_OF_RANGE;
    }
    p = &tp->points[tp->count++];
    p->x = x;
    p->y = y;
    p->z = z;
    p->tool = tool;
    return GK_OK;
}

int gk_export_toolpath(const gk_toolpath *tp, char *buf, size_t len)
{
    int off = 0;
    int i;
    if (tp == NULL || buf == NULL) {
        return 0;
    }
    buf[0] = '\0';
    off = gk__append(buf, len, off, "TOOLPATH\n");
    for (i = 0; i < tp->count; ++i) {
        off = gk__append_fmt(buf, len, off, "T%d X%.3f Y%.3f Z%.3f\n",
                             tp->points[i].tool, tp->points[i].x,
                             tp->points[i].y, tp->points[i].z);
    }
    off = gk__append_fmt(buf, len, off, "POINTS %d\n", tp->count);
    return off;
}

int gk_export_report(const char *title, const gk_toolpath *tp, char *buf,
                     size_t len)
{
    int off = 0;
    if (buf == NULL) {
        return 0;
    }
    buf[0] = '\0';
    off = gk__append(buf, len, off, "=== MACHINING REPORT ===\n");
    off = gk__append_fmt(buf, len, off, "Title: %s\n",
                         title != NULL ? title : "(untitled)");
    off = gk__append_fmt(buf, len, off, "Toolpath points: %d\n",
                         tp != NULL ? tp->count : 0);
    off = gk__append(buf, len, off, "END REPORT\n");
    return off;
}

int gk_export_csv(const gk_toolpath *tp, char *buf, size_t len)
{
    int off = 0;
    int i;
    if (tp == NULL || buf == NULL) {
        return 0;
    }
    buf[0] = '\0';
    off = gk__append(buf, len, off, "x,y,z,tool\n");
    for (i = 0; i < tp->count; ++i) {
        off = gk__append_fmt(buf, len, off, "%.3f,%.3f,%.3f,%d\n",
                             tp->points[i].x, tp->points[i].y,
                             tp->points[i].z, tp->points[i].tool);
    }
    return off;
}

int gk_export_json(const gk_toolpath *tp, char *buf, size_t len)
{
    int off = 0;
    int i;
    if (tp == NULL || buf == NULL) {
        return 0;
    }
    buf[0] = '\0';
    off = gk__append(buf, len, off, "{\"points\":[");
    for (i = 0; i < tp->count; ++i) {
        off = gk__append_fmt(buf, len, off,
                             "%s{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f,\"tool\":%d}",
                             i > 0 ? "," : "", tp->points[i].x,
                             tp->points[i].y, tp->points[i].z,
                             tp->points[i].tool);
    }
    off = gk__append_fmt(buf, len, off, "],\"count\":%d}", tp->count);
    return off;
}

int gk_export_pdf(const char *title, char *buf, size_t len)
{
    int off = 0;
    if (buf == NULL) {
        return 0;
    }
    buf[0] = '\0';
    off = gk__append(buf, len, off, "%PDF-1.4\n");
    off = gk__append(buf, len, off, "1 0 obj << /Type /Catalog >> endobj\n");
    off = gk__append_fmt(buf, len, off, "%% %s\n",
                         title != NULL ? title : "report");
    off = gk__append(buf, len, off, "%%EOF\n");
    return off;
}

/* ---------------- recording ---------------- */

void gk_recording_init(gk_recording *r, int fps)
{
    if (r == NULL) {
        return;
    }
    memset(r, 0, sizeof(*r));
    r->fps = fps > 0 ? fps : 30;
}

gk_status gk_recording_add(gk_recording *r, double time, const char *label)
{
    gk_record_frame *f;
    if (r == NULL || r->count >= 1024) {
        return GK_ERR_OUT_OF_RANGE;
    }
    f = &r->frames[r->count++];
    f->frame = r->count - 1;
    f->time = time;
    gk__copy(f->label, sizeof(f->label), label);
    return GK_OK;
}

double gk_recording_duration(const gk_recording *r)
{
    if (r == NULL || r->count == 0) {
        return 0.0;
    }
    return r->frames[r->count - 1].time - r->frames[0].time;
}

int gk_recording_export_mp4(const gk_recording *r, char *buf, size_t len)
{
    int off = 0;
    if (r == NULL || buf == NULL) {
        return 0;
    }
    buf[0] = '\0';
    off = gk__append_fmt(buf, len, off,
                         "MP4|fps=%d|frames=%d|dur=%.3f\n",
                         r->fps, r->count, gk_recording_duration(r));
    return off;
}

/* ---------------- screenshot ---------------- */

gk_status gk_screenshot_capture(gk_screenshot *s, int w, int h,
                                unsigned char fill)
{
    int i;
    if (s == NULL || w <= 0 || h <= 0 || w > 64 || h > 64) {
        return GK_ERR_OUT_OF_RANGE;
    }
    s->width = w;
    s->height = h;
    memset(s->pixels, fill, sizeof(s->pixels));
    for (i = 0; i < w * h; ++i) {
        s->pixels[i * 4 + 3] = 255;   /* opaque alpha */
    }
    return GK_OK;
}

int gk_screenshot_save_png(const gk_screenshot *s, char *buf, size_t len)
{
    int off = 0;
    if (s == NULL || buf == NULL) {
        return 0;
    }
    buf[0] = '\0';
    off = gk__append(buf, len, off, "\x89PNG\r\n\x1a\n");
    off = gk__append_fmt(buf, len, off, "IHDR %dx%d\n", s->width, s->height);
    off = gk__append(buf, len, off, "IEND\n");
    return off;
}

/* ---------------- save states ---------------- */

void gk_save_states_init(gk_save_states *s)
{
    if (s != NULL) {
        memset(s, 0, sizeof(*s));
    }
}

gk_status gk_save_state(gk_save_states *s, const char *name,
                        const gk_machine_state *st)
{
    int i;
    if (s == NULL || name == NULL || st == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < s->count; ++i) {
        if (strcmp(s->slots[i].name, name) == 0) {
            s->slots[i].state = *st;
            return GK_OK;
        }
    }
    if (s->count >= 16) {
        return GK_ERR_OUT_OF_RANGE;
    }
    gk__copy(s->slots[s->count].name, sizeof(s->slots[s->count].name), name);
    s->slots[s->count].state = *st;
    s->count++;
    return GK_OK;
}

gk_status gk_load_state(const gk_save_states *s, const char *name,
                        gk_machine_state *out)
{
    int i;
    if (s == NULL || name == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < s->count; ++i) {
        if (strcmp(s->slots[i].name, name) == 0) {
            *out = s->slots[i].state;
            return GK_OK;
        }
    }
    return GK_ERR_NOT_FOUND;
}

/* ---------------- transfer ---------------- */

const char *gk_transfer_kind_name(gk_transfer_kind k)
{
    switch (k) {
    case GK_XFER_DNC: return "dnc";
    case GK_XFER_SERIAL: return "serial";
    case GK_XFER_USB: return "usb";
    case GK_XFER_NETWORK: return "network";
    case GK_XFER_CLOUD: return "cloud";
    default: return "unknown";
    }
}

gk_status gk_transfer_init(gk_transfer *t, gk_transfer_kind kind)
{
    if (t == NULL || kind < 0 || kind >= GK_XFER_COUNT) {
        return GK_ERR_INVALID_ARG;
    }
    memset(t, 0, sizeof(*t));
    t->kind = kind;
    t->baud = kind == GK_XFER_SERIAL ? 9600.0 : 115200.0;
    t->bytes_total = 1024;
    return GK_OK;
}

gk_status gk_transfer_connect(gk_transfer *t)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    t->connected = 1;
    return GK_OK;
}

gk_status gk_transfer_disconnect(gk_transfer *t)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    t->connected = 0;
    return GK_OK;
}

gk_status gk_transfer_step(gk_transfer *t, double elapsed)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!t->connected) {
        return GK_ERR_STATE;
    }
    if (t->baud <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    t->bytes_sent += (int)(t->baud / 8.0 * elapsed);
    if (t->bytes_sent >= t->bytes_total) {
        t->bytes_sent = t->bytes_total;
        t->progress = 1.0;
    } else {
        t->progress = (double)t->bytes_sent / (double)t->bytes_total;
    }
    return GK_OK;
}

int gk_transfer_done(const gk_transfer *t)
{
    return t != NULL && t->bytes_total > 0 && t->bytes_sent >= t->bytes_total;
}

/* ---------------- auto-save / history / encryption ---------------- */

void gk_autosave_init(gk_autosave *a, double interval)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    a->enabled = 1;
    a->interval = interval > 0.0 ? interval : 60.0;
}

int gk_autosave_tick(gk_autosave *a, double dt)
{
    if (a == NULL || !a->enabled || a->interval <= 0.0) {
        return 0;
    }
    a->accumulator += dt;
    if (a->accumulator >= a->interval) {
        a->accumulator -= a->interval;
        a->saves++;
        return 1;
    }
    return 0;
}

void gk_history_init(gk_history *h)
{
    if (h != NULL) {
        memset(h, 0, sizeof(*h));
    }
}

gk_status gk_history_commit(gk_history *h, const char *author,
                            const char *content)
{
    gk_version *v;
    if (h == NULL || content == NULL || h->count >= GK_FILE_MAX_VERSIONS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    v = &h->versions[h->count++];
    memset(v, 0, sizeof(*v));
    v->revision = h->count;
    gk__copy(v->author, sizeof(v->author), author);
    gk__copy(v->content, sizeof(v->content), content);
    v->size = (int)strlen(v->content);
    return GK_OK;
}

int gk_history_count(const gk_history *h)
{
    return h != NULL ? h->count : 0;
}

const gk_version *gk_history_at(const gk_history *h, int i)
{
    if (h == NULL || i < 0 || i >= h->count) {
        return NULL;
    }
    return &h->versions[i];
}

gk_status gk_history_rollback(gk_history *h, int revision, char *out,
                              size_t len)
{
    int i;
    if (h == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < h->count; ++i) {
        if (h->versions[i].revision == revision) {
            gk__copy(out, len, h->versions[i].content);
            return GK_OK;
        }
    }
    return GK_ERR_NOT_FOUND;
}

void gk_file_encrypt(const char *in, size_t len, unsigned char key,
                     char *out)
{
    size_t i;
    if (in == NULL || out == NULL) {
        return;
    }
    for (i = 0; i < len; ++i) {
        out[i] = (char)((unsigned char)in[i] ^ key);
    }
}

void gk_file_decrypt(const char *in, size_t len, unsigned char key,
                     char *out)
{
    gk_file_encrypt(in, len, key, out);
}

int gk_file_is_encrypted(const char *buf, size_t len)
{
    size_t i;
    if (buf == NULL || len == 0) {
        return 0;
    }
    for (i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)buf[i];
        if (c == 0 || (c < 9) || (c > 13 && c < 32)) {
            return 1;
        }
    }
    return 0;
}
