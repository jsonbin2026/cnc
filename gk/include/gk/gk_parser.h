#ifndef GK_PARSER_H
#define GK_PARSER_H

#include <stddef.h>
#include "gk/gk_error.h"
#include "gk/gk_mem.h"
#include "gk/gk_vec.h"
#include "gk/gk_lexer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_MAX_WORDS_PER_BLOCK 32

typedef struct {
    char letter;
    double value;
    int ivalue;
    int has_value;
    int is_gcode;
    int is_mcode;
    int is_line_no;
    int decimal_count;
    size_t column;
} gk_word;

typedef struct {
    size_t line;
    int block_number;      /* optional Nxxxx */
    int has_block_number;
    int block_skip;        /* leading / */
    gk_word words[GK_MAX_WORDS_PER_BLOCK];
    size_t word_count;
    char comment[128];
    int has_comment;
} gk_block;

typedef enum {
    GK_DIAG_INFO = 0,
    GK_DIAG_WARN,
    GK_DIAG_ERROR
} gk_diag_severity;

typedef struct {
    gk_diag_severity severity;
    size_t line;
    size_t column;
    int code;
    char message[160];
} gk_diagnostic;

typedef struct {
    gk_block *blocks;
    size_t block_count;
    gk_diagnostic *diagnostics;
    size_t diagnostic_count;
    size_t diagnostic_cap;
    size_t error_count;
    size_t warn_count;
    gk_allocator alloc;
} gk_program;

void gk_program_init(gk_program *p, const gk_allocator *alloc);
void gk_program_free(gk_program *p);

gk_status gk_program_parse(gk_program *p, const char *source, size_t length);
gk_status gk_program_validate(gk_program *p);

const gk_block *gk_program_block(const gk_program *p, size_t index);
size_t gk_program_block_count(const gk_program *p);
const gk_diagnostic *gk_program_diag(const gk_program *p, size_t index);
size_t gk_program_diag_count(const gk_program *p);

int gk_gcode_is_defined(int code);
int gk_mcode_is_defined(int code);
const char *gk_gcode_name(int code);
const char *gk_mcode_name(int code);

#ifdef __cplusplus
}
#endif

#endif
