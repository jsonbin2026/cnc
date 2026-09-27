#ifndef GK_REGISTRY_H
#define GK_REGISTRY_H

#include <stddef.h>
#include "gk/gk_error.h"
#include "gk/gk_mem.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *key;
    void *value;
} gk_registry_entry;

typedef struct {
    gk_registry_entry *entries;
    size_t len;
    size_t cap;
    gk_allocator alloc;
} gk_registry;

gk_status gk_registry_init(gk_registry *r, const gk_allocator *alloc);
void gk_registry_destroy(gk_registry *r);
gk_status gk_registry_set(gk_registry *r, const char *key, void *value);
void *gk_registry_get(const gk_registry *r, const char *key);
gk_status gk_registry_remove(gk_registry *r, const char *key);
size_t gk_registry_size(const gk_registry *r);

#ifdef __cplusplus
}
#endif

#endif
