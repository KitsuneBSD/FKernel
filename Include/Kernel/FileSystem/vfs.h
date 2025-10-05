#pragma once 

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/Container/static_vector.h>
#include <LibFK/Container/fixed_string.h>
#include <LibFK/Memory/own_ptr.h>
#include <LibFK/Traits/type_traits.h>

constexpr size_t VFS_MAX_PATH = 4096;

enum class FileMode : uint32_t
{
    Read = 0x1,
    Write = 0x2,
    ReadWrite = Read | Write,
    Append = 0x4,
    Create = 0x8,
    Truncate = 0x10
};

enum class FileType : uint8_t {
    Unknown,
    Regular,
    Directory,
    CharDevice,
    BlockDevice,
    Symlink
};

struct FilePermissions {
    uint16_t user;    ///< Owner permissions (rwx)
    uint16_t group;   ///< Group permissions
    uint16_t others;  ///< Other permissions
};

struct VFSNode;

struct VFSOps {
    virtual ssize_t read(VFSNode *node, void *buf, size_t size, size_t offset) = 0;
    virtual ssize_t write(VFSNode *node, const void *buf, size_t size, size_t offset) = 0;
    virtual int mkdir(VFSNode *node, const char *name, FilePermissions perms) = 0;
    virtual int unlink(VFSNode *node, const char *name) = 0;
    virtual int rename(VFSNode *node, const char *old_name, const char *new_name) = 0;
    virtual int open(VFSNode *node, FileMode mode) = 0;
    virtual int close(VFSNode *node) = 0;
    virtual VFSNode *lookup(VFSNode *node, const char *name) = 0;
    virtual ~VFSOps() = default;
};

struct VFSNode {
    fixed_string<VFS_MAX_PATH> name;
    FileType type{FileType::Unknown};
    FilePermissions perms{};
    size_t size{0};
    VFSOps *ops{nullptr};
    VFSNode *parent{nullptr};

    VFSNode(const char* n, FileType t, FilePermissions p) 
        : name(n), type(t), perms(p) {}

    static_vector<OwnPtr<VFSNode>, 128> children;

    uint32_t refcount{1};

    void retain() { ++refcount; }
    void release() {
        if (--refcount == 0) {
            delete this;
        }
    }
};

struct VFS {
    static VFSNode* root();
    static void init();
    static int mount(VFSNode *node, const char *path);
    static VFSNode *resolve_path(const char *path);
    static int mkdir(const char *path, FilePermissions perms);
    static int unlink(const char *path);
    static int rename(const char *old_path, const char *new_path);
    static ssize_t read(VFSNode *node, void *buf, size_t size, size_t offset);
    static ssize_t write(VFSNode *node, const void *buf, size_t size, size_t offset);
    static int open(VFSNode *node, FileMode mode);
    static int close(VFSNode *node);
};
