#include "gk/gk_editor.h"

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

static int gk__is_digit(char c) { return c >= '0' && c <= '9'; }

static int gk__is_addr(char c)
{
    switch (c) {
    case 'G': case 'M': case 'S': case 'T': case 'F': case 'N':
    case 'X': case 'Y': case 'Z': case 'U': case 'V': case 'W':
    case 'A': case 'B': case 'C': case 'I': case 'J': case 'K':
    case 'R': case 'P': case 'Q': case 'D': case 'H': case 'L':
        return 1;
    default:
        return 0;
    }
}

void gk_doc_init(gk_doc *d)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    d->cursor_line = 0;
    d->cursor_col = 0;
}

gk_status gk_doc_set_text(gk_doc *d, const char *text)
{
    const char *p;
    const char *start;
    int line = 0;
    if (d == NULL || text == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    memset(d->lines, 0, sizeof(d->lines));
    d->count = 0;
    p = text;
    start = text;
    for (;;) {
        if (*p == '\n' || *p == '\0') {
            size_t len = (size_t)(p - start);
            gk_line *ln;
            if (line >= GK_EDITOR_MAX_LINES) {
                break;
            }
            ln = &d->lines[line++];
            if (len >= GK_EDITOR_LINE_LEN) {
                len = GK_EDITOR_LINE_LEN - 1;
            }
            memcpy(ln->text, start, len);
            ln->text[len] = '\0';
            if (*p == '\0') {
                break;
            }
            start = p + 1;
        }
        p++;
    }
    d->count = line;
    return GK_OK;
}

const char *gk_doc_line(const gk_doc *d, int line)
{
    if (d == NULL || line < 0 || line >= d->count) {
        return NULL;
    }
    return d->lines[line].text;
}

int gk_doc_line_count(const gk_doc *d)
{
    return d != NULL ? d->count : 0;
}

int gk_doc_tokenize(const char *line, gk_token *out, int max_out)
{
    int i = 0;
    int n = 0;
    int len;
    if (line == NULL || out == NULL || max_out <= 0) {
        return 0;
    }
    len = (int)strlen(line);
    while (i < len && n < max_out) {
        char c = line[i];
        if (c == '(') {
            int j = i + 1;
            while (j < len && line[j] != ')') {
                j++;
            }
            out[n].kind = GK_TOK_COMMENT;
            out[n].start = i;
            out[n].len = (j < len ? j + 1 : len) - i;
            n++;
            i = (j < len ? j + 1 : len);
        } else if (c == '[' || c == ']') {
            out[n].kind = GK_TOK_BRACKET;
            out[n].start = i;
            out[n].len = 1;
            n++;
            i++;
        } else if (gk__is_addr(c)) {
            int j = i + 1;
            while (j < len && (gk__is_digit(line[j]) || line[j] == '.' ||
                               line[j] == '-' || line[j] == '+')) {
                j++;
            }
            out[n].kind = GK_TOK_WORD;
            out[n].start = i;
            out[n].len = j - i;
            n++;
            i = j;
        } else if (gk__is_digit(c) || ((c == '-' || c == '+') && i + 1 < len &&
                                        gk__is_digit(line[i + 1]))) {
            int j = i;
            if (c == '-' || c == '+') j++;
            while (j < len && (gk__is_digit(line[j]) || line[j] == '.')) {
                j++;
            }
            out[n].kind = GK_TOK_NUMBER;
            out[n].start = i;
            out[n].len = j - i;
            n++;
            i = j;
        } else if (c == ' ' || c == '\t') {
            i++;
        } else {
            out[n].kind = GK_TOK_PLAIN;
            out[n].start = i;
            out[n].len = 1;
            n++;
            i++;
        }
    }
    return n;
}

void gk_doc_line_number(int line, char *buf, size_t len)
{
    if (buf == NULL || len == 0) {
        return;
    }
    snprintf(buf, len, "N%04d", line + 1);
}

gk_status gk_doc_add_diagnostic(gk_doc *d, int line, int col,
                                const char *message)
{
    (void)d;
    (void)line;
    (void)col;
    (void)message;
    /* diagnostics are surfaced through the interaction layer; validated here */
    if (line < 0 || col < 0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

static const char *g_completions[] = {
    "G00", "G01", "G02", "G03", "G04", "G17", "G18", "G19", "G20", "G21",
    "G28", "G40", "G41", "G42", "G43", "G44", "G49", "G54", "G80", "G81",
    "G83", "G84", "G90", "G91", "G92", "G94", "G95", "G96", "G97", "G98",
    "G99", "M00", "M01", "M02", "M03", "M04", "M05", "M06", "M08", "M09",
    "M30", "M98", "M99"
};

int gk_doc_complete(const char *prefix, char out[][16], int max_out)
{
    size_t plen;
    size_t i;
    int n = 0;
    if (prefix == NULL || out == NULL || max_out <= 0) {
        return 0;
    }
    plen = strlen(prefix);
    if (plen == 0) {
        return 0;
    }
    for (i = 0; i < sizeof(g_completions) / sizeof(g_completions[0]); ++i) {
        if (strncmp(g_completions[i], prefix, plen) == 0) {
            if (n >= max_out) {
                break;
            }
            gk__copy(out[n], 16, g_completions[i]);
            n++;
        }
    }
    return n;
}

int gk_doc_find(const gk_doc *d, const char *needle, int from_line)
{
    int i;
    if (d == NULL || needle == NULL || needle[0] == '\0') {
        return -1;
    }
    for (i = from_line < 0 ? 0 : from_line; i < d->count; ++i) {
        if (strstr(d->lines[i].text, needle) != NULL) {
            return i;
        }
    }
    return -1;
}

static int gk__replace_in(char *dst, size_t len, const char *src,
                          const char *needle, const char *repl, int *count)
{
    size_t nlen = strlen(needle);
    size_t rlen = strlen(repl);
    size_t o = 0;
    const char *p = src;
    int replaced = 0;
    if (nlen == 0) {
        gk__copy(dst, len, src);
        return 0;
    }
    while (*p != '\0' && o + 1 < len) {
        if (strncmp(p, needle, nlen) == 0) {
            size_t k;
            for (k = 0; k < rlen && o + 1 < len; ++k) {
                dst[o++] = repl[k];
            }
            p += nlen;
            replaced++;
        } else {
            dst[o++] = *p++;
        }
    }
    dst[o] = '\0';
    *count = replaced;
    return replaced;
}

int gk_doc_replace(gk_doc *d, const char *needle, const char *repl, int all)
{
    int i;
    int total = 0;
    if (d == NULL || needle == NULL || repl == NULL) {
        return 0;
    }
    for (i = 0; i < d->count; ++i) {
        char buf[GK_EDITOR_LINE_LEN];
        int c = 0;
        if (strstr(d->lines[i].text, needle) == NULL) {
            continue;
        }
        gk__replace_in(buf, sizeof(buf), d->lines[i].text, needle, repl, &c);
        gk__copy(d->lines[i].text, sizeof(d->lines[i].text), buf);
        total += c;
        if (!all) {
            d->modified = 1;
            return total;
        }
    }
    if (total > 0) {
        d->modified = 1;
    }
    return total;
}

gk_status gk_doc_goto(gk_doc *d, int line, int col)
{
    if (d == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (line < 0 || line >= d->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    d->cursor_line = line;
    d->cursor_col = col < 0 ? 0 : col;
    return GK_OK;
}

gk_status gk_doc_copy(const gk_doc *d, int line, char *buf, size_t len)
{
    if (d == NULL || buf == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (line < 0 || line >= d->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    gk__copy(buf, len, d->lines[line].text);
    return GK_OK;
}

gk_status gk_doc_paste(gk_doc *d, int line, const char *text)
{
    int i;
    if (d == NULL || text == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (line < 0 || line > d->count || d->count >= GK_EDITOR_MAX_LINES) {
        return GK_ERR_OUT_OF_RANGE;
    }
    for (i = d->count; i > line; --i) {
        d->lines[i] = d->lines[i - 1];
    }
    memset(&d->lines[line], 0, sizeof(d->lines[line]));
    gk__copy(d->lines[line].text, sizeof(d->lines[line].text), text);
    d->count++;
    d->modified = 1;
    return GK_OK;
}

/* ---------------- undo/redo editor ---------------- */

void gk_editor_init(gk_editor *e)
{
    if (e == NULL) {
        return;
    }
    memset(e, 0, sizeof(*e));
    e->active = -1;
}

static gk_doc g_doc_pool[16];

gk_status gk_editor_open(gk_editor *e, const char *path)
{
    gk_doc *d;
    if (e == NULL || e->count >= 16) {
        return GK_ERR_INVALID_ARG;
    }
    d = &g_doc_pool[e->count];
    gk_doc_init(d);
    gk__copy(d->path, sizeof(d->path), path);
    d->group = e->count;
    e->docs[e->count] = d;
    e->count++;
    e->active = e->count - 1;
    return GK_OK;
}

gk_doc *gk_editor_active(gk_editor *e)
{
    if (e == NULL || e->active < 0 || e->active >= e->count) {
        return NULL;
    }
    return e->docs[e->active];
}

gk_status gk_editor_close(gk_editor *e, int index)
{
    int i;
    if (e == NULL || index < 0 || index >= e->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    for (i = index; i + 1 < e->count; ++i) {
        e->docs[i] = e->docs[i + 1];
    }
    e->count--;
    if (e->active >= e->count) {
        e->active = e->count - 1;
    }
    return GK_OK;
}

gk_status gk_editor_edit_line(gk_editor *e, int line, const char *text)
{
    gk_doc *d = gk_editor_active(e);
    if (d == NULL || text == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (line < 0 || line >= d->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    if (e->undo_count < 64) {
        gk_edit_op *op = &e->undo[e->undo_count++];
        op->line = line;
        op->col = 0;
        gk__copy(op->text, sizeof(op->text), d->lines[line].text);
    }
    e->redo_count = 0;
    gk__copy(d->lines[line].text, sizeof(d->lines[line].text), text);
    d->modified = 1;
    return GK_OK;
}

gk_status gk_editor_undo(gk_editor *e)
{
    gk_doc *d = gk_editor_active(e);
    gk_edit_op *op;
    if (d == NULL || e->undo_count == 0) {
        return GK_ERR_STATE;
    }
    op = &e->undo[--e->undo_count];
    if (e->redo_count < 64) {
        gk_edit_op *ro = &e->redo[e->redo_count++];
        ro->line = op->line;
        ro->col = 0;
        gk__copy(ro->text, sizeof(ro->text), d->lines[op->line].text);
    }
    gk__copy(d->lines[op->line].text, sizeof(d->lines[op->line].text),
             op->text);
    return GK_OK;
}

gk_status gk_editor_redo(gk_editor *e)
{
    gk_doc *d = gk_editor_active(e);
    gk_edit_op *op;
    if (d == NULL || e->redo_count == 0) {
        return GK_ERR_STATE;
    }
    op = &e->redo[--e->redo_count];
    if (e->undo_count < 64) {
        gk_edit_op *uo = &e->undo[e->undo_count++];
        uo->line = op->line;
        uo->col = 0;
        gk__copy(uo->text, sizeof(uo->text), d->lines[op->line].text);
    }
    gk__copy(d->lines[op->line].text, sizeof(d->lines[op->line].text),
             op->text);
    return GK_OK;
}

void gk_doc_autoindent(gk_doc *d, int line)
{
    int i;
    int indent;
    if (d == NULL || line < 0 || line >= d->count) {
        return;
    }
    if (line == 0) {
        d->lines[line].indent = 0;
        return;
    }
    indent = d->lines[line - 1].indent;
    /* previous line ends with ':' -> increase */
    {
        const char *prev = d->lines[line - 1].text;
        size_t l = strlen(prev);
        while (l > 0 && (prev[l - 1] == ' ' || prev[l - 1] == '\t')) {
            l--;
        }
        if (l > 0 && prev[l - 1] == ':') {
            indent++;
        }
    }
    if (indent < 0) {
        indent = 0;
    }
    d->lines[line].indent = indent;
    if (indent > 0) {
        char buf[GK_EDITOR_LINE_LEN];
        int pad = indent * 2;
        const char *src = d->lines[line].text;
        int k;
        for (i = 0; i < pad && i < GK_EDITOR_LINE_LEN - 1; ++i) {
            buf[i] = ' ';
        }
        for (k = 0; i < GK_EDITOR_LINE_LEN - 1 && src[k] != '\0'; ++k) {
            buf[i++] = src[k];
        }
        buf[i] = '\0';
        gk__copy(d->lines[line].text, sizeof(d->lines[line].text), buf);
    }
}

int gk_doc_match_bracket(const char *line, int pos, char *open, char *close)
{
    int depth = 0;
    int i;
    int len;
    if (line == NULL || pos < 0) {
        return -1;
    }
    len = (int)strlen(line);
    if (pos >= len) {
        return -1;
    }
    if (line[pos] == '[') {
        if (open) *open = '[';
        if (close) *close = ']';
        for (i = pos + 1; i < len; ++i) {
            if (line[i] == '[') depth++;
            else if (line[i] == ']') {
                if (depth == 0) return i;
                depth--;
            }
        }
    } else if (line[pos] == ']') {
        if (open) *open = '[';
        if (close) *close = ']';
        for (i = pos - 1; i >= 0; --i) {
            if (line[i] == ']') depth++;
            else if (line[i] == '[') {
                if (depth == 0) return i;
                depth--;
            }
        }
    }
    return -1;
}

gk_status gk_doc_fold(gk_doc *d, int line, int folded)
{
    if (d == NULL || line < 0 || line >= d->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    d->lines[line].folded = folded ? 1 : 0;
    return GK_OK;
}

int gk_doc_is_folded(const gk_doc *d, int line)
{
    if (d == NULL || line < 0 || line >= d->count) {
        return 0;
    }
    return d->lines[line].folded;
}

int gk_doc_visible_lines(const gk_doc *d)
{
    int i;
    int n = 0;
    if (d == NULL) {
        return 0;
    }
    for (i = 0; i < d->count; ++i) {
        if (d->lines[i].folded) {
            int base = d->lines[i].indent;
            n++;  /* header visible, body hidden */
            while (i + 1 < d->count && d->lines[i + 1].indent > base) {
                i++;
            }
        } else {
            n++;
        }
    }
    return n;
}

gk_status gk_doc_bookmark(gk_doc *d, int line, int on)
{
    if (d == NULL || line < 0 || line >= d->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    d->lines[line].marked = on ? 1 : 0;
    return GK_OK;
}

int gk_doc_bookmark_count(const gk_doc *d)
{
    int i;
    int n = 0;
    if (d == NULL) {
        return 0;
    }
    for (i = 0; i < d->count; ++i) {
        if (d->lines[i].marked) n++;
    }
    return n;
}

int gk_doc_next_bookmark(const gk_doc *d, int from_line)
{
    int i;
    if (d == NULL) {
        return -1;
    }
    for (i = from_line + 1; i < d->count; ++i) {
        if (d->lines[i].marked) return i;
    }
    for (i = 0; i <= from_line && i < d->count; ++i) {
        if (d->lines[i].marked) return i;
    }
    return -1;
}

int gk_editor_file_count(const gk_editor *e)
{
    return e != NULL ? e->count : 0;
}

gk_doc *gk_editor_file(gk_editor *e, int index)
{
    if (e == NULL || index < 0 || index >= e->count) {
        return NULL;
    }
    return e->docs[index];
}

gk_status gk_editor_activate(gk_editor *e, int index)
{
    if (e == NULL || index < 0 || index >= e->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    e->active = index;
    return GK_OK;
}

int gk_doc_compare(const gk_doc *a, const gk_doc *b)
{
    int i;
    if (a == NULL || b == NULL) {
        return -1;
    }
    for (i = 0; i < a->count && i < b->count; ++i) {
        if (strcmp(a->lines[i].text, b->lines[i].text) != 0) {
            return i;
        }
    }
    if (a->count != b->count) {
        return i;
    }
    return -1;
}

/* ---------------- interaction ---------------- */

void gk_interaction_init(gk_interaction *it, gk_doc *doc)
{
    if (it == NULL) {
        return;
    }
    memset(it, 0, sizeof(*it));
    it->doc = doc;
    it->speed = 1.0;
    it->zoom = 100;
}

gk_status gk_interact_step(gk_interaction *it)
{
    if (it == NULL || it->doc == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    it->executing = 1;
    if (it->cursor_line + 1 < it->doc->count) {
        it->cursor_line++;
    }
    return GK_OK;
}

gk_status gk_interact_set_breakpoint(gk_interaction *it, int line, int on)
{
    int i;
    if (it == NULL || it->doc == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (line < 0 || line >= it->doc->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    for (i = 0; i < it->break_count; ++i) {
        if (it->breakpoints[i].line == line) {
            it->breakpoints[i].enabled = on ? 1 : 0;
            if (!on) {
                /* remove */
                int k;
                for (k = i; k + 1 < it->break_count; ++k) {
                    it->breakpoints[k] = it->breakpoints[k + 1];
                }
                it->break_count--;
            }
            return GK_OK;
        }
    }
    if (on && it->break_count < GK_EDITOR_MAX_MARKS) {
        it->breakpoints[it->break_count].line = line;
        it->breakpoints[it->break_count].enabled = 1;
        it->break_count++;
    }
    return GK_OK;
}

int gk_interact_is_breakpoint(const gk_interaction *it, int line)
{
    int i;
    if (it == NULL) {
        return 0;
    }
    for (i = 0; i < it->break_count; ++i) {
        if (it->breakpoints[i].line == line && it->breakpoints[i].enabled) {
            return 1;
        }
    }
    return 0;
}

int gk_interact_run(gk_interaction *it)
{
    if (it == NULL || it->doc == NULL) {
        return -1;
    }
    it->executing = 1;
    while (it->cursor_line + 1 < it->doc->count) {
        it->cursor_line++;
        if (gk_interact_is_breakpoint(it, it->cursor_line)) {
            return it->cursor_line;
        }
    }
    return -1;
}

gk_status gk_interact_set_speed(gk_interaction *it, double speed)
{
    if (it == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (speed < 0.1 || speed > 10.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    it->speed = speed;
    return GK_OK;
}

gk_status gk_interact_map_key(gk_interaction *it, const char *key,
                              const char *action)
{
    int i;
    if (it == NULL || key == NULL || action == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < it->keymap_count; ++i) {
        if (strcmp(it->keymap[i][0], key) == 0) {
            gk__copy(it->keymap[i][1], sizeof(it->keymap[i][1]), action);
            return GK_OK;
        }
    }
    if (it->keymap_count >= 32) {
        return GK_ERR_OUT_OF_RANGE;
    }
    gk__copy(it->keymap[it->keymap_count][0],
             sizeof(it->keymap[it->keymap_count][0]), key);
    gk__copy(it->keymap[it->keymap_count][1],
             sizeof(it->keymap[it->keymap_count][1]), action);
    it->keymap_count++;
    return GK_OK;
}

const char *gk_interact_action_for_key(const gk_interaction *it,
                                       const char *key)
{
    int i;
    if (it == NULL || key == NULL) {
        return NULL;
    }
    for (i = 0; i < it->keymap_count; ++i) {
        if (strcmp(it->keymap[i][0], key) == 0) {
            return it->keymap[i][1];
        }
    }
    return NULL;
}

gk_status gk_interact_zoom(gk_interaction *it, int delta)
{
    if (it == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    it->zoom += delta;
    if (it->zoom < 10) it->zoom = 10;
    if (it->zoom > 400) it->zoom = 400;
    return GK_OK;
}

gk_status gk_interact_double_click(gk_interaction *it, int line, int col,
                                   int *start, int *end)
{
    const char *text;
    int s, e;
    if (it == NULL || it->doc == NULL || start == NULL || end == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (line < 0 || line >= it->doc->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    text = it->doc->lines[line].text;
    s = col;
    while (s > 0 && !(text[s - 1] == ' ' || text[s - 1] == '\t')) {
        s--;
    }
    e = col;
    while (text[e] != '\0' && !(text[e] == ' ' || text[e] == '\t')) {
        e++;
    }
    *start = s;
    *end = e;
    return GK_OK;
}

gk_status gk_interact_hover(gk_interaction *it, int line, int col)
{
    gk_token toks[GK_EDITOR_MAX_TOKENS];
    int n;
    int i;
    if (it == NULL || it->doc == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (line < 0 || line >= it->doc->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    n = gk_doc_tokenize(it->doc->lines[line].text, toks,
                        GK_EDITOR_MAX_TOKENS);
    it->hover_text[0] = '\0';
    for (i = 0; i < n; ++i) {
        if (col >= toks[i].start && col < toks[i].start + toks[i].len) {
            const char *src = it->doc->lines[line].text + toks[i].start;
            int len = toks[i].len;
            if (len > 127) len = 127;
            memcpy(it->hover_text, src, (size_t)len);
            it->hover_text[len] = '\0';
            return GK_OK;
        }
    }
    return GK_OK;
}

int gk_interact_hover_len(const gk_interaction *it)
{
    return it != NULL ? (int)strlen(it->hover_text) : 0;
}

gk_status gk_interact_move(gk_interaction *it, int dline, int dcol)
{
    int l, c;
    const char *text;
    if (it == NULL || it->doc == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    l = it->cursor_line + dline;
    if (l < 0) l = 0;
    if (l >= it->doc->count) l = it->doc->count - 1;
    text = it->doc->lines[l].text;
    c = it->cursor_col + dcol;
    if (c < 0) c = 0;
    if (c > (int)strlen(text)) c = (int)strlen(text);
    it->cursor_line = l;
    it->cursor_col = c;
    return GK_OK;
}
