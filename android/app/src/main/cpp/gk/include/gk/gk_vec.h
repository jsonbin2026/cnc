#ifndef GK_VEC_H
#define GK_VEC_H

#include <stddef.h>
#include "gk/gk_error.h"
#include "gk/gk_mem.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *data;
    size_t elem_size;
    size_t len;
    size_t cap;
    gk_allocator alloc;
} gk_vec;

gk_status gk_vec_init(gk_vec *v, size_t elem_size, size_t cap,
                      const gk_allocator *alloc);
void gk_vec_destroy(gk_vec *v);
gk_status gk_vec_reserve(gk_vec *v, size_t new_cap);
gk_status gk_vec_push(gk_vec *v, const void *elem);
void *gk_vec_at(gk_vec *v, size_t index);
const void *gk_vec_at_const(const gk_vec *v, size_t index);
size_t gk_vec_size(const gk_vec *v);
void gk_vec_clear(gk_vec *v);

#ifdef __cplusplus
}
#endif

#endif
