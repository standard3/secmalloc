#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <alloca.h>
#include <string.h>
#include <fcntl.h>

#include "utils.h"

int log_fd = -1; // Default log file descriptor

/** Usage example
 * init_logging();
 * log_general(log_fd, LOG_INFO, "Hello, %s", "world");
 */

/**
 * @brief Get the current time
 *
 * @return pointer to the current time
 */
struct tm *get_current_time()
{
    time_t raw_time;
    struct tm *timeinfo;
    time(&raw_time);
    timeinfo = localtime(&raw_time);

    return timeinfo;
}

/**
 * @brief Generic logging function that puts the log message in
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

    // Deactivated time logging because of mystery infinite loop, will fix later
    // const struct tm *timeinfo = get_current_time();
    const pid_t pid = getpid();

    // char time_buffer[20] = {0};
    // char prefix[34] = {0};
    char prefix[15] = {0};

    // Buffer for our time string
    // strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    // Buffer for our header
    // snprintf(prefix, sizeof(prefix), "%s %d [%s] ", time_buffer, pid, log_name);
    snprintf(prefix, sizeof(prefix), "%d [%s] ", pid, log_name);

    // Calculate the required size for the log message
    va_list args_copy;
    va_start(args_copy, format);
    int len = vsnprintf(NULL, 0, format, args_copy) + 1; // +1 for null terminator
    va_end(args_copy);

    // Allocate memory for the log message
    char *buffer = alloca(len);

    // Format the log message with header
    va_start(args_copy, format);
    vsnprintf(buffer, len, format, args_copy); // -1 to leave space for the null terminator
    va_end(args_copy);

    // Concatenate log prefix and message
    char log_message[len + sizeof(prefix)];
    snprintf(log_message, sizeof(log_message), "%s%s\n", prefix, buffer);

    // Write to file descriptor
    write(fd, log_message, sizeof(log_message));
}

/**
 * @brief Create a log file with the specified path
 * If the file already exists, it will be overwritten.
 * The file descriptor is returned.
 * If the file cannot be created, -1 is returned.
 *
 * @param path pointer to the path of the log file
 * @return file descriptor
 */
int create_log_file(const char *path)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
        return -1;

    return fd;
}

/**
 * @brief Initialize the log file.
 * Handles the presence of the MSM_OUTPUT environment variable
 * and creates the log file accordingly.
 *
 */
void init_logging()
{
    // Get the path from the environment variable
    const char *path = getenv("MSM_OUTPUT");
    if (path == NULL)
    {
        log_fd = DEACTIVATE_LOGGING;
        return;
    }

    // If stdout, set the file descriptor to stdout
    if (strcmp(path, "stdout") == 0)
    {
        log_fd = STDOUT_FILENO;
        return;
    }

    // If already opened, return
    if (log_fd != -1)
        return;

    log_fd = create_log_file(path);
    if (log_fd == -1)
    {
        log_fd = STDERR_FILENO;
        LOG_ERROR("Failed to create log file");
        return;
    }
}

/**
 * @brief Close the log file.
 *
 */
void close_logging()
{
    if (log_fd == DEACTIVATE_LOGGING)
        return;

    LOG_INFO("Closing log file");

    close(log_fd);
}
