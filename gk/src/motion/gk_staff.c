#include "gk/gk_staff.h"

#include <math.h>
#include <string.h>

static void gk__staff_copy(char *dst, size_t cap, const char *src)
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

const char *gk_staff_role_name(gk_staff_role r)
{
    switch (r) {
    case GK_STAFF_OPERATOR: return "operator";
    case GK_STAFF_PROGRAMMER: return "programmer";
    case GK_STAFF_PROCESS_ENG: return "process-engineer";
    case GK_STAFF_INSPECTOR: return "inspector";
    case GK_STAFF_MAINTAINER: return "maintainer";
    case GK_STAFF_TEAM_LEADER: return "team-leader";
    case GK_STAFF_WORKSHOP_HEAD: return "workshop-head";
    case GK_STAFF_PROD_MANAGER: return "production-manager";
    default: return "unknown";
    }
}

int gk_staff_role_level(gk_staff_role r)
{
    switch (r) {
    case GK_STAFF_OPERATOR: return 1;
    case GK_STAFF_PROGRAMMER: return 2;
    case GK_STAFF_INSPECTOR: return 2;
    case GK_STAFF_PROCESS_ENG: return 3;
    case GK_STAFF_MAINTAINER: return 3;
    case GK_STAFF_TEAM_LEADER: return 4;
    case GK_STAFF_WORKSHOP_HEAD: return 5;
    case GK_STAFF_PROD_MANAGER: return 6;
    default: return 0;
    }
}

void gk_staff_perms_for_role(gk_staff_role r, gk_staff_perms *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->can_start_machine = 1;
    switch (r) {
    case GK_STAFF_PROGRAMMER:
        out->can_edit_program = 1;
        break;
    case GK_STAFF_PROCESS_ENG:
        out->can_edit_program = 1;
        out->can_set_param = 1;
        break;
    case GK_STAFF_TEAM_LEADER:
        out->can_edit_program = 1;
        out->can_set_param = 1;
        break;
    case GK_STAFF_WORKSHOP_HEAD:
        out->can_edit_program = 1;
        out->can_set_param = 1;
        out->can_manage_users = 1;
        break;
    case GK_STAFF_PROD_MANAGER:
        out->can_edit_program = 1;
        out->can_set_param = 1;
        out->can_manage_users = 1;
        break;
    default:
        break;
    }
}

int gk_staff_can(const gk_staff_perms *p, const char *action)
{
    if (p == NULL || action == NULL) {
        return 0;
    }
    if (strcmp(action, "edit-program") == 0) {
        return p->can_edit_program;
    }
    if (strcmp(action, "set-param") == 0) {
        return p->can_set_param;
    }
    if (strcmp(action, "start-machine") == 0) {
        return p->can_start_machine;
    }
    if (strcmp(action, "manage-users") == 0) {
        return p->can_manage_users;
    }
    return 0;
}

void gk_staff_init(gk_staff_member *m, const char *name, gk_staff_role r)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    gk__staff_copy(m->name, sizeof(m->name), name);
    m->role = r;
    gk_staff_perms_for_role(r, &m->perms);
    m->present = 1;
}

gk_status gk_staff_add_skill(gk_staff_member *m, int skill_level)
{
    if (m == NULL || skill_level < 0 || skill_level > 10) {
        return GK_ERR_OUT_OF_RANGE;
    }
    if (m->skill_count >= GK_STAFF_SKILL_MAX) {
        return GK_ERR_OVERFLOW;
    }
    m->skills[m->skill_count] = skill_level;
    m->skill_count++;
    return GK_OK;
}

int gk_staff_skill_max(const gk_staff_member *m)
{
    int i, mx = 0;
    if (m == NULL) {
        return 0;
    }
    for (i = 0; i < m->skill_count; i++) {
        if (m->skills[i] > mx) {
            mx = m->skills[i];
        }
    }
    return mx;
}

/* ===================================================================
 * Shift (1395)
 * =================================================================== */

void gk_staff_shift_init(gk_staff_shift *s, int shift_index)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->shift_index = shift_index;
}

