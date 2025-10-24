#pragma once

#include <LibFK/Memory/retain_ptr.h>

// Forward declare VNode
struct VNode;

#include <LibFK/Container/static_vector.h>

#include <LibC/stdint.h>

struct FileDescriptor {
    int file_descriptor{-1};
    RetainPtr<VNode> vnode;
    int flags{0};
    uint64_t offset{0};
    bool used{false};
};

class FileDescriptorTable {
public:
    static FileDescriptorTable &the();

    int allocate(RetainPtr<VNode> vnode, int flags);
    FileDescriptor *get(int file_descriptor);
    int close(int file_descriptor);

private:
    FileDescriptorTable();
    static const size_t MAX_file_descriptorS = 256;
    static_vector<FileDescriptor, MAX_file_descriptorS> m_file_descriptors;
};

// High-level helpers that bind VFS operations to file descriptors
int file_descriptor_open_path(const char *path, int flags);
int file_descriptor_read(int file_descriptor, void *buf, size_t sz);
int file_descriptor_write(int file_descriptor, const void *buf, size_t sz);
int file_descriptor_close(int file_descriptor);