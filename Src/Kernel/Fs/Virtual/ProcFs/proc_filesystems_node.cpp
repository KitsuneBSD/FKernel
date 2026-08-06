#include <LibFK/Text/string.h>

#include <Kernel/Fs/Virtual/ProcFs/proc_filesystems_node.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>

using namespace fk::core;

fk::core::Result<size_t, fk::core::Error> ProcFilesystemsNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
  if (!m_cached.is_empty()) {
    if (offset >= m_cached.size()) return static_cast<size_t>(0);
    size_t available = m_cached.size() - (size_t)offset;
    size_t to_copy = (size < available) ? size : available;
    for (size_t i = 0; i < to_copy; ++i) buffer[i] = m_cached[(size_t)offset + i];
    return to_copy;
  }

  // Collect unique fstype names from all mounts.
  static const char* k_types[] = { "tmpfs", "proc", "devtmpfs", "ext2", "ext4",
                                    "fat32", "iso9660", "ufs", nullptr };
  fk::text::String out;
  for (size_t i = 0; k_types[i]; ++i) {
    char line[64];
    snprintf(line, sizeof(line), "nodev\t%s\n", k_types[i]);
    out = out + fk::text::String(line);
  }
  fkernel::VirtualFileSystem::for_each_mount([](const char*, const char* fstype, void* ctx) {
    auto* s = reinterpret_cast<fk::text::String*>(ctx);
    char line[64];
    snprintf(line, sizeof(line), "\t%s\n", fstype);
    *s = *s + fk::text::String(line);
  }, &out);

  for (size_t i = 0; i < out.length(); ++i)
    TRY(m_cached.push_back(static_cast<uint8_t>(out[i])));

  if (offset >= m_cached.size()) return static_cast<size_t>(0);
  size_t available = m_cached.size() - (size_t)offset;
  size_t to_copy = (size < available) ? size : available;
  for (size_t i = 0; i < to_copy; ++i) buffer[i] = m_cached[(size_t)offset + i];
  return to_copy;
}
