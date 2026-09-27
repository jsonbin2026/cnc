#include "gk_test.h"

#include "gk/gk_editor.h"
#include "gk/gk_file.h"

#include <math.h>
#include <string.h>

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

static void test_tokens_tokens(void)
{
    gk_token t[16];
    int n = gk_doc_tokenize("N10 G01 X12.5 (rapid) [1+2]", t, 16);
    GK_CHECK(n > 0);
    GK_CHECK_EQ_INT(t[0].kind, GK_TOK_WORD);
    GK_CHECK_EQ_INT(t[0].start, 0);
    GK_CHECK_EQ_INT(t[0].len, 3);   /* "N10" */
    /* find comment token */
    {
        int i;
        int found = 0;
        for (i = 0; i < n; ++i) {
            if (t[i].kind == GK_TOK_COMMENT) found = 1;
        }
        GK_CHECK(found);
    }
}

static void test_doc_basic(void)
{
    gk_doc d;
    char num[16];
    gk_doc_init(&d);
    GK_CHECK_EQ_INT(gk_doc_set_text(&d, "N10 G00 X0\nN20 G01 X10\nN30 M30"),
                    GK_OK);
    GK_CHECK_EQ_INT(gk_doc_line_count(&d), 3);
    GK_CHECK_STR_EQ(gk_doc_line(&d, 0), "N10 G00 X0");
    GK_CHECK_STR_EQ(gk_doc_line(&d, 2), "N30 M30");
    GK_CHECK(gk_doc_line(&d, 5) == NULL);
    gk_doc_line_number(0, num, sizeof(num));
    GK_CHECK_STR_EQ(num, "N0001");
    gk_doc_line_number(41, num, sizeof(num));
    GK_CHECK_STR_EQ(num, "N0042");
}

static void test_find_replace(void)
{
    gk_doc d;
    gk_doc_init(&d);
    gk_doc_set_text(&d, "G00 X10 Y10\nG01 X20\nG00 X30");
    GK_CHECK_EQ_INT(gk_doc_find(&d, "G00", 0), 0);
    GK_CHECK_EQ_INT(gk_doc_find(&d, "G00", 1), 2);
    GK_CHECK_EQ_INT(gk_doc_find(&d, "G02", 0), -1);
    GK_CHECK_EQ_INT(gk_doc_replace(&d, "G00", "G01", 0), 1);
    GK_CHECK_STR_EQ(gk_doc_line(&d, 0), "G01 X10 Y10");
    GK_CHECK_STR_EQ(gk_doc_line(&d, 2), "G00 X30");
    GK_CHECK_EQ_INT(gk_doc_replace(&d, "G0", "GG", 1), 3);
    GK_CHECK_STR_EQ(gk_doc_line(&d, 0), "GG1 X10 Y10");
}

static void test_clipboard_undo(void)
{
    gk_editor e;
    gk_doc *d;
    char buf[64];
    gk_editor_init(&e);
    GK_CHECK_EQ_INT(gk_editor_open(&e, "/tmp/a.nc"), GK_OK);
    d = gk_editor_active(&e);
    gk_doc_set_text(d, "line one\nline two");
    GK_CHECK_EQ_INT(gk_doc_copy(d, 0, buf, sizeof(buf)), GK_OK);
    GK_CHECK_STR_EQ(buf, "line one");
    GK_CHECK_EQ_INT(gk_doc_paste(d, 1, "inserted"), GK_OK);
    GK_CHECK_EQ_INT(gk_doc_line_count(d), 3);
    GK_CHECK_STR_EQ(gk_doc_line(d, 1), "inserted");
    GK_CHECK_EQ_INT(gk_doc_copy(d, 99, buf, sizeof(buf)), GK_ERR_OUT_OF_RANGE);

    GK_CHECK_EQ_INT(gk_editor_edit_line(&e, 0, "changed"), GK_OK);
    GK_CHECK_STR_EQ(gk_doc_line(d, 0), "changed");
    GK_CHECK_EQ_INT(gk_editor_undo(&e), GK_OK);
    GK_CHECK_STR_EQ(gk_doc_line(d, 0), "line one");
    GK_CHECK_EQ_INT(gk_editor_redo(&e), GK_OK);
    GK_CHECK_STR_EQ(gk_doc_line(d, 0), "changed");
    GK_CHECK_EQ_INT(gk_editor_undo(&e), GK_OK);
    GK_CHECK_EQ_INT(gk_editor_undo(&e), GK_ERR_STATE);
}

