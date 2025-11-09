#include <Kernel/FileSystem/file_descriptor.h>
#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <LibC/stdio.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>

RetainPtr<VNode> VirtualFS::root() const { return v_root; }

int VirtualFS::mount(const char *name, RetainPtr<VNode> root) {
  if (v_mounts.is_full()) {
    kwarn("VFS", "Failed to mount '%s': mount table full", name);
    return -1;
  }

  Mountpoint m(name, root);

  if (!v_root) {
    v_root = root;  // primeiro mount se torna '/'
    v_cwd = v_root; // inicializa cwd
    klog("VFS", "Root mounted at '/'");
  } else {
    v_root->dir_entries.push_back(DirEntry{name, root});
    klog("VFS", "Mounted '%s' at '/%s' successfully", name, name);
  }

  v_mounts.push_back(m);
  klog("VFS", "Mounted '%s' successfully", name);
  return 0;
}

RetainPtr<VNode> VirtualFS::resolve_path(const char *path,
                                         RetainPtr<VNode> cwd) {
  if (!path || !*path || !v_root) {
    kwarn("VFS", "Resolve path failed: empty path or root not mounted");
    return RetainPtr<VNode>();
  }

  RetainPtr<VNode> current = (path[0] == '/') ? v_root : (cwd ? cwd : v_cwd);

  kdebug("VFS", "Resolving path: '%s'", path);

  const char *p = path;
  while (*p) {
    while (*p == '/')
      ++p;
    if (!*p)
      break;

    const char *end = p;
    while (*end && *end != '/')
      ++end;
    size_t len = end - p;
    if (len == 0)
      break;

    fixed_string<256> token;
    if (len >= token.capacity()) {
      kwarn("VFS", "Path component too long: '%.*s'", int(len), p);
      return RetainPtr<VNode>();
    }

    token.assign(p, len);

    if (strcmp(token.c_str(), ".") == 0) {
      // nada
    } else if (strcmp(token.c_str(), "..") == 0) {
      if (current->parent)
        current = current->parent;
    } else {
      RetainPtr<VNode> next;
      if (current->lookup(token.c_str(), next) != 0) {
        kdebug("VFS", "Component '%s' not found in '%s'", token.c_str(),
               current->m_name.c_str());
        return RetainPtr<VNode>();
      }
      current = next;
    }

    p = end;
  }

  kdebug("VFS", "Resolved path successfully: '%s'", path);
  return current;
}

RetainPtr<VNode> VirtualFS::cwd() const { return v_cwd; }

void VirtualFS::set_cwd(RetainPtr<VNode> vnode) {
  if (vnode)
    v_cwd = vnode;
}

int VirtualFS::lookup(const char *path, RetainPtr<VNode> &out) {
  RetainPtr<VNode> vnode = resolve_path(path, v_cwd);
  if (!vnode) {
    kwarn("VFS", "Lookup failed: path '%s' not found", path);
    return -1;
  }
  out = vnode;
  klog("VFS", "Lookup: path '%s' found", path);
  return 0;
}

int VirtualFS::open(const char *path, int flags) {
  RetainPtr<VNode> vnode = resolve_path(path, v_cwd);
  if (!vnode) {
    kwarn("VFS", "Open failed: path '%s' not found", path);
    return -1;
  }

  int fd = FileDescriptorTable::the().allocate(vnode, flags);
  if (fd < 0) {
    kwarn("VFS", "Open failed: could not allocate file descriptor for path '%s'", path);
    return -1;
  }

  FileDescriptor *file_descriptor_ptr = FileDescriptorTable::the().get(fd);
  if (!file_descriptor_ptr) {
    kwarn("VFS", "Open failed: could not retrieve file descriptor pointer for path '%s'", path);
    FileDescriptorTable::the().close(fd);
    return -1;
  }

  int ret = vnode->open(file_descriptor_ptr, flags);
  if (ret < 0) {
    FileDescriptorTable::the().close(fd); // Close the allocated FD on VNode open failure
    return ret;
  }
  return fd;
}

int VirtualFS::read(int file_descriptor, void *buf, size_t sz, size_t off) {
  FileDescriptor *f = FileDescriptorTable::the().get(file_descriptor);
  if (!f || !f->vnode) {
    kdebug("VFS", "Read failed: invalid file descriptor %d", file_descriptor);
    return -1;
  }

  int ret = f->vnode->read(f, buf, sz, off);
  kdebug("VNode", "Read %d bytes from '%s' (offset=%zu)", ret,
         f->vnode->m_name.c_str(), off);
  return ret;
}

int VirtualFS::write(int file_descriptor, const void *buf, size_t sz, size_t off) {
  FileDescriptor *f = FileDescriptorTable::the().get(file_descriptor);
  if (!f || !f->vnode) {
    kdebug("VFS", "Write failed: invalid file descriptor %d", file_descriptor);
    return -1;
  }

  int ret = f->vnode->write(f, buf, sz, off);
  kdebug("VNode", "Wrote %d bytes to '%s' (offset=%zu)", ret,
         f->vnode->m_name.c_str(), off);
  return ret;
}
