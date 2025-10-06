#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/Algorithms/log.h>
#include <Kernel/Posix/errno.h>
#include <Kernel/FileSystem/VirtualFS/vfs.h>

#pragma once

#include <LibFK/Memory/own_ptr.h>
#include <Kernel/FileSystem/VirtualFS/vfs_node.h>
#include <LibFK/Container/static_vector.h>

constexpr size_t MAX_FD = 65536;

struct FDEntry
{
    VFSNode *node{nullptr}; // não possui, apenas referencia
    size_t offset{0};

    FDEntry() = default;

    explicit FDEntry(VFSNode *n) : node(n)
    {
        if (node)
            node->retain(); // incrementa refcount
    }

    // Movimentação segura
    FDEntry(FDEntry &&other) noexcept
        : node(other.node), offset(other.offset)
    {
        other.node = nullptr;
    }

    FDEntry &operator=(FDEntry &&other) noexcept
    {
        if (this != &other)
        {
            if (node)
                node->release();
            node = other.node;
            offset = other.offset;
            other.node = nullptr;
        }
        return *this;
    }

    FDEntry(const FDEntry &) = delete;
    FDEntry &operator=(const FDEntry &) = delete;

    bool valid() const { return node != nullptr; }
};

class FDTable
{

private:
    FDTable() = default;

    static_vector<FDEntry, MAX_FD> m_entries;

    // Proibido copiar/mover
    FDTable(const FDTable &) = delete;
    FDTable &operator=(const FDTable &) = delete;
    FDTable(FDTable &&) = delete;
    FDTable &operator=(FDTable &&) = delete;

public:
    static FDTable &the()
    {
        static FDTable inst;
        return inst;
    }

    void init();

    int allocate(VFSNode *node);

    void release(int fd);

    FDEntry *get(int fd);
};

/* access() flags */
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

/* lseek whence */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* basic types */
typedef int pid_t;
typedef int uid_t;
typedef int gid_t;
typedef long off_t;
typedef unsigned int mode_t;

/* file operations */
int open(const char *pathname, int flags, int mode);
int close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
off_t lseek(int fd, off_t offset, int whence);
int unlink(const char *pathname);
int mkdir(const char *pathname, mode_t mode);
