#ifndef GK_STRING_H
#define GK_STRING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t gk_strlcpy(char *dst, const char *src, size_t dst_size);
size_t gk_strlcat(char *dst, const char *src, size_t dst_size);
int gk_strcasecmp(const char *a, const char *b);
int gk_str_eq(const char *a, const char *b);
char *gk_str_trim_inplace(char *s);
int gk_str_has_prefix(const char *s, const char *prefix);
int gk_str_has_suffix(const char *s, const char *suffix);

#ifdef __cplusplus
}
#endif

#endif
