#include <Kernel/FileSystem/RamFS/ramfs.h>
#include <Kernel/FileSystem/RamFS/ramfs_file.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>

RamFS::RamFS() {
  VNode *root_node = new VNode();
  root_node->m_name = "/";
  root_node->type = VNodeType::Directory;
  root_node->inode = new Inode(1);
  root_node->ops = &ops;
  r_root = fk::memory::adopt_retain(root_node);
}

RamFS &RamFS::the() {
  static RamFS instance;
  return instance;
}

fk::memory::RetainPtr<VNode> RamFS::root() { return r_root; }

int RamFS::ramfs_read(VNode *vnode, FileDescriptor *fd, void *buffer,
                      size_t size, size_t offset) {
  (void)fd;
  if (!vnode) {
    fk::algorithms::kwarn("RamFS", "Read failed: vnode is nullptr");
    return -1;
  }

  if (vnode->type != VNodeType::Regular) {
    fk::algorithms::kwarn("RamFS",
                          "Read failed: vnode '%s' is not a regular file",
                          vnode->m_name.c_str());
    return -1;
  }

  if (!vnode->fs_private) {
    fk::algorithms::kwarn("RamFS",
                          "Read failed: vnode '%s' has no fs_private data",
                          vnode->m_name.c_str());
    return -1;
  }

  RamFile *file = reinterpret_cast<RamFile *>(vnode->fs_private);

  if (offset >= file->r_size) {
    fk::algorithms::klog(
        "RamFS", "Read request at offset %zu exceeds file size %zu in '%s'",
        offset, file->r_size, vnode->m_name.c_str());
    return 0;
  }

  size_t available = file->r_size - offset;
  size_t to_read = (size < available) ? size : available;

  if (!buffer) {
    fk::algorithms::kwarn("RamFS", "Read failed: buffer is nullptr");
    return -1;
  }

  memcpy(buffer, file->r_data + offset, to_read);

  fk::algorithms::klog(
      "RamFS", "Read %zu bytes from '%s' at offset %zu (file size: %zu)",
      to_read, vnode->m_name.c_str(), offset, file->r_size);
  return static_cast<int>(to_read);
}

int RamFS::ramfs_write(VNode *vnode, FileDescriptor *fd, const void *buffer,
                       size_t size, size_t offset) {
  (void)fd;
  if (!vnode) {
    fk::algorithms::kwarn("RamFS", "Write failed: vnode is nullptr");
    return -1;
  }

  if (vnode->type != VNodeType::Regular) {
    fk::algorithms::kwarn("RamFS",
                          "Write failed: vnode '%s' is not a regular file",
                          vnode->m_name.c_str());
    return -1;
  }

  if (!vnode->fs_private) {
    fk::algorithms::kwarn("RamFS",
                          "Write failed: vnode '%s' has no fs_private data",
                          vnode->m_name.c_str());
    return -1;
  }

  if (!buffer) {
    fk::algorithms::kwarn("RamFS", "Write failed: buffer is nullptr");
    return -1;
  }

  RamFile *file = reinterpret_cast<RamFile *>(vnode->fs_private);
  const size_t MAX_SIZE = sizeof(file->r_data);

  if (offset >= MAX_SIZE) {
    fk::algorithms::kwarn(
        "RamFS",
        "Write failed: offset %zu exceeds maximum file size %zu in '%s'",
        offset, MAX_SIZE, vnode->m_name.c_str());
    return -1;
  }

  size_t space_left = MAX_SIZE - offset;
  size_t to_write = (size < space_left) ? size : space_left;

  memcpy(file->r_data + offset, buffer, to_write);

  if (offset + to_write > file->r_size)
    file->r_size = offset + to_write;

  vnode->size = file->r_size;

  fk::algorithms::klog(
      "RamFS", "Wrote %zu bytes to '%s' at offset %zu (new file size: %zu)",
      to_write, vnode->m_name.c_str(), offset, vnode->size);
  return static_cast<int>(to_write);
}

int RamFS::ramfs_open(VNode *vnode, FileDescriptor *fd, int flags) {
  (void)fd;
  (void)vnode;
  (void)flags;
  return 0;
}

int RamFS::ramfs_close(VNode *vnode, FileDescriptor *fd) {
  (void)fd;
  (void)vnode;
  return 0;
}

int RamFS::ramfs_lookup(VNode *vnode, FileDescriptor *fd, const char *name,
                        fk::memory::RetainPtr<VNode> &out) {
  (void)fd;
  return vnode->lookup(name, out);
}

int RamFS::ramfs_create(VNode *vnode, FileDescriptor *fd, const char *name,
                        VNodeType type, fk::memory::RetainPtr<VNode> &out) {
  (void)fd;
  if (!vnode || vnode->type != VNodeType::Directory)
    return -1;

  auto new_node = fk::memory::adopt_retain(new VNode());
  new_node->m_name = name;
  new_node->type = type;
  new_node->parent = vnode;
  new_node->inode = new Inode(vnode->inode_number + 1);
  new_node->inode_number = new_node->inode->i_number;
  new_node->ops = &ops;

  if (type == VNodeType::Regular) {
    auto file = new RamFile();
    memset(file, 0, sizeof(RamFile));
    strncpy(file->r_name, name, sizeof(file->r_name) - 1);
    new_node->fs_private = file;

    fk::algorithms::klog("RamFS", "Allocated RamFile for vnode '%s' at %p",
                         name, file);
  }

  vnode->dir_entries.push_back(DirEntry{name, new_node});
  out = new_node;

  fk::algorithms::klog("RamFS", "Created '%s' in directory '%s'", name,
                       vnode->m_name.c_str());
  return 0;
}

int RamFS::ramfs_readdir(VNode *vnode, FileDescriptor *fd, void *buffer,
                         size_t max_entries) {
  (void)fd;
  if (!vnode || vnode->type != VNodeType::Directory)
    return -1;

  DirEntry *buf = reinterpret_cast<DirEntry *>(buffer);
  size_t n = vnode->dir_entries.size();
  if (n > max_entries)
    n = max_entries;

  for (size_t i = 0; i < n; ++i)
    buf[i] = vnode->dir_entries[i];

  return static_cast<int>(n);
}

int RamFS::ramfs_unlink(VNode *vnode, FileDescriptor *fd, const char *name) {
  (void)fd;
  if (!vnode || vnode->type != VNodeType::Directory)
    return -1;

  for (size_t i = 0; i < vnode->dir_entries.size(); ++i) {
    if (strcmp(vnode->dir_entries[i].m_name.c_str(), name) == 0) {
      vnode->dir_entries.erase(i);
      fk::algorithms::klog("RamFS", "Unlinked '%s' from directory '%s'", name,
                           vnode->m_name.c_str());
      return 0;
    }
  }

  return -1; // não encontrado
}

// Define a tabela de operações
VNodeOps RamFS::ops = {
    .read = &RamFS::ramfs_read,
    .write = &RamFS::ramfs_write,
    .open = &RamFS::ramfs_open,
    .close = &RamFS::ramfs_close,
    .lookup = &RamFS::ramfs_lookup,
    .create = &RamFS::ramfs_create,
    .readdir = &RamFS::ramfs_readdir,
    .unlink = &RamFS::ramfs_unlink,
};
