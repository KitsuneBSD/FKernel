#pragma once

#include <LibC/stdint.h>
#include <LibC/stddef.h>

#define NO_FD (-1)
#define MAX_FD (2 << 16)

// TODO: Add synchronization (spinlocks or mutexes) for multi-threaded safety.
// TODO: Integrate with VFS layer when File and inode abstractions are available.
// TODO: Implement close-on-exec flag handling (FD_CLOEXEC).
// TODO: Expand MAX_FD dynamically when kernel memory allocators are ready.
// TODO: Handle per-thread vs. per-process descriptor sharing for fork/exec semantics.
// TODO: Add debug utilities for dumping descriptor tables.

/**
 * @brief Represents a kernel-level file object.
 *
 * This is a placeholder structure until the VFS layer is implemented.
 * Each file keeps a simple reference counter for basic resource management.
 */
struct file
{
    int refcount{0};
    int dummy; ///< Placeholder field, to be replaced by real VFS data.
};

/**
 * @brief Single file descriptor entry within a descriptor table.
 *
 * Each entry holds a pointer to a file and a flag field (e.g., FD_CLOEXEC).
 */
struct file_desc_entry
{
    struct file *file;
    int flags;
};

/**
 * @brief Represents a process-level file descriptor table.
 *
 * This structure stores references to open files, indexed by descriptor number.
 * It is meant to be attached to a process structure and manipulated by syscalls.
 */
struct file_desc
{
    struct file_desc_entry fd_files[MAX_FD];
    int fd_count;
};

/**
 * @brief Increment the reference count of a file.
 *
 * @param f Pointer to the file structure.
 */
void file_ref(struct file *f);

/**
 * @brief Decrement the reference count of a file and release it if necessary.
 *
 * @param f Pointer to the file structure.
 */
void file_unref(struct file *f);

/**
 * @brief Initialize a file descriptor table.
 *
 * @param fdp Pointer to the file_desc structure to initialize.
 */
void filedesc_init(struct file_desc *fdp);

/**
 * @brief Allocate a new file descriptor and associate it with a file.
 *
 * @param fdp Pointer to the file descriptor table.
 * @param fp Pointer to the file to be associated.
 * @param flags Descriptor flags.
 * @return The newly allocated descriptor index, or NO_FD if full.
 */
int filedesc_alloc(struct file_desc *fdp, struct file *fp, int flags);

/**
 * @brief Retrieve the file pointer associated with a given descriptor.
 *
 * @param fdp Pointer to the file descriptor table.
 * @param fd Descriptor number.
 * @return Pointer to the associated file, or NULL if invalid.
 */
struct file *filedesc_get(struct file_desc *fdp, int fd);

/**
 * @brief Close a file descriptor and release its file reference.
 *
 * @param fdp Pointer to the file descriptor table.
 * @param fd Descriptor number to close.
 */
void filedesc_close(struct file_desc *fdp, int fd);

/**
 * @brief Duplicate an existing file descriptor.
 *
 * @param fdp Pointer to the file descriptor table.
 * @param oldfd Existing descriptor to duplicate.
 * @return The new descriptor number, or NO_FD on error.
 */
int filedesc_dup(struct file_desc *fdp, int oldfd);
