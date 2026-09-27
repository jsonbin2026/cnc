#ifndef GK_LEXER_H
#define GK_LEXER_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GK_TOK_EOF = 0,
    GK_TOK_WORD,      /* a letter address followed by a numeric value */
    GK_TOK_LINE_NO,   /* N address */
    GK_TOK_GCODE,
    GK_TOK_MCODE,
    GK_TOK_LPAREN,    /* ( */
    GK_TOK_RPAREN,    /* ) */
    GK_TOK_LBRACKET,  /* [ */
    GK_TOK_RBRACKET,  /* ] */
    GK_TOK_PLUS,
    GK_TOK_MINUS,
    GK_TOK_STAR,
    GK_TOK_SLASH,     /* / used as divide or block skip */
    GK_TOK_EQ,
    GK_TOK_GT,
    GK_TOK_LT,
    GK_TOK_GE,
    GK_TOK_LE,
    GK_TOK_NE,
    GK_TOK_HASH,      /* # */
    GK_TOK_COLON,     /* : block/section label */
    GK_TOK_SEMI,      /* ; end of block */
    GK_TOK_COMMENT,   /* ( ... ) or ; text */
    GK_TOK_UNKNOWN
} gk_token_kind;

typedef struct {
    gk_token_kind kind;
    char letter;        /* address letter, or 0 */
    double value;       /* numeric value */
    int ivalue;         /* integer value (G/M/N code) */
    int has_value;
    int decimal_count;  /* number of digits after '.' */
    size_t line;        /* 1-based source line */
    size_t column;      /* 1-based column start */
    size_t offset;      /* byte offset in source */
    size_t length;      /* token length in bytes */
} gk_token;

typedef struct {
    const char *source;
    size_t length;
    size_t pos;
    size_t line;
    size_t line_start;   /* offset of current line start */
    int has_error;
    gk_status error;
    size_t error_line;
    size_t error_column;
    char error_message[128];
} gk_lexer;

void gk_lexer_init(gk_lexer *lx, const char *source, size_t length);
gk_status gk_lexer_next(gk_lexer *lx, gk_token *out);
const char *gk_token_kind_name(gk_token_kind kind);
int gk_token_kind_is_word(gk_token_kind kind);

#ifdef __cplusplus
}
#endif

#endif
