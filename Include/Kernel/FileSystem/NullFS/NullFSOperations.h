#pragma once

#include "Kernel/FileSystem/VFS/VFSTypes.h"
namespace FileSystem {

extern VNodeOperations nullfs_operations;

int nullfs_open(VNode* vnode, LibC::uint32_t flags);
LibC::ssize_t nullfs_read(VNode* vnode, LibC::uint64_t offset, void* buffer, LibC::size_t size);
LibC::ssize_t nullfs_write(VNode* vnode, LibC::uint64_t offset, void const* buffer, LibC::size_t size);
int nullfs_close(VNode* vnode);
int nullfs_stat(VNode* vnode, FileStat* stat);
int nullfs_readdir(VNode* vnode, LibC::uint64_t index, char* name_out, VNode** out_vnode);
VNode* nullfs_lookup(VNode* vnode, char const* name);
VNode* nullfs_create(VNode* parent, char const* name, VNodeType type);
int nullfs_mkdir(VNode* vnode, char const* name, LibC::uint32_t permissions);
int nullfs_unlink(VNode* vnode, char const* name);
int nullfs_rename(VNode* vnode, char const* oldname, char const* newname);
int nullfs_symlink(VNode* vnode, char const* target, char const* linkname);
LibC::ssize_t nullfs_readlink(VNode* vnode, char* buf, LibC::size_t bufsize);
int nullfs_chmod(VNode* vnode, LibC::uint32_t new_permissions);
int nullfs_chown(VNode* vnode, LibC::uint32_t new_uid, LibC::uint32_t new_gid);

}
