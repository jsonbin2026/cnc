#ifndef GK_EDITOR_H
#define GK_EDITOR_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_EDITOR_MAX_LINES 4096
#define GK_EDITOR_LINE_LEN 256
#define GK_EDITOR_MAX_MARKS 512
#define GK_EDITOR_MAX_TOKENS 64

/* ---------------- syntax tokenizer (425) ---------------- */

typedef enum {
    GK_TOK_PLAIN = 0,
    GK_TOK_WORD,      /* G/M/S/T/F/N address */
    GK_TOK_NUMBER,
    GK_TOK_COMMENT,
    GK_TOK_BRACKET,
    GK_TOK_ERROR,
    GK_TOK_KIND_COUNT
} gk_token_kind;

typedef struct {
    gk_token_kind kind;
    int start;         /* column */
    int len;
} gk_token;

typedef struct {
    int line;
    int column;
    char message[128];
    int active;
} gk_diagnostic;

/* ---------------- editor document ---------------- */

typedef struct {
    char text[GK_EDITOR_LINE_LEN];
    int marked;         /* bookmark */
    int folded;         /* fold state */
    int indent;         /* indent level */
} gk_line;

typedef struct {
    gk_line lines[GK_EDITOR_MAX_LINES];
    int count;
    int cursor_line;
    int cursor_col;
    int modified;
    char path[256];
    int group;          /* for multi-file tab grouping */
} gk_doc;

typedef struct {
    int line;
    int col;
    char text[GK_EDITOR_LINE_LEN];
} gk_edit_op;

typedef struct {
    gk_doc *docs[GK_EDITOR_MAX_LINES > 64 ? 64 : 16];
    int count;
    int active;
    gk_edit_op undo[64];
    int undo_count;
    gk_edit_op redo[64];
    int redo_count;
} gk_editor;

/* 424 program edit area */
void gk_doc_init(gk_doc *d);
gk_status gk_doc_set_text(gk_doc *d, const char *text);   /* splits on \n */
const char *gk_doc_line(const gk_doc *d, int line);
int gk_doc_line_count(const gk_doc *d);

/* 425 syntax highlighting: tokenize a line. Returns token count. */
int gk_doc_tokenize(const char *line, gk_token *out, int max_out);

/* 426 line numbers are implicit via gk_doc_line; helper formats them. */
void gk_doc_line_number(int line, char *buf, size_t len);

/* 427 error indication */
gk_status gk_doc_add_diagnostic(gk_doc *d, int line, int col,
                                const char *message);

/* 428 autocompletion: suggest known codes matching the prefix. */
int gk_doc_complete(const char *prefix, char out[][16], int max_out);

/* 429 find / 430 replace / 431 goto line */
int gk_doc_find(const gk_doc *d, const char *needle, int from_line);
int gk_doc_replace(gk_doc *d, const char *needle, const char *repl, int all);
gk_status gk_doc_goto(gk_doc *d, int line, int col);

/* 432 copy/paste */
gk_status gk_doc_copy(const gk_doc *d, int line, char *buf, size_t len);
gk_status gk_doc_paste(gk_doc *d, int line, const char *text);

/* 433 undo/redo */
void gk_editor_init(gk_editor *e);
gk_status gk_editor_open(gk_editor *e, const char *path);   /* new tab */
gk_doc *gk_editor_active(gk_editor *e);
gk_status gk_editor_close(gk_editor *e, int index);
gk_status gk_editor_edit_line(gk_editor *e, int line, const char *text);
gk_status gk_editor_undo(gk_editor *e);
gk_status gk_editor_redo(gk_editor *e);

/* 434 auto-indent, 435 bracket matching */
void gk_doc_autoindent(gk_doc *d, int line);
int gk_doc_match_bracket(const char *line, int pos, char *open, char *close);

/* 436 code folding */
gk_status gk_doc_fold(gk_doc *d, int line, int folded);
int gk_doc_is_folded(const gk_doc *d, int line);
int gk_doc_visible_lines(const gk_doc *d);

/* 437 bookmarks */
gk_status gk_doc_bookmark(gk_doc *d, int line, int on);
int gk_doc_bookmark_count(const gk_doc *d);
int gk_doc_next_bookmark(const gk_doc *d, int from_line);

/* 438 multi-file edit */
int gk_editor_file_count(const gk_editor *e);
gk_doc *gk_editor_file(gk_editor *e, int index);
gk_status gk_editor_activate(gk_editor *e, int index);

/* 439 file compare: returns first differing line index or -1 when equal. */
int gk_doc_compare(const gk_doc *a, const gk_doc *b);

/* ---------------- interaction (440-453) ---------------- */

typedef struct {
    int line;
    int enabled;
} gk_breakpt;

typedef struct {
    gk_doc *doc;
    int cursor_line;
    int cursor_col;
    int selecting;
    int sel_anchor;
    int executing;
    gk_breakpt breakpoints[GK_EDITOR_MAX_MARKS];
    int break_count;
    double speed;          /* 443 rate slider, 0.1..10 */
    int touch_enabled;     /* 444 */
    int gamepad_enabled;   /* 445 */
    char keymap[32][2][24];/* 446 custom shortcuts */
    int keymap_count;
    int zoom;              /* 449 wheel zoom % */
    char hover_text[128];  /* 452 */
} gk_interaction;

void gk_interaction_init(gk_interaction *it, gk_doc *doc);
/* 440 single step: advances executing flag and cursor line. */
gk_status gk_interact_step(gk_interaction *it);
/* 441 set / 442 clear breakpoints */
gk_status gk_interact_set_breakpoint(gk_interaction *it, int line, int on);
int gk_interact_is_breakpoint(const gk_interaction *it, int line);
/* run until next breakpoint or end; returns the line it stopped on or -1. */
int gk_interact_run(gk_interaction *it);
gk_status gk_interact_set_speed(gk_interaction *it, double speed);
/* 446 shortcut remap: bind an action to a key. */
gk_status gk_interact_map_key(gk_interaction *it, const char *key,
                              const char *action);
const char *gk_interact_action_for_key(const gk_interaction *it,
                                       const char *key);
/* 449 zoom */
gk_status gk_interact_zoom(gk_interaction *it, int delta);
/* 451 double click selects a word; returns column range via out. */
gk_status gk_interact_double_click(gk_interaction *it, int line, int col,
                                   int *start, int *end);
/* 452 hover tooltip */
gk_status gk_interact_hover(gk_interaction *it, int line, int col);
int gk_interact_hover_len(const gk_interaction *it);
/* 453 keyboard navigation */
gk_status gk_interact_move(gk_interaction *it, int dline, int dcol);

#ifdef __cplusplus
}
#endif

#endif /* GK_EDITOR_H */
