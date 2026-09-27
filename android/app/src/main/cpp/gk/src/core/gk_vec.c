#include "gk/gk_vec.h"

#include <string.h>

gk_status gk_vec_init(gk_vec *v, size_t elem_size, size_t cap,
                      const gk_allocator *alloc)
{
    if (v == NULL || elem_size == 0) {
        return GK_ERR_INVALID_ARG;
    }
    v->data = NULL;
    v->elem_size = elem_size;
    v->len = 0;
    v->cap = 0;
    v->alloc = alloc != NULL ? *alloc : gk_allocator_default();
    if (cap > 0) {
        return gk_vec_reserve(v, cap);
    }
    return GK_OK;
}

void gk_vec_destroy(gk_vec *v)
{
    if (v == NULL) {
        return;
    }
    gk_free(&v->alloc, v->data);
    v->data = NULL;
    v->len = 0;
    v->cap = 0;
}

gk_status gk_vec_reserve(gk_vec *v, size_t new_cap)
{
    void *next;
    if (v == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (new_cap <= v->cap) {
        return GK_OK;
    }
    if (new_cap > (size_t)-1 / v->elem_size) {
        return GK_ERR_OVERFLOW;
    }
    next = gk_realloc(&v->alloc, v->data, new_cap * v->elem_size);
    if (next == NULL) {
        return GK_ERR_NO_MEMORY;
    }
    v->data = next;
    v->cap = new_cap;
    return GK_OK;
}

gk_status gk_vec_push(gk_vec *v, const void *elem)
{
    gk_status st;
    if (v == NULL || elem == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (v->len == v->cap) {
        size_t next = v->cap == 0 ? 8 : v->cap * 2;
        if (next < v->cap) {
            return GK_ERR_OVERFLOW;
        }
        st = gk_vec_reserve(v, next);
        if (st != GK_OK) {
            return st;
        }
    }
    memcpy((char *)v->data + v->len * v->elem_size, elem, v->elem_size);
    v->len += 1;
    return GK_OK;
}

void *gk_vec_at(gk_vec *v, size_t index)
{
    if (v == NULL || index >= v->len) {
        return NULL;
    }
    return (char *)v->data + index * v->elem_size;
}

const void *gk_vec_at_const(const gk_vec *v, size_t index)
{
    if (v == NULL || index >= v->len) {
        return NULL;
    }
    return (const char *)v->data + index * v->elem_size;
}

size_t gk_vec_size(const gk_vec *v)
{
    return v != NULL ? v->len : 0;
}

void gk_vec_clear(gk_vec *v)
{
    if (v != NULL) {
        v->len = 0;
    }
}
