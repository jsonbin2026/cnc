#ifndef GK_MEM_H
#define GK_MEM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*gk_alloc_fn)(size_t size, void *user);
typedef void *(*gk_realloc_fn)(void *ptr, size_t size, void *user);
typedef void (*gk_free_fn)(void *ptr, void *user);

typedef struct {
    gk_alloc_fn alloc;
    gk_realloc_fn realloc;
    gk_free_fn free;
    void *user;
} gk_allocator;

gk_allocator gk_allocator_default(void);

void *gk_alloc(const gk_allocator *a, size_t size);
void *gk_calloc(const gk_allocator *a, size_t count, size_t size);
void *gk_realloc(const gk_allocator *a, void *ptr, size_t size);
void gk_free(const gk_allocator *a, void *ptr);

#ifdef __cplusplus
}
#endif

#endif
