#include <Kernel/Fs/Virtual/ProcFs/proc_mounts_node.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Text/string.h>

using namespace fk::core;

static size_t read_from_buf(const char* buf, size_t len, uint64_t offset, size_t size, uint8_t* buffer) {
  if (offset >= len) return 0;
  size_t available = len - (size_t)offset;
  size_t to_copy = (size < available) ? size : available;
  for (size_t i = 0; i < to_copy; ++i) buffer[i] = static_cast<uint8_t>(buf[(size_t)offset + i]);
  return to_copy;
}

fk::core::Result<size_t, fk::core::Error> ProcMountsNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
  if (!m_cached.is_empty()) {
    return read_from_buf(reinterpret_cast<const char*>(m_cached.begin()), m_cached.size(), offset, size, buffer);
  }

  fk::text::String out("rootfs / tmpfs rw 0 0\n");

  fkernel::VirtualFileSystem::for_each_mount([](const char* path, const char* fstype, void* ctx) {
    auto* s = reinterpret_cast<fk::text::String*>(ctx);
    char line[256];
    snprintf(line, sizeof(line), "none %s %s rw 0 0\n", path, fstype);
    *s = *s + fk::text::String(line);
  }, &out);

  for (size_t i = 0; i < out.length(); ++i)
    m_cached.push_back(static_cast<uint8_t>(out[i]));

  return read_from_buf(reinterpret_cast<const char*>(m_cached.begin()), m_cached.size(), offset, size, buffer);
}
