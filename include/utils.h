#ifndef _UTILS_H
#define _UTILS_H

#include <stdarg.h>

#define LOG_TYPE_INFO "INFO"
#define LOG_TYPE_WARN "WARN"
#define LOG_TYPE_ERROR "ERROR"

#define DEACTIVATE_LOGGING 100

#define LOG_GENERAL(log_type, format, ...) log_general(log_fd, log_type, format __VA_OPT__(, ) __VA_ARGS__)
#define LOG_INFO(format, ...) LOG_GENERAL(LOG_TYPE_INFO, format, __VA_ARGS__)
#define LOG_WARN(format, ...) LOG_GENERAL(LOG_TYPE_WARN, format, __VA_ARGS__)
#define LOG_ERROR(format, ...) LOG_GENERAL(LOG_TYPE_ERROR, format, __VA_ARGS__)

struct tm *get_current_time(void);

void log_general(const int fd, const char *log_name, const char *format, ...);
int create_log_file(const char *filename);
void init_logging(void);
void close_logging(void);

#endif
