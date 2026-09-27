#include "gk/gk_parser.h"
#include "gk/gk_codes.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static void diag_add(gk_program *p, gk_diag_severity sev, size_t line,
                     size_t column, int code, const char *fmt, ...)
{
    gk_diagnostic d;
    va_list args;
    memset(&d, 0, sizeof(d));
    d.severity = sev;
    d.line = line;
    d.column = column;
    d.code = code;
    va_start(args, fmt);
    vsnprintf(d.message, sizeof(d.message), fmt, args);
    va_end(args);

    if (p->diagnostic_count == p->diagnostic_cap) {
        size_t next = p->diagnostic_cap == 0 ? 8 : p->diagnostic_cap * 2;
        gk_diagnostic *mem = gk_realloc(&p->alloc, p->diagnostics,
                                        next * sizeof(*mem));
        if (mem == NULL) {
            return;
        }
        p->diagnostics = mem;
        p->diagnostic_cap = next;
    }
    p->diagnostics[p->diagnostic_count++] = d;
    if (sev == GK_DIAG_ERROR) {
        p->error_count += 1;
    } else if (sev == GK_DIAG_WARN) {
        p->warn_count += 1;
    }
}

void gk_program_init(gk_program *p, const gk_allocator *alloc)
{
    if (p == NULL) {
        return;
    }
    p->blocks = NULL;
    p->block_count = 0;
    p->diagnostics = NULL;
    p->diagnostic_count = 0;
    p->diagnostic_cap = 0;
    p->error_count = 0;
    p->warn_count = 0;
    p->alloc = alloc != NULL ? *alloc : gk_allocator_default();
}

void gk_program_free(gk_program *p)
{
    if (p == NULL) {
        return;
    }
    gk_free(&p->alloc, p->blocks);
    gk_free(&p->alloc, p->diagnostics);
    p->blocks = NULL;
    p->block_count = 0;
    p->diagnostics = NULL;
    p->diagnostic_count = 0;
    p->diagnostic_cap = 0;
}

static gk_status push_block(gk_program *p, const gk_block *b)
{
    gk_block *mem = gk_realloc(&p->alloc, p->blocks,
                               (p->block_count + 1) * sizeof(*mem));
    if (mem == NULL) {
        return GK_ERR_NO_MEMORY;
    }
    p->blocks = mem;
    p->blocks[p->block_count] = *b;
    p->block_count += 1;
    return GK_OK;
}

static void begin_block(gk_block *b, size_t line)
{
    memset(b, 0, sizeof(*b));
    b->line = line;
}

static gk_status flush_block(gk_program *p, gk_block *b, size_t *next_line)
{
    /* A block with no words and no comment is an empty line: skip. */
    if (b->word_count == 0 && !b->has_comment) {
        begin_block(b, *next_line);
        return GK_OK;
    }
    b->line = b->line == 0 ? *next_line : b->line;
    return push_block(p, b);
}

