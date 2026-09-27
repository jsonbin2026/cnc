#include "gk_test.h"

#include "gk/gk_lexer.h"
#include "gk/gk_parser.h"
#include "gk/gk_codes.h"

#include <string.h>

static void test_lexer_basic(void)
{
    const char *src = "N10 G01 X10.5 Y-2.25 F100 (rough pass)";
    gk_lexer lx;
    gk_token t;

    gk_lexer_init(&lx, src, strlen(src));

    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.kind, GK_TOK_LINE_NO);
    GK_CHECK_EQ_INT(t.ivalue, 10);

    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.kind, GK_TOK_GCODE);
    GK_CHECK_EQ_INT(t.ivalue, 1);

    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.kind, GK_TOK_WORD);
    GK_CHECK_EQ_INT(t.letter, 'X');
    GK_CHECK(t.value > 10.49 && t.value < 10.51);
    GK_CHECK_EQ_INT(t.decimal_count, 1);

    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.letter, 'Y');
    GK_CHECK(t.value > -2.26 && t.value < -2.24);
    GK_CHECK_EQ_INT(t.decimal_count, 2);

    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.letter, 'F');
    GK_CHECK_EQ_INT(t.ivalue, 100);

    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.kind, GK_TOK_COMMENT);
    GK_CHECK_STR_EQ(gk_token_kind_name(GK_TOK_GCODE), "GCODE");

    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.kind, GK_TOK_EOF);
}

static void test_lexer_symbols(void)
{
    const char *src = "#1=[2+3]*4/2";
    gk_lexer lx;
    gk_token t;
    gk_lexer_init(&lx, src, strlen(src));

    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.kind, GK_TOK_WORD);
    GK_CHECK_EQ_INT(t.letter, '#');

    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.kind, GK_TOK_EQ);

    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.kind, GK_TOK_LBRACKET);

    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.kind, GK_TOK_WORD);
    GK_CHECK_EQ_INT(t.letter, 0);
    GK_CHECK_EQ_INT(t.ivalue, 2);

    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.kind, GK_TOK_PLUS);
    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.kind, GK_TOK_WORD);
    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.kind, GK_TOK_RBRACKET);
    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.kind, GK_TOK_STAR);
    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.kind, GK_TOK_WORD);
    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.kind, GK_TOK_SLASH);
    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.kind, GK_TOK_WORD);
    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(t.kind, GK_TOK_EOF);
}

static void test_lexer_errors(void)
{
    gk_lexer lx;
    gk_token t;
    /* unterminated comment */
    gk_lexer_init(&lx, "(oops", 5);
    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_ERR_PARSE);
    GK_CHECK(lx.has_error);
    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_ERR_PARSE);

    /* invalid char */
    gk_lexer_init(&lx, "G01 @", 5);
    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_OK);
    GK_CHECK_EQ_INT(gk_lexer_next(&lx, &t), GK_ERR_PARSE);
}

static void test_parser_program(void)
{
    const char *src =
        "N10 G21 G90 G17\n"
        "N20 G00 X0 Y0 Z5\n"
        "N30 M03 S1200\n"
        "N40 G01 Z-2.0 F120 (plunge)\n"
        "N50 G01 X50.0 Y0\n"
        "N60 M05\n"
        "N70 M30\n";
    gk_program p;

    gk_program_init(&p, NULL);
    GK_CHECK_EQ_INT(gk_program_parse(&p, src, strlen(src)), GK_OK);
    GK_CHECK_EQ_INT(gk_program_block_count(&p), 7);
    GK_CHECK_EQ_INT(gk_program_diag_count(&p), 0);
    GK_CHECK_EQ_INT(p.error_count, 0);

    {
        const gk_block *b = gk_program_block(&p, 0);
        GK_CHECK(b != NULL);
        GK_CHECK_EQ_INT(b->block_number, 10);
        GK_CHECK(b->has_block_number);
        GK_CHECK_EQ_INT(b->word_count, 3);
    }
    {
        const gk_block *b = gk_program_block(&p, 3);
        GK_CHECK(b != NULL);
        GK_CHECK(b->has_comment);
        GK_CHECK_STR_EQ(b->comment, "plunge");
    }
    {
        const gk_block *b = gk_program_block(&p, 6);
        GK_CHECK(b != NULL);
        GK_CHECK_EQ_INT(b->words[0].letter, 'M');
        GK_CHECK_EQ_INT(b->words[0].ivalue, 30);
    }
    gk_program_free(&p);
}

static void test_parser_validate(void)
{
    gk_program p;
    const char *bad = "G99 X1\nG01 X2 M77\n";
    gk_program_init(&p, NULL);
    GK_CHECK_EQ_INT(gk_program_parse(&p, bad, strlen(bad)), GK_OK);
    GK_CHECK_EQ_INT(gk_program_validate(&p), GK_ERR_PARSE);
    gk_program_free(&p);

    gk_program_init(&p, NULL);
    {
        const char *unknown = "M77\n";
        GK_CHECK_EQ_INT(gk_program_parse(&p, unknown, strlen(unknown)), GK_OK);
        GK_CHECK_EQ_INT(gk_program_validate(&p), GK_ERR_PARSE);
        GK_CHECK(p.error_count >= 1);
    }
    gk_program_free(&p);

    gk_program_init(&p, NULL);
    {
        const char *unknown_g = "G999\n";
        GK_CHECK_EQ_INT(gk_program_parse(&p, unknown_g,
                                         strlen(unknown_g)), GK_OK);
        GK_CHECK_EQ_INT(gk_program_validate(&p), GK_ERR_PARSE);
    }
    gk_program_free(&p);
}

static void test_parser_block_skip_and_multi(void)
{
    gk_program p;
    const char *src = "/N10 G01 X1\nG02 X2 Y2 I1 J0\n";
    gk_program_init(&p, NULL);
    GK_CHECK_EQ_INT(gk_program_parse(&p, src, strlen(src)), GK_OK);
    GK_CHECK_EQ_INT(gk_program_block_count(&p), 2);
    {
        const gk_block *b = gk_program_block(&p, 0);
        GK_CHECK(b != NULL);
        GK_CHECK_EQ_INT(b->block_skip, 1);
    }
    gk_program_free(&p);
}

static void test_codes(void)
{
    size_t ng = 0;
    size_t nm = 0;
    gk_gcode_table(&ng);
    gk_mcode_table(&nm);
    GK_CHECK_EQ_INT(ng, 57);
    GK_CHECK_EQ_INT(nm, 19);
    GK_CHECK(gk_gcode_is_defined(1));
    GK_CHECK(gk_gcode_is_defined(6));
    GK_CHECK(!gk_gcode_is_defined(7));
    GK_CHECK(gk_mcode_is_defined(30));
    GK_CHECK(!gk_mcode_is_defined(42));
    GK_CHECK_STR_EQ(gk_gcode_lookup(6, 2), "G06.2 NURBS 插补");
    GK_CHECK(gk_mcode_name(6) != NULL);
}

int main(void)
{
    test_lexer_basic();
    test_lexer_symbols();
    test_lexer_errors();
    test_parser_program();
    test_parser_validate();
    test_parser_block_skip_and_multi();
    test_codes();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
