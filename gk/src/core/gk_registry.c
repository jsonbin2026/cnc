#include "gk/gk_registry.h"

#include <string.h>

gk_status gk_registry_init(gk_registry *r, const gk_allocator *alloc)
{
    if (r == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    r->entries = NULL;
    r->len = 0;
    r->cap = 0;
    r->alloc = alloc != NULL ? *alloc : gk_allocator_default();
    return GK_OK;
}

void gk_registry_destroy(gk_registry *r)
{
    if (r == NULL) {
        return;
    }
    gk_free(&r->alloc, r->entries);
    r->entries = NULL;
    r->len = 0;
    r->cap = 0;
}

static size_t find_index(const gk_registry *r, const char *key)
{
    size_t i;
    for (i = 0; i < r->len; ++i) {
        if (r->entries[i].key != NULL && strcmp(r->entries[i].key, key) == 0) {
            return i;
        }
    }
    return (size_t)-1;
}

gk_status gk_registry_set(gk_registry *r, const char *key, void *value)
{
    size_t idx;
    if (r == NULL || key == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    idx = find_index(r, key);
    if (idx != (size_t)-1) {
        r->entries[idx].value = value;
        return GK_OK;
    }
    if (r->len == r->cap) {
        size_t next = r->cap == 0 ? 8 : r->cap * 2;
        void *mem = gk_realloc(&r->alloc, r->entries, next * sizeof(*r->entries));
        if (mem == NULL) {
            return GK_ERR_NO_MEMORY;
        }
        r->entries = mem;
        r->cap = next;
    }
    r->entries[r->len].key = key;
    r->entries[r->len].value = value;
    r->len += 1;
    return GK_OK;
}

void *gk_registry_get(const gk_registry *r, const char *key)
{
    size_t idx;
    if (r == NULL || key == NULL) {
        return NULL;
    }
    idx = find_index(r, key);
    if (idx == (size_t)-1) {
        return NULL;
    }
    return r->entries[idx].value;
}

gk_status gk_registry_remove(gk_registry *r, const char *key)
{
    size_t idx;
    if (r == NULL || key == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    idx = find_index(r, key);
    if (idx == (size_t)-1) {
        return GK_ERR_NOT_FOUND;
    }
    memmove(&r->entries[idx], &r->entries[idx + 1],
            (r->len - idx - 1) * sizeof(*r->entries));
    r->len -= 1;
    return GK_OK;
}

size_t gk_registry_size(const gk_registry *r)
{
    return r != NULL ? r->len : 0;
}
