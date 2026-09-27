#ifndef GK_ERROR_H
#define GK_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GK_OK = 0,
    GK_ERR_INVALID_ARG,
    GK_ERR_OUT_OF_RANGE,
    GK_ERR_NO_MEMORY,
    GK_ERR_NOT_FOUND,
    GK_ERR_ALREADY_EXISTS,
    GK_ERR_OVERFLOW,
    GK_ERR_IO,
    GK_ERR_PARSE,
    GK_ERR_STATE,
    GK_ERR_UNSUPPORTED,
    GK_ERR_COUNT
} gk_status;

const char *gk_status_name(gk_status status);
const char *gk_status_message(gk_status status);
int gk_status_is_error(gk_status status);

#ifdef __cplusplus
}
#endif

#endif