static void test_bracket_indent(void)
{
    gk_doc d;
    char o, c;
    gk_doc_init(&d);
    gk_doc_set_text(&d, "IF [1+2] GOTO 10");
    GK_CHECK_EQ_INT(gk_doc_match_bracket(gk_doc_line(&d, 0), 3, &o, &c), 7);
    GK_CHECK_EQ_INT(o, '[');
    GK_CHECK_EQ_INT(c, ']');
    GK_CHECK_EQ_INT(gk_doc_match_bracket(gk_doc_line(&d, 0), 0, &o, &c), -1);

    gk_doc_init(&d);
    gk_doc_set_text(&d, "LOOP:\nBODY\nEND");
    gk_doc_autoindent(&d, 1);
    GK_CHECK_EQ_INT(d.lines[1].indent, 1);
    gk_doc_autoindent(&d, 2);
    GK_CHECK_EQ_INT(d.lines[2].indent, 1);
}

static void test_fold_bookmark(void)
{
    gk_doc d;
    gk_doc_init(&d);
    gk_doc_set_text(&d, "A:\n  B\n  C\nD");
    d.lines[0].indent = 0;
    d.lines[1].indent = 1;
    d.lines[2].indent = 1;
    d.lines[3].indent = 0;
    GK_CHECK_EQ_INT(gk_doc_visible_lines(&d), 4);
    gk_doc_fold(&d, 0, 1);
    GK_CHECK(gk_doc_is_folded(&d, 0));
    GK_CHECK_EQ_INT(gk_doc_visible_lines(&d), 2);

    gk_doc_bookmark(&d, 1, 1);
    gk_doc_bookmark(&d, 3, 1);
    GK_CHECK_EQ_INT(gk_doc_bookmark_count(&d), 2);
    GK_CHECK_EQ_INT(gk_doc_next_bookmark(&d, 0), 1);
    GK_CHECK_EQ_INT(gk_doc_next_bookmark(&d, 1), 3);
    GK_CHECK_EQ_INT(gk_doc_next_bookmark(&d, 3), 1);
}

static void test_multifile_compare(void)
{
    gk_editor e;
    gk_doc *a;
    gk_doc *b;
    gk_editor_init(&e);
    gk_editor_open(&e, "/a");
    gk_editor_open(&e, "/b");
    GK_CHECK_EQ_INT(gk_editor_file_count(&e), 2);
    a = gk_editor_file(&e, 0);
    b = gk_editor_file(&e, 1);
    gk_doc_set_text(a, "same\ndiff\nend");
    gk_doc_set_text(b, "same\nother\nend");
    GK_CHECK_EQ_INT(gk_doc_compare(a, b), 1);
    gk_doc_set_text(b, "same\ndiff\nend");
    GK_CHECK_EQ_INT(gk_doc_compare(a, b), -1);
    GK_CHECK_EQ_INT(gk_editor_activate(&e, 1), GK_OK);
    GK_CHECK(gk_editor_active(&e) == b);
    GK_CHECK_EQ_INT(gk_editor_close(&e, 0), GK_OK);
    GK_CHECK_EQ_INT(gk_editor_file_count(&e), 1);
}

static void test_complete(void)
{
    char out[8][16];
    int n = gk_doc_complete("G0", out, 8);
    GK_CHECK(n >= 2);
    GK_CHECK_STR_EQ(out[0], "G00");
    n = gk_doc_complete("M0", out, 8);
    GK_CHECK(n >= 1);
    GK_CHECK_STR_EQ(out[0], "M00");
    GK_CHECK_EQ_INT(gk_doc_complete("Z9", out, 8), 0);
    GK_CHECK_EQ_INT(gk_doc_complete("", out, 8), 0);
}