gk_status gk_staff_shift_assign(gk_staff_shift *s, const char *name)
{
    if (s == NULL || name == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (s->count >= GK_STAFF_MAX) {
        return GK_ERR_OVERFLOW;
    }
    gk__staff_copy(s->member[s->count], GK_STAFF_NAME, name);
    s->count++;
    return GK_OK;
}

int gk_staff_shift_size(const gk_staff_shift *s)
{
    if (s == NULL) {
        return 0;
    }
    return s->count;
}

/* ===================================================================
 * Attendance (1396)
 * =================================================================== */

void gk_staff_attendance_init(gk_staff_attendance *a)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
}

gk_status gk_staff_attendance_mark(gk_staff_attendance *a, int present)
{
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (present) {
        a->present_days++;
    } else {
        a->absent_days++;
    }
    return GK_OK;
}

double gk_staff_attendance_rate(const gk_staff_attendance *a)
{
    int total;
    if (a == NULL) {
        return 0.0;
    }
    total = a->present_days + a->absent_days;
    if (total == 0) {
        return 0.0;
    }
    return (double)a->present_days / (double)total;
}

/* ===================================================================
 * Performance (1397/1420)
 * =================================================================== */

void gk_staff_performance_init(gk_staff_performance *p)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
}

double gk_staff_performance_score(const gk_staff_performance *p)
{
    if (p == NULL) {
        return 0.0;
    }
    return 0.4 * p->quality_score + 0.4 * p->efficiency +
           0.2 * p->attendance;
}

/* ===================================================================
 * Training (1398)
 * =================================================================== */

void gk_staff_training_init(gk_staff_training *t, int required_hours)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->required_hours = required_hours;
}

gk_status gk_staff_training_complete(gk_staff_training *t, int hours)
{
    if (t == NULL || hours < 0) {
        return GK_ERR_INVALID_ARG;
    }
    t->completed_hours += hours;
    return GK_OK;
}

int gk_staff_training_done(const gk_staff_training *t)
{
    if (t == NULL) {
        return 0;
    }
    return t->completed_hours >= t->required_hours && t->required_hours > 0;
}

/* ===================================================================
 * Skill matrix (1399)
 * =================================================================== */

void gk_staff_matrix_init(gk_staff_matrix *m, int rows, int cols)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    m->rows = (rows > GK_STAFF_MAX) ? GK_STAFF_MAX : rows;
    m->cols = (cols > GK_STAFF_SKILL_MAX) ? GK_STAFF_SKILL_MAX : cols;
}

gk_status gk_staff_matrix_set(gk_staff_matrix *m, int row, int col, int level)
{
    if (m == NULL || row < 0 || row >= m->rows || col < 0 || col >= m->cols ||
        level < 0 || level > 10) {
        return GK_ERR_OUT_OF_RANGE;
    }
    m->matrix[row][col] = level;
    return GK_OK;
}

int gk_staff_matrix_get(const gk_staff_matrix *m, int row, int col)
{
    if (m == NULL || row < 0 || row >= m->rows || col < 0 || col >= m->cols) {
        return 0;
    }
    return m->matrix[row][col];
}

int gk_staff_matrix_row_sum(const gk_staff_matrix *m, int row)
{
    int c, s = 0;
    if (m == NULL || row < 0 || row >= m->rows) {
        return 0;
    }
    for (c = 0; c < m->cols; c++) {
        s += m->matrix[row][c];
    }
    return s;
}

int gk_staff_matrix_best_for(const gk_staff_matrix *m, int col)
{
    int r, best = -1, bestv = -1;
    if (m == NULL || col < 0 || col >= m->cols) {
        return -1;
    }
    for (r = 0; r < m->rows; r++) {
        if (m->matrix[r][col] > bestv) {
            bestv = m->matrix[r][col];
            best = r;
        }
    }
    return best;
}

/* ===================================================================
 * Dispatch (1400)
 * =================================================================== */

gk_status gk_staff_dispatch(const int *skills, int n, const int *required,
                            int *assignment)
{
    int i;
    if (skills == NULL || required == NULL || assignment == NULL || n < 0) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < n; i++) {
        if (skills[i] >= required[i]) {
            assignment[i] = i;
        } else {
            assignment[i] = -1;
        }
    }
    return GK_OK;
}
