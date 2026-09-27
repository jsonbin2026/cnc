#include "gk/gk_mem.h"

#include <stdlib.h>
#include <string.h>

static void *default_alloc(size_t size, void *user)
{
    (void)user;
    return malloc(size);
}

static void *default_realloc(void *ptr, size_t size, void *user)
{
    (void)user;
    return realloc(ptr, size);
}

static void default_free(void *ptr, void *user)
{
    (void)user;
    free(ptr);
}

gk_allocator gk_allocator_default(void)
{
    gk_allocator a;
    a.alloc = default_alloc;
    a.realloc = default_realloc;
    a.free = default_free;
    a.user = NULL;
    return a;
}

void *gk_alloc(const gk_allocator *a, size_t size)
{
    if (a == NULL || a->alloc == NULL || size == 0) {
        return NULL;
    }
    return a->alloc(size, a->user);
}

void *gk_calloc(const gk_allocator *a, size_t count, size_t size)
{
    void *ptr;
    if (count != 0 && size > (size_t)-1 / count) {
        return NULL;
    }
    ptr = gk_alloc(a, count * size);
    if (ptr != NULL) {
        memset(ptr, 0, count * size);
    }
    return ptr;
}

void *gk_realloc(const gk_allocator *a, void *ptr, size_t size)
{
    if (a == NULL || a->realloc == NULL) {
        return NULL;
    }
    return a->realloc(ptr, size, a->user);
}

void gk_free(const gk_allocator *a, void *ptr)
{
    if (a == NULL || a->free == NULL || ptr == NULL) {
        return;
    }
    a->free(ptr, a->user);
}