static void test_interaction(void)
{
    gk_doc d;
    gk_interaction it;
    int s, e;
    gk_doc_init(&d);
    gk_doc_set_text(&d, "N10 G00\nN20 G01\nN30 G01\nN40 M30");
    gk_interaction_init(&it, &d);
    GK_CHECK(near(it.speed, 1.0, 1e-9));
    GK_CHECK_EQ_INT(it.zoom, 100);

    GK_CHECK_EQ_INT(gk_interact_step(&it), GK_OK);
    GK_CHECK_EQ_INT(it.cursor_line, 1);

    GK_CHECK_EQ_INT(gk_interact_set_breakpoint(&it, 3, 1), GK_OK);
    GK_CHECK(gk_interact_is_breakpoint(&it, 3));
    it.cursor_line = 1;
    GK_CHECK_EQ_INT(gk_interact_run(&it), 3);
    GK_CHECK_EQ_INT(gk_interact_set_breakpoint(&it, 3, 0), GK_OK);
    GK_CHECK(!gk_interact_is_breakpoint(&it, 3));
    GK_CHECK_EQ_INT(it.break_count, 0);
    GK_CHECK_EQ_INT(gk_interact_set_breakpoint(&it, 99, 1),
                    GK_ERR_OUT_OF_RANGE);

    GK_CHECK_EQ_INT(gk_interact_set_speed(&it, 2.5), GK_OK);
    GK_CHECK(near(it.speed, 2.5, 1e-9));
    GK_CHECK_EQ_INT(gk_interact_set_speed(&it, 50.0), GK_ERR_OUT_OF_RANGE);

    GK_CHECK_EQ_INT(gk_interact_map_key(&it, "F5", "run"), GK_OK);
    GK_CHECK_STR_EQ(gk_interact_action_for_key(&it, "F5"), "run");
    GK_CHECK(gk_interact_action_for_key(&it, "F9") == NULL);
    GK_CHECK_EQ_INT(gk_interact_map_key(&it, "F5", "halt"), GK_OK);
    GK_CHECK_STR_EQ(gk_interact_action_for_key(&it, "F5"), "halt");

    GK_CHECK_EQ_INT(gk_interact_zoom(&it, 50), GK_OK);
    GK_CHECK_EQ_INT(it.zoom, 150);
    GK_CHECK_EQ_INT(gk_interact_zoom(&it, -1000), GK_OK);
    GK_CHECK_EQ_INT(it.zoom, 10);

    GK_CHECK_EQ_INT(gk_interact_double_click(&it, 0, 5, &s, &e), GK_OK);
    GK_CHECK(s <= 5 && e > 5);

    GK_CHECK_EQ_INT(gk_interact_hover(&it, 0, 5), GK_OK);
    GK_CHECK(gk_interact_hover_len(&it) > 0);

    it.cursor_line = 0;
    it.cursor_col = 0;
    GK_CHECK_EQ_INT(gk_interact_move(&it, 1, 3), GK_OK);
    GK_CHECK_EQ_INT(it.cursor_line, 1);
}

static void test_nc_files(void)
{
    gk_file_manager m;
    gk_file_manager_init(&m);
    GK_CHECK_EQ_INT(gk_file_new(&m, "PART1"), GK_OK);
    GK_CHECK_EQ_INT(m.count, 1);
    GK_CHECK_EQ_INT(gk_file_save(&m, "G00 X0"), GK_OK);
    GK_CHECK_STR_EQ(m.files[0].content, "G00 X0");
    GK_CHECK_EQ_INT(m.files[0].size, 6);
    GK_CHECK_EQ_INT(gk_file_set_onumber(&m, 0, 1001), GK_OK);
    GK_CHECK_EQ_INT(gk_file_find_by_onumber(&m, 1001), 0);
    GK_CHECK_EQ_INT(gk_file_find_by_onumber(&m, 2002), -1);

    GK_CHECK_EQ_INT(gk_file_open(&m, "/nc/part2.nc"), GK_OK);
    GK_CHECK_EQ_INT(m.count, 2);
    GK_CHECK_STR_EQ(m.files[1].name, "part2.nc");
    GK_CHECK_EQ_INT(gk_file_recent_count(&m), 1);
    GK_CHECK_STR_EQ(gk_file_recent(&m, 0), "/nc/part2.nc");

    GK_CHECK_EQ_INT(gk_file_save_as(&m, "/nc/part2b.nc"), GK_OK);
    GK_CHECK_STR_EQ(m.files[1].name, "part2b.nc");
    GK_CHECK_EQ_INT(gk_file_recent_count(&m), 2);
    GK_CHECK_STR_EQ(gk_file_recent(&m, 0), "/nc/part2b.nc");

    GK_CHECK_EQ_INT(gk_file_close(&m, 0), GK_OK);
    GK_CHECK_EQ_INT(m.count, 1);
    GK_CHECK_EQ_INT(gk_file_close(&m, 5), GK_ERR_OUT_OF_RANGE);
}

