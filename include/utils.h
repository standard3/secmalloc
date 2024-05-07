#ifndef _UTILS_H
#define _UTILS_H

#include <stdarg.h>

#define LOG_INFO "INFO"
#define LOG_WARNING "WARN"
#define LOG_ERROR "ERROR"

#define DEACTIVATE_LOGGING 100

struct tm *get_current_time(void);

void log_general(const int fd, const char *log_name, const char *format, ...);
int create_log_file(const char *filename);
void init_logging(void);
void close_logging(void);

#endif
