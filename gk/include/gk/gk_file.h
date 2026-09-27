#ifndef GK_FILE_H
#define GK_FILE_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_FILE_PATH 256
#define GK_FILE_MAX_RECENT 16
#define GK_FILE_MAX_VERSIONS 32
#define GK_FILE_BUF 8192
#define GK_FILE_NAME 128
#define GK_FILE_GROUP 128

/* ---- NC file documents (454-459, 470) ---- */

typedef struct {
    int number;                 /* O-number */
    char name[GK_FILE_NAME];
    char comment[GK_FILE_NAME];
    char content[GK_FILE_BUF];
    int size;
    int dirty;
    char path[GK_FILE_PATH];
} gk_nc_file;

typedef struct {
    gk_nc_file files[GK_FILE_GROUP];
    int count;
    int active;
    gk_nc_file *recent[GK_FILE_MAX_RECENT];
    char recent_path[GK_FILE_MAX_RECENT][GK_FILE_PATH];
    int recent_count;
} gk_file_manager;

void gk_file_manager_init(gk_file_manager *m);

/* 458 new */
gk_status gk_file_new(gk_file_manager *m, const char *name);
/* 454 open / 459 close */
gk_status gk_file_open(gk_file_manager *m, const char *path);
gk_status gk_file_close(gk_file_manager *m, int index);
/* 455 save / 456 save-as: writes into the in-memory virtual fs. */
gk_status gk_file_save(gk_file_manager *m, const char *content);
gk_status gk_file_save_as(gk_file_manager *m, const char *path);
/* 457 recently used files */
int gk_file_recent_count(const gk_file_manager *m);
const char *gk_file_recent(const gk_file_manager *m, int i);

/* 470 program O-number management */
gk_status gk_file_set_onumber(gk_file_manager *m, int index, int number);
int gk_file_find_by_onumber(const gk_file_manager *m, int number);

/* ---- export (460-467) ---- */

typedef struct {
    double x, y, z;
    int tool;
} gk_toolpath_point;

typedef struct {
    gk_toolpath_point points[512];
    int count;
} gk_toolpath;

void gk_toolpath_init(gk_toolpath *tp);
gk_status gk_toolpath_add(gk_toolpath *tp, double x, double y, double z,
                          int tool);
/* 460 export toolpath to a simple text format */
int gk_export_toolpath(const gk_toolpath *tp, char *buf, size_t len);
/* 461 export report */
int gk_export_report(const char *title, const gk_toolpath *tp, char *buf,
                     size_t len);
/* 462 export CSV */
int gk_export_csv(const gk_toolpath *tp, char *buf, size_t len);
/* 463 export JSON */
int gk_export_json(const gk_toolpath *tp, char *buf, size_t len);
/* 464 export PDF: minimal valid PDF header + one page with text. */
int gk_export_pdf(const char *title, char *buf, size_t len);

/* 465 machining recording / 466 export MP4: frame list metadata. */
typedef struct {
    int frame;
    double time;
    char label[GK_FILE_NAME];
} gk_record_frame;

typedef struct {
    gk_record_frame frames[1024];
    int count;
    int fps;
} gk_recording;

void gk_recording_init(gk_recording *r, int fps);
gk_status gk_recording_add(gk_recording *r, double time, const char *label);
double gk_recording_duration(const gk_recording *r);
int gk_recording_export_mp4(const gk_recording *r, char *buf, size_t len);

/* 467 screenshot: stores an RGBA pixel buffer. */
typedef struct {
    int width, height;
    unsigned char pixels[64 * 64 * 4];
} gk_screenshot;

gk_status gk_screenshot_capture(gk_screenshot *s, int w, int h,
                                unsigned char fill);
int gk_screenshot_save_png(const gk_screenshot *s, char *buf, size_t len);

/* ---- save states (468-469) ---- */

typedef struct {
    double axis[9];
    double spindle;
    double feed;
    int tool;
    int line;
} gk_machine_state;

typedef struct {
    char name[GK_FILE_NAME];
    gk_machine_state state;
} gk_save_slot;

typedef struct {
    gk_save_slot slots[16];
    int count;
} gk_save_states;

void gk_save_states_init(gk_save_states *s);
/* 468 save / 469 load */
gk_status gk_save_state(gk_save_states *s, const char *name,
                        const gk_machine_state *st);
gk_status gk_load_state(const gk_save_states *s, const char *name,
                        gk_machine_state *out);

/* ---- transfer / network (471-475) ---- */

typedef enum {
    GK_XFER_DNC = 0,   /* 471 */
    GK_XFER_SERIAL,    /* 472 */
    GK_XFER_USB,       /* 473 */
    GK_XFER_NETWORK,   /* 474 */
    GK_XFER_CLOUD,     /* 475 */
    GK_XFER_COUNT
} gk_transfer_kind;

const char *gk_transfer_kind_name(gk_transfer_kind k);

typedef struct {
    gk_transfer_kind kind;
    double baud;          /* for serial */
    double progress;      /* 0..1 */
    int connected;
    int bytes_sent;
    int bytes_total;
} gk_transfer;

gk_status gk_transfer_init(gk_transfer *t, gk_transfer_kind kind);
gk_status gk_transfer_connect(gk_transfer *t);
gk_status gk_transfer_disconnect(gk_transfer *t);
/* advance transfer by `elapsed` seconds at the configured rate */
gk_status gk_transfer_step(gk_transfer *t, double elapsed);
int gk_transfer_done(const gk_transfer *t);

/* ---- auto-save / version history / encryption (476-478) ---- */

typedef struct {
    int enabled;
    double interval;      /* seconds */
    double accumulator;
    int saves;
} gk_autosave;

void gk_autosave_init(gk_autosave *a, double interval);
/* returns 1 when a save was triggered */
int gk_autosave_tick(gk_autosave *a, double dt);

typedef struct {
    int revision;
    char content[GK_FILE_BUF];
    char author[GK_FILE_NAME];
    int size;
} gk_version;

typedef struct {
    gk_version versions[GK_FILE_MAX_VERSIONS];
    int count;
} gk_history;

void gk_history_init(gk_history *h);
gk_status gk_history_commit(gk_history *h, const char *author,
                            const char *content);
int gk_history_count(const gk_history *h);
const gk_version *gk_history_at(const gk_history *h, int i);
gk_status gk_history_rollback(gk_history *h, int revision,
                              char *out, size_t len);

/* 478 xor-based reversible file encryption for the simulator. */
void gk_file_encrypt(const char *in, size_t len, unsigned char key,
                     char *out);
void gk_file_decrypt(const char *in, size_t len, unsigned char key,
                     char *out);
int gk_file_is_encrypted(const char *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* GK_FILE_H */
