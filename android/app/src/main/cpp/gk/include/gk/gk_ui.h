#ifndef GK_UI_H
#define GK_UI_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_UI_MAX_MENU_ITEMS 32
#define GK_UI_MAX_STRING 64
#define GK_UI_MAX_WIDGETS 96
#define GK_UI_IO_SIGNALS 32

/* ---------------- theme / language / view (375-380) ---------------- */

typedef enum {
    GK_THEME_DARK = 0,   /* 377 */
    GK_THEME_LIGHT       /* 378 */
} gk_theme;

typedef enum {
    GK_LANG_EN = 0,      /* 380 */
    GK_LANG_ZH
} gk_lang;

typedef enum {
    GK_VIEW_ISO = 0,
    GK_VIEW_TOP,
    GK_VIEW_FRONT,
    GK_VIEW_SIDE,
    GK_VIEW_COUNT
} gk_view_mode;

const char *gk_view_mode_name(gk_view_mode v);

/* ---------------- menu tree (359-364) ---------------- */

typedef struct {
    char label[GK_UI_MAX_STRING];
    char shortcut[GK_UI_MAX_STRING];
    int enabled;
} gk_menu_item;

typedef struct {
    char title[GK_UI_MAX_STRING];
    gk_menu_item items[GK_UI_MAX_MENU_ITEMS];
    int count;
} gk_menu;

typedef enum {
    GK_MENU_MAIN = 0,   /* 359 */
    GK_MENU_FILE,       /* 360 */
    GK_MENU_EDIT,       /* 361 */
    GK_MENU_VIEW,       /* 362 */
    GK_MENU_TOOLS,      /* 363 */
    GK_MENU_HELP,       /* 364 */
    GK_MENU_COUNT
} gk_menu_kind;

gk_status gk_menu_init(gk_menu *m, gk_menu_kind kind, const char *title);
gk_status gk_menu_add_item(gk_menu *m, const char *label, const char *shortcut);
int gk_menu_find(const gk_menu *m, const char *label);
gk_status gk_menu_enable(gk_menu *m, const char *label, int enabled);

typedef struct {
    gk_menu menus[GK_MENU_COUNT];
    int count;
} gk_menu_bar;

gk_status gk_menu_bar_init(gk_menu_bar *b);
gk_menu *gk_menu_bar_get(gk_menu_bar *b, gk_menu_kind kind);

/* ---------------- toolbar / status bar (365-366) ---------------- */

typedef struct {
    char name[GK_UI_MAX_STRING];
    char tooltip[GK_UI_MAX_STRING];
    int enabled;
    int toggled;
} gk_toolbar_button;

typedef struct {
    gk_toolbar_button buttons[GK_UI_MAX_MENU_ITEMS];
    int count;
} gk_toolbar;

gk_status gk_toolbar_add(gk_toolbar *t, const char *name, const char *tooltip);
int gk_toolbar_click(gk_toolbar *t, const char *name);
int gk_toolbar_toggle(const gk_toolbar *t, const char *name);

typedef struct {
    char text[GK_UI_MAX_STRING];
    gk_status level;      /* derived status; GK_OK means ready */
} gk_status_cell;

typedef struct {
    gk_status_cell cells[GK_UI_MAX_MENU_ITEMS];
    int count;
} gk_status_bar;

gk_status gk_status_bar_set(gk_status_bar *s, int index, const char *text,
                            gk_status level);
const char *gk_status_bar_text(const gk_status_bar *s, int index);

/* ---------------- data panel (367-374) ---------------- */

typedef struct {
    double machine[9];    /* X Y Z U V W A B C machine position */
    double work[9];       /* X Y Z U V W A B C work position */
    double spindle_rpm;
    double feed;
    double feed_override;    /* 0..2, 1 = 100% */
    double rapid_override;
    double spindle_override;
    double elapsed;          /* seconds */
    double total_estimate;   /* seconds, total program estimate */
    double progress;         /* 0..1 */
} gk_data_panel;

void gk_data_panel_init(gk_data_panel *d);
void gk_data_panel_update(gk_data_panel *d, double elapsed, double feed);
double gk_data_panel_remaining(const gk_data_panel *d);
void gk_data_panel_progress(gk_data_panel *d);
int gk_data_panel_progress_bar(const gk_data_panel *d, int width);

