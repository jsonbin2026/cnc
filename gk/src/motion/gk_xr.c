#include "gk/gk_xr.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define GK_XR_DEG2RAD (3.14159265358979323846 / 180.0)

static void gk_xr_copy(char *dst, const char *src, size_t cap)
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

static float clampf(float v, float lo, float hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

/* ================= device names ================= */

const char *gk_xr_device_name(gk_xr_device_type t)
{
    switch (t) {
    case GK_XR_DEVICE_NONE: return "none";
    case GK_XR_DEVICE_HEADSET: return "headset";
    case GK_XR_DEVICE_GLASSES: return "ar-glasses";
    case GK_XR_DEVICE_PHONE: return "phone";
    case GK_XR_DEVICE_CONTROLLER: return "controller";
    default: return "unknown";
    }
}

const char *gk_xr_pad_name(gk_xr_pad_type t)
{
    switch (t) {
    case GK_XR_PAD_XBOX: return "xbox";
    case GK_XR_PAD_PS: return "playstation";
    case GK_XR_PAD_VR: return "vr-controller";
    case GK_XR_PAD_GENERIC: return "generic";
    default: return "unknown";
    }
}

/* ================= session / 731-736 ================= */

gk_status gk_xr_init(gk_xr_session *s)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    memset(s, 0, sizeof(*s));
    s->initialized = 1;
    s->predicted_display_time = 0.0;
    return GK_OK;
}

gk_status gk_xr_session_begin(gk_xr_session *s)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!s->initialized) {
        return GK_ERR_STATE;
    }
    s->session_active = 1;
    s->frame_count = 0;
    return GK_OK;
}

gk_status gk_xr_session_end(gk_xr_session *s)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!s->session_active) {
        return GK_ERR_STATE;
    }
    s->session_active = 0;
    return GK_OK;
}

gk_status gk_xr_frame_begin(gk_xr_session *s, double predicted_time)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!s->session_active) {
        return GK_ERR_STATE;
    }
    s->predicted_display_time = predicted_time;
    s->frame_count++;
    return GK_OK;
}

int gk_xr_device_add(gk_xr_session *s, gk_xr_device_type type,
                     const char *name)
{
    gk_xr_device *d;
    if (s == NULL || type == GK_XR_DEVICE_NONE || type >= GK_XR_DEVICE_COUNT) {
        return -1;
    }
    if (s->device_count >= GK_XR_MAX_DEVICES) {
        return -1;
    }
    d = &s->devices[s->device_count];
    memset(d, 0, sizeof(*d));
    d->id = s->device_count + 1;
    d->type = type;
    gk_xr_copy(d->name, name, GK_XR_NAME);
    d->connected = 1;
    d->tracked = 0;
    s->device_count++;
    return d->id;
}

