#ifndef GK_LOG_H
#define GK_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GK_LOG_TRACE = 0,
    GK_LOG_DEBUG,
    GK_LOG_INFO,
    GK_LOG_WARN,
    GK_LOG_ERROR,
    GK_LOG_OFF
} gk_log_level;

void gk_log_set_level(gk_log_level level);
gk_log_level gk_log_get_level(void);
const char *gk_log_level_name(gk_log_level level);
int gk_log_level_from_name(const char *name, gk_log_level *out);

void gk_log_write(gk_log_level level, const char *file, int line,
                  const char *fmt, ...);

#define GK_LOG_TRACE(...) gk_log_write(GK_LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define GK_LOG_DEBUG(...) gk_log_write(GK_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define GK_LOG_INFO(...)  gk_log_write(GK_LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define GK_LOG_WARN(...)  gk_log_write(GK_LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define GK_LOG_ERROR(...) gk_log_write(GK_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