gk_status gk_program_parse(gk_program *p, const char *source, size_t length)
{
    gk_lexer lx;
    gk_token tok;
    gk_block cur;
    gk_status st;

    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (source == NULL) {
        return GK_ERR_INVALID_ARG;
    }

    gk_lexer_init(&lx, source, length);
    begin_block(&cur, 1);

    for (;;) {
        st = gk_lexer_next(&lx, &tok);
        if (st != GK_OK) {
            diag_add(p, GK_DIAG_ERROR, lx.error_line, lx.error_column, (int)st,
                     "lexical error: %s",
                     lx.error_message[0] != '\0' ? lx.error_message
                                                 : gk_status_message(st));
            return st;
        }

        if (tok.kind == GK_TOK_EOF) {
            st = flush_block(p, &cur, &tok.line);
            if (st != GK_OK) {
                return st;
            }
            break;
        }

        /* New source line: NC treats one line as one block unless a
         * semicolon continues it. Flush the pending block first. */
        if (cur.word_count > 0 && tok.line > cur.line) {
            st = flush_block(p, &cur, &tok.line);
            if (st != GK_OK) {
                return st;
            }
            begin_block(&cur, tok.line);
        }

        if (tok.kind == GK_TOK_SEMI) {
            /* end of block */
            st = flush_block(p, &cur, &tok.line);
            if (st != GK_OK) {
                return st;
            }
            begin_block(&cur, lx.line);
            continue;
        }

        if (tok.kind == GK_TOK_COMMENT) {
            size_t n = tok.length >= 2 ? tok.length - 2 : 0;
            const char *src = source + tok.offset + 1;
            if (n >= sizeof(cur.comment)) {
                n = sizeof(cur.comment) - 1;
            }
            memcpy(cur.comment, src, n);
            cur.comment[n] = '\0';
            cur.has_comment = 1;
            continue;
        }

        /* line number at start of a block */
        if (tok.kind == GK_TOK_LINE_NO && cur.word_count == 0 &&
            !cur.has_block_number) {
            cur.block_number = tok.ivalue;
            cur.has_block_number = 1;
            cur.line = tok.line;
            continue;
        }

        if (gk_token_kind_is_word(tok.kind)) {
            gk_word *w;
            if (cur.word_count >= GK_MAX_WORDS_PER_BLOCK) {
                diag_add(p, GK_DIAG_ERROR, tok.line, tok.column, 0,
                         "too many words in block (max %d)",
                         GK_MAX_WORDS_PER_BLOCK);
                continue;
            }
            w = &cur.words[cur.word_count++];
            memset(w, 0, sizeof(*w));
            w->letter = tok.letter;
            w->value = tok.value;
            w->ivalue = tok.ivalue;
            w->has_value = tok.has_value;
            w->decimal_count = tok.decimal_count;
            w->column = tok.column;
            w->is_gcode = (tok.kind == GK_TOK_GCODE);
            w->is_mcode = (tok.kind == GK_TOK_MCODE);
            w->is_line_no = (tok.kind == GK_TOK_LINE_NO);
            if (cur.word_count == 1 && tok.kind != GK_TOK_LINE_NO) {
                cur.line = tok.line;
            }
            continue;
        }

        /* Slash at start of block is block-skip marker */
        if (tok.kind == GK_TOK_SLASH && cur.word_count == 0) {
            cur.block_skip = 1;
            cur.line = tok.line;
            continue;
        }

        diag_add(p, GK_DIAG_ERROR, tok.line, tok.column, 0,
                 "unexpected token '%s' in block",
                 gk_token_kind_name(tok.kind));
    }

    return p->error_count == 0 ? GK_OK : GK_ERR_PARSE;
}

int gk_gcode_is_defined(int code)
{
    size_t i;
    size_t n;
    const gk_code_def *t = gk_gcode_table(&n);
    for (i = 0; i < n; ++i) {
        if (t[i].code == code) {
            return 1;
        }
    }
    return 0;
}

int gk_mcode_is_defined(int code)
{
    return gk_mcode_lookup(code) != NULL;
}

const char *gk_gcode_name(int code)
{
    return gk_gcode_lookup(code, 0);
}

const char *gk_mcode_name(int code)
{
    return gk_mcode_lookup(code);
}

gk_status gk_program_validate(gk_program *p)
{
    size_t i;
    size_t initial_errors;
    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    initial_errors = p->error_count;

    for (i = 0; i < p->block_count; ++i) {
        gk_block *b = &p->blocks[i];
        size_t j;
        int has_motion = 0;
        int has_coord = 0;
        for (j = 0; j < b->word_count; ++j) {
            gk_word *w = &b->words[j];
            if (w->is_gcode) {
                if (!gk_gcode_is_defined(w->ivalue)) {
                    diag_add(p, GK_DIAG_ERROR, b->line, w->column, w->ivalue,
                             "undefined G code G%02d", w->ivalue);
                } else if (w->ivalue >= 0 && w->ivalue <= 3) {
                    has_motion = 1;
                }
            } else if (w->is_mcode) {
                if (!gk_mcode_is_defined(w->ivalue)) {
                    diag_add(p, GK_DIAG_ERROR, b->line, w->column, w->ivalue,
                             "undefined M code M%02d", w->ivalue);
                }
            } else if ((w->letter == 'X' || w->letter == 'Y' ||
                        w->letter == 'Z' || w->letter == 'A' ||
                        w->letter == 'B' || w->letter == 'C' ||
                        w->letter == 'I' || w->letter == 'J' ||
                        w->letter == 'K' || w->letter == 'R') &&
                       w->has_value) {
                has_coord = 1;
            }
        }
        (void)has_motion;
        (void)has_coord;
    }

    return p->error_count == initial_errors ? GK_OK : GK_ERR_PARSE;
}

const gk_block *gk_program_block(const gk_program *p, size_t index)
{
    if (p == NULL || index >= p->block_count) {
        return NULL;
    }
    return &p->blocks[index];
}

size_t gk_program_block_count(const gk_program *p)
{
    return p != NULL ? p->block_count : 0;
}

const gk_diagnostic *gk_program_diag(const gk_program *p, size_t index)
{
    if (p == NULL || index >= p->diagnostic_count) {
        return NULL;
    }
    return &p->diagnostics[index];
}

size_t gk_program_diag_count(const gk_program *p)
{
    return p != NULL ? p->diagnostic_count : 0;
}
