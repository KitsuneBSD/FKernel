#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Fs/Vfs/Core/file_description.h>
#include <Kernel/Fs/Vfs/Core/node.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Fs/Virtual/PipeFs/pipe_node.h>

FileDescription::FileDescription(fk::RefPtr<fkernel::Dentry> dentry, int flags)
    : m_dentry(dentry), m_flags(flags) {}

bool FileDescription::resolve_dentry() const {
    if (!m_dentry) return false;
    return m_dentry->top_node() != nullptr;
}

FileDescription::~FileDescription() {
    auto n = node();
    if (n) n->on_close();
    if ((m_flags & O_ACCMODE) == O_RDONLY) {
        auto* pipe = fkernel::PipeNode::from_node(n.ptr());
        if (pipe) pipe->remove_reader();
    }
}

fk::RefPtr<fkernel::Dentry> FileDescription::dentry() const { return m_dentry; }
fk::RefPtr<Node> FileDescription::node() const { return m_dentry ? m_dentry->top_node() : nullptr; }

fk::core::Result<size_t, fk::core::Error>
FileDescription::read(size_t size, uint8_t *buffer) {
  if (!resolve_dentry())
    return fk::core::Error::InvalidHandle;
  auto m_node = node();
  if (!m_node) {
    return fk::core::Error::InvalidHandle;
  }

  int access_mode = m_flags & O_ACCMODE;
  if (access_mode == O_WRONLY) {
    fk::algorithms::kwarn("FILE_DESC", "Read permission denied");
    return fk::core::Error::PermissionDenied;
  }

  bool is_dir = m_node->is_directory();
  if (is_dir) {
    return fk::core::Error::IsDirectory;
  }

  m_offset_lock.lock();
  uint64_t read_offset = m_current_offset;
  m_offset_lock.unlock();

  auto result = m_node->read(read_offset, size, buffer);
  if (result.is_error()) return result.error();

  m_offset_lock.lock();
  m_current_offset = read_offset + result.value();
  m_offset_lock.unlock();
  return result;
}

fk::core::Result<size_t, fk::core::Error>
FileDescription::write(size_t size, const uint8_t *buffer) {
  if (!resolve_dentry())
    return fk::core::Error::InvalidHandle;
  auto m_node = node();
  if (!m_node) {
    return fk::core::Error::InvalidHandle;
  }

  int access_mode = m_flags & O_ACCMODE;
  if (access_mode == O_RDONLY) {
    fk::algorithms::kwarn("FILE_DESC", "Write permission denied");
    return fk::core::Error::PermissionDenied;
  }

  // append mode handling could be here or in node, usually VFS handles offset
  // update but append implicitly seeks to end? POSIX says O_APPEND causes write
  // to happen at end of file.
  m_offset_lock.lock();
  if (m_flags & O_APPEND)
    m_current_offset = m_node->size();
  uint64_t write_offset = m_current_offset;
  m_offset_lock.unlock();

  auto result = m_node->write(write_offset, size, buffer);
  if (result.is_error()) return result.error();

  m_offset_lock.lock();
  m_current_offset = write_offset + result.value();
  m_offset_lock.unlock();
  return result;
}

fk::core::Result<uint64_t, fk::core::Error>
FileDescription::seek(uint64_t offset, SeekMode mode) {
  if (!resolve_dentry())
    return fk::core::Error::InvalidHandle;
  auto m_node = node();
  if (!m_node) return fk::core::Error::InvalidHandle;

  fk::synchronization::ScopedLockIRQ lock(m_offset_lock);
  uint64_t new_offset = m_current_offset;
  size_t file_size = m_node->size();

  switch (mode) {
  case SeekMode::Set:
    new_offset = offset;
    break;
  case SeekMode::Current:
    if (offset > UINT64_MAX - new_offset) return fk::core::Error::InvalidParameter;
    new_offset += offset;
    break;
  case SeekMode::End:
    if (offset > UINT64_MAX - file_size) return fk::core::Error::InvalidParameter;
    new_offset = file_size + offset;
    break;
  }

  m_current_offset = new_offset;
  return new_offset;
}

fk::core::Result<int, fk::core::Error>
FileDescription::ioctl(uint64_t request, uint64_t arg) {
  if (!resolve_dentry())
    return fk::core::Error::InvalidHandle;
  auto m_node = node();
  if (!m_node) {
    return fk::core::Error::InvalidHandle;
  }
  return m_node->ioctl(request, arg);
}
