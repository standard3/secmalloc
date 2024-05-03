#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <alloca.h>
#include <string.h>

#include "utils.h"

#define LOG_INFO "INFO"
#define LOG_WARNING "WARN"
#define LOG_ERROR "ERROR"

/**
 * Get the current time
 *
 * @return pointer to the current time
 */
struct tm *
get_current_time()
{
    time_t raw_time;
    struct tm *timeinfo;
    time(&raw_time);
    timeinfo = localtime(&raw_time);

    return timeinfo;
}

/**
 * Generic logging function that puts the log message in
 * the specified file descriptor
 *
 * @param fd file descriptor identifier
 * @param name pointer to the name of the log message
 * @param format pointer to the format string
 * @return
 */
void log_general(const int fd, const char *log_name, const char *format, ...)
{
    const struct tm *timeinfo = get_current_time();
    const pid_t pid = getpid();

    char time_buffer[20];
    char prefix[34];

    // Buffer for our time string
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    // Buffer for our header
    snprintf(prefix, sizeof(prefix), "%s %d [%s] ", time_buffer, pid, log_name);

    // Calculate the required size for the log message
    va_list args_copy;
    va_start(args_copy, format);
    int len = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    // Allocate memory for the log message
    char *buffer = alloca(len + 1); // +1 for null terminator and +1 for newline

    // Format the log message with header
    va_start(args_copy, format);
    vsnprintf(buffer, len + 1, format, args_copy); // -1 to leave space for the null terminator
    va_end(args_copy);

    // Concatenate log prefix and message
    char log_message[len + sizeof(prefix) + 1]; // +1 for null terminator
    snprintf(log_message, sizeof(log_message), "%s%s\n", prefix, buffer);

    // Write to file descriptor
    write(fd, log_message, sizeof(log_message));
}
