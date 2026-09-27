#include "gk/gk_progflow.h"

#include <math.h>
#include <string.h>

static void gk__progflow_copy(char *dst, size_t cap, const char *src)
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

const char *gk_progflow_stage_name(gk_progflow_stage s)
{
    switch (s) {
    case GK_PROGFLOW_DRAWING: return "drawing-analysis";
    case GK_PROGFLOW_PROCESS: return "process-analysis";
    case GK_PROGFLOW_MODEL: return "modeling";
    case GK_PROGFLOW_CAM: return "programming";
    case GK_PROGFLOW_SIMULATION: return "simulation";
    case GK_PROGFLOW_POST: return "post-processing";
    case GK_PROGFLOW_VERIFY: return "verification";
    case GK_PROGFLOW_FIRST_CUT: return "first-article-cut";
    case GK_PROGFLOW_OPTIMIZE: return "optimization";
    case GK_PROGFLOW_FREEZE: return "freezing";
    case GK_PROGFLOW_ARCHIVE: return "archiving";
    case GK_PROGFLOW_VERSION: return "version-management";
    case GK_PROGFLOW_PERMISSION: return "permission-management";
    case GK_PROGFLOW_BACKUP: return "backup";
    case GK_PROGFLOW_RESTORE: return "restore";
    default: return "unknown";
    }
}

int gk_progflow_stage_index(gk_progflow_stage s)
{
    if ((int)s < 0 || (int)s > 14) {
        return -1;
    }
    return (int)s;
}

void gk_progflow_init(gk_progflow *p, const char *name)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    gk__progflow_copy(p->name, sizeof(p->name), name);
    p->revision = 1;
    p->current = 0;
}

gk_status gk_progflow_advance(gk_progflow *p, gk_progflow_stage stage)
{
    if (p == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (gk_progflow_stage_index(stage) < 0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    if (p->frozen && stage != GK_PROGFLOW_RESTORE &&
        stage != GK_PROGFLOW_PERMISSION && stage != GK_PROGFLOW_ARCHIVE) {
        return GK_ERR_STATE;
    }
    if (stage == GK_PROGFLOW_FIRST_CUT && !p->simulated) {
        return GK_ERR_STATE;
    }
    if (stage == GK_PROGFLOW_ARCHIVE && !p->frozen) {
        return GK_ERR_STATE;
    }
    if (stage == GK_PROGFLOW_FREEZE) {
        p->frozen = 1;
    }
    if (stage == GK_PROGFLOW_ARCHIVE) {
        p->archived = 1;
    }
    if (stage == GK_PROGFLOW_SIMULATION) {
        p->simulated = 1;
    }
    if (stage == GK_PROGFLOW_VERIFY) {
        p->verified = 1;
    }
    if (stage == GK_PROGFLOW_BACKUP) {
        p->backed_up = 1;
    }
    p->current = (int)stage;
    return GK_OK;
}

gk_status gk_progflow_set_flag(gk_progflow *p, const char *flag, int value)
{
    if (p == NULL || flag == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (strcmp(flag, "simulated") == 0) {
        p->simulated = value ? 1 : 0;
    } else if (strcmp(flag, "verified") == 0) {
        p->verified = value ? 1 : 0;
    } else if (strcmp(flag, "frozen") == 0) {
        p->frozen = value ? 1 : 0;
    } else if (strcmp(flag, "archived") == 0) {
        p->archived = value ? 1 : 0;
    } else if (strcmp(flag, "backed-up") == 0) {
        p->backed_up = value ? 1 : 0;
    } else if (strcmp(flag, "permission-ok") == 0) {
        p->permission_ok = value ? 1 : 0;
    } else {
        return GK_ERR_NOT_FOUND;
    }
    return GK_OK;
}

int gk_progflow_flag(const gk_progflow *p, const char *flag)
{
    if (p == NULL || flag == NULL) {
        return 0;
    }
    if (strcmp(flag, "simulated") == 0) {
        return p->simulated;
    }
    if (strcmp(flag, "verified") == 0) {
        return p->verified;
    }
    if (strcmp(flag, "frozen") == 0) {
        return p->frozen;
    }
    if (strcmp(flag, "archived") == 0) {
        return p->archived;
    }
    if (strcmp(flag, "backed-up") == 0) {
        return p->backed_up;
    }
    if (strcmp(flag, "permission-ok") == 0) {
        return p->permission_ok;
    }
    return 0;
}

double gk_progflow_progress(const gk_progflow *p)
{
    if (p == NULL) {
        return 0.0;
    }
    return (double)p->current / 14.0;
}

/* ===================================================================
 * Simulation
 * =================================================================== */

void gk_progflow_sim_init(gk_progflow_sim *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
}

int gk_progflow_sim_clean(const gk_progflow_sim *s)
{
    if (s == NULL) {
        return 0;
    }
    return s->collisions == 0 && s->overtravels == 0;
}

/* ===================================================================
 * First article
 * =================================================================== */

void gk_progflow_first_article_init(gk_progflow_first_article *f, double nominal,
                                    double tolerance, double measured)
{
    if (f == NULL) {
        return;
    }
    f->nominal = nominal;
    f->tolerance = tolerance;
    f->measured = measured;
}

int gk_progflow_first_article_ok(const gk_progflow_first_article *f)
{
    if (f == NULL) {
        return 0;
    }
    return fabs(f->measured - f->nominal) <= f->tolerance;
}

/* ===================================================================
 * Backup / restore
 * =================================================================== */

void gk_progflow_backup_init(gk_progflow_backup *b, const char *path,
                             int revision)
{
    if (b == NULL) {
        return;
    }
    memset(b, 0, sizeof(*b));
    gk__progflow_copy(b->path, sizeof(b->path), path);
    b->revision = revision;
}

gk_status gk_progflow_restore(gk_progflow *p, const gk_progflow_backup *b)
{
    if (p == NULL || b == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (b->path[0] == '\0' || b->revision <= 0) {
        return GK_ERR_STATE;
    }
    p->revision = b->revision;
    p->frozen = 0;
    p->current = (int)GK_PROGFLOW_RESTORE;
    return GK_OK;
}
