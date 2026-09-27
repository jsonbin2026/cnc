#ifndef GK_A11Y_H
#define GK_A11Y_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_A11Y_NAME 64
#define GK_A11Y_MAX_ITEMS 64

/* 894 colour-blind friendly modes */
typedef enum {
    GK_A11Y_CB_NONE = 0,
    GK_A11Y_CB_DEUTERANOPIA,
    GK_A11Y_CB_PROTANOPIA,
    GK_A11Y_CB_TRITANOPIA
} gk_a11y_colorblind;

const char *gk_a11y_colorblind_name(gk_a11y_colorblind m);
/* map an RGB triple to a colour-blind safe approximation */
void gk_a11y_colorblind_map(gk_a11y_colorblind m, unsigned char *r,
                            unsigned char *g, unsigned char *b);

/* 895 high contrast */
typedef struct {
    int enabled;
    int contrast;   /* percent, 100 = normal */
    int invert;
} gk_a11y_contrast;

void gk_a11y_contrast_init(gk_a11y_contrast *c);
gk_status gk_a11y_contrast_set(gk_a11y_contrast *c, int contrast_percent);
double gk_a11y_contrast_luminance(const gk_a11y_contrast *c, double input);

/* 896 large font */
typedef struct {
    double scale;
    int min_size;
    int max_size;
} gk_a11y_font;

void gk_a11y_font_init(gk_a11y_font *f);
gk_status gk_a11y_font_set_scale(gk_a11y_font *f, double scale);
int gk_a11y_font_size(const gk_a11y_font *f, int base_size);

/* 897 voice control */
typedef struct {
    char phrase[GK_A11Y_NAME];
    char action[GK_A11Y_NAME];
} gk_a11y_voice_cmd;

typedef struct {
    gk_a11y_voice_cmd cmds[GK_A11Y_MAX_ITEMS];
    int count;
    int listening;
} gk_a11y_voice;

void gk_a11y_voice_init(gk_a11y_voice *v);
gk_status gk_a11y_voice_add(gk_a11y_voice *v, const char *phrase,
                            const char *action);
/* returns the matched action, or NULL */
const char *gk_a11y_voice_match(const gk_a11y_voice *v, const char *spoken);

/* 898 one-hand operation */
typedef struct {
    double grip_center_x;
    double grip_center_y;
    int left_handed;
    int mirrored;
} gk_a11y_onehand;

void gk_a11y_onehand_init(gk_a11y_onehand *o);
gk_status gk_a11y_onehand_enable(gk_a11y_onehand *o, int left_handed);
/* remap a UI control coordinate into the reachable one-hand arc */
gk_status gk_a11y_onehand_remap(const gk_a11y_onehand *o, double x, double y,
                                double *out_x, double *out_y);

/* 899 subtitles */
typedef struct {
    char text[GK_A11Y_NAME];
    double start_s;
    double end_s;
} gk_a11y_subtitle;

typedef struct {
    gk_a11y_subtitle lines[GK_A11Y_MAX_ITEMS];
    int count;
    int enabled;
} gk_a11y_subtitles;

void gk_a11y_subtitles_init(gk_a11y_subtitles *s);
gk_status gk_a11y_subtitles_add(gk_a11y_subtitles *s, const char *text,
                                double start_s, double end_s);
const char *gk_a11y_subtitles_at(const gk_a11y_subtitles *s, double t);

/* 900 slow motion */
typedef struct {
    double time_scale;    /* 0.25 = quarter speed */
    int enabled;
} gk_a11y_slowmo;

void gk_a11y_slowmo_init(gk_a11y_slowmo *m);
gk_status gk_a11y_slowmo_set(gk_a11y_slowmo *m, double scale);
double gk_a11y_slowmo_apply(const gk_a11y_slowmo *m, double delta_s);

#ifdef __cplusplus
}
#endif

#endif /* GK_A11Y_H */