static void test_export(void)
{
    gk_toolpath tp;
    char buf[2048];
    gk_toolpath_init(&tp);
    GK_CHECK_EQ_INT(gk_toolpath_add(&tp, 0, 0, 0, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_toolpath_add(&tp, 10, 5, -2, 1), GK_OK);
    GK_CHECK_EQ_INT(tp.count, 2);

    GK_CHECK(gk_export_toolpath(&tp, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "TOOLPATH") != NULL);
    GK_CHECK(strstr(buf, "POINTS 2") != NULL);

    GK_CHECK(gk_export_report("Part A", &tp, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "Part A") != NULL);

    GK_CHECK(gk_export_csv(&tp, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "x,y,z,tool") != NULL);
    GK_CHECK(strstr(buf, "10.000,5.000,-2.000,1") != NULL);

    GK_CHECK(gk_export_json(&tp, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "\"count\":2") != NULL);

    GK_CHECK(gk_export_pdf("Part A", buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "%PDF-1.4") != NULL);
    GK_CHECK(strstr(buf, "%%EOF") != NULL);
}

static void test_recording_screenshot(void)
{
    gk_recording r;
    char buf[512];
    gk_screenshot s;
    gk_recording_init(&r, 30);
    GK_CHECK_EQ_INT(r.fps, 30);
    GK_CHECK_EQ_INT(gk_recording_add(&r, 0.0, "start"), GK_OK);
    GK_CHECK_EQ_INT(gk_recording_add(&r, 2.5, "end"), GK_OK);
    GK_CHECK(near(gk_recording_duration(&r), 2.5, 1e-9));
    GK_CHECK(gk_recording_export_mp4(&r, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "fps=30") != NULL);
    GK_CHECK(strstr(buf, "frames=2") != NULL);

    GK_CHECK_EQ_INT(gk_screenshot_capture(&s, 64, 32, 0x80), GK_OK);
    GK_CHECK_EQ_INT(s.width, 64);
    GK_CHECK_EQ_INT(s.pixels[3], 255);
    GK_CHECK_EQ_INT(gk_screenshot_capture(&s, 128, 32, 0), GK_ERR_OUT_OF_RANGE);
    GK_CHECK(gk_screenshot_save_png(&s, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "IHDR 64x32") != NULL);
}

static void test_save_states(void)
{
    gk_save_states ss;
    gk_machine_state st;
    gk_machine_state out;
    gk_save_states_init(&ss);
    memset(&st, 0, sizeof(st));
    st.axis[0] = 12.5;
    st.spindle = 3000;
    st.tool = 7;
    st.line = 0;
    GK_CHECK_EQ_INT(gk_save_state(&ss, "slot1", &st), GK_OK);
    GK_CHECK_EQ_INT(ss.count, 1);
    st.axis[0] = 99.0;
    GK_CHECK_EQ_INT(gk_save_state(&ss, "slot1", &st), GK_OK);
    GK_CHECK_EQ_INT(ss.count, 1);
    st.axis[0] = 12.5;
    GK_CHECK_EQ_INT(gk_save_state(&ss, "slot1", &st), GK_OK);
    GK_CHECK_EQ_INT(gk_load_state(&ss, "slot1", &out), GK_OK);
    GK_CHECK(near(out.axis[0], 12.5, 1e-9));
    GK_CHECK_EQ_INT(out.tool, 7);
    GK_CHECK_EQ_INT(gk_load_state(&ss, "nope", &out), GK_ERR_NOT_FOUND);
}

static void test_transfer(void)
{
    gk_transfer t;
    GK_CHECK_STR_EQ(gk_transfer_kind_name(GK_XFER_SERIAL), "serial");
    GK_CHECK_STR_EQ(gk_transfer_kind_name(GK_XFER_USB), "usb");
    GK_CHECK_STR_EQ(gk_transfer_kind_name(GK_XFER_CLOUD), "cloud");
    GK_CHECK_EQ_INT(gk_transfer_init(&t, GK_XFER_SERIAL), GK_OK);
    GK_CHECK(near(t.baud, 9600.0, 1e-9));
    GK_CHECK_EQ_INT(gk_transfer_step(&t, 1.0), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_transfer_connect(&t), GK_OK);
    GK_CHECK(t.connected);
    GK_CHECK_EQ_INT(gk_transfer_step(&t, 0.1), GK_OK);
    GK_CHECK(t.bytes_sent > 0);
    GK_CHECK_EQ_INT(gk_transfer_step(&t, 100.0), GK_OK);
    GK_CHECK(gk_transfer_done(&t));
    GK_CHECK(near(t.progress, 1.0, 1e-9));
    GK_CHECK_EQ_INT(gk_transfer_init(&t, (gk_transfer_kind)99),
                    GK_ERR_INVALID_ARG);
}

static void test_autosave_history(void)
{
    gk_autosave a;
    gk_history h;
    gk_version *v;
    char out[GK_FILE_BUF];

    gk_autosave_init(&a, 10.0);
    GK_CHECK_EQ_INT(gk_autosave_tick(&a, 5.0), 0);
    GK_CHECK_EQ_INT(gk_autosave_tick(&a, 6.0), 1);
    GK_CHECK_EQ_INT(a.saves, 1);
    a.enabled = 0;
    GK_CHECK_EQ_INT(gk_autosave_tick(&a, 100.0), 0);

    gk_history_init(&h);
    GK_CHECK_EQ_INT(gk_history_commit(&h, "alice", "rev1"), GK_OK);
    GK_CHECK_EQ_INT(gk_history_commit(&h, "bob", "rev2"), GK_OK);
    GK_CHECK_EQ_INT(gk_history_count(&h), 2);
    v = (gk_version *)gk_history_at(&h, 1);
    GK_CHECK(v != NULL);
    GK_CHECK_STR_EQ(v->author, "bob");
    GK_CHECK_EQ_INT(v->revision, 2);
    GK_CHECK_EQ_INT(gk_history_rollback(&h, 1, out, sizeof(out)), GK_OK);
    GK_CHECK_STR_EQ(out, "rev1");
    GK_CHECK_EQ_INT(gk_history_rollback(&h, 9, out, sizeof(out)),
                    GK_ERR_NOT_FOUND);
}

static void test_encrypt(void)
{
    const char *plain = "G00 X10";
    size_t len = strlen(plain);
    char enc[32];
    char dec[32];
    gk_file_encrypt(plain, len, 0x5A, enc);
    GK_CHECK(memcmp(plain, enc, len) != 0);
    GK_CHECK(gk_file_is_encrypted(enc, len));
    gk_file_decrypt(enc, len, 0x5A, dec);
    dec[len] = '\0';
    GK_CHECK_STR_EQ(dec, "G00 X10");
    GK_CHECK(!gk_file_is_encrypted("G00 X10", 7));
}

int main(void)
{
    test_tokens_tokens();
    test_doc_basic();
    test_find_replace();
    test_clipboard_undo();
    test_bracket_indent();
    test_fold_bookmark();
    test_multifile_compare();
    test_complete();
    test_interaction();
    test_nc_files();
    test_export();
    test_recording_screenshot();
    test_save_states();
    test_transfer();
    test_autosave_history();
    test_encrypt();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
