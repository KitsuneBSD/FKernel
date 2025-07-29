#include <Kernel/FileSystem/NullFS/NullFSOperations.h>

namespace FileSystem {

VNodeOperations nullfs_operations = {
    .open = nullfs_open,
    .read = nullfs_read,
    .write = nullfs_write,
    .close = nullfs_close,
    .stat = nullfs_stat,
    .readdir = nullfs_readdir,
    .lookup = nullfs_lookup,
    .mkdir = nullfs_mkdir,
    .unlink = nullfs_unlink,
    .create = nullfs_create,
    .rename = nullfs_rename,
    .symlink = nullfs_symlink,
    .readlink = nullfs_readlink,
    .chmod = nullfs_chmod,
    .chown = nullfs_chown,
};

}
