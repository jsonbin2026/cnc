#include "gk/gk_error.h"

static const char *const k_names[GK_ERR_COUNT] = {
    "GK_OK",
    "GK_ERR_INVALID_ARG",
    "GK_ERR_OUT_OF_RANGE",
    "GK_ERR_NO_MEMORY",
    "GK_ERR_NOT_FOUND",
    "GK_ERR_ALREADY_EXISTS",
    "GK_ERR_OVERFLOW",
    "GK_ERR_IO",
    "GK_ERR_PARSE",
    "GK_ERR_STATE",
    "GK_ERR_UNSUPPORTED",
};

static const char *const k_messages[GK_ERR_COUNT] = {
    "success",
    "invalid argument",
    "value out of range",
    "out of memory",
    "item not found",
    "item already exists",
    "arithmetic overflow",
    "input/output failure",
    "parse failure",
    "invalid state transition",
    "operation not supported",
};

static int gk_status_valid(gk_status status)
{
    return (int)status >= 0 && (int)status < (int)GK_ERR_COUNT;
}

const char *gk_status_name(gk_status status)
{
    if (!gk_status_valid(status)) {
        return "GK_ERR_UNKNOWN";
    }
    return k_names[(int)status];
}

const char *gk_status_message(gk_status status)
{
    if (!gk_status_valid(status)) {
        return "unknown error";
    }
    return k_messages[(int)status];
}

int gk_status_is_error(gk_status status)
{
    return status != GK_OK;
}
