#ifndef _UTILS_H
#define _UTILS_H

#include <stdarg.h>

struct tm *get_current_time();

void log_general(const int fd, const char *log_name, const char *format, ...);

#endif
