#include "gk/gk_ux.h"

#include <math.h>
#include <string.h>

static void gk_ux_copy(char *dst, const char *src, size_t cap)
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

/* ================= 750-754 devices ================= */

void gk_ux_glove_init(gk_ux_thermal_glove *g)
{
    if (g == NULL) {
        return;
    }
    memset(g, 0, sizeof(*g));
    g->target_temp = 25.0f;
    g->current_temp = 25.0f;
    g->heat_rate = 4.0f;
}

gk_status gk_ux_glove_set(gk_ux_thermal_glove *g, float temp)
{
    if (g == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (temp < 0.0f || temp > 60.0f) {
        return GK_ERR_OUT_OF_RANGE;
    }
    g->target_temp = temp;
    g->active = 1;
    return GK_OK;
}

gk_status gk_ux_glove_update(gk_ux_thermal_glove *g, float dt)
{
    float diff;
    if (g == NULL || dt < 0.0f) {
        return GK_ERR_INVALID_ARG;
    }
    diff = g->target_temp - g->current_temp;
    if (fabsf(diff) <= g->heat_rate * dt) {
        g->current_temp = g->target_temp;
        g->active = 0;
    } else {
        g->current_temp += (diff > 0.0f ? 1.0f : -1.0f) * g->heat_rate * dt;
    }
    return GK_OK;
}

void gk_ux_scent_init(gk_ux_scent *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->cartridge = -1;
}

gk_status gk_ux_scent_emit(gk_ux_scent *s, int cartridge, float intensity,
                           float duration)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (cartridge < 0 || duration <= 0.0f) {
        return GK_ERR_INVALID_ARG;
    }
    s->cartridge = cartridge;
    s->intensity = clampf(intensity, 0.0f, 1.0f);
    s->remaining_s = duration;
    return GK_OK;
}

gk_status gk_ux_scent_update(gk_ux_scent *s, float dt)
{
    if (s == NULL || dt < 0.0f) {
        return GK_ERR_INVALID_ARG;
    }
    if (s->remaining_s <= 0.0f) {
        return GK_ERR_STATE;
    }
    s->remaining_s -= dt;
    if (s->remaining_s <= 0.0f) {
        s->remaining_s = 0.0f;
        s->intensity = 0.0f;
    }
    return GK_OK;
}

int gk_ux_scent_active(const gk_ux_scent *s)
{
    if (s == NULL) {
        return 0;
    }
    return s->remaining_s > 0.0f;
}

void gk_ux_foot_switch_init(gk_ux_foot_switch *f)
{
    if (f == NULL) {
        return;
    }
    memset(f, 0, sizeof(*f));
}

gk_status gk_ux_foot_switch_set(gk_ux_foot_switch *f, int index, int down)
{
    if (f == NULL || index < 0 || index >= 8) {
        return GK_ERR_INVALID_ARG;
    }
    if (down) {
        f->switches |= (1u << index);
    } else {
        f->switches &= ~(1u << index);
    }
    f->pressed = f->switches != 0;
    return GK_OK;
}

int gk_ux_foot_switch_down(const gk_ux_foot_switch *f, int index)
{
    if (f == NULL || index < 0 || index >= 8) {
        return 0;
    }
    return (f->switches & (1u << index)) != 0;
}

void gk_ux_print_head_init(gk_ux_print_head *p)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->nozzle_temp = 25.0f;
}

gk_status gk_ux_print_head_extrude(gk_ux_print_head *p, float feed_rate,
                                   float dt)
{
    if (p == NULL || feed_rate < 0.0f || dt < 0.0f) {
        return GK_ERR_INVALID_ARG;
    }
    if (feed_rate > 200.0f) {
        return GK_ERR_OUT_OF_RANGE;
    }
    p->feed_rate = feed_rate;
    p->extruding = feed_rate > 0.0f;
    p->filament_used_mm += feed_rate * dt;
    return GK_OK;
}

void gk_ux_haptic_seat_init(gk_ux_haptic_seat *s, int channels)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->channels = (channels <= 0) ? 1 : channels;
}

gk_status gk_ux_haptic_seat_pulse(gk_ux_haptic_seat *s, float amp, float freq)
{
    if (s == NULL || freq <= 0.0f) {
        return GK_ERR_INVALID_ARG;
    }
    s->amplitude = clampf(amp, 0.0f, 1.0f);
    s->frequency = freq;
    s->active = 1;
    return GK_OK;
}

