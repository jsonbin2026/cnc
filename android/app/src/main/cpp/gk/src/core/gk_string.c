#include "gk/gk_string.h"

#include <ctype.h>
#include <string.h>

size_t gk_strlcpy(char *dst, const char *src, size_t dst_size)
{
    size_t len;
    if (dst == NULL || src == NULL) {
        return 0;
    }
    len = strlen(src);
    if (dst_size == 0) {
        return len;
    }
    if (len >= dst_size) {
        memcpy(dst, src, dst_size - 1);
        dst[dst_size - 1] = '\0';
    } else {
        memcpy(dst, src, len + 1);
    }
    return len;
}

size_t gk_strlcat(char *dst, const char *src, size_t dst_size)
{
    size_t dlen;
    size_t slen;
    if (dst == NULL || src == NULL || dst_size == 0) {
        return 0;
    }
    dlen = strlen(dst);
    if (dlen >= dst_size) {
        return dst_size + strlen(src);
    }
    slen = strlen(src);
    if (slen >= dst_size - dlen) {
        memcpy(dst + dlen, src, dst_size - dlen - 1);
        dst[dst_size - 1] = '\0';
    } else {
        memcpy(dst + dlen, src, slen + 1);
    }
    return dlen + slen;
}

int gk_strcasecmp(const char *a, const char *b)
{
    if (a == NULL || b == NULL) {
        return a == b ? 0 : (a == NULL ? -1 : 1);
    }
    while (*a != '\0' && *b != '\0') {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) {
            return ca - cb;
        }
        ++a;
        ++b;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

int gk_str_eq(const char *a, const char *b)
{
    if (a == NULL || b == NULL) {
        return a == b;
    }
    return strcmp(a, b) == 0;
}

char *gk_str_trim_inplace(char *s)
{
    char *end;
    if (s == NULL) {
        return NULL;
    }
    while (*s != '\0' && isspace((unsigned char)*s)) {
        ++s;
    }
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) {
        --end;
    }
    *end = '\0';
    return s;
}

int gk_str_has_prefix(const char *s, const char *prefix)
{
    size_t n;
    if (s == NULL || prefix == NULL) {
        return 0;
    }
    n = strlen(prefix);
    return strncmp(s, prefix, n) == 0;
}

int gk_str_has_suffix(const char *s, const char *suffix)
{
    size_t ls;
    size_t lx;
    if (s == NULL || suffix == NULL) {
        return 0;
    }
    ls = strlen(s);
    lx = strlen(suffix);
    if (lx > ls) {
        return 0;
    }
    return strcmp(s + (ls - lx), suffix) == 0;
}
