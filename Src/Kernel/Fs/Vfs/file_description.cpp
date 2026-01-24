#include <Kernel/Fs/Vfs/file_description.h>
#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <LibFK/Algorithms/log.h>

FileDescription::FileDescription(fk::RefPtr<Node> node, int flags)
    : m_node(node), m_flags(flags) {}

FileDescription::~FileDescription() {}

fk::RefPtr<Node> FileDescription::node() const { return m_node; }

fk::core::Result<size_t, fk::core::Error>
FileDescription::read(size_t size, uint8_t *buffer) {
  /*
  fk::algorithms::klog("FILE_DESCRIPTION",
                       "FileDescription::read starting. this=%p, node=%p", this,
                       m_node.get());
  */

  if (!m_node) {
    return fk::core::Error::InvalidHandle;
  }

  int access_mode = m_flags & O_ACCMODE;
  if (access_mode == O_WRONLY) {
    fk::algorithms::kwarn("FILE DESCRIPTION", "Read permission denied");
    return fk::core::Error::PermissionDenied;
  }

  bool is_dir = m_node->is_directory();
  if (is_dir) {
    return fk::core::Error::IsDirectory;
  }

  auto result = m_node->read(m_current_offset, size, buffer);
  if (result.is_error()) {
    return result.error();
  }

  m_current_offset += result.value();
  return result;
}

fk::core::Result<size_t, fk::core::Error>
FileDescription::write(size_t size, const uint8_t *buffer) {
  if (!m_node) {
    return fk::core::Error::InvalidHandle;
  }

  int access_mode = m_flags & O_ACCMODE;
  if (access_mode == O_RDONLY) {
    fk::algorithms::kwarn("FILE DESCRIPTION", "Write permission denied");
    return fk::core::Error::PermissionDenied;
  }

  // append mode handling could be here or in node, usually VFS handles offset
  // update but append implicitly seeks to end? POSIX says O_APPEND causes write
  // to happen at end of file.
  if (m_flags & O_APPEND) {
    m_current_offset = m_node->size();
  }

  auto result = m_node->write(m_current_offset, size, buffer);
  if (result.is_error()) {
    return result.error();
  }

  m_current_offset += result.value();
  return result;
}

fk::core::Result<uint64_t, fk::core::Error>
FileDescription::seek(uint64_t offset, SeekMode mode) {
  if (!m_node) {
    return fk::core::Error::InvalidHandle;
  }

  uint64_t new_offset = m_current_offset;
  size_t file_size = m_node->size();

  switch (mode) {
  case SeekMode::Set:
    new_offset = offset;
    break;
  case SeekMode::Current:
    // Check overflow?
    new_offset += offset;
    break;
  case SeekMode::End:
    new_offset = file_size + offset;
    break;
  }

  // Basic validity check? Some FS allow simple seek past end (sparse files).
  // For now we allow it.

  m_current_offset = new_offset;
  return new_offset;
}

fk::core::Result<int, fk::core::Error>
FileDescription::ioctl(uint64_t request, uint64_t arg) {
  if (!m_node) {
    return fk::core::Error::InvalidHandle;
  }
  return m_node->ioctl(request, arg);
}
