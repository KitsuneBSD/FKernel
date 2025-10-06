#include <Kernel/Posix/unistd.h>
#include <Kernel/FileSystem/FileDescriptor.h> // assume que o file_desc.h já está pronto
#include <LibC/stdint.h>
#include <LibC/stdio.h> // para mock prints

// TODO: Replace with proper kernel process/thread context
static struct file_desc kernel_fd_table;

/**
 * @brief Mock _exit syscall
 */
void _exit(int status)
{
    // In a real kernel, this would terminate the current process/thread
    // For mock purposes, just print a message
    kprintf("[mock _exit] Process exited with status %d\n", status);
    while (1)
    {
    } // halt
}

/**
 * @brief Mock write syscall
 */
ssize_t write(int fd, const void *buf, size_t count)
{
    (void)buf;
    struct file *f = filedesc_get(&kernel_fd_table, fd);
    if (!f)
    {
        return -1; // invalid fd
    }

    // For STDOUT/STDERR, just print to console
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO)
    {
        const char *cbuf = (const char *)buf;
        for (size_t i = 0; i < count; i++)
            kprintf("%s\n", cbuf[i]);
        return count;
    }

    // TODO: Implement real file writing
    return -1; // mock does nothing for other fds
}

/**
 * @brief Mock read syscall
 */
ssize_t read(int fd, void *buf, size_t count)
{
    (void)count;
    (void)buf;
    struct file *f = filedesc_get(&kernel_fd_table, fd);
    if (!f)
    {
        return -1; // invalid fd
    }

    // For STDIN, just return 0 for now (mock)
    if (fd == STDIN_FILENO)
    {
        return 0;
    }

    // TODO: Implement real file reading
    return -1; // mock does nothing for other fds
}

/**
 * @brief Mock close syscall
 */
int close(int fd)
{
    struct file *f = filedesc_get(&kernel_fd_table, fd);
    if (!f)
        return -1;
    filedesc_close(&kernel_fd_table, fd);
    return 0;
}

/**
 * @brief Mock open syscall
 */
int open(const char *pathname, int flags)
{
    (void)pathname;
    // TODO: Actually open files in the kernel FS
    struct file *f = new struct file();
    file_ref(f);
    int fd = filedesc_alloc(&kernel_fd_table, f, flags);
    if (fd < 0)
    {
        delete f;
        return -1;
    }
    return fd;
}

/**
 * @brief Mock dup syscall
 */
int dup(int oldfd)
{
    return filedesc_dup(&kernel_fd_table, oldfd);
}

/**
 * @brief Mock dup2 syscall
 */
int dup2(int oldfd, int newfd)
{
    struct file *f = filedesc_get(&kernel_fd_table, oldfd);
    if (!f)
        return -1;

    if (newfd < 0 || newfd >= MAX_FD)
        return -1;

    filedesc_close(&kernel_fd_table, newfd);
    file_ref(f);
    kernel_fd_table.fd_files[newfd].file = f;
    kernel_fd_table.fd_files[newfd].flags = 0;
    kernel_fd_table.fd_count++;
    return newfd;
}

/**
 * @brief Initialize the mock kernel fd table
 */
void init_mock_fd_table()
{
    filedesc_init(&kernel_fd_table);

    // Setup standard fds (mock)
    struct file *stdin_file = new struct file();
    file_ref(stdin_file);
    kernel_fd_table.fd_files[STDIN_FILENO].file = stdin_file;

    struct file *stdout_file = new struct file();
    file_ref(stdout_file);
    kernel_fd_table.fd_files[STDOUT_FILENO].file = stdout_file;

    struct file *stderr_file = new struct file();
    file_ref(stderr_file);
    kernel_fd_table.fd_files[STDERR_FILENO].file = stderr_file;

    kernel_fd_table.fd_count = 3;
}
