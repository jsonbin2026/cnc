#include "gk/gk_lexer.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const char *const k_kind_names[] = {
    "EOF", "WORD", "LINE_NO", "GCODE", "MCODE", "LPAREN", "RPAREN",
    "LBRACKET", "RBRACKET", "PLUS", "MINUS", "STAR", "SLASH", "EQ",
    "GT", "LT", "GE", "LE", "NE", "HASH", "COLON", "SEMI", "COMMENT",
    "UNKNOWN",
};

static void set_error(gk_lexer *lx, gk_status st, size_t col, const char *msg)
{
    if (lx->has_error) {
        return;
    }
    lx->has_error = 1;
    lx->error = st;
    lx->error_line = lx->line;
    lx->error_column = col;
    if (msg != NULL) {
        size_t i = 0;
        while (msg[i] != '\0' && i + 1 < sizeof(lx->error_message)) {
            lx->error_message[i] = msg[i];
            ++i;
        }
        lx->error_message[i] = '\0';
    } else {
        lx->error_message[0] = '\0';
    }
}

void gk_lexer_init(gk_lexer *lx, const char *source, size_t length)
{
    if (lx == NULL) {
        return;
    }
    lx->source = source;
    lx->length = source != NULL ? length : 0;
    lx->pos = 0;
    lx->line = 1;
    lx->line_start = 0;
    lx->has_error = 0;
    lx->error = GK_OK;
    lx->error_line = 1;
    lx->error_column = 1;
    lx->error_message[0] = '\0';
}

static int peek(const gk_lexer *lx)
{
    if (lx->pos >= lx->length) {
        return -1;
    }
    return (unsigned char)lx->source[lx->pos];
}

static int advance(gk_lexer *lx)
{
    int c = peek(lx);
    if (c < 0) {
        return -1;
    }
    lx->pos += 1;
    if (c == '\n') {
        lx->line += 1;
        lx->line_start = lx->pos;
    }
    return c;
}

static void skip_spaces(gk_lexer *lx)
{
    for (;;) {
        int c = peek(lx);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance(lx);
        } else {
            break;
        }
    }
}

static void fill(gk_token *t, gk_token_kind kind, const gk_lexer *lx,
                 size_t start_pos, size_t start_line)
{
    t->kind = kind;
    t->offset = start_pos;
    t->line = start_line;
    t->column = start_pos - lx->line_start + 1;
    t->length = lx->pos - start_pos;
}

/* Parse a numeric literal at the cursor.
 * allow_sign: consume a leading '+'/'-' as part of the value (letter
 * addresses such as X-1.5), rather than emitting a MINUS token. */
static int parse_number(gk_lexer *lx, gk_token *t, int allow_sign)
{
    int any = 0;
    int decimals = 0;
    int after_dot = 0;
    char buf[64];
    size_t n = 0;
    if (allow_sign) {
        int c = peek(lx);
        if (c == '+' || c == '-') {
            buf[n++] = (char)c;
            advance(lx);
        }
    }
    while (n + 1 < sizeof(buf)) {
        int c = peek(lx);
        if (c == '.' && !after_dot) {
            after_dot = 1;
            decimals = 0;
            buf[n++] = (char)c;
            advance(lx);
            continue;
        }
        if (c >= '0' && c <= '9') {
            buf[n++] = (char)c;
            advance(lx);
            any = 1;
            if (after_dot) {
                decimals += 1;
            }
            continue;
        }
        break;
    }
    if (!any) {
        return 0;
    }
    buf[n] = '\0';
    t->value = strtod(buf, NULL);
    t->has_value = 1;
    t->decimal_count = decimals;
    t->ivalue = (int)(t->value >= 0 ? t->value + 0.5 : t->value - 0.5);
    return 1;
}

