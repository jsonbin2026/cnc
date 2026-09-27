#include "gk/gk_cadcam.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int gk__streq_ci(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        char ca = *a;
        char cb = *b;
        if (ca >= 'A' && ca <= 'Z') {
            ca = (char)(ca - 'A' + 'a');
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb = (char)(cb - 'A' + 'a');
        }
        if (ca != cb) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static void gk__copy(char *dst, size_t cap, const char *src)
{
    size_t n;
    if (dst == NULL || cap == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    n = strlen(src);
    if (n >= cap) {
        n = cap - 1;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
}

/* ===================================================================
 * Part A: 2D drawing
 * =================================================================== */

const char *gk_cad_entity_name(gk_cad_entity_kind k)
{
    switch (k) {
    case GK_CAD_LINE: return "line";
    case GK_CAD_ARC: return "arc";
    case GK_CAD_FILLET: return "fillet";
    case GK_CAD_CHAMFER: return "chamfer";
    default: return "unknown";
    }
}

void gk_cad_sketch_init(gk_cad_sketch *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
}

int gk_cad_add_line(gk_cad_sketch *s, double x1, double y1, double x2,
                    double y2)
{
    gk_cad_entity *e;
    if (s == NULL || s->count >= GK_CAD_MAX_ENTITIES) {
        return -1;
    }
    e = &s->entities[s->count];
    memset(e, 0, sizeof(*e));
    e->kind = GK_CAD_LINE;
    e->x1 = x1;
    e->y1 = y1;
    e->x2 = x2;
    e->y2 = y2;
    s->count++;
    return s->count;
}

int gk_cad_add_arc(gk_cad_sketch *s, double cx, double cy, double radius,
                   int clockwise)
{
    gk_cad_entity *e;
    if (s == NULL || radius <= 0.0 || s->count >= GK_CAD_MAX_ENTITIES) {
        return -1;
    }
    e = &s->entities[s->count];
    memset(e, 0, sizeof(*e));
    e->kind = GK_CAD_ARC;
    e->cx = cx;
    e->cy = cy;
    e->radius = radius;
    e->clockwise = clockwise ? 1 : 0;
    s->count++;
    return s->count;
}

gk_status gk_cad_add_fillet(gk_cad_sketch *s, double radius)
{
    gk_cad_entity *e;
    if (s == NULL || radius <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    if (s->count >= GK_CAD_MAX_ENTITIES) {
        return GK_ERR_OUT_OF_RANGE;
    }
    e = &s->entities[s->count];
    memset(e, 0, sizeof(*e));
    e->kind = GK_CAD_FILLET;
    e->radius = radius;
    s->count++;
    return GK_OK;
}

gk_status gk_cad_add_chamfer(gk_cad_sketch *s, double distance)
{
    gk_cad_entity *e;
    if (s == NULL || distance <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    if (s->count >= GK_CAD_MAX_ENTITIES) {
        return GK_ERR_OUT_OF_RANGE;
    }
    e = &s->entities[s->count];
    memset(e, 0, sizeof(*e));
    e->kind = GK_CAD_CHAMFER;
    e->radius = distance;
    s->count++;
    return GK_OK;
}

double gk_cad_sketch_length(const gk_cad_sketch *s)
{
    int i;
    double total = 0.0;
    if (s == NULL) {
        return 0.0;
    }
    for (i = 0; i < s->count; i++) {
        const gk_cad_entity *e = &s->entities[i];
        switch (e->kind) {
        case GK_CAD_LINE:
            total += hypot(e->x2 - e->x1, e->y2 - e->y1);
            break;
        case GK_CAD_ARC:
            total += M_PI * e->radius;
            break;
        case GK_CAD_FILLET:
            total += 0.5 * M_PI * e->radius;
            break;
        case GK_CAD_CHAMFER:
            total += sqrt(2.0) * e->radius;
            break;
        default:
            break;
        }
    }
    return total;
}

/* ===================================================================
 * Part B: 3D modelling
 * =================================================================== */

void gk_cad_solid_init(gk_cad_solid *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->revolve_steps = 36;
    s->revolve_angle = 360.0;
}

gk_status gk_cad_extrude(gk_cad_solid *s, double height, gk_cad_box *out)
{
    int i;
    if (s == NULL || out == NULL || height <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    if (s->profile_count == 0) {
        return GK_ERR_STATE;
    }
    out->x1 = out->x2 = s->profile[0].x1;
    out->y1 = out->y2 = s->profile[0].y1;
    out->z1 = 0.0;
    out->z2 = height;
    for (i = 0; i < s->profile_count; i++) {
        const gk_cad_entity *e = &s->profile[i];
        double px[2];
        double py[2];
        int k;
        px[0] = e->x1;
        py[0] = e->y1;
        px[1] = e->x2;
        py[1] = e->y2;
        for (k = 0; k < 2; k++) {
            if (px[k] < out->x1) {
                out->x1 = px[k];
            }
            if (px[k] > out->x2) {
                out->x2 = px[k];
            }
            if (py[k] < out->y1) {
                out->y1 = py[k];
            }
            if (py[k] > out->y2) {
                out->y2 = py[k];
            }
        }
    }
    s->height = height;
    return GK_OK;
}

gk_status gk_cad_revolve(gk_cad_solid *s, double angle_deg, double radius,
                         double *out_volume)
{
    double frac;
    if (s == NULL || radius <= 0.0 || angle_deg <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    if (angle_deg > 360.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    frac = angle_deg / 360.0;
    s->revolve_angle = angle_deg;
    if (out_volume != NULL) {
        *out_volume = frac * M_PI * radius * radius * (radius > 0.0 ? s->height : 0.0);
    }
    return GK_OK;
}

double gk_cad_box_volume(const gk_cad_box *b)
{
    double dx, dy, dz;
    if (b == NULL) {
        return 0.0;
    }
    dx = b->x2 - b->x1;
    dy = b->y2 - b->y1;
    dz = b->z2 - b->z1;
    if (dx < 0.0 || dy < 0.0 || dz < 0.0) {
        return 0.0;
    }
    return dx * dy * dz;
}

gk_status gk_cad_boolean(const gk_cad_box *a, const gk_cad_box *b,
                         gk_cad_boolean_op op, gk_cad_box *out)
{
    if (a == NULL || b == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    switch (op) {
    case GK_CAD_BOOL_UNION:
        out->x1 = (a->x1 < b->x1) ? a->x1 : b->x1;
        out->y1 = (a->y1 < b->y1) ? a->y1 : b->y1;
        out->z1 = (a->z1 < b->z1) ? a->z1 : b->z1;
        out->x2 = (a->x2 > b->x2) ? a->x2 : b->x2;
        out->y2 = (a->y2 > b->y2) ? a->y2 : b->y2;
        out->z2 = (a->z2 > b->z2) ? a->z2 : b->z2;
        break;
    case GK_CAD_BOOL_INTERSECT:
        out->x1 = (a->x1 > b->x1) ? a->x1 : b->x1;
        out->y1 = (a->y1 > b->y1) ? a->y1 : b->y1;
        out->z1 = (a->z1 > b->z1) ? a->z1 : b->z1;
        out->x2 = (a->x2 < b->x2) ? a->x2 : b->x2;
        out->y2 = (a->y2 < b->y2) ? a->y2 : b->y2;
        out->z2 = (a->z2 < b->z2) ? a->z2 : b->z2;
        if (out->x2 < out->x1 || out->y2 < out->y1 || out->z2 < out->z1) {
            memset(out, 0, sizeof(*out));
        }
        break;
    case GK_CAD_BOOL_SUBTRACT:
        /* conservative: keep A where it is not fully covered by B in X */
        *out = *a;
        if (gk_cad_box_volume(a) <= gk_cad_box_volume(b)) {
            memset(out, 0, sizeof(*out));
        }
        break;
    default:
        return GK_ERR_INVALID_ARG;
    }
    return GK_OK;
}

/* ===================================================================
 * Part C: CAM
 * =================================================================== */

const char *gk_cam_op_name(gk_cam_op_kind k)
{
    switch (k) {
    case GK_CAM_POCKET: return "pocket";
    case GK_CAM_CONTOUR_Z: return "waterline";
    case GK_CAM_PARALLEL: return "parallel";
    case GK_CAM_PROFILE: return "profile";
    case GK_CAM_DRILL: return "drill";
    case GK_CAM_THREAD_MILL: return "thread_mill";
    case GK_CAM_TAP: return "tap";
    default: return "unknown";
    }
}

void gk_cam_job_init(gk_cam_job *j)
{
    if (j == NULL) {
        return;
    }
    memset(j, 0, sizeof(*j));
}

int gk_cam_add_op(gk_cam_job *j, const gk_cam_op *op)
{
    if (j == NULL || op == NULL || j->count >= GK_CAD_MAX_ENTITIES) {
        return -1;
    }
    j->ops[j->count] = *op;
    j->count++;
    return j->count;
}

double gk_cam_op_time(const gk_cam_op *op)
{
    double depth;
    double feed;
    double length;
    if (op == NULL) {
        return 0.0;
    }
    depth = op->z_top - op->z_bottom;
    if (depth < 0.0) {
        depth = -depth;
    }
    feed = (op->feed > 0.0) ? op->feed : 100.0;
    switch (op->kind) {
    case GK_CAM_DRILL:
    case GK_CAM_TAP:
        length = depth * (double)(op->holes > 0 ? op->holes : 1) * 2.0;
        break;
    case GK_CAM_THREAD_MILL:
        length = depth * (op->thread_pitch > 0.0 ? 1.0 : 1.0) + depth;
        break;
    case GK_CAM_POCKET:
    case GK_CAM_CONTOUR_Z:
    case GK_CAM_PARALLEL:
    case GK_CAM_PROFILE:
        length = depth * 4.0 * (1.0 + op->tool_diameter / 10.0);
        break;
    default:
        length = depth;
        break;
    }
    return length / feed * 60.0 + 2.0;
}

static const char *gk__op_code(gk_cam_op_kind k)
{
    switch (k) {
    case GK_CAM_POCKET: return "POCKET";
    case GK_CAM_CONTOUR_Z: return "WATERLINE";
    case GK_CAM_PARALLEL: return "PARALLEL";
    case GK_CAM_PROFILE: return "PROFILE";
    case GK_CAM_DRILL: return "DRILL";
    case GK_CAM_THREAD_MILL: return "THREADMILL";
    case GK_CAM_TAP: return "TAP";
    default: return "OP";
    }
}

gk_status gk_cam_generate(gk_cam_job *j)
{
    int i;
    int off;
    if (j == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    off = snprintf(j->program, sizeof(j->program), "G21 G90\n");
    if (off < 0 || (size_t)off >= sizeof(j->program)) {
        return GK_ERR_OUT_OF_RANGE;
    }
    for (i = 0; i < j->count; i++) {
        const gk_cam_op *op = &j->ops[i];
        int n;
        n = snprintf(j->program + off, sizeof(j->program) - (size_t)off,
                     "(OP %d %s)\nG0 X%.3f Y%.3f\nG1 Z%.3f F%.1f\n"
                     "G1 Z%.3f\n",
                     i, gk__op_code(op->kind), op->x, op->y, op->z_top,
                     op->feed, op->z_bottom);
        if (n < 0 || (size_t)n >= sizeof(j->program) - (size_t)off) {
            return GK_ERR_OUT_OF_RANGE;
        }
        off += n;
    }
    {
        int n = snprintf(j->program + off, sizeof(j->program) - (size_t)off,
                         "G0 Z50.0\nM30\n");
        if (n < 0 || (size_t)n >= sizeof(j->program) - (size_t)off) {
            return GK_ERR_OUT_OF_RANGE;
        }
    }
    return GK_OK;
}

/* ===================================================================
 * Part D: verify / post / drawing
 * =================================================================== */

void gk_cam_verify_init(gk_cam_verify *v)
{
    if (v == NULL) {
        return;
    }
    memset(v, 0, sizeof(*v));
}

static void gk__bounds_add(gk_cam_bounds *b, double x, double y, double z,
                           int *seen)
{
    if (!*seen) {
        b->min_x = b->max_x = x;
        b->min_y = b->max_y = y;
        b->min_z = b->max_z = z;
        *seen = 1;
        return;
    }
    if (x < b->min_x) {
        b->min_x = x;
    }
    if (x > b->max_x) {
        b->max_x = x;
    }
    if (y < b->min_y) {
        b->min_y = y;
    }
    if (y > b->max_y) {
        b->max_y = y;
    }
    if (z < b->min_z) {
        b->min_z = z;
    }
    if (z > b->max_z) {
        b->max_z = z;
    }
}

gk_status gk_cam_verify_run(const char *program, gk_cam_bounds *bounds,
                            gk_cam_verify *v)
{
    const char *p;
    int seen = 0;
    double X = 0.0, Y = 0.0, Z = 0.0;
    int have[3] = {0, 0, 0};
    if (program == NULL || bounds == NULL || v == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    gk_cam_verify_init(v);
    p = program;
    while (*p != '\0') {
        const char *line_end = strchr(p, '\n');
        int is_rapid = 0;
        const char *q = p;
        /* detect G0 / G00 */
        while (*q == ' ' || *q == '\t') {
            q++;
        }
        if ((q[0] == 'G' || q[0] == 'g') && q[1] == '0' && q[2] == '0') {
            is_rapid = 1;
        } else if ((q[0] == 'G' || q[0] == 'g') && q[1] == '0' &&
                   (q[2] == ' ' || q[2] == '\n' || q[2] == '\0')) {
            is_rapid = 1;
        }
        /* parse X/Y/Z on this line */
        {
            const char *r = p;
            while (line_end == NULL || r < line_end) {
                char c = *r;
                if (c == '\0' || c == '\n') {
                    break;
                }
                if (c == 'X' || c == 'x' || c == 'Y' || c == 'y' ||
                    c == 'Z' || c == 'z') {
                    int idx = (c == 'X' || c == 'x') ? 0
                              : (c == 'Y' || c == 'y') ? 1 : 2;
                    char *endp = NULL;
                    double val = strtod(r + 1, &endp);
                    if (endp != r + 1) {
                        if (idx == 0) {
                            X = val;
                        } else if (idx == 1) {
                            Y = val;
                        } else {
                            Z = val;
                        }
                        have[idx] = 1;
                        if (have[0] && have[1] && have[2]) {
                            gk__bounds_add(bounds, X, Y, Z, &seen);
                        }
                        r = endp;
                        continue;
                    }
                }
                r++;
            }
        }
        if (is_rapid && v != NULL) {
            v->rapid_distance += 1.0;
            if (have[2] && Z < 0.0) {
                v->rapid_into_material++;
            }
        } else if (v != NULL) {
            v->cutting_distance += 1.0;
        }
        if (line_end == NULL) {
            break;
        }
        p = line_end + 1;
    }
    if (!seen) {
        memset(bounds, 0, sizeof(*bounds));
    }
    return GK_OK;
}

const char *gk_cam_post_name(gk_cam_post p)
{
    switch (p) {
    case GK_CAM_POST_FANUC: return "fanuc";
    case GK_CAM_POST_SIEMENS: return "siemens";
    case GK_CAM_POST_HAAS: return "haas";
    case GK_CAM_POST_LINUXCNC: return "linuxcnc";
    default: return "unknown";
    }
}

gk_status gk_cam_post_transform(gk_cam_post post, const char *in,
                                char *out, size_t out_cap)
{
    if (in == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    switch (post) {
    case GK_CAM_POST_FANUC:
        gk__copy(out, out_cap, in);
        break;
    case GK_CAM_POST_SIEMENS:
        /* prepend a Siemens-style header */
        {
            size_t used;
            int n = snprintf(out, out_cap, ";SIEMENS\n");
            if (n < 0 || (size_t)n >= out_cap) {
                return GK_ERR_OUT_OF_RANGE;
            }
            used = (size_t)n;
            gk__copy(out + used, out_cap - used, in);
        }
        break;
    case GK_CAM_POST_HAAS:
        {
            size_t used;
            int n = snprintf(out, out_cap, "O0001\n");
            if (n < 0 || (size_t)n >= out_cap) {
                return GK_ERR_OUT_OF_RANGE;
            }
            used = (size_t)n;
            {
                size_t inlen = strlen(in);
                size_t remain = out_cap - used;
                size_t copy = inlen < remain - 1 ? inlen : remain - 1;
                memcpy(out + used, in, copy);
                out[used + copy] = '\0';
            }
        }
        break;
    case GK_CAM_POST_LINUXCNC:
        {
            size_t used;
            int n = snprintf(out, out_cap, ";LINUXCNC\n");
            if (n < 0 || (size_t)n >= out_cap) {
                return GK_ERR_OUT_OF_RANGE;
            }
            used = (size_t)n;
            gk__copy(out + used, out_cap - used, in);
        }
        break;
    default:
        return GK_ERR_INVALID_ARG;
    }
    return GK_OK;
}

void gk_cad_drawing_init(gk_cad_drawing *d, const char *title)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    gk__copy(d->title, sizeof(d->title), title);
    gk__copy(d->drawing_number, sizeof(d->drawing_number), "DWG-0001");
    d->scale = 1.0;
    d->views = 7;
    d->projection = 1;
}

gk_status gk_cad_drawing_generate(const gk_cad_drawing *d,
                                  const gk_cam_bounds *bounds, char *out,
                                  size_t out_cap)
{
    if (d == NULL || bounds == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    {
        int n = snprintf(out, out_cap,
                         "DRAWING %s [%s]\nSCALE %.2f PROJECTION %s VIEWS %d\n"
                         "EXTENTS X[%.2f,%.2f] Y[%.2f,%.2f] Z[%.2f,%.2f]\n",
                         d->title, d->drawing_number, d->scale,
                         d->projection ? "third" : "first", d->views,
                         bounds->min_x, bounds->max_x, bounds->min_y,
                         bounds->max_y, bounds->min_z, bounds->max_z);
        if (n < 0 || (size_t)n >= out_cap) {
            return GK_ERR_OUT_OF_RANGE;
        }
    }
    return GK_OK;
}

/* ===================================================================
 * Part E: CAD import
 * =================================================================== */

const char *gk_cad_import_name(gk_cad_import_format f)
{
    switch (f) {
    case GK_CAD_IMPORT_DXF: return "dxf";
    case GK_CAD_IMPORT_STEP: return "step";
    case GK_CAD_IMPORT_IGES: return "iges";
    case GK_CAD_IMPORT_STL: return "stl";
    case GK_CAD_IMPORT_OBJ: return "obj";
    case GK_CAD_IMPORT_3MF: return "3mf";
    case GK_CAD_IMPORT_PARASOLID: return "parasolid";
    default: return "unknown";
    }
}

gk_cad_import_format gk_cad_import_detect(const char *filename)
{
    const char *dot;
    if (filename == NULL) {
        return GK_CAD_IMPORT_COUNT;
    }
    dot = strrchr(filename, '.');
    if (dot == NULL) {
        return GK_CAD_IMPORT_COUNT;
    }
    if (gk__streq_ci(dot, ".dxf")) {
        return GK_CAD_IMPORT_DXF;
    }
    if (gk__streq_ci(dot, ".step") || gk__streq_ci(dot, ".stp")) {
        return GK_CAD_IMPORT_STEP;
    }
    if (gk__streq_ci(dot, ".iges") || gk__streq_ci(dot, ".igs")) {
        return GK_CAD_IMPORT_IGES;
    }
    if (gk__streq_ci(dot, ".stl")) {
        return GK_CAD_IMPORT_STL;
    }
    if (gk__streq_ci(dot, ".obj")) {
        return GK_CAD_IMPORT_OBJ;
    }
    if (gk__streq_ci(dot, ".3mf")) {
        return GK_CAD_IMPORT_3MF;
    }
    if (gk__streq_ci(dot, ".x_t") || gk__streq_ci(dot, ".x_b")) {
        return GK_CAD_IMPORT_PARASOLID;
    }
    return GK_CAD_IMPORT_COUNT;
}

gk_status gk_cad_import_probe(const char *header, gk_cad_import_format fmt,
                              int *out_entities)
{
    int count = 0;
    const char *p;
    if (header == NULL || out_entities == NULL || fmt >= GK_CAD_IMPORT_COUNT) {
        return GK_ERR_INVALID_ARG;
    }
    switch (fmt) {
    case GK_CAD_IMPORT_DXF:
        p = header;
        while ((p = strstr(p, "0\n")) != NULL) {
            count++;
            p += 2;
        }
        break;
    case GK_CAD_IMPORT_STEP:
        p = header;
        while ((p = strstr(p, "ADVANCED_FACE")) != NULL) {
            count++;
            p += 13;
        }
        break;
    case GK_CAD_IMPORT_IGES:
        p = header;
        while ((p = strstr(p, "P")) != NULL) {
            count++;
            p += 1;
        }
        break;
    case GK_CAD_IMPORT_STL:
        if (strstr(header, "solid") != NULL) {
            p = header;
            while ((p = strstr(p, "facet normal")) != NULL) {
                count++;
                p += 12;
            }
        }
        break;
    case GK_CAD_IMPORT_OBJ:
        p = header;
        while ((p = strchr(p, 'f')) != NULL) {
            if (p[1] == ' ' || p[1] == '\t') {
                count++;
            }
            p++;
        }
        break;
    case GK_CAD_IMPORT_3MF:
        p = header;
        while ((p = strstr(p, "<triangle")) != NULL) {
            count++;
            p += 9;
        }
        break;
    case GK_CAD_IMPORT_PARASOLID:
        count = 1;
        break;
    default:
        return GK_ERR_INVALID_ARG;
    }
    *out_entities = count;
    return GK_OK;
}

/* ===================================================================
 * Part F: platform & testing
 * =================================================================== */

const char *gk_cad_platform_name(gk_cad_platform p)
{
    switch (p) {
    case GK_CAD_PLATFORM_WINDOWS: return "windows";
    case GK_CAD_PLATFORM_LINUX: return "linux";
    case GK_CAD_PLATFORM_MACOS: return "macos";
    default: return "unknown";
    }
}

gk_cad_platform gk_cad_platform_current(void)
{
#if defined(_WIN32)
    return GK_CAD_PLATFORM_WINDOWS;
#elif defined(__APPLE__)
    return GK_CAD_PLATFORM_MACOS;
#else
    return GK_CAD_PLATFORM_LINUX;
#endif
}

int gk_cad_platform_supported(gk_cad_platform p)
{
    return p >= GK_CAD_PLATFORM_WINDOWS && p <= GK_CAD_PLATFORM_MACOS;
}

void gk_cad_test_suite_init(gk_cad_test_suite *t)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
}

int gk_cad_test_add(gk_cad_test_suite *t, const char *name, gk_cad_test_fn fn)
{
    if (t == NULL || fn == NULL || t->count >= GK_CAD_MAX_ENTITIES) {
        return -1;
    }
    gk__copy(t->cases[t->count].name, sizeof(t->cases[t->count].name), name);
    t->cases[t->count].fn = fn;
    t->count++;
    return t->count;
}

int gk_cad_test_run(gk_cad_test_suite *t)
{
    int i;
    if (t == NULL) {
        return -1;
    }
    t->passed = 0;
    t->failed = 0;
    for (i = 0; i < t->count; i++) {
        if (t->cases[i].fn() == 0) {
            t->passed++;
        } else {
            t->failed++;
        }
    }
    return t->passed;
}