gk_status gk_ux_haptic_seat_stop(gk_ux_haptic_seat *s)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    s->amplitude = 0.0f;
    s->active = 0;
    return GK_OK;
}

/* ================= 755-759 sonification ================= */

const char *gk_ux_mapping_name(gk_ux_mapping_kind k)
{
    switch (k) {
    case GK_UX_MAP_LOAD_TONE: return "load-tone";
    case GK_UX_MAP_TEMP_COLOR: return "temperature-color";
    case GK_UX_MAP_TEXTURE_HAPTIC: return "texture-haptic";
    case GK_UX_MAP_GCODE_MUSIC: return "gcode-music";
    default: return "unknown";
    }
}

float gk_ux_normalize(float value, float lo, float hi)
{
    if (hi <= lo) {
        return 0.0f;
    }
    return clampf((value - lo) / (hi - lo), 0.0f, 1.0f);
}

/* pentatonic major: scale degrees in semitones */
static const int g_ux_pentatonic[5] = {0, 2, 4, 7, 9};

float gk_ux_load_to_pitch(float load)
{
    float n;
    int degree;
    int octave;
    int semi;
    float base = 220.0f; /* A3 */
    n = clampf(load, 0.0f, 1.0f) * 4.999f;
    degree = (int)n;
    octave = degree / 5;
    semi = g_ux_pentatonic[degree % 5] + 12 * octave;
    return base * powf(2.0f, (float)semi / 12.0f);
}

gk_status gk_ux_temp_to_color(float temp_c, float cold, float hot,
                              unsigned *out_rgb)
{
    float t;
    unsigned r, g, b;
    if (out_rgb == NULL || hot <= cold) {
        return GK_ERR_INVALID_ARG;
    }
    t = clampf((temp_c - cold) / (hot - cold), 0.0f, 1.0f);
    /* blue (0,64,255) -> red (255,32,0) */
    r = (unsigned)(0.0f + t * 255.0f + 0.5f);
    g = (unsigned)(64.0f + (32.0f - 64.0f) * t + 0.5f);
    b = (unsigned)(255.0f + (0.0f - 255.0f) * t + 0.5f);
    *out_rgb = (r << 16) | (g << 8) | b;
    return GK_OK;
}

