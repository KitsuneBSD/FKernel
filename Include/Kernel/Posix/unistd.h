#pragma once 

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/Algorithms/log.h>
#include <Kernel/Posix/errno.h>
#include <Kernel/FileSystem/vfs.h>

struct FDEntry {
    OwnPtr<VFSNode> node;
    size_t offset = 0;

    FDEntry() = default;
    FDEntry(VFSNode* n) : node(n) {}
};

constexpr size_t MAX_FD = 65536;

static static_vector<FDEntry, MAX_FD>* fdtable = nullptr;

void fdtable_init();
int fd_allocate(VFSNode *node);
void fd_release(int fd);

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
