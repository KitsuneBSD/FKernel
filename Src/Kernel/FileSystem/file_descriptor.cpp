#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <Kernel/FileSystem/file_descriptor.h>
#include <LibFK/Algorithms/log.h>

FileDescriptorTable::FileDescriptorTable() { m_file_descriptors.clear(); }

FileDescriptorTable &FileDescriptorTable::the() {
  static FileDescriptorTable instance;
  return instance;
}

int FileDescriptorTable::allocate(fk::memory::RetainPtr<VNode> vnode,
                                  int flags) {
  if (m_file_descriptors.is_full())
    return -1;

  FileDescriptor file_descriptor;
  file_descriptor.file_descriptor = -1;
  file_descriptor.vnode = vnode;
  file_descriptor.flags = flags;
  file_descriptor.offset = 0;
  file_descriptor.used = true;

  if (!m_file_descriptors.push_back(file_descriptor))
    return -1;

  int idx = static_cast<int>(m_file_descriptors.size()) - 1;
  m_file_descriptors[idx].file_descriptor = idx;

  fk::algorithms::kdebug("FD", "Allocated file_descriptor %d for vnode '%s'",
                         idx, vnode->m_name.c_str());
  return idx;
}

int FileDescriptorTable::close(int file_descriptor) {
  FileDescriptor *f = get(file_descriptor);
  if (!f)
    return -1;
  if (f->vnode)
    f->vnode->close(f);
  f->used = false;

  fk::algorithms::kdebug("FD", "Closed file_descriptor %d", file_descriptor);
  return 0;
}

FileDescriptor *FileDescriptorTable::get(int file_descriptor) {
  if (file_descriptor < 0 ||
      static_cast<size_t>(file_descriptor) >= m_file_descriptors.size())
    return nullptr;
  FileDescriptor &f = m_file_descriptors[file_descriptor];
  return f.used ? &f : nullptr;
}

int file_descriptor_open_path(const char *path, int flags) {
  int fd = VirtualFS::the().open(path, flags);
  if (fd < 0)
    return fd;
  return fd;
}

int file_descriptor_read(int file_descriptor, void *buf, size_t sz) {
  FileDescriptor *f = FileDescriptorTable::the().get(file_descriptor);
  if (!f || !f->vnode)
    return -1;
  int ret = f->vnode->read(f, buf, sz, f->offset);
  if (ret > 0)
    f->offset += static_cast<uint64_t>(ret);
  return ret;
}

int file_descriptor_write(int file_descriptor, const void *buf, size_t sz) {
  FileDescriptor *f = FileDescriptorTable::the().get(file_descriptor);
  if (!f || !f->vnode)
    return -1;
  int ret = f->vnode->write(f, buf, sz, f->offset);
  if (ret > 0)
    f->offset += static_cast<uint64_t>(ret);
  return ret;
}

int file_descriptor_close(int file_descriptor) {
  return FileDescriptorTable::the().close(file_descriptor);
}

int FileDescriptorTable::dup2(int old_fd, int new_fd) {

  FileDescriptor *old_f = FileDescriptorTable::the().get(old_fd);

  if (!old_f || !old_f->used) {
    fk::algorithms::kwarn("FD", "dup2: invalid old file descriptor %d", old_fd);
    return -1;
  }

  // If new_fd is already open, close it first.

  if (FileDescriptorTable::the().get(new_fd)) {

    if (FileDescriptorTable::the().close(new_fd) < 0) {

      fk::algorithms::kwarn(
          "FD", "dup2: failed to close existing new file descriptor %d",
          new_fd);

      return -1;
    }
  }

  // Allocate a new file descriptor entry for new_fd, copying from old_fd.

  // We need to manually create a FileDescriptor entry and push it,

  // as FileDescriptorTable::allocate expects a VNode and flags, and we want to

  // reuse the existing VNode and flags.

  if (static_cast<size_t>(new_fd) >=
      FileDescriptorTable::MAX_file_descriptorS) {

    fk::algorithms::kwarn("FD", "dup2: new file descriptor %d is out of bounds",
                          new_fd);

    return -1;
  }

  FileDescriptor new_file_descriptor;

  new_file_descriptor.file_descriptor = new_fd;

  new_file_descriptor.vnode = old_f->vnode; // Share the VNode

  new_file_descriptor.flags = old_f->flags; // Copy flags

  new_file_descriptor.offset = old_f->offset; // Copy offset

  new_file_descriptor.used = true;

  // Ensure the static_vector has enough capacity if new_fd is larger than
  // current size.

  // This is a simplification; a real implementation might need more robust
  // handling.

  while (static_cast<size_t>(new_fd) >=
         FileDescriptorTable::the().m_file_descriptors.size()) {

    FileDescriptor dummy_fd;

    dummy_fd.used = false; // Mark as unused

    if (!FileDescriptorTable::the().m_file_descriptors.push_back(dummy_fd)) {

      fk::algorithms::kwarn(
          "FD", "dup2: failed to resize file descriptor table for %d", new_fd);

      return -1;
    }
  }

  // Place the new file descriptor at the correct index.

  FileDescriptorTable::the().m_file_descriptors[new_fd] = new_file_descriptor;

  fk::algorithms::kdebug("FD", "dup2: duplicated fd %d to %d", old_fd, new_fd);

  return 0;
}
