#include "gk/gk_ui.h"

#include <math.h>
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

static int gk__clamp_index(int i, int n)
{
    if (i < 0) return 0;
    if (i >= n) return n - 1;
    return i;
}

/* ---------------- views ---------------- */

const char *gk_view_mode_name(gk_view_mode v)
{
    switch (v) {
    case GK_VIEW_ISO: return "iso";
    case GK_VIEW_TOP: return "top";
    case GK_VIEW_FRONT: return "front";
    case GK_VIEW_SIDE: return "side";
    default: return "unknown";
    }
}

/* ---------------- menus ---------------- */

gk_status gk_menu_init(gk_menu *m, gk_menu_kind kind, const char *title)
{
    (void)kind;
    if (m == NULL || title == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    memset(m, 0, sizeof(*m));
    gk__copy(m->title, sizeof(m->title), title);
    return GK_OK;
}

gk_status gk_menu_add_item(gk_menu *m, const char *label, const char *shortcut)
{
    gk_menu_item *it;
    if (m == NULL || label == NULL || m->count >= GK_UI_MAX_MENU_ITEMS) {
        return GK_ERR_INVALID_ARG;
    }
    if (gk_menu_find(m, label) >= 0) {
        return GK_ERR_ALREADY_EXISTS;
    }
    it = &m->items[m->count++];
    memset(it, 0, sizeof(*it));
    gk__copy(it->label, sizeof(it->label), label);
    gk__copy(it->shortcut, sizeof(it->shortcut), shortcut);
    it->enabled = 1;
    return GK_OK;
}

int gk_menu_find(const gk_menu *m, const char *label)
{
    int i;
    if (m == NULL || label == NULL) {
        return -1;
    }
    for (i = 0; i < m->count; ++i) {
        if (strcmp(m->items[i].label, label) == 0) {
            return i;
        }
    }
    return -1;
}

gk_status gk_menu_enable(gk_menu *m, const char *label, int enabled)
{
    int idx = gk_menu_find(m, label);
    if (idx < 0) {
        return GK_ERR_NOT_FOUND;
    }
    m->items[idx].enabled = enabled ? 1 : 0;
    return GK_OK;
}

static const char *g_default_titles[GK_MENU_COUNT] = {
    "Main", "File", "Edit", "View", "Tools", "Help"
};

gk_status gk_menu_bar_init(gk_menu_bar *b)
{
    int i;
    if (b == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    memset(b, 0, sizeof(*b));
    for (i = 0; i < GK_MENU_COUNT; ++i) {
        gk_menu_init(&b->menus[i], (gk_menu_kind)i, g_default_titles[i]);
    }
    b->count = GK_MENU_COUNT;
    return GK_OK;
}

gk_menu *gk_menu_bar_get(gk_menu_bar *b, gk_menu_kind kind)
{
    if (b == NULL || kind < 0 || kind >= GK_MENU_COUNT) {
        return NULL;
    }
    return &b->menus[kind];
}

/* ---------------- toolbar / status bar ---------------- */

gk_status gk_toolbar_add(gk_toolbar *t, const char *name, const char *tooltip)
{
    gk_toolbar_button *btn;
    if (t == NULL || name == NULL || t->count >= GK_UI_MAX_MENU_ITEMS) {
        return GK_ERR_INVALID_ARG;
    }
    btn = &t->buttons[t->count++];
    memset(btn, 0, sizeof(*btn));
    gk__copy(btn->name, sizeof(btn->name), name);
    gk__copy(btn->tooltip, sizeof(btn->tooltip), tooltip);
    btn->enabled = 1;
    return GK_OK;
}

int gk_toolbar_click(gk_toolbar *t, const char *name)
{
    int i;
    if (t == NULL || name == NULL) {
        return -1;
    }
    for (i = 0; i < t->count; ++i) {
        if (strcmp(t->buttons[i].name, name) == 0) {
            if (!t->buttons[i].enabled) {
                return -1;
            }
            t->buttons[i].toggled = !t->buttons[i].toggled;
            return t->buttons[i].toggled;
        }
    }
    return -1;
}

int gk_toolbar_toggle(const gk_toolbar *t, const char *name)
{
    int i;
    if (t == NULL || name == NULL) {
        return -1;
    }
    for (i = 0; i < t->count; ++i) {
        if (strcmp(t->buttons[i].name, name) == 0) {
            return t->buttons[i].toggled;
        }
    }
    return -1;
}

gk_status gk_status_bar_set(gk_status_bar *s, int index, const char *text,
                            gk_status level)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (index < 0) {
        index = s->count < GK_UI_MAX_MENU_ITEMS ? s->count : -1;
    }
    if (index < 0 || index >= GK_UI_MAX_MENU_ITEMS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    gk__copy(s->cells[index].text, sizeof(s->cells[index].text), text);
    s->cells[index].level = level;
    if (index >= s->count) {
        s->count = index + 1;
    }
    return GK_OK;
}

const char *gk_status_bar_text(const gk_status_bar *s, int index)
{
    if (s == NULL || index < 0 || index >= s->count) {
        return NULL;
    }
    return s->cells[index].text;
}

/* ---------------- data panel ---------------- */

void gk_data_panel_init(gk_data_panel *d)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    d->feed_override = 1.0;
    d->rapid_override = 1.0;
    d->spindle_override = 1.0;
}

void gk_data_panel_update(gk_data_panel *d, double elapsed, double feed)
{
    if (d == NULL) {
        return;
    }
    d->elapsed = elapsed;
    d->feed = feed;
    gk_data_panel_progress(d);
}

double gk_data_panel_remaining(const gk_data_panel *d)
{
    double rem;
    if (d == NULL || d->total_estimate <= 0.0) {
        return 0.0;
    }
    rem = d->total_estimate * (1.0 - d->progress);
    return rem > 0.0 ? rem : 0.0;
}

void gk_data_panel_progress(gk_data_panel *d)
{
    if (d == NULL || d->total_estimate <= 0.0) {
        return;
    }
    d->progress = d->elapsed / d->total_estimate;
    if (d->progress > 1.0) {
        d->progress = 1.0;
    }
    if (d->progress < 0.0) {
        d->progress = 0.0;
    }
}

int gk_data_panel_progress_bar(const gk_data_panel *d, int width)
{
    double p;
    if (d == NULL || width <= 0) {
        return 0;
    }
    p = d->progress;
    if (p < 0.0) p = 0.0;
    if (p > 1.0) p = 1.0;
    return (int)(p * width + 0.5);
}

/* ---------------- ui container ---------------- */

void gk_ui_init(gk_ui *ui)
{
    if (ui == NULL) {
        return;
    }
    memset(ui, 0, sizeof(*ui));
    ui->theme = GK_THEME_DARK;
    ui->lang = GK_LANG_EN;
    ui->view = GK_VIEW_ISO;
}

int gk_ui_add_widget(gk_ui *ui, const char *name, int x, int y, int w, int h)
{
    gk_widget *wd;
    if (ui == NULL || name == NULL || ui->count >= GK_UI_MAX_WIDGETS) {
        return -1;
    }
    if (gk_ui_find(ui, name) != NULL) {
        return -1;
    }
    wd = &ui->widgets[ui->count++];
    memset(wd, 0, sizeof(*wd));
    gk__copy(wd->name, sizeof(wd->name), name);
    wd->x = x;
    wd->y = y;
    wd->w = w;
    wd->h = h;
    wd->visible = 1;
    return ui->count - 1;
}

gk_widget *gk_ui_find(gk_ui *ui, const char *name)
{
    int i;
    if (ui == NULL || name == NULL) {
        return NULL;
    }
    for (i = 0; i < ui->count; ++i) {
        if (strcmp(ui->widgets[i].name, name) == 0) {
            return &ui->widgets[i];
        }
    }
    return NULL;
}

void gk_ui_set_visible(gk_ui *ui, const char *name, int visible)
{
    gk_widget *w = gk_ui_find(ui, name);
    if (w != NULL) {
        w->visible = visible ? 1 : 0;
    }
}

void gk_ui_switch_view(gk_ui *ui, gk_view_mode v)
{
    if (ui == NULL || v < 0 || v >= GK_VIEW_COUNT) {
        return;
    }
    ui->view = v;
}

void gk_ui_toggle_fullscreen(gk_ui *ui)
{
    if (ui != NULL) {
        ui->fullscreen = !ui->fullscreen;
    }
}

void gk_ui_set_theme(gk_ui *ui, gk_theme t)
{
    if (ui != NULL && (t == GK_THEME_DARK || t == GK_THEME_LIGHT)) {
        ui->theme = t;
    }
}

void gk_ui_set_lang(gk_ui *ui, gk_lang l)
{
    if (ui != NULL && (l == GK_LANG_EN || l == GK_LANG_ZH)) {
        ui->lang = l;
    }
}

typedef struct {
    const char *key;
    const char *en;
    const char *zh;
} gk_tr_entry;

static const gk_tr_entry g_tr[] = {
    { "menu.file", "File", "文件" },
    { "menu.edit", "Edit", "编辑" },
    { "menu.view", "View", "视图" },
    { "menu.tools", "Tools", "工具" },
    { "menu.help", "Help", "帮助" },
    { "mode.auto", "Auto", "自动" },
    { "mode.mdi", "MDI", "MDI" },
    { "mode.jog", "Jog", "手动" },
    { "status.ready", "Ready", "就绪" },
    { "status.alarm", "Alarm", "报警" },
};

const char *gk_ui_tr(gk_lang l, const char *key)
{
    size_t i;
    if (key == NULL) {
        return "";
    }
    for (i = 0; i < sizeof(g_tr) / sizeof(g_tr[0]); ++i) {
        if (strcmp(g_tr[i].key, key) == 0) {
            return l == GK_LANG_ZH ? g_tr[i].zh : g_tr[i].en;
        }
    }
    return key;
}

/* ---------------- panel styles ---------------- */

const char *gk_panel_style_name(gk_panel_style s)
{
    switch (s) {
    case GK_PANEL_FANUC: return "FANUC";
    case GK_PANEL_SIEMENS: return "SIEMENS";
    case GK_PANEL_MITSUBISHI: return "MITSUBISHI";
    case GK_PANEL_HAAS: return "HAAS";
    default: return "unknown";
    }
}

static const char *g_softkeys[GK_PANEL_COUNT][8] = {
    { "POS", "PROG", "OFFSET", "SYSTEM", "MESSAGE", "CUSTOM", "GRAPH", "PARAM" },
    { "MACHINE", "PARAM", "PROGRAM", "DIAGNOSIS", "STARTUP", "COMPENS", "", "" },
    { "MONITOR", "SETUP", "EDIT", "DIAG", "PARAM", "I/O", "", "" },
    { "SETNG", "POSIT", "ALARM", "CURNT", "PARAM", "GRAPH", "HELP", "" }
};

const char *gk_panel_softkey(gk_panel_style s, int index)
{
    if (s < 0 || s >= GK_PANEL_COUNT || index < 0 || index >= 8) {
        return "";
    }
    return g_softkeys[s][index];
}

/* ---------------- keyboard ---------------- */

void gk_keyboard_init(gk_keyboard *k)
{
    if (k == NULL) {
        return;
    }
    memset(k, 0, sizeof(*k));
}

int gk_keyboard_is_digit(const char *label)
{
    if (label == NULL || label[0] == '\0' || label[1] != '\0') {
        return 0;
    }
    return label[0] >= '0' && label[0] <= '9';
}

int gk_keyboard_is_letter(const char *label)
{
    if (label == NULL || label[0] == '\0' || label[1] != '\0') {
        return 0;
    }
    return (label[0] >= 'A' && label[0] <= 'Z') ||
           (label[0] >= 'a' && label[0] <= 'z');
}

gk_status gk_keyboard_press(gk_keyboard *k, const char *label)
{
    size_t l;
    gk_key *key;
    if (k == NULL || label == NULL || label[0] == '\0') {
        return GK_ERR_INVALID_ARG;
    }
    if (k->count < GK_UI_MAX_WIDGETS) {
        key = &k->keys[k->count++];
        memset(key, 0, sizeof(*key));
        gk__copy(key->label, sizeof(key->label), label);
        if (gk_keyboard_is_digit(label)) {
            key->kind = GK_KEY_DIGIT;
        } else if (gk_keyboard_is_letter(label)) {
            key->kind = GK_KEY_LETTER;
        } else if (label[0] == 'F' && label[1] >= '0' && label[1] <= '9') {
            key->kind = GK_KEY_FUNC;
        } else {
            key->kind = GK_KEY_SOFT;
        }
    }
    l = strlen(label);
    if (k->len + l < sizeof(k->buffer)) {
        memcpy(k->buffer + k->len, label, l);
        k->len += (int)l;
        k->buffer[k->len] = '\0';
    }
    return GK_OK;
}

void gk_keyboard_clear(gk_keyboard *k)
{
    if (k != NULL) {
        k->len = 0;
        k->buffer[0] = '\0';
    }
}

/* ---------------- MPG ---------------- */

void gk_mpg_init(gk_mpg *m)
{
    if (m == NULL) {
        return;
    }
    m->multiplier = 1;
    m->axis = -1;
}

gk_status gk_mpg_set_multiplier(gk_mpg *m, int mult)
{
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (mult != 1 && mult != 10 && mult != 100) {
        return GK_ERR_INVALID_ARG;
    }
    m->multiplier = mult;
    return GK_OK;
}

gk_status gk_mpg_select_axis(gk_mpg *m, int axis)
{
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (axis < 0 || axis > 5) {
        return GK_ERR_OUT_OF_RANGE;
    }
    m->axis = axis;
    return GK_OK;
}

double gk_mpg_step(const gk_mpg *m)
{
    if (m == NULL) {
        return 0.0;
    }
    return m->multiplier * 0.001;
}

/* ---------------- overrides / modes ---------------- */

void gk_overrides_init(gk_overrides *o)
{
    if (o == NULL) {
        return;
    }
    o->feed_override = 1.0;
    o->rapid_override = 1.0;
    o->spindle_override = 1.0;
}

gk_status gk_overrides_set(gk_overrides *o, int which, double value)
{
    if (o == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (value < 0.0 || value > 2.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    switch (which) {
    case 0: o->feed_override = value; break;
    case 1: o->rapid_override = value; break;
    case 2: o->spindle_override = value; break;
    default: return GK_ERR_INVALID_ARG;
    }
    return GK_OK;
}

const char *gk_op_mode_name(gk_op_mode m)
{
    switch (m) {
    case GK_MODE_EDIT: return "edit";
    case GK_MODE_AUTO: return "auto";
    case GK_MODE_MDI: return "mdi";
    case GK_MODE_JOG: return "jog";
    case GK_MODE_HANDLE: return "handle";
    case GK_MODE_MANUAL: return "manual";
    case GK_MODE_HOME: return "home";
    case GK_MODE_DNC: return "dnc";
    default: return "unknown";
    }
}

void gk_mode_switch_init(gk_mode_switch *s)
{
    if (s != NULL) {
        s->mode = GK_MODE_EDIT;
    }
}

gk_status gk_mode_switch_set(gk_mode_switch *s, gk_op_mode m)
{
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (m < 0 || m >= GK_MODE_COUNT) {
        return GK_ERR_INVALID_ARG;
    }
    s->mode = m;
    return GK_OK;
}

int gk_mode_switch_is_auto(const gk_mode_switch *s)
{
    return s != NULL && s->mode == GK_MODE_AUTO;
}

/* ---------------- panel buttons ---------------- */

void gk_panel_buttons_init(gk_panel_buttons *b)
{
    if (b == NULL) {
        return;
    }
    memset(b, 0, sizeof(*b));
    b->key_switch = 0;   /* locked by default */
}

gk_status gk_panel_press(gk_panel_buttons *b, int button_id)
{
    if (b == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    switch (button_id) {
    case GK_BTN_CYCLE_START:
        if (b->estop) {
            return GK_ERR_STATE;
        }
        b->cycle_start = 1;
        break;
    case GK_BTN_CYCLE_STOP:
        b->cycle_stop = 1;
        break;
    case GK_BTN_ESTOP:
        b->estop = 1;
        b->cycle_start = 0;
        break;
    case GK_BTN_KEY:
        b->key_switch = !b->key_switch;
        break;
    case GK_BTN_COOLANT:
        b->coolant = !b->coolant;
        break;
    case GK_BTN_SPINDLE:
        b->spindle = !b->spindle;
        break;
    case GK_BTN_LIGHT:
        b->light = !b->light;
        break;
    default:
        return GK_ERR_INVALID_ARG;
    }
    return GK_OK;
}

/* ---------------- diagnostic pages ---------------- */

gk_status gk_alarm_page_add(gk_alarm_page *p, int code, int severity,
                            const char *message)
{
    gk_alarm_row *r;
    if (p == NULL || p->count >= GK_UI_MAX_MENU_ITEMS) {
        return GK_ERR_INVALID_ARG;
    }
    r = &p->rows[p->count++];
    r->code = code;
    r->severity = severity;
    gk__copy(r->message, sizeof(r->message), message);
    return GK_OK;
}

gk_status gk_param_page_add(gk_param_page *p, int number, const char *name,
                            double value, const char *unit)
{
    gk_param_row *r;
    if (p == NULL || name == NULL || p->count >= GK_UI_MAX_MENU_ITEMS) {
        return GK_ERR_INVALID_ARG;
    }
    r = &p->rows[p->count++];
    r->number = number;
    gk__copy(r->name, sizeof(r->name), name);
    r->value = value;
    gk__copy(r->unit, sizeof(r->unit), unit);
    return GK_OK;
}

gk_status gk_param_page_set(gk_param_page *p, int number, double value)
{
    int i;
    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < p->count; ++i) {
        if (p->rows[i].number == number) {
            p->rows[i].value = value;
            return GK_OK;
        }
    }
    return GK_ERR_NOT_FOUND;
}

gk_status gk_diag_page_add(gk_diag_page *p, int index, const char *name,
                           const char *unit)
{
    gk_diag_row *r;
    if (p == NULL || name == NULL || p->count >= GK_UI_MAX_MENU_ITEMS) {
        return GK_ERR_INVALID_ARG;
    }
    r = &p->rows[p->count++];
    r->index = index;
    gk__copy(r->name, sizeof(r->name), name);
    r->value = 0.0;
    gk__copy(r->unit, sizeof(r->unit), unit);
    return GK_OK;
}

gk_status gk_diag_page_set(gk_diag_page *p, int index, double value)
{
    int i;
    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < p->count; ++i) {
        if (p->rows[i].index == index) {
            p->rows[i].value = value;
            return GK_OK;
        }
    }
    return GK_ERR_NOT_FOUND;
}

/* ---------------- waveform ---------------- */

void gk_waveform_init(gk_waveform *w)
{
    if (w != NULL) {
        memset(w, 0, sizeof(*w));
    }
}

void gk_waveform_push(gk_waveform *w, double sample)
{
    if (w == NULL) {
        return;
    }
    w->samples[w->head] = sample;
    w->head = (w->head + 1) % GK_WAVE_MAX;
    if (w->count < GK_WAVE_MAX) {
        w->count++;
    }
}

double gk_waveform_at(const gk_waveform *w, int i)
{
    int start;
    if (w == NULL || i < 0 || i >= w->count) {
        return 0.0;
    }
    start = (w->head - w->count + GK_WAVE_MAX) % GK_WAVE_MAX;
    return w->samples[(start + i) % GK_WAVE_MAX];
}

double gk_waveform_peak(const gk_waveform *w)
{
    double peak = 0.0;
    int i;
    if (w == NULL) {
        return 0.0;
    }
    for (i = 0; i < w->count; ++i) {
        double v = fabs(gk_waveform_at(w, i));
        if (v > peak) {
            peak = v;
        }
    }
    return peak;
}

/* ---------------- ladder ---------------- */

static gk_ladder_rung *gk__rung(gk_ladder *l, int id)
{
    int i;
    if (l == NULL) {
        return NULL;
    }
    for (i = 0; i < l->count; ++i) {
        if (l->rungs[i].id == id) {
            return &l->rungs[i];
        }
    }
    return NULL;
}

gk_status gk_ladder_add(gk_ladder *l, int id, const char *name)
{
    gk_ladder_rung *r;
    if (l == NULL || name == NULL || l->count >= GK_LADDER_MAX) {
        return GK_ERR_INVALID_ARG;
    }
    if (gk__rung(l, id) != NULL) {
        return GK_ERR_ALREADY_EXISTS;
    }
    r = &l->rungs[l->count++];
    memset(r, 0, sizeof(*r));
    r->id = id;
    gk__copy(r->name, sizeof(r->name), name);
    return GK_OK;
}

gk_status gk_ladder_set_input(gk_ladder *l, int id, int index, int value)
{
    gk_ladder_rung *r = gk__rung(l, id);
    if (r == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    if (index < 0 || index >= 8) {
        return GK_ERR_OUT_OF_RANGE;
    }
    r->inputs[index] = value ? 1 : 0;
    if (index >= r->input_count) {
        r->input_count = index + 1;
    }
    return GK_OK;
}

int gk_ladder_evaluate(const gk_ladder *l, int id)
{
    int i;
    const gk_ladder_rung *r = NULL;
    if (l == NULL) {
        return 0;
    }
    for (i = 0; i < l->count; ++i) {
        if (l->rungs[i].id == id) {
            r = &l->rungs[i];
            break;
        }
    }
    if (r == NULL) {
        return 0;
    }
    for (i = 0; i < r->input_count; ++i) {
        if (!r->inputs[i]) {
            return 0;
        }
    }
    return 1;
}

/* ---------------- PLC view ---------------- */

void gk_plc_view_init(gk_plc_view *p)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->run = 1;
}

void gk_plc_scan(gk_plc_view *p, double dt_ms)
{
    if (p == NULL || !p->run) {
        return;
    }
    p->scan_count++;
    p->scan_time_ms = dt_ms;
}

void gk_plc_set_io(gk_plc_view *p, int index, int value)
{
    if (p != NULL && index >= 0 && index < GK_UI_IO_SIGNALS) {
        p->io_signals[index] = value ? 1 : 0;
    }
}

int gk_plc_read_io(const gk_plc_view *p, int index)
{
    if (p == NULL || index < 0 || index >= GK_UI_IO_SIGNALS) {
        return -1;
    }
    return p->io_signals[index];
}

/* keep the clamp helper referenced for future layout use */
int gk_ui__clamp(int i, int n) { return gk__clamp_index(i, n); }
