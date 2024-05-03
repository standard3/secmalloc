#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <alloca.h>
#include <string.h>

#include "utils.h"

int log_fd = -1; // Default log file descriptor

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
    // Do we need to log ?
    if (log_fd == DEACTIVATE_LOGGING)
        return;

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

/**
 * Create a log file with the specified path
 * If the file already exists, it will be overwritten.
 * The file descriptor is returned.
 * If the file cannot be created, -1 is returned.
 *
 * @param path pointer to the path of the log file
 * @return file descriptor
 */
int create_log_file(const char *path)
{
    FILE *log_file = fopen(path, "w");
    if (log_file == NULL)
        return -1;

    return log_file->_fileno;
}

/**
 * Initialize the log file.
 * Handles the presence of the MSM_OUTPUT environment variable
 * and creates the log file accordingly.
 *
 */
void init_logging()
{
    const char *path = getenv("MSM_OUTPUT");
    if (path == NULL)
    {
        log_fd = DEACTIVATE_LOGGING;
        return;
    }

    log_fd = create_log_file(path);
    if (log_fd == -1)
    {
        log_general(STDERR_FILENO, LOG_ERROR, "Failed to create log file");
        return;
    }
}

/**
 * Close the log file.
 *
 */
void close_logging()
{
    if (log_fd == DEACTIVATE_LOGGING)
        return;

    close(log_fd);
}
