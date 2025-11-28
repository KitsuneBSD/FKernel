#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <Kernel/FileSystem/file_descriptor.h>
#include <LibC/stdio.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>

fk::memory::RetainPtr<VNode> VirtualFS::root() const { return v_root; }

int VirtualFS::mount(const char *name, fk::memory::RetainPtr<VNode> root, fk::memory::OwnPtr<fkernel::fs::Filesystem> fs_instance /* = nullptr */) {
  if (v_mounts.is_full()) {
    fk::algorithms::kwarn("VFS", "Failed to mount '%s': mount table full",
                          name);
    return -1;
  }

  Mountpoint m(name, root, fk::types::move(fs_instance));

  if (!v_root) {
    v_root = root;  // primeiro mount se torna '/'
    v_cwd = v_root; // inicializa cwd
    fk::algorithms::klog("VFS", "Root mounted at '/'");
  } else {
    v_root->dir_entries.push_back(DirEntry{name, root});
    fk::algorithms::klog("VFS", "Mounted '%s' at '/%s' successfully", name,
                         name);
  }

  v_mounts.push_back(fk::types::move(m));
  fk::algorithms::klog("VFS", "Mounted '%s' successfully", name);
  return 0;
}

fk::memory::RetainPtr<VNode>
VirtualFS::resolve_path(const char *path, fk::memory::RetainPtr<VNode> cwd) {
  if (!path || !*path || !v_root) {
    fk::algorithms::kwarn(
        "VFS", "Resolve path failed: empty path or root not mounted");
    return fk::memory::RetainPtr<VNode>();
  }

  fk::memory::RetainPtr<VNode> current =
      (path[0] == '/') ? v_root : (cwd ? cwd : v_cwd);

  fk::algorithms::kdebug("VFS", "Resolving path: '%s'", path);

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

    fk::text::fixed_string<256> token;
    if (len >= token.capacity()) {
      fk::algorithms::kwarn("VFS", "Path component too long: '%.*s'", int(len),
                            p);
      return fk::memory::RetainPtr<VNode>();
    }

    token.assign(p, len);

    if (strcmp(token.c_str(), ".") == 0) {
      // nada
    } else if (strcmp(token.c_str(), "..") == 0) {
      if (current->parent)
        current = current->parent;
    } else {
      fk::memory::RetainPtr<VNode> next;
      if (current->lookup(token.c_str(), next) != 0) {
        fk::algorithms::kdebug("VFS", "Component '%s' not found in '%s'",
                               token.c_str(), current->m_name.c_str());
        return fk::memory::RetainPtr<VNode>();
      }
      current = next;
    }

    p = end;
  }

  fk::algorithms::kdebug("VFS", "Resolved path successfully: '%s'", path);
  return current;
}

fk::memory::RetainPtr<VNode> VirtualFS::cwd() const { return v_cwd; }

void VirtualFS::set_cwd(fk::memory::RetainPtr<VNode> vnode) {
  if (vnode) {
    v_cwd = vnode;
    fk::algorithms::klog("VFS", "Changed current working directory to '%s'", vnode->m_name.c_str());
  } else {
    fk::algorithms::kwarn("VFS", "Attempted to set_cwd to a null vnode.");
  }
}

int VirtualFS::lookup(const char *path, fk::memory::RetainPtr<VNode> &out) {
  fk::memory::RetainPtr<VNode> vnode = resolve_path(path, v_cwd);
  if (!vnode) {
    fk::algorithms::kwarn("VFS", "Lookup failed: path '%s' not found", path);
    return -1;
  }
  out = vnode;
  fk::algorithms::klog("VFS", "Lookup: path '%s' found", path);

  return 0;
}

int VirtualFS::open(const char *path, int flags) {
  fk::memory::RetainPtr<VNode> vnode = resolve_path(path, v_cwd);
  if (!vnode) {
    fk::algorithms::kwarn("VFS", "Open failed: path '%s' not found", path);
    return -1;
  }

  int fd = FileDescriptorTable::the().allocate(vnode, flags);
  if (fd < 0) {
    fk::algorithms::kwarn(
        "VFS", "Open failed: could not allocate file descriptor for path '%s'",
        path);
    return -1;
  }

  FileDescriptor *file_descriptor_ptr = FileDescriptorTable::the().get(fd);
  if (!file_descriptor_ptr) {
    fk::algorithms::kwarn(
        "VFS",
        "Open failed: could not retrieve file descriptor pointer for path '%s'",
        path);
    FileDescriptorTable::the().close(fd);
    return -1;
  }

  int ret = vnode->open(file_descriptor_ptr, flags);
  if (ret < 0) {
    FileDescriptorTable::the().close(
        fd); // Close the allocated FD on VNode open failure
    return ret;
  }
  return fd;
}

int VirtualFS::read(int file_descriptor, void *buf, size_t sz, size_t off) {
  FileDescriptor *f = FileDescriptorTable::the().get(file_descriptor);
  if (!f || !f->vnode) {
    fk::algorithms::kdebug("VFS", "Read failed: invalid file descriptor %d",
                           file_descriptor);
    return -1;
  }

  int ret = f->vnode->read(f, buf, sz, off);
  fk::algorithms::kdebug("VNode", "Read %d bytes from '%s' (offset=%zu)", ret,
                         f->vnode->m_name.c_str(), off);
  return ret;
}

int VirtualFS::write(int file_descriptor, const void *buf, size_t sz,
                     size_t off) {
  FileDescriptor *f = FileDescriptorTable::the().get(file_descriptor);
  if (!f || !f->vnode) {
    fk::algorithms::kdebug("VFS", "Write failed: invalid file descriptor %d",
                           file_descriptor);
    return -1;
  }

  int ret = f->vnode->write(f, buf, sz, off);
  fk::algorithms::kdebug("VNode", "Wrote %d bytes to '%s' (offset=%zu)", ret,
                         f->vnode->m_name.c_str(), off);
  return ret;
}