/* ---------------- widget registry (375 view switch / 376 fullscreen) ------- */

typedef struct {
    char name[GK_UI_MAX_STRING];
    int visible;
    int x, y, w, h;
} gk_widget;

typedef struct {
    gk_widget widgets[GK_UI_MAX_WIDGETS];
    int count;
    gk_view_mode view;
    int fullscreen;
    gk_theme theme;
    gk_lang lang;
} gk_ui;

void gk_ui_init(gk_ui *ui);
int gk_ui_add_widget(gk_ui *ui, const char *name, int x, int y, int w, int h);
gk_widget *gk_ui_find(gk_ui *ui, const char *name);
void gk_ui_set_visible(gk_ui *ui, const char *name, int visible);
void gk_ui_switch_view(gk_ui *ui, gk_view_mode v);
void gk_ui_toggle_fullscreen(gk_ui *ui);
void gk_ui_set_theme(gk_ui *ui, gk_theme t);
void gk_ui_set_lang(gk_ui *ui, gk_lang l);
/* 379/380 localized string lookup by key. */
const char *gk_ui_tr(gk_lang l, const char *key);

/* ---------------- control panel style (381-384) ---------------- */

typedef enum {
    GK_PANEL_FANUC = 0,    /* 381 */
    GK_PANEL_SIEMENS,      /* 382 */
    GK_PANEL_MITSUBISHI,   /* 383 */
    GK_PANEL_HAAS,         /* 384 */
    GK_PANEL_COUNT
} gk_panel_style;

const char *gk_panel_style_name(gk_panel_style s);
/* Panel-specific label for a soft key, "" when unavailable. */
const char *gk_panel_softkey(gk_panel_style s, int index);

/* ---------------- MDI keyboard (385-389) ---------------- */

typedef enum {
    GK_KEY_DIGIT = 0,
    GK_KEY_LETTER,
    GK_KEY_FUNC,
    GK_KEY_SOFT,
    GK_KEY_COUNT
} gk_key_kind;

typedef struct {
    char label[GK_UI_MAX_STRING];
    gk_key_kind kind;
} gk_key;

typedef struct {
    gk_key keys[GK_UI_MAX_WIDGETS];
    int count;
    char buffer[256];
    int len;
} gk_keyboard;

void gk_keyboard_init(gk_keyboard *k);
gk_status gk_keyboard_press(gk_keyboard *k, const char *label);
int gk_keyboard_is_digit(const char *label);
int gk_keyboard_is_letter(const char *label);
void gk_keyboard_clear(gk_keyboard *k);

/* ---------------- handwheel / MPG (390-398) ---------------- */

typedef struct {
    int multiplier;      /* 1, 10, 100 */
    int axis;            /* 0..5 -> X Y Z 4th 5th spindle, -1 none */
} gk_mpg;

void gk_mpg_init(gk_mpg *m);
gk_status gk_mpg_set_multiplier(gk_mpg *m, int mult);   /* 391-393 */
gk_status gk_mpg_select_axis(gk_mpg *m, int axis);      /* 394-398 */
double gk_mpg_step(const gk_mpg *m);                    /* multiplier * 0.001 */

/* ---------------- rotary switches (399-409) ---------------- */

typedef struct {
    double feed_override;     /* 399 */
    double rapid_override;    /* 400 */
    double spindle_override;  /* 401 */
} gk_overrides;

void gk_overrides_init(gk_overrides *o);
gk_status gk_overrides_set(gk_overrides *o, int which, double value);

typedef enum {
    GK_MODE_EDIT = 0,   /* 403 */
    GK_MODE_AUTO,       /* 404 */
    GK_MODE_MDI,        /* 405 */
    GK_MODE_JOG,        /* 406 */
    GK_MODE_HANDLE,     /* handwheel */
    GK_MODE_MANUAL,     /* 407 */
    GK_MODE_HOME,       /* 408 */
    GK_MODE_DNC,        /* 409 */
    GK_MODE_COUNT
} gk_op_mode;

const char *gk_op_mode_name(gk_op_mode m);

typedef struct {
    gk_op_mode mode;        /* 402 */
} gk_mode_switch;

