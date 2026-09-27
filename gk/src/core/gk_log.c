#include "gk/gk_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static gk_log_level g_level = GK_LOG_INFO;

static const char *const k_level_names[GK_LOG_OFF + 1] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "OFF",
};

void gk_log_set_level(gk_log_level level)
{
    if ((int)level < (int)GK_LOG_TRACE || (int)level > (int)GK_LOG_OFF) {
        return;
    }
    g_level = level;
}

gk_log_level gk_log_get_level(void)
{
    return g_level;
}

const char *gk_log_level_name(gk_log_level level)
{
    if ((int)level < 0 || (int)level > (int)GK_LOG_OFF) {
        return "UNKNOWN";
    }
    return k_level_names[(int)level];
}

int gk_log_level_from_name(const char *name, gk_log_level *out)
{
    int i;
    if (name == NULL || out == NULL) {
        return 0;
    }
    for (i = 0; i <= (int)GK_LOG_OFF; ++i) {
        if (strcmp(name, k_level_names[i]) == 0) {
            *out = (gk_log_level)i;
            return 1;
        }
    }
    return 0;
}

void gk_log_write(gk_log_level level, const char *file, int line,
                  const char *fmt, ...)
{
    va_list args;
    if ((int)level < (int)g_level) {
        return;
    }
    fprintf(stderr, "[%s] %s:%d: ", gk_log_level_name(level),
            file != NULL ? file : "?", line);
    va_start(args, fmt);
    vfprintf(stderr, fmt != NULL ? fmt : "", args);
    va_end(args);
    fputc('\n', stderr);
}
