#include <LibFK/Container/static_vector.h>
#include <LibFK/Memory/own_ptr.h>
#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <Kernel/Posix/unistd.h>
#include <Kernel/Posix/errno.h>

int fd_allocate(VFSNode* node) {
    if (fdtable->size() >= MAX_FD) {
        errno = EMFILE;
        return -1;
    }
    node->retain();
    fdtable->push_back(FDEntry(node));
    return fdtable->size() - 1;
}

void fd_release(int fd) {
    if (fd < 0 || fd >= (int)fdtable->size()) return;
    (*fdtable)[fd].node->release();
    fdtable->erase(fd);
}

void fdtable_init() {
    fdtable = new static_vector<FDEntry, MAX_FD>();

    klog("FD", "File descriptor table initialized");
}


/* open a file and allocate a file descriptor */
int open(const char* pathname, int flags, int mode) {
    (void)flags;
    (void)mode;

    if (!fdtable) fdtable_init();

    VFSNode *node = VFS::resolve_path(pathname);
    if (!node) {
        errno = ENOENT;
        return -1;
    }

    int fd = fd_allocate(node);
    if (fd < 0) return -1;
    return fd;
}

/* close a file descriptor */
int close(int fd) {
    if (!fdtable) return -1;

    if (fd < 0 || fd >= (int)fdtable->size()) {
        errno = EBADF;
        return -1;
    }

    fd_release(fd);
    return 0;
}

/* read from a file descriptor */
ssize_t read(int fd, void* buf, size_t size) {
    if (!fdtable || fd < 0 || fd >= (int)fdtable->size() || !(*fdtable)[fd].node) {
        errno = EBADF;
        return -1;
    }

    FDEntry& entry = (*fdtable)[fd];
    ssize_t ret = VFS::read(entry.node.ptr(), buf, size, entry.offset);
    if (ret > 0) entry.offset += ret;
    return ret;
}

/* write to a file descriptor */
ssize_t write(int fd, const void* buf, size_t size) {
    if (!fdtable || fd < 0 || fd >= (int)fdtable->size() || !(*fdtable)[fd].node) {
        errno = EBADF;
        return -1;
    }

    FDEntry& entry = (*fdtable)[fd];
    ssize_t ret = VFS::write(entry.node.ptr(), buf, size, entry.offset);
    if (ret > 0) entry.offset += ret;
    return ret;
}