gk_status gk_lexer_next(gk_lexer *lx, gk_token *out)
{
    int c;
    size_t start_pos;
    size_t start_line;

    if (lx == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (lx->has_error) {
        return lx->error;
    }

    skip_spaces(lx);
    memset(out, 0, sizeof(*out));
    start_pos = lx->pos;
    start_line = lx->line;
    c = peek(lx);

    if (c < 0) {
        fill(out, GK_TOK_EOF, lx, start_pos, start_line);
        return GK_OK;
    }

    /* ( ... ) comment */
    if (c == '(') {
        advance(lx);
        while (peek(lx) >= 0 && peek(lx) != ')') {
            advance(lx);
        }
        if (peek(lx) == ')') {
            advance(lx);
            fill(out, GK_TOK_COMMENT, lx, start_pos, start_line);
            return GK_OK;
        }
        set_error(lx, GK_ERR_PARSE, out->column, "unterminated comment");
        return GK_ERR_PARSE;
    }

    /* letter address: N/G/M + number, or other letter + number */
    if (isalpha(c)) {
        char letter = (char)toupper(c);
        advance(lx);
        if (!parse_number(lx, out, 1)) {
            fill(out, GK_TOK_UNKNOWN, lx, start_pos, start_line);
            out->letter = letter;
            set_error(lx, GK_ERR_PARSE, out->column, "expected number after letter");
            return GK_ERR_PARSE;
        }
        out->letter = letter;
        out->ivalue = (int)(out->value >= 0 ? out->value + 0.5
                                              : out->value - 0.5);
        out->offset = start_pos;
        out->line = start_line;
        out->column = start_pos - lx->line_start + 1;
        out->length = lx->pos - start_pos;
        if (letter == 'N') {
            out->kind = GK_TOK_LINE_NO;
        } else if (letter == 'G') {
            out->kind = GK_TOK_GCODE;
        } else if (letter == 'M') {
            out->kind = GK_TOK_MCODE;
        } else {
            out->kind = GK_TOK_WORD;
        }
        return GK_OK;
    }

    /* # variable */
    if (c == '#') {
        advance(lx);
        if (peek(lx) == '+' || peek(lx) == '-') {
            if (!parse_number(lx, out, 1)) {
                set_error(lx, GK_ERR_PARSE, start_pos - lx->line_start + 1,
                          "expected number after # sign");
                return GK_ERR_PARSE;
            }
        } else if (isdigit(peek(lx))) {
            parse_number(lx, out, 0);
        } else {
            fill(out, GK_TOK_HASH, lx, start_pos, start_line);
            return GK_OK;
        }
        fill(out, GK_TOK_WORD, lx, start_pos, start_line);
        out->letter = '#';
        return GK_OK;
    }

    /* standalone numbers (rare in NC, allowed for macro expressions) */
    if (isdigit(c) || c == '.') {
        if (!parse_number(lx, out, 0)) {
            advance(lx);
            fill(out, GK_TOK_UNKNOWN, lx, start_pos, start_line);
            set_error(lx, GK_ERR_PARSE, out->column, "invalid number");
            return GK_ERR_PARSE;
        }
        fill(out, GK_TOK_WORD, lx, start_pos, start_line);
        out->letter = 0;
        return GK_OK;
    }

    advance(lx);
    switch (c) {
    case ')': fill(out, GK_TOK_RPAREN, lx, start_pos, start_line); break;
    case '[': fill(out, GK_TOK_LBRACKET, lx, start_pos, start_line); break;
    case ']': fill(out, GK_TOK_RBRACKET, lx, start_pos, start_line); break;
    case '+': fill(out, GK_TOK_PLUS, lx, start_pos, start_line); break;
    case '-': fill(out, GK_TOK_MINUS, lx, start_pos, start_line); break;
    case '*': fill(out, GK_TOK_STAR, lx, start_pos, start_line); break;
    case '/': fill(out, GK_TOK_SLASH, lx, start_pos, start_line); break;
    case '#': fill(out, GK_TOK_HASH, lx, start_pos, start_line); break;
    case ':': fill(out, GK_TOK_COLON, lx, start_pos, start_line); break;
    case ';':
        fill(out, GK_TOK_SEMI, lx, start_pos, start_line);
        break;
    case '=': fill(out, GK_TOK_EQ, lx, start_pos, start_line); break;
    case '>':
        if (peek(lx) == '=') {
            advance(lx);
            fill(out, GK_TOK_GE, lx, start_pos, start_line);
        } else {
            fill(out, GK_TOK_GT, lx, start_pos, start_line);
        }
        break;
    case '<':
        if (peek(lx) == '=') {
            advance(lx);
            fill(out, GK_TOK_LE, lx, start_pos, start_line);
        } else if (peek(lx) == '>') {
            advance(lx);
            fill(out, GK_TOK_NE, lx, start_pos, start_line);
        } else {
            fill(out, GK_TOK_LT, lx, start_pos, start_line);
        }
        break;
    default:
        fill(out, GK_TOK_UNKNOWN, lx, start_pos, start_line);
        set_error(lx, GK_ERR_PARSE, out->column, "unexpected character");
        return GK_ERR_PARSE;
    }
    return GK_OK;
}

const char *gk_token_kind_name(gk_token_kind kind)
{
    if ((int)kind < 0 || (int)kind > (int)GK_TOK_UNKNOWN) {
        return "INVALID";
    }
    return k_kind_names[(int)kind];
}

int gk_token_kind_is_word(gk_token_kind kind)
{
    return kind == GK_TOK_WORD || kind == GK_TOK_LINE_NO ||
           kind == GK_TOK_GCODE || kind == GK_TOK_MCODE;
}
