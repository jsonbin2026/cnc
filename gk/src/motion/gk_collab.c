#include "gk/gk_collab.h"

#include <stdio.h>
#include <string.h>

static void gk__copy(char *dst, size_t len, const char *src)
{
    size_t i;
    if (dst == NULL || len == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    for (i = 0; i + 1 < len && src[i] != '\0'; ++i) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

/* ---- session ---- */

void gk_session_init(gk_session *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->next_id = 1;
    s->session_open = 1;
}

int gk_session_add_user(gk_session *s, const char *name, gk_user_role role)
{
    gk_user *u;
    if (s == NULL || name == NULL || s->count >= GK_COLLAB_MAX_USERS) {
        return -1;
    }
    u = &s->users[s->count];
    memset(u, 0, sizeof(*u));
    u->id = s->next_id++;
    gk__copy(u->name, sizeof(u->name), name);
    u->role = role;
    u->online = 1;
    s->count++;
    return u->id;
}

gk_user *gk_session_user(gk_session *s, int id)
{
    int i;
    if (s == NULL) {
        return NULL;
    }
    for (i = 0; i < s->count; ++i) {
        if (s->users[i].id == id) {
            return &s->users[i];
        }
    }
    return NULL;
}

int gk_session_count_role(const gk_session *s, gk_user_role role)
{
    int i;
    int n = 0;
    if (s == NULL) {
        return 0;
    }
    for (i = 0; i < s->count; ++i) {
        if (s->users[i].role == role) n++;
    }
    return n;
}

gk_status gk_session_set_online(gk_session *s, int id, int online)
{
    gk_user *u = gk_session_user(s, id);
    if (u == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    u->online = online ? 1 : 0;
    return GK_OK;
}

/* ---- monitoring ---- */

void gk_monitor_init(gk_monitor_board *m)
{
    if (m != NULL) {
        memset(m, 0, sizeof(*m));
    }
}

gk_status gk_monitor_update(gk_monitor_board *m, const gk_monitor_frame *f)
{
    int i;
    if (m == NULL || f == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < m->count; ++i) {
        if (m->frames[i].user_id == f->user_id) {
            m->frames[i] = *f;
            return GK_OK;
        }
    }
    if (m->count >= GK_COLLAB_MAX_USERS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    m->frames[m->count++] = *f;
    return GK_OK;
}

const gk_monitor_frame *gk_monitor_of(const gk_monitor_board *m, int user_id)
{
    int i;
    if (m == NULL) {
        return NULL;
    }
    for (i = 0; i < m->count; ++i) {
        if (m->frames[i].user_id == user_id) {
            return &m->frames[i];
        }
    }
    return NULL;
}

int gk_monitor_peak_load(const gk_monitor_board *m)
{
    int i;
    int best = -1;
    double peak = -1.0;
    if (m == NULL) {
        return -1;
    }
    for (i = 0; i < m->count; ++i) {
        if (m->frames[i].load > peak) {
            peak = m->frames[i].load;
            best = m->frames[i].user_id;
        }
    }
    return best;
}

/* ---- takeover ---- */

void gk_takeover_init(gk_takeover *t)
{
    if (t != NULL) {
        memset(t, 0, sizeof(*t));
    }
}

gk_status gk_takeover_start(gk_takeover *t, int controller, int target)
{
    if (t == NULL || controller == target) {
        return GK_ERR_INVALID_ARG;
    }
    if (t->active) {
        return GK_ERR_STATE;
    }
    t->controller_id = controller;
    t->target_id = target;
    t->active = 1;
    return GK_OK;
}

gk_status gk_takeover_act(gk_takeover *t)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!t->active) {
        return GK_ERR_STATE;
    }
    t->actions++;
    return GK_OK;
}

gk_status gk_takeover_stop(gk_takeover *t)
{
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    t->active = 0;
    return GK_OK;
}

/* ---- task board ---- */

void gk_task_board_init(gk_task_board *t)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->next_id = 1;
}

static gk_assignment *gk__task(gk_task_board *t, int id)
{
    int i;
    if (t == NULL) {
        return NULL;
    }
    for (i = 0; i < t->count; ++i) {
        if (t->items[i].id == id) {
            return &t->items[i];
        }
    }
    return NULL;
}

int gk_task_dispatch(gk_task_board *t, const char *title, int user_id,
                     double deadline)
{
    gk_assignment *a;
    if (t == NULL || title == NULL || t->count >= GK_COLLAB_MAX_ITEMS) {
        return -1;
    }
    a = &t->items[t->count++];
    memset(a, 0, sizeof(*a));
    a->id = t->next_id++;
    gk__copy(a->title, sizeof(a->title), title);
    a->assigned_to = user_id;
    a->deadline = deadline;
    return a->id;
}

gk_status gk_task_submit(gk_task_board *t, int task_id, const char *content)
{
    gk_assignment *a = gk__task(t, task_id);
    if (a == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    if (a->status != 0) {
        return GK_ERR_STATE;
    }
    gk__copy(a->content, sizeof(a->content), content);
    a->status = 1;
    return GK_OK;
}

gk_status gk_task_grade(gk_task_board *t, int task_id, double score)
{
    gk_assignment *a = gk__task(t, task_id);
    if (a == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    if (a->status != 1) {
        return GK_ERR_STATE;
    }
    a->score = score;
    a->status = 2;
    return GK_OK;
}

int gk_task_open_count(const gk_task_board *t)
{
    int i;
    int n = 0;
    if (t == NULL) {
        return 0;
    }
    for (i = 0; i < t->count; ++i) {
        if (t->items[i].status == 0) n++;
    }
    return n;
}

int gk_task_count_for_user(const gk_task_board *t, int user_id)
{
    int i;
    int n = 0;
    if (t == NULL) {
        return 0;
    }
    for (i = 0; i < t->count; ++i) {
        if (t->items[i].assigned_to == user_id) n++;
    }
    return n;
}

/* ---- class rank ---- */

void gk_class_rank_init(gk_class_rank *r)
{
    if (r != NULL) {
        memset(r, 0, sizeof(*r));
    }
}

gk_status gk_class_rank_submit(gk_class_rank *r, const char *name,
                               double score)
{
    int i;
    int found = 0;
    if (r == NULL || name == NULL || r->count >= GK_COLLAB_MAX_USERS) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < r->count; ++i) {
        if (strcmp(r->entries[i].name, name) == 0) {
            r->entries[i].score = score;
            found = 1;
        }
    }
    if (!found) {
        gk__copy(r->entries[r->count].name, sizeof(r->entries[r->count].name),
                 name);
        r->entries[r->count].score = score;
        r->count++;
    }
    /* insertion sort descending */
    for (i = 1; i < r->count; ++i) {
        int j = i;
        while (j > 0 && r->entries[j].score > r->entries[j - 1].score) {
            gk_rank_entry tmp = r->entries[j];
            r->entries[j] = r->entries[j - 1];
            r->entries[j - 1] = tmp;
            j--;
        }
    }
    return GK_OK;
}

int gk_class_rank_of(const gk_class_rank *r, const char *name)
{
    int i;
    if (r == NULL || name == NULL) {
        return -1;
    }
    for (i = 0; i < r->count; ++i) {
        if (strcmp(r->entries[i].name, name) == 0) {
            return i + 1;
        }
    }
    return -1;
}

/* ---- network ---- */

void gk_network_init(gk_network *n, gk_net_topology topo)
{
    if (n == NULL) {
        return;
    }
    memset(n, 0, sizeof(*n));
    n->topology = topo;
    n->latency_ms = 5;
}

gk_status gk_network_add_machine(gk_network *n, int machine_id)
{
    if (n == NULL || n->count >= GK_COLLAB_MAX_USERS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    n->nodes[n->count].machine_id = machine_id;
    n->nodes[n->count].peer_count = 0;
    n->count++;
    return GK_OK;
}

static gk_machine_node *gk__node(gk_network *n, int id)
{
    int i;
    if (n == NULL) {
        return NULL;
    }
    for (i = 0; i < n->count; ++i) {
        if (n->nodes[i].machine_id == id) {
            return &n->nodes[i];
        }
    }
    return NULL;
}

gk_status gk_network_link(gk_network *n, int a, int b)
{
    gk_machine_node *na = gk__node(n, a);
    gk_machine_node *nb = gk__node(n, b);
    if (na == NULL || nb == NULL || a == b) {
        return GK_ERR_NOT_FOUND;
    }
    if (na->peer_count < GK_COLLAB_MAX_USERS) {
        na->peers[na->peer_count++] = b;
    }
    if (nb->peer_count < GK_COLLAB_MAX_USERS) {
        nb->peers[nb->peer_count++] = a;
    }
    na->linked = 1;
    nb->linked = 1;
    return GK_OK;
}

int gk_network_links(const gk_network *n)
{
    int i;
    int total = 0;
    if (n == NULL) {
        return 0;
    }
    for (i = 0; i < n->count; ++i) {
        total += n->nodes[i].peer_count;
    }
    return total / 2;
}

/* ---- chat ---- */

void gk_chat_init(gk_chat *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    c->max_len = GK_COLLAB_TEXT - 1;
}

gk_status gk_chat_send(gk_chat *c, int user_id, const char *text)
{
    gk_danmaku *d;
    if (c == NULL || text == NULL || c->count >= GK_COLLAB_MAX_ITEMS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    if ((int)strlen(text) > c->max_len) {
        return GK_ERR_INVALID_ARG;
    }
    d = &c->messages[c->count++];
    d->user_id = user_id;
    gk__copy(d->text, sizeof(d->text), text);
    d->time = (double)c->count;
    return GK_OK;
}

int gk_chat_count(const gk_chat *c)
{
    return c != NULL ? c->count : 0;
}

int gk_chat_allowed(const gk_chat *c, double now, double min_gap)
{
    if (c == NULL || c->count == 0) {
        return 1;
    }
    return (now - c->messages[c->count - 1].time) >= min_gap;
}

/* ---- broadcast ---- */

void gk_broadcast_init(gk_broadcast *b)
{
    if (b != NULL) {
        memset(b, 0, sizeof(*b));
    }
}

gk_status gk_broadcast_start(gk_broadcast *b, int broadcaster, int quality)
{
    if (b == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (b->active) {
        return GK_ERR_STATE;
    }
    b->broadcaster = broadcaster;
    b->quality = quality;
    b->active = 1;
    b->viewer_count = 0;
    return GK_OK;
}

gk_status gk_broadcast_join(gk_broadcast *b, int viewer)
{
    int i;
    if (b == NULL || !b->active) {
        return GK_ERR_STATE;
    }
    if (viewer == b->broadcaster) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < b->viewer_count; ++i) {
        if (b->viewers[i] == viewer) {
            return GK_ERR_ALREADY_EXISTS;
        }
    }
    if (b->viewer_count >= GK_COLLAB_MAX_USERS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    b->viewers[b->viewer_count++] = viewer;
    return GK_OK;
}

gk_status gk_broadcast_stop(gk_broadcast *b)
{
    if (b == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    b->active = 0;
    b->viewer_count = 0;
    return GK_OK;
}

/* ---- groups ---- */

void gk_group_set_init(gk_group_set *g)
{
    if (g != NULL) {
        memset(g, 0, sizeof(*g));
    }
}

int gk_group_create(gk_group_set *g, const char *name)
{
    gk_group *grp;
    if (g == NULL || name == NULL || g->count >= 16) {
        return -1;
    }
    grp = &g->groups[g->count];
    memset(grp, 0, sizeof(*grp));
    grp->id = g->count + 1;
    gk__copy(grp->name, sizeof(grp->name), name);
    g->count++;
    return grp->id;
}

static gk_group *gk__group(gk_group_set *g, int id)
{
    int i;
    if (g == NULL) {
        return NULL;
    }
    for (i = 0; i < g->count; ++i) {
        if (g->groups[i].id == id) {
            return &g->groups[i];
        }
    }
    return NULL;
}

gk_status gk_group_join(gk_group_set *g, int group_id, int user_id)
{
    gk_group *grp = gk__group(g, group_id);
    if (grp == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    if (grp->member_count >= GK_COLLAB_MAX_USERS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    grp->members[grp->member_count++] = user_id;
    return GK_OK;
}

int gk_group_size(const gk_group_set *g, int group_id)
{
    int i;
    if (g == NULL) {
        return 0;
    }
    for (i = 0; i < g->count; ++i) {
        if (g->groups[i].id == group_id) {
            return g->groups[i].member_count;
        }
    }
    return 0;
}

/* ---- competition ---- */

void gk_competition_init(gk_competition *c)
{
    if (c != NULL) {
        memset(c, 0, sizeof(*c));
    }
}

gk_status gk_competition_start(gk_competition *c, const char *name,
                               double start, double duration)
{
    if (c == NULL || name == NULL || duration <= 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    memset(c, 0, sizeof(*c));
    gk__copy(c->name, sizeof(c->name), name);
    c->start = start;
    c->duration = duration;
    c->running = 1;
    c->winner = -1;
    c->best_time = 1e30;
    return GK_OK;
}

gk_status gk_competition_record(gk_competition *c, int user_id, double time)
{
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (!c->running) {
        return GK_ERR_STATE;
    }
    if (time < c->best_time) {
        c->best_time = time;
        c->winner = user_id;
    }
    return GK_OK;
}

int gk_competition_is_over(const gk_competition *c, double now)
{
    if (c == NULL) {
        return 1;
    }
    return now >= c->start + c->duration;
}

/* ---- classroom ---- */

void gk_classroom_init(gk_classroom *c, const char *name)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
    gk__copy(c->name, sizeof(c->name), name);
}

gk_status gk_classroom_add_student(gk_classroom *c, int user_id)
{
    if (c == NULL || c->student_count >= GK_COLLAB_MAX_USERS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    if (gk_classroom_has_student(c, user_id)) {
        return GK_ERR_ALREADY_EXISTS;
    }
    c->students[c->student_count++] = user_id;
    return GK_OK;
}

gk_status gk_classroom_remove_student(gk_classroom *c, int user_id)
{
    int i;
    if (c == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < c->student_count; ++i) {
        if (c->students[i] == user_id) {
            memmove(&c->students[i], &c->students[i + 1],
                    (size_t)(c->student_count - i - 1) * sizeof(int));
            c->student_count--;
            return GK_OK;
        }
    }
    return GK_ERR_NOT_FOUND;
}

int gk_classroom_has_student(const gk_classroom *c, int user_id)
{
    int i;
    if (c == NULL) {
        return 0;
    }
    for (i = 0; i < c->student_count; ++i) {
        if (c->students[i] == user_id) return 1;
    }
    return 0;
}

int gk_classroom_size(const gk_classroom *c)
{
    return c != NULL ? c->student_count : 0;
}

/* ---- progress tracker ---- */

void gk_progress_tracker_init(gk_progress_tracker *p)
{
    if (p != NULL) {
        memset(p, 0, sizeof(*p));
    }
}

gk_status gk_course_assign_add(gk_progress_tracker *p, const char *course,
                               int user_id)
{
    gk_course_assign *a;
    if (p == NULL || course == NULL || p->count >= GK_COLLAB_MAX_ITEMS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    a = &p->items[p->count++];
    memset(a, 0, sizeof(*a));
    gk__copy(a->course, sizeof(a->course), course);
    a->assigned_to = user_id;
    return GK_OK;
}

gk_status gk_course_progress(gk_progress_tracker *p, const char *course,
                             int user_id, double progress)
{
    int i;
    if (p == NULL || course == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < p->count; ++i) {
        if (p->items[i].assigned_to == user_id &&
            strcmp(p->items[i].course, course) == 0) {
            p->items[i].progress = progress;
            if (progress >= 1.0) {
                p->items[i].completed = 1;
            }
            return GK_OK;
        }
    }
    return GK_ERR_NOT_FOUND;
}

double gk_course_avg_progress(const gk_progress_tracker *p, const char *course)
{
    double sum = 0.0;
    int n = 0;
    int i;
    if (p == NULL || course == NULL) {
        return 0.0;
    }
    for (i = 0; i < p->count; ++i) {
        if (strcmp(p->items[i].course, course) == 0) {
            sum += p->items[i].progress;
            n++;
        }
    }
    return n > 0 ? sum / n : 0.0;
}

/* ---- messages ---- */

void gk_message_box_init(gk_message_box *m)
{
    if (m != NULL) {
        memset(m, 0, sizeof(*m));
    }
}

gk_status gk_message_send(gk_message_box *m, const char *from, const char *to,
                          const char *text, double time)
{
    gk_message *msg;
    if (m == NULL || from == NULL || to == NULL || text == NULL ||
        m->count >= GK_COLLAB_MAX_ITEMS) {
        return GK_ERR_OUT_OF_RANGE;
    }
    msg = &m->messages[m->count++];
    gk__copy(msg->from, sizeof(msg->from), from);
    gk__copy(msg->to, sizeof(msg->to), to);
    gk__copy(msg->message, sizeof(msg->message), text);
    msg->time = time;
    return GK_OK;
}

int gk_message_count_to(const gk_message_box *m, const char *to)
{
    int i;
    int n = 0;
    if (m == NULL || to == NULL) {
        return 0;
    }
    for (i = 0; i < m->count; ++i) {
        if (strcmp(m->messages[i].to, to) == 0) n++;
    }
    return n;
}

/* ---- course editor ---- */

void gk_course_editor_init(gk_course_editor *e)
{
    if (e != NULL) {
        memset(e, 0, sizeof(*e));
    }
}

int gk_course_add_slide(gk_course_editor *e, const char *title,
                        const char *body)
{
    gk_course_slide *s;
    if (e == NULL || title == NULL || e->count >= GK_COLLAB_MAX_ITEMS) {
        return -1;
    }
    s = &e->slides[e->count];
    memset(s, 0, sizeof(*s));
    s->id = e->count + 1;
    s->order = e->count;
    gk__copy(s->title, sizeof(s->title), title);
    gk__copy(s->body, sizeof(s->body), body);
    e->count++;
    return s->id;
}

gk_status gk_course_move_slide(gk_course_editor *e, int id, int new_order)
{
    int i;
    if (e == NULL || new_order < 0 || new_order >= e->count) {
        return GK_ERR_OUT_OF_RANGE;
    }
    for (i = 0; i < e->count; ++i) {
        if (e->slides[i].id == id) {
            e->slides[i].order = new_order;
            return GK_OK;
        }
    }
    return GK_ERR_NOT_FOUND;
}

const gk_course_slide *gk_course_next(const gk_course_editor *e,
                                      int current_order)
{
    int i;
    const gk_course_slide *best = NULL;
    if (e == NULL) {
        return NULL;
    }
    for (i = 0; i < e->count; ++i) {
        if (e->slides[i].order > current_order) {
            if (best == NULL || e->slides[i].order < best->order) {
                best = &e->slides[i];
            }
        }
    }
    return best;
}

/* ---- scene ---- */

void gk_scene_init(gk_scene *s)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    gk__copy(s->environment, sizeof(s->environment), "workshop");
}

int gk_scene_add(gk_scene *s, const char *name, double x, double y, double z)
{
    gk_scene_object *o;
    if (s == NULL || name == NULL || s->count >= GK_COLLAB_MAX_ITEMS) {
        return -1;
    }
    o = &s->objects[s->count];
    memset(o, 0, sizeof(*o));
    o->id = s->count + 1;
    gk__copy(o->name, sizeof(o->name), name);
    o->x = x;
    o->y = y;
    o->z = z;
    o->scale = 1.0;
    o->visible = 1;
    s->count++;
    return o->id;
}

gk_status gk_scene_transform(gk_scene *s, int id, double x, double y, double z,
                             double rot)
{
    int i;
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < s->count; ++i) {
        if (s->objects[i].id == id) {
            s->objects[i].x = x;
            s->objects[i].y = y;
            s->objects[i].z = z;
            s->objects[i].rot = rot;
            return GK_OK;
        }
    }
    return GK_ERR_NOT_FOUND;
}

gk_status gk_scene_remove(gk_scene *s, int id)
{
    int i;
    if (s == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < s->count; ++i) {
        if (s->objects[i].id == id) {
            memmove(&s->objects[i], &s->objects[i + 1],
                    (size_t)(s->count - i - 1) * sizeof(gk_scene_object));
            s->count--;
            return GK_OK;
        }
    }
    return GK_ERR_NOT_FOUND;
}

/* ---- question bank ---- */

void gk_question_bank_init(gk_question_bank *b)
{
    if (b != NULL) {
        memset(b, 0, sizeof(*b));
    }
}

int gk_question_add(gk_question_bank *b, const char *prompt,
                    gk_question_kind kind, const char *answer, double points,
                    int difficulty)
{
    gk_question *q;
    if (b == NULL || prompt == NULL || b->count >= GK_COLLAB_MAX_ITEMS) {
        return -1;
    }
    q = &b->questions[b->count];
    memset(q, 0, sizeof(*q));
    q->id = b->count + 1;
    gk__copy(q->prompt, sizeof(q->prompt), prompt);
    q->kind = kind;
    gk__copy(q->answer, sizeof(q->answer), answer);
    q->points = points;
    q->difficulty = difficulty;
    b->count++;
    return q->id;
}

int gk_exam_generate(const gk_question_bank *b, int n, double min_points,
                     int *out_ids, int max_out)
{
    int count = 0;
    double total = 0.0;
    int i = 0;
    int limit;
    if (b == NULL || out_ids == NULL || n <= 0 || max_out <= 0) {
        return 0;
    }
    limit = n < max_out ? n : max_out;
    /* pick up to n questions */
    while (count < limit && i < b->count) {
        out_ids[count++] = b->questions[i].id;
        total += b->questions[i].points;
        i++;
    }
    /* keep adding until the point target is met or the bank runs out */
    while (total < min_points && i < b->count && count < max_out) {
        out_ids[count++] = b->questions[i].id;
        total += b->questions[i].points;
        i++;
    }
    return count;
}

/* ---- library ---- */

void gk_library_init(gk_library *l, const char *domain)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
    gk__copy(l->domain, sizeof(l->domain), domain);
}

int gk_library_add(gk_library *l, const char *name, const char *vendor,
                   double value, const char *unit)
{
    gk_lib_item *it;
    if (l == NULL || name == NULL || l->count >= GK_COLLAB_MAX_ITEMS) {
        return -1;
    }
    it = &l->items[l->count];
    memset(it, 0, sizeof(*it));
    it->id = l->count + 1;
    gk__copy(it->name, sizeof(it->name), name);
    gk__copy(it->vendor, sizeof(it->vendor), vendor);
    it->value = value;
    gk__copy(it->unit, sizeof(it->unit), unit);
    l->count++;
    return it->id;
}

const gk_lib_item *gk_library_find(const gk_library *l, const char *name)
{
    int i;
    if (l == NULL || name == NULL) {
        return NULL;
    }
    for (i = 0; i < l->count; ++i) {
        if (strcmp(l->items[i].name, name) == 0) {
            return &l->items[i];
        }
    }
    return NULL;
}

int gk_library_count(const gk_library *l)
{
    return l != NULL ? l->count : 0;
}

/* ---- updater ---- */

void gk_updater_init(gk_updater *u, int major, int minor, int patch)
{
    if (u == NULL) {
        return;
    }
    memset(u, 0, sizeof(*u));
    u->current.major = major;
    u->current.minor = minor;
    u->current.patch = patch;
    u->latest = u->current;
}

static int gk__version_cmp(gk_version_num a, gk_version_num b)
{
    if (a.major != b.major) return a.major - b.major;
    if (a.minor != b.minor) return a.minor - b.minor;
    return a.patch - b.patch;
}

gk_status gk_updater_check(gk_updater *u, int major, int minor, int patch,
                           double size_mb)
{
    if (u == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    u->latest.major = major;
    u->latest.minor = minor;
    u->latest.patch = patch;
    u->size_mb = size_mb;
    u->update_available = gk__version_cmp(u->latest, u->current) > 0;
    return GK_OK;
}

int gk_updater_install(gk_updater *u)
{
    if (u == NULL || !u->update_available) {
        return 0;
    }
    u->current = u->latest;
    u->update_available = 0;
    return 1;
}

const char *gk_updater_version_string(const gk_updater *u, char *buf,
                                      size_t len)
{
    if (u == NULL || buf == NULL) {
        return NULL;
    }
    snprintf(buf, len, "%d.%d.%d", u->current.major, u->current.minor,
             u->current.patch);
    return buf;
}

/* ---- distribution ---- */

void gk_distribution_init(gk_distribution *d, int peers, double bandwidth)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    d->peers = peers;
    d->bandwidth_mbps = bandwidth;
}

gk_status gk_distribution_push(gk_distribution *d, int chunks)
{
    if (d == NULL || chunks < 0) {
        return GK_ERR_INVALID_ARG;
    }
    d->chunks = chunks;
    d->distributed = chunks * d->peers;
    return GK_OK;
}

/* ---- content history ---- */

void gk_content_history_init(gk_content_history *h)
{
    if (h != NULL) {
        memset(h, 0, sizeof(*h));
    }
}

int gk_content_commit(gk_content_history *h, const char *note, int author)
{
    gk_content_version *v;
    if (h == NULL || note == NULL || h->count >= GK_COLLAB_MAX_ITEMS) {
        return -1;
    }
    v = &h->versions[h->count];
    v->revision = h->count + 1;
    gk__copy(v->note, sizeof(v->note), note);
    v->author_id = author;
    h->count++;
    return v->revision;
}

const gk_content_version *gk_content_version_at(const gk_content_history *h,
                                                int revision)
{
    int i;
    if (h == NULL) {
        return NULL;
    }
    for (i = 0; i < h->count; ++i) {
        if (h->versions[i].revision == revision) {
            return &h->versions[i];
        }
    }
    return NULL;
}

/* ---- UGC ---- */

void gk_ugc_store_init(gk_ugc_store *s)
{
    if (s != NULL) {
        memset(s, 0, sizeof(*s));
    }
}

int gk_ugc_publish(gk_ugc_store *s, int author_id, const char *title,
                   const char *type)
{
    gk_ugc_item *it;
    if (s == NULL || title == NULL || s->count >= GK_COLLAB_MAX_ITEMS) {
        return -1;
    }
    it = &s->items[s->count];
    memset(it, 0, sizeof(*it));
    it->author_id = author_id;
    gk__copy(it->title, sizeof(it->title), title);
    gk__copy(it->type, sizeof(it->type), type);
    it->published = 1;
    s->count++;
    return s->count;
}

static gk_ugc_item *gk__ugc(gk_ugc_store *s, int id)
{
    if (s == NULL || id < 1 || id > s->count) {
        return NULL;
    }
    return &s->items[id - 1];
}

gk_status gk_ugc_rate(gk_ugc_store *s, int id, double rating)
{
    gk_ugc_item *it = gk__ugc(s, id);
    if (it == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    if (rating < 0.0 || rating > 5.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    it->rating = (it->rating * it->rating_count + rating) /
                 (double)(it->rating_count + 1);
    it->rating_count++;
    return GK_OK;
}

double gk_ugc_rating(const gk_ugc_store *s, int id)
{
    if (s == NULL || id < 1 || id > s->count) {
        return 0.0;
    }
    return s->items[id - 1].rating;
}

int gk_ugc_count_by_author(const gk_ugc_store *s, int author_id)
{
    int i;
    int n = 0;
    if (s == NULL) {
        return 0;
    }
    for (i = 0; i < s->count; ++i) {
        if (s->items[i].author_id == author_id) n++;
    }
    return n;
}
