#pragma once

#include <Kernel/FileSystem/VFS/VFSTypes.h>

namespace FileSystem {

extern VNodeOperations ramfs_operations;

int ramfs_open(VNode* vnode, LibC::uint32_t flags);
LibC::ssize_t ramfs_read(VNode* vnode, LibC::uint64_t offset, void* buffer, LibC::size_t size);
LibC::ssize_t ramfs_write(VNode* vnode, LibC::uint64_t offset, void const* buffer, LibC::size_t size);
int ramfs_close(VNode* vnode);
int ramfs_stat(VNode* vnode, FileStat* stat);
int ramfs_readdir(VNode* vnode, LibC::uint64_t index, char* name_out, VNode** out_vnode);
VNode* ramfs_lookup(VNode* vnode, char const* name);
VNode* ramfs_create(VNode* parent, char const* name, VNodeType type);
int ramfs_mkdir(VNode* vnode, char const* name, LibC::uint32_t permissions);
int ramfs_unlink(VNode* vnode, char const* name);
int ramfs_rename(VNode* vnode, char const* oldname, char const* newname);
int ramfs_symlink(VNode* vnode, char const* target, char const* linkname);
LibC::ssize_t ramfs_readlink(VNode* vnode, char* buf, LibC::size_t bufsize);
int ramfs_chmod(VNode* vnode, LibC::uint32_t new_permissions);
int ramfs_chown(VNode* vnode, LibC::uint32_t new_uid, LibC::uint32_t new_gid);
}
