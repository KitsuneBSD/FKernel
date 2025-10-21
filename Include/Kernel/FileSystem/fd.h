#pragma once

#include <LibFK/Memory/retain_ptr.h>

// Forward declare VNode
struct VNode;

#include <LibFK/Container/static_vector.h>

#include <LibC/stdint.h>

struct FileDescriptor {
    int fd{-1};
    RetainPtr<VNode> vnode;
    int flags{0};
    uint64_t offset{0};
    bool used{false};
};

class FDTable {
public:
    static FDTable &the();

    int allocate(RetainPtr<VNode> vnode, int flags);
    FileDescriptor *get(int fd);
    int close(int fd);

private:
    FDTable();
    static const size_t MAX_FDS = 256;
    static_vector<FileDescriptor, MAX_FDS> m_fds;
};

// High-level helpers that bind VFS operations to file descriptors
int fd_open_path(const char *path, int flags);
int fd_read(int fd, void *buf, size_t sz);
int fd_write(int fd, const void *buf, size_t sz);
int fd_close(int fd);