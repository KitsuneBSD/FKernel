#include <LibFK/Utilities/memory.h>

#include <Kernel/Fs/Virtual/ProcFs/proc_cmdline_node.h>
#include <Kernel/Boot/Core/boot_info.h>

static const char* get_cmdline() {
  return boot::BootInfo::the().get_kernel_cmdline();
}

static size_t cmdline_len() {
  const char* s = get_cmdline();
  size_t n = 0;
  while (s[n]) ++n;
  return n;
}

size_t ProcCmdlineNode::size() const {
  return cmdline_len() + 1; // include trailing newline
}

fk::core::Result<size_t, fk::core::Error> ProcCmdlineNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
  const char* cmdline = get_cmdline();
  size_t len = cmdline_len();

  // Expose as "cmdline\n"
  char tmp[514];
  size_t out_len = 0;
  while (out_len < len && out_len < sizeof(tmp) - 2) {
    tmp[out_len] = cmdline[out_len];
    ++out_len;
  }
  tmp[out_len++] = '\n';
  tmp[out_len] = '\0';

  if (offset >= out_len) return static_cast<size_t>(0);
  size_t available = out_len - (size_t)offset;
  size_t to_copy = (size < available) ? size : available;
  fk::memory::copy(buffer, reinterpret_cast<const uint8_t*>(tmp) + offset, to_copy);
  return to_copy;
}