gk_status gk_ux_texture_to_haptic(float roughness_um, float *out_freq,
                                  float *out_amp)
{
    float n;
    if (out_freq == NULL || out_amp == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    n = clampf(roughness_um / 10.0f, 0.0f, 1.0f);
    *out_freq = 50.0f + n * 250.0f;   /* smoother = lower frequency */
    *out_amp = 0.2f + n * 0.8f;
    return GK_OK;
}

int gk_ux_gcode_to_note(char letter)
{
    /* map common G-code letters to scale degrees (mod 12) */
    static const char letters[] = "XYZFSTGMN";
    static const int notes[] = {0, 2, 4, 5, 7, 9, 11, 1, 3};
    size_t i;
    for (i = 0; i < sizeof(letters) - 1; i++) {
        if (letter == letters[i]) {
            return notes[i];
        }
        if (letter == letters[i] + 32) { /* lower case */
            return notes[i];
        }
    }
    return -1;
}

float gk_ux_feed_to_tempo(float feed)
{
    /* 0..1000 mm/min -> 40..220 BPM */
    float n = clampf(feed / 1000.0f, 0.0f, 1.0f);
    return 40.0f + n * 180.0f;
}

void gk_ux_sonifier_init(gk_ux_sonifier *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
}

gk_status gk_ux_sonify(gk_ux_sonifier *s, float load, float temperature,
                       float feed)
{
    unsigned rgb;
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    s->last_pitch_hz = gk_ux_load_to_pitch(load);
    s->last_tempo = gk_ux_feed_to_tempo(feed);
    if (gk_ux_temp_to_color(temperature, 20.0f, 120.0f, &rgb) != GK_OK) {
        return GK_ERR_INVALID_ARG;
    }
    s->last_color = rgb;
    if (s->last_pitch_hz > 200.0f) {
        s->notes_played++;
    }
    return GK_OK;
}

/* ================= 760-762 explanation ================= */

void gk_ux_narration_init(gk_ux_narration *n)
{
    if (n == NULL) {
        return;
    }
    memset(n, 0, sizeof(*n));
}

gk_status gk_ux_narration_say(gk_ux_narration *n, const char *fmt,
                              const char *subject)
{
    if (n == NULL || fmt == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (subject != NULL && strstr(fmt, "%s") != NULL) {
        /* substitute the single %s placeholder with the subject text */
        const char *pos = strstr(fmt, "%s");
        size_t head = (size_t)(pos - fmt);
        size_t tail = strlen(pos + 2);
        size_t subj = strlen(subject);
        char out[GK_UX_TEXT];
        size_t cap = sizeof(out);
        size_t w = 0;
        size_t i;
        if (head >= cap) {
            head = cap - 1;
        }
        for (i = 0; i < head; i++) {
            out[w++] = fmt[i];
        }
        for (i = 0; i < subj && w + 1 < cap; i++) {
            out[w++] = subject[i];
        }
        for (i = 0; i < tail && w + 1 < cap; i++) {
            out[w++] = pos[2 + i];
        }
        out[w] = '\0';
        gk_ux_copy(n->text, out, sizeof(n->text));
    } else {
        gk_ux_copy(n->text, fmt, sizeof(n->text));
    }
    n->step++;
    return GK_OK;
}

int gk_ux_chart_link(int series_count, int point_index, int *out_series)
{
    if (series_count <= 0 || point_index < 0) {
        return -1;
    }
    /* round-robin highlighting across series */
    if (out_series != NULL) {
        *out_series = point_index % series_count;
    }
    return point_index % series_count;
}

int gk_ux_animation_frames(float duration_s, float fps, float *out_times,
                           int max_out)
{
    int total;
    int i;
    if (duration_s <= 0.0f || fps <= 0.0f) {
        return 0;
    }
    total = (int)(duration_s * fps) + 1;
    for (i = 0; i < total && i < max_out; i++) {
        if (out_times != NULL) {
            out_times[i] = (float)i / fps;
        }
    }
    return total;
}

/* ================= 763-768 cues ================= */

const char *gk_ux_cue_name(gk_ux_cue_kind k)
{
    switch (k) {
    case GK_UX_CUE_ANNOTATION: return "annotation";
    case GK_UX_CUE_ARROW: return "arrow";
    case GK_UX_CUE_HIGHLIGHT: return "highlight";
    case GK_UX_CUE_SPEECH: return "speech";
    case GK_UX_CUE_VIBRATION: return "vibration";
    case GK_UX_CUE_LIGHT: return "light";
    default: return "unknown";
    }
}

void gk_ux_cue_list_init(gk_ux_cue_list *l)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
}

int gk_ux_cue_add(gk_ux_cue_list *l, gk_ux_cue_kind kind, const char *text,
                  float x, float y, float z)
{
    gk_ux_cue *c;
    if (l == NULL || kind >= GK_UX_CUE_COUNT || l->count >= GK_UX_MAX_ITEMS) {
        return -1;
    }
    c = &l->cues[l->count];
    memset(c, 0, sizeof(*c));
    c->id = l->count + 1;
    c->kind = kind;
    gk_ux_copy(c->text, text, sizeof(c->text));
    c->x = x;
    c->y = y;
    c->z = z;
    c->priority = 0;
    c->active = 1;
    l->count++;
    return c->id; /* id is 1-based */
}

gk_status gk_ux_cue_set_active(gk_ux_cue_list *l, int id, int active)
{
    if (l == NULL || id <= 0 || id > l->count) {
        return GK_ERR_INVALID_ARG;
    }
    l->cues[id - 1].active = active ? 1 : 0;
    return GK_OK;
}

int gk_ux_cue_highest(const gk_ux_cue_list *l, gk_ux_cue_kind kind)
{
    int i;
    int best = -1;
    int best_pri = -1;
    if (l == NULL) {
        return -1;
    }
    for (i = 0; i < l->count; i++) {
        if (l->cues[i].kind == kind && l->cues[i].active &&
            l->cues[i].priority > best_pri) {
            best_pri = l->cues[i].priority;
            best = l->cues[i].id;
        }
    }
    return best;
}

const char *gk_ux_cue_speech(const gk_ux_cue_list *l, int id)
{
    if (l == NULL || id <= 0 || id > l->count) {
        return NULL;
    }
    if (l->cues[id - 1].kind != GK_UX_CUE_SPEECH) {
        return NULL;
    }
    return l->cues[id - 1].text;
}