gk_status gk_xr_device_set_pose(gk_xr_session *s, int id,
                                gk_xr_vec3 pos, gk_xr_quat rot)
{
    gk_xr_device *d;
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    d = gk_xr_device_find(s, id);
    if (d == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    if (!d->connected) {
        return GK_ERR_STATE;
    }
    d->position = pos;
    d->orientation = rot;
    d->tracked = 1;
    return GK_OK;
}

gk_status gk_xr_device_disconnect(gk_xr_session *s, int id)
{
    gk_xr_device *d;
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    d = gk_xr_device_find(s, id);
    if (d == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    d->connected = 0;
    d->tracked = 0;
    return GK_OK;
}

gk_xr_device *gk_xr_device_find(gk_xr_session *s, int id)
{
    int i;
    if (s == NULL) {
        return NULL;
    }
    for (i = 0; i < s->device_count; i++) {
        if (s->devices[i].id == id) {
            return &s->devices[i];
        }
    }
    return NULL;
}

int gk_xr_device_count(const gk_xr_session *s, gk_xr_device_type type)
{
    int i;
    int n = 0;
    if (s == NULL) {
        return 0;
    }
    for (i = 0; i < s->device_count; i++) {
        if (s->devices[i].type == type && s->devices[i].connected) {
            n++;
        }
    }
    return n;
}

gk_status gk_xr_locate_relative(const gk_xr_session *s, int device_id,
                                gk_xr_vec3 *out_pos)
{
    const gk_xr_device *head = NULL;
    const gk_xr_device *dev = NULL;
    int i;
    if (s == NULL || out_pos == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < s->device_count; i++) {
        if (s->devices[i].type == GK_XR_DEVICE_HEADSET &&
            s->devices[i].connected) {
            head = &s->devices[i];
            break;
        }
    }
    for (i = 0; i < s->device_count; i++) {
        if (s->devices[i].id == device_id) {
            dev = &s->devices[i];
            break;
        }
    }
    if (head == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    if (dev == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    out_pos->x = dev->position.x - head->position.x;
    out_pos->y = dev->position.y - head->position.y;
    out_pos->z = dev->position.z - head->position.z;
    return GK_OK;
}

/* ================= gamepad / 733,742,743 ================= */

void gk_xr_gamepad_init(gk_xr_gamepad *p, gk_xr_pad_type type)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->type = type;
    p->axis_count = 6;
    p->trigger_l = 0.0f;
    p->trigger_r = 0.0f;
}

gk_status gk_xr_gamepad_set_axis(gk_xr_gamepad *p, int index, float value)
{
    if (p == NULL || index < 0 || index >= GK_XR_MAX_CHANNELS) {
        return GK_ERR_INVALID_ARG;
    }
    p->axes[index] = clampf(value, -1.0f, 1.0f);
    if (index == 4) {
        p->trigger_l = (p->axes[index] + 1.0f) * 0.5f;
    } else if (index == 5) {
        p->trigger_r = (p->axes[index] + 1.0f) * 0.5f;
    }
    return GK_OK;
}

gk_status gk_xr_gamepad_set_button(gk_xr_gamepad *p, int index, int down)
{
    unsigned mask;
    if (p == NULL || index < 0 || index >= 32) {
        return GK_ERR_INVALID_ARG;
    }
    mask = 1u << index;
    if (down) {
        p->buttons |= mask;
    } else {
        p->buttons &= ~mask;
    }
    return GK_OK;
}

int gk_xr_gamepad_button_down(const gk_xr_gamepad *p, int index)
{
    if (p == NULL || index < 0 || index >= 32) {
        return 0;
    }
    return (p->buttons & (1u << index)) != 0;
}

int gk_xr_gamepad_button_pressed(const gk_xr_gamepad *p, int index)
{
    unsigned mask;
    if (p == NULL || index < 0 || index >= 32) {
        return 0;
    }
    mask = 1u << index;
    return (p->buttons & mask) != 0 && (p->prev_buttons & mask) == 0;
}

int gk_xr_gamepad_button_released(const gk_xr_gamepad *p, int index)
{
    unsigned mask;
    if (p == NULL || index < 0 || index >= 32) {
        return 0;
    }
    mask = 1u << index;
    return (p->buttons & mask) == 0 && (p->prev_buttons & mask) != 0;
}

void gk_xr_gamepad_set_rumble(gk_xr_gamepad *p, float left, float right)
{
    if (p == NULL) {
        return;
    }
    p->rumble_l = clampf(left, 0.0f, 1.0f);
    p->rumble_r = clampf(right, 0.0f, 1.0f);
}

void gk_xr_gamepad_next_frame(gk_xr_gamepad *p)
{
    if (p == NULL) {
        return;
    }
    p->prev_buttons = p->buttons;
}

/* ================= AR layer / 734,736 ================= */

void gk_xr_ar_init(gk_xr_ar_layer *a)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    a->overlay_enabled = 1;
}

int gk_xr_ar_anchor_add(gk_xr_ar_layer *a, const char *label,
                        gk_xr_vec3 pos, float size)
{
    gk_xr_anchor *an;
    if (a == NULL || a->count >= GK_XR_MAX_ANCHORS) {
        return -1;
    }
    an = &a->anchors[a->count];
    memset(an, 0, sizeof(*an));
    an->id = a->count + 1;
    gk_xr_copy(an->label, label, GK_XR_NAME);
    an->world_pos = pos;
    an->size = size;
    an->visible = 1;
    a->count++;
    return an->id;
}

gk_status gk_xr_ar_anchor_set_visible(gk_xr_ar_layer *a, int id, int visible)
{
    int i;
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < a->count; i++) {
        if (a->anchors[i].id == id) {
            a->anchors[i].visible = visible ? 1 : 0;
            return GK_OK;
        }
    }
    return GK_ERR_NOT_FOUND;
}

int gk_xr_ar_project(const gk_xr_ar_layer *a, int id, float fov_deg, float aspect,
                     float *out_x, float *out_y)
{
    int i;
    if (a == NULL || out_x == NULL || out_y == NULL || fov_deg <= 0.0f) {
        return 0;
    }
    for (i = 0; i < a->count; i++) {
        if (a->anchors[i].id == id) {
            const gk_xr_anchor *an = &a->anchors[i];
            float half;
            if (an->world_pos.z <= 0.001f) {
                return 0; /* behind the camera */
            }
            half = tanf(fov_deg * 0.5f * (float)GK_XR_DEG2RAD);
            if (half <= 0.0f) {
                return 0;
            }
            *out_x = (an->world_pos.x / (an->world_pos.z * half * aspect));
            *out_y = (an->world_pos.y / (an->world_pos.z * half));
            return 1;
        }
    }
    return 0;
}

gk_status gk_xr_phone_ar_pose(float marker_pixel_size, float known_size_mm,
                              float focal_px, gk_xr_vec3 *out_translation)
{
    float ratio;
    if (out_translation == NULL || focal_px <= 0.0f) {
        return GK_ERR_INVALID_ARG;
    }
    if (marker_pixel_size <= 0.0f) {
        return GK_ERR_OUT_OF_RANGE;
    }
    ratio = focal_px / marker_pixel_size; /* px per px -> scale factor */
    out_translation->x = 0.0f;
    out_translation->y = 0.0f;
    out_translation->z = known_size_mm * ratio *
                         (float)GK_XR_DEG2RAD; /* pseudo depth in metres */
    return GK_OK;
}

/* ================= haptics / 737-739 ================= */

void gk_xr_haptic_init(gk_xr_haptic *h)
{
    if (h == NULL) {
        return;
    }
    memset(h, 0, sizeof(*h));
}

gk_status gk_xr_haptic_play(gk_xr_haptic *h, float freq, float amp,
                            float duration)
{
    if (h == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (freq <= 0.0f || duration <= 0.0f) {
        return GK_ERR_INVALID_ARG;
    }
    h->current.frequency_hz = freq;
    h->current.amplitude = clampf(amp, 0.0f, 1.0f);
    h->current.duration_s = duration;
    h->remaining_s = duration;
    h->active = 1;
    return GK_OK;
}

gk_status gk_xr_haptic_update(gk_xr_haptic *h, float dt)
{
    float t;
    if (h == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!h->active) {
        return GK_ERR_STATE;
    }
    h->remaining_s -= dt;
    if (h->remaining_s <= 0.0f) {
        h->remaining_s = 0.0f;
        h->active = 0;
        h->motor_l = 0.0f;
        h->motor_r = 0.0f;
        return GK_OK;
    }
    t = h->current.duration_s - h->remaining_s;
    h->motor_l = h->current.amplitude *
                 (0.5f + 0.5f * sinf(2.0f * (float)M_PI *
                                     h->current.frequency_hz * t));
    h->motor_r = h->motor_l;
    return GK_OK;
}

int gk_xr_haptic_active(const gk_xr_haptic *h)
{
    if (h == NULL) {
        return 0;
    }
    return h->active;
}

void gk_xr_force_feedback_init(gk_xr_force_feedback *f, float inertia,
                               float damping, float stiffness)
{
    if (f == NULL) {
        return;
    }
    memset(f, 0, sizeof(*f));
    f->inertia = inertia > 0.0f ? inertia : 1.0f;
    f->damping = damping;
    f->stiffness = stiffness;
    f->torque_limit = 10.0f;
}

gk_status gk_xr_force_feedback_step(gk_xr_force_feedback *f, float applied,
                                    float dt)
{
    float accel;
    float torque;
    if (f == NULL || dt <= 0.0f) {
        return GK_ERR_INVALID_ARG;
    }
    if (applied > f->torque_limit) {
        applied = f->torque_limit;
    } else if (applied < -f->torque_limit) {
        applied = -f->torque_limit;
    }
    torque = applied - f->damping * f->omega - f->stiffness * f->angle;
    accel = torque / f->inertia;
    f->omega += accel * dt;
    f->angle += f->omega * dt;
    return GK_OK;
}

float gk_xr_force_feedback_detent(const gk_xr_force_feedback *f, int detents,
                                  float strength)
{
    float nearest;
    float step;
    float err;
    if (f == NULL || detents <= 0) {
        return 0.0f;
    }
    step = 360.0f / (float)detents;
    nearest = roundf(f->angle / step) * step;
    err = nearest - f->angle;
    return err * strength;
}

/* ================= HRTF / spatial audio / 740,741 ================= */

void gk_xr_hrtf_compute(gk_xr_hrtf *h, float listener_x, float listener_z,
                        float source_x, float source_z, float source_y)
{
    float dx, dz, dy;
    if (h == NULL) {
        return;
    }
    dx = source_x - listener_x;
    dz = source_z - listener_z;
    dy = source_y;
    h->azimuth_deg = atan2f(dx, fabsf(dz) + 1e-6f) / (float)GK_XR_DEG2RAD;
    h->distance_m = sqrtf(dx * dx + dy * dy + dz * dz);
    if (h->distance_m > 1e-6f) {
        h->elevation_deg = asinf(clampf(dy / h->distance_m, -1.0f, 1.0f)) /
                           (float)GK_XR_DEG2RAD;
    } else {
        h->elevation_deg = 0.0f;
    }
    /* Woodworth approximation of interaural time difference */
    h->itd_ms = 0.63f * sinf(clampf(h->azimuth_deg, -90.0f, 90.0f) *
                             (float)GK_XR_DEG2RAD);
    /* simple level difference grows with azimuth */
    h->ild_db = 0.5f * clampf(h->azimuth_deg, -90.0f, 90.0f);
}

void gk_xr_spatial_audio_compute(gk_xr_spatial_audio *out,
                                 const gk_xr_hrtf *h, float gain_at_1m,
                                 float ref_distance)
{
    float dist;
    if (out == NULL || h == NULL || ref_distance <= 0.0f) {
        return;
    }
    dist = h->distance_m < 0.01f ? 0.01f : h->distance_m;
    out->distance = dist;
    out->gain = gain_at_1m * (ref_distance / dist);
    if (out->gain > 1.0f) {
        out->gain = 1.0f;
    }
    if (h->itd_ms >= 0.0f) {
        out->delay_left_ms = fabsf(h->itd_ms);
        out->delay_right_ms = 0.0f;
    } else {
        out->delay_left_ms = 0.0f;
        out->delay_right_ms = fabsf(h->itd_ms);
    }
}

/* ================= handwheel / 744,745 ================= */

void gk_xr_handwheel_init(gk_xr_handwheel *h, double counts_per_rev)
{
    if (h == NULL) {
        return;
    }
    memset(h, 0, sizeof(*h));
    h->counts_per_rev = counts_per_rev > 0.0 ? counts_per_rev : 100.0;
}

gk_status gk_xr_handwheel_connect(gk_xr_handwheel *h, unsigned vid,
                                  unsigned pid)
{
    if (h == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    h->vendor_id = vid;
    h->product_id = pid;
    h->axis_count = GK_XR_MAX_CHANNELS;
    h->connected = 1;
    h->position = 0.0;
    h->last_position = 0.0;
    h->has_baseline = 0;
    return GK_OK;
}

gk_status gk_xr_handwheel_report(gk_xr_handwheel *h, const float *axes,
                                 int axes_count)
{
    int i;
    if (h == NULL || axes == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!h->connected) {
        return GK_ERR_STATE;
    }
    if (axes_count > GK_XR_MAX_CHANNELS) {
        axes_count = GK_XR_MAX_CHANNELS;
    }
    if (!h->has_baseline) {
        /* first report establishes the zero reference: no movement yet */
        h->position = axes[0];
        h->last_position = axes[0];
        h->has_baseline = 1;
    } else {
        h->last_position = h->position;
        h->position = axes[0];
    }
    for (i = 0; i < axes_count; i++) {
        h->axis[i] = axes[i];
    }
    return GK_OK;
}

double gk_xr_handwheel_delta_deg(gk_xr_handwheel *h)
{
    double delta;
    if (h == NULL) {
        return 0.0;
    }
    delta = h->position - h->last_position;
    return delta / h->counts_per_rev * 360.0;
}

int gk_xr_handwheel_is_usb(const gk_xr_handwheel *h)
{
    if (h == NULL) {
        return 0;
    }
    return h->connected && h->vendor_id != 0;
}

/* ================= multiscreen / 746 ================= */

void gk_xr_multiscreen_init(gk_xr_multiscreen *m)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    m->swap_barrier = 1;
}

int gk_xr_multiscreen_add(gk_xr_multiscreen *m, gk_xr_screen_role role,
                          int width, int height)
{
    gk_xr_screen *s;
    if (m == NULL || m->count >= GK_XR_MAX_DEVICES) {
        return -1;
    }
    if (width <= 0 || height <= 0) {
        return -1;
    }
    s = &m->screens[m->count];
    memset(s, 0, sizeof(*s));
    s->id = m->count + 1;
    s->role = role;
    s->width = width;
    s->height = height;
    m->count++;
    return s->id;
}

gk_status gk_xr_multiscreen_sync(gk_xr_multiscreen *m, double frame_time)
{
    int i;
    int masters = 0;
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (m->count == 0) {
        return GK_ERR_STATE;
    }
    for (i = 0; i < m->count; i++) {
        if (m->screens[i].role == GK_XR_SCREEN_MASTER) {
            masters++;
        }
    }
    if (masters != 1) {
        return GK_ERR_STATE; /* exactly one master required */
    }
    for (i = 0; i < m->count; i++) {
        /* latency proportional to pixel count and a fixed scan-out cost */
        double px = (double)m->screens[i].width *
                    (double)m->screens[i].height;
        m->screens[i].latency_ms = px / 2.0e6 + frame_time * 1000.0 * 0.1;
    }
    if (m->swap_barrier) {
        double max_lat = 0.0;
        for (i = 0; i < m->count; i++) {
            if (m->screens[i].latency_ms > max_lat) {
                max_lat = m->screens[i].latency_ms;
            }
        }
        for (i = 0; i < m->count; i++) {
            m->screens[i].latency_ms = max_lat;
        }
    }
    return GK_OK;
}

double gk_xr_multiscreen_max_latency(const gk_xr_multiscreen *m)
{
    int i;
    double max_lat = 0.0;
    if (m == NULL) {
        return 0.0;
    }
    for (i = 0; i < m->count; i++) {
        if (m->screens[i].latency_ms > max_lat) {
            max_lat = m->screens[i].latency_ms;
        }
    }
    return max_lat;
}

/* ================= eye tracking / 747 ================= */

void gk_xr_eye_init(gk_xr_eye_tracker *e)
{
    if (e == NULL) {
        return;
    }
    memset(e, 0, sizeof(*e));
}

gk_status gk_xr_eye_add_point(gk_xr_eye_tracker *e, float x, float y)
{
    gk_xr_gaze_point *p;
    if (e == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (e->count >= GK_XR_MAX_GAZE_POINTS) {
        /* shift window to make room for the newest sample */
        memmove(&e->points[0], &e->points[1],
                sizeof(gk_xr_gaze_point) * (GK_XR_MAX_GAZE_POINTS - 1));
        e->count = GK_XR_MAX_GAZE_POINTS - 1;
    }
    p = &e->points[e->count];
    p->x = x;
    p->y = y;
    p->valid = 1;
    e->count++;
    return GK_OK;
}

gk_status gk_xr_eye_update(gk_xr_eye_tracker *e, double dt)
{
    int i;
    double sx = 0.0;
    double sy = 0.0;
    int n = 0;
    if (e == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (dt < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < e->count; i++) {
        if (e->points[i].valid) {
            sx += e->points[i].x;
            sy += e->points[i].y;
            n++;
        }
    }
    if (n == 0) {
        e->dwell_s = 0.0;
        return GK_ERR_STATE;
    }
    e->fixation_x = (float)(sx / n);
    e->fixation_y = (float)(sy / n);
    e->dwell_s += (float)dt;
    return GK_OK;
}

int gk_xr_eye_select(const gk_xr_eye_tracker *e, float threshold, double dwell)
{
    if (e == NULL) {
        return -1;
    }
    if (threshold <= 0.0f || dwell <= 0.0) {
        return -1;
    }
    if (e->dwell_s >= dwell) {
        return 1; /* fixation held long enough to select */
    }
    return -1;
}

/* ================= EEG / 748 ================= */

const char *gk_xr_eeg_band_name(gk_xr_eeg_band b)
{
    switch (b) {
    case GK_XR_EEG_DELTA: return "delta";
    case GK_XR_EEG_THETA: return "theta";
    case GK_XR_EEG_ALPHA: return "alpha";
    case GK_XR_EEG_BETA: return "beta";
    case GK_XR_EEG_GAMMA: return "gamma";
    default: return "unknown";
    }
}

void gk_xr_eeg_init(gk_xr_eeg *e)
{
    if (e == NULL) {
        return;
    }
    memset(e, 0, sizeof(*e));
    e->signal_quality = 100;
}

gk_status gk_xr_eeg_set_band(gk_xr_eeg *e, gk_xr_eeg_band b, float value)
{
    if (e == NULL || b >= GK_XR_EEG_BAND_COUNT) {
        return GK_ERR_INVALID_ARG;
    }
    if (value < 0.0f) {
        return GK_ERR_INVALID_ARG;
    }
    e->bands[b] = value;
    return GK_OK;
}

gk_status gk_xr_eeg_evaluate(gk_xr_eeg *e)
{
    float t, a, b;
    if (e == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    t = e->bands[GK_XR_EEG_THETA];
    a = e->bands[GK_XR_EEG_ALPHA];
    b = e->bands[GK_XR_EEG_BETA];
    /* attention rises with beta, meditation with alpha */
    if (b + a > 1e-6f) {
        e->attention = clampf(b / (b + a), 0.0f, 1.0f);
    } else {
        e->attention = 0.0f;
    }
    if (a + t > 1e-6f) {
        e->meditation = clampf(a / (a + t), 0.0f, 1.0f);
    } else {
        e->meditation = 0.0f;
    }
    return GK_OK;
}

float gk_xr_eeg_cognitive_load(const gk_xr_eeg *e)
{
    float a, t, b;
    if (e == NULL) {
        return 0.0f;
    }
    a = e->bands[GK_XR_EEG_ALPHA];
    t = e->bands[GK_XR_EEG_THETA];
    b = e->bands[GK_XR_EEG_BETA];
    if (a + t < 1e-6f) {
        return 0.0f;
    }
    return clampf(b / (a + t), 0.0f, 10.0f);
}

/* ================= heart rate / 749 ================= */

void gk_xr_heart_init(gk_xr_heart *h)
{
    if (h == NULL) {
        return;
    }
    memset(h, 0, sizeof(*h));
}

gk_status gk_xr_heart_connect(gk_xr_heart *h)
{
    if (h == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    h->connected = 1;
    return GK_OK;
}

gk_status gk_xr_heart_report(gk_xr_heart *h, int bpm, double rr_interval_ms)
{
    double prev_avg;
    if (h == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!h->connected) {
        return GK_ERR_STATE;
    }
    if (bpm <= 0) {
        return GK_ERR_INVALID_ARG;
    }
    prev_avg = h->avg_bpm;
    h->bpm = bpm;
    h->last_beat_s = rr_interval_ms / 1000.0;
    h->samples++;
    h->avg_bpm = prev_avg + (bpm - prev_avg) / (double)h->samples;
    /* HRV as the RMSSD-like deviation of the RR interval from the mean */
    if (h->samples > 1) {
        double mean_rr = 60000.0 / h->avg_bpm;
        h->hrv_ms = fabs(rr_interval_ms - mean_rr);
    }
    return GK_OK;
}

float gk_xr_heart_stress(const gk_xr_heart *h)
{
    float hr_term;
    float hrv_term;
    if (h == NULL || !h->connected) {
        return 0.0f;
    }
    /* elevated heart rate above a 70 bpm baseline */
    hr_term = clampf(((float)h->bpm - 70.0f) / 80.0f, 0.0f, 1.0f);
    /* low HRV indicates stress */
    hrv_term = clampf(1.0f - (float)h->hrv_ms / 100.0f, 0.0f, 1.0f);
    return clampf(0.6f * hr_term + 0.4f * hrv_term, 0.0f, 1.0f);
}