void gk_mode_switch_init(gk_mode_switch *s);
gk_status gk_mode_switch_set(gk_mode_switch *s, gk_op_mode m);
int gk_mode_switch_is_auto(const gk_mode_switch *s);

/* ---------------- hard buttons (410-416) ---------------- */

typedef struct {
    int cycle_start;   /* 410 */
    int cycle_stop;    /* 411 */
    int estop;         /* 412 */
    int key_switch;    /* 413 key switch: 0=lock 1=unlock */
    int coolant;       /* 414 */
    int spindle;       /* 415 */
    int light;         /* 416 */
} gk_panel_buttons;

void gk_panel_buttons_init(gk_panel_buttons *b);
gk_status gk_panel_press(gk_panel_buttons *b, int button_id);

typedef enum {
    GK_BTN_CYCLE_START = 0,
    GK_BTN_CYCLE_STOP,
    GK_BTN_ESTOP,
    GK_BTN_KEY,
    GK_BTN_COOLANT,
    GK_BTN_SPINDLE,
    GK_BTN_LIGHT,
    GK_BTN_COUNT
} gk_button_id;

/* ---------------- diagnostic pages (417-423) ---------------- */

typedef struct {
    int code;
    int severity;
    char message[GK_UI_MAX_STRING];
} gk_alarm_row;

typedef struct {
    gk_alarm_row rows[GK_UI_MAX_MENU_ITEMS];
    int count;
} gk_alarm_page;   /* 417 */

gk_status gk_alarm_page_add(gk_alarm_page *p, int code, int severity,
                            const char *message);

typedef struct {
    int number;
    char name[GK_UI_MAX_STRING];
    double value;
    char unit[GK_UI_MAX_STRING];
} gk_param_row;    /* 418 parameter page */

typedef struct {
    gk_param_row rows[GK_UI_MAX_MENU_ITEMS];
    int count;
} gk_param_page;

gk_status gk_param_page_add(gk_param_page *p, int number, const char *name,
                            double value, const char *unit);
gk_status gk_param_page_set(gk_param_page *p, int number, double value);

typedef struct {
    int index;
    char name[GK_UI_MAX_STRING];
    double value;
    char unit[GK_UI_MAX_STRING];
} gk_diag_row;     /* 419 diagnostic page */

typedef struct {
    gk_diag_row rows[GK_UI_MAX_MENU_ITEMS];
    int count;
} gk_diag_page;

gk_status gk_diag_page_add(gk_diag_page *p, int index, const char *name,
                           const char *unit);
gk_status gk_diag_page_set(gk_diag_page *p, int index, double value);

/* 420 servo waveform: ring buffer of samples. */
#define GK_WAVE_MAX 256
typedef struct {
    double samples[GK_WAVE_MAX];
    int count;
    int head;
} gk_waveform;

void gk_waveform_init(gk_waveform *w);
void gk_waveform_push(gk_waveform *w, double sample);
double gk_waveform_at(const gk_waveform *w, int i);
double gk_waveform_peak(const gk_waveform *w);

/* 421 ladder monitoring: simple relay network. */
#define GK_LADDER_MAX 32
typedef struct {
    int id;
    char name[GK_UI_MAX_STRING];
    int inputs[8];
    int input_count;
    int output;
} gk_ladder_rung;

typedef struct {
    gk_ladder_rung rungs[GK_LADDER_MAX];
    int count;
} gk_ladder;

gk_status gk_ladder_add(gk_ladder *l, int id, const char *name);
gk_status gk_ladder_set_input(gk_ladder *l, int id, int index, int value);
int gk_ladder_evaluate(const gk_ladder *l, int id);   /* AND of all inputs */

/* 422 PLC state / 423 I/O status */
typedef struct {
    int run;
    int scan_count;
    double scan_time_ms;
    int io_signals[GK_UI_IO_SIGNALS];
} gk_plc_view;

void gk_plc_view_init(gk_plc_view *p);
void gk_plc_scan(gk_plc_view *p, double dt_ms);
void gk_plc_set_io(gk_plc_view *p, int index, int value);
int gk_plc_read_io(const gk_plc_view *p, int index);

#ifdef __cplusplus
}
#endif

#endif /* GK_UI_H */
