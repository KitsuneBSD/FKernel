#pragma once

#include <Kernel/FileSystem/VFS/VFSTypes.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>

namespace FileSystem {

extern VNodeOperations devfs_operations;

int devfs_open(VNode* vnode, LibC::uint32_t flags);
LibC::ssize_t devfs_read(VNode* vnode, LibC::uint64_t offset, void* buffer, LibC::size_t size);
LibC::ssize_t devfs_write(VNode* vnode, LibC::uint64_t offset, void const* buffer, LibC::size_t size);
int devfs_close(VNode* vnode);
int devfs_stat(VNode* vnode, FileStat* stat);
int devfs_readdir(VNode* vnode, LibC::uint64_t index, char* name_out, VNode** out_vnode);
VNode* devfs_lookup(VNode* vnode, char const* name);
int devfs_mkdir(VNode* vnode, char const* name, LibC::uint32_t permissions);
int devfs_unlink(VNode* vnode, char const* name);
VNode* devfs_create(VNode* parent, char const* name, VNodeType type);
int devfs_rename(VNode* vnode, char const* oldname, char const* newname);
int devfs_symlink(VNode* vnode, char const* target, char const* linkname);
LibC::ssize_t devfs_readlink(VNode* vnode, char* buf, LibC::size_t bufsize);
int devfs_chmod(VNode* vnode, LibC::uint32_t new_permissions);
int devfs_chown(VNode* vnode, LibC::uint32_t new_uid, LibC::uint32_t new_gid);

}
