#include <LibFK/Utilities/memory.h>

#include <Kernel/Fs/Virtual/SysFs/sys_attr_node.h>

fk::core::Result<size_t, fk::core::Error> SysAttrNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
  // Value with a trailing newline
  const char* data = m_value.c_str();
  size_t len = m_value.size();

  char tmp[66];
  size_t i = 0;
  while (i < len && i < 64) { tmp[i] = data[i]; ++i; }
  tmp[i++] = '\n';
  tmp[i] = '\0';

  size_t total = i;
  if (offset >= total) return static_cast<size_t>(0);
  size_t avail = total - (size_t)offset;
  size_t to_copy = (size < avail) ? size : avail;
  fk::memory::copy(buffer, reinterpret_cast<const uint8_t*>(tmp) + offset, to_copy);
  return to_copy;
}
