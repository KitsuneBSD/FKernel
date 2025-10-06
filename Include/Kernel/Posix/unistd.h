#pragma once

#include <LibC/stdint.h>
#include <LibC/stddef.h>

// Standard file descriptors
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

// Maximum syscall ID (for validation)
#define SYS_MAX 4096

/**
 * @brief Exit the current process
 * @param status Exit status code
 */
void _exit(int status);

/**
 * @brief Write to a file descriptor
 * @param fd File descriptor
 * @param buf Buffer to write
 * @param count Number of bytes
 * @return Number of bytes written or -1 on error
 */
ssize_t write(int fd, const void *buf, size_t count);

/**
 * @brief Read from a file descriptor
 * @param fd File descriptor
 * @param buf Buffer to store data
 * @param count Number of bytes to read
 * @return Number of bytes read or -1 on error
 */
ssize_t read(int fd, void *buf, size_t count);

/**
 * @brief Close a file descriptor
 * @param fd File descriptor
 * @return 0 on success, -1 on error
 */
int close(int fd);

/**
 * @brief Open a file
 * @param pathname Path to the file
 * @param flags Open flags
 * @return File descriptor or -1 on error
 */
int open(const char *pathname, int flags);

/**
 * @brief Duplicate a file descriptor
 * @param oldfd Old file descriptor
 * @return New file descriptor or -1 on error
 */
int dup(int oldfd);

/**
 * @brief Duplicate a file descriptor to a specific value
 * @param oldfd Old file descriptor
 * @param newfd New file descriptor
 * @return New file descriptor or -1 on error
 */
int dup2(int oldfd, int newfd);