#include <LibFK/Utilities/memory.h>

#include <Kernel/Fs/Virtual/ProcFs/proc_version_node.h>

using namespace fk::core;

static constexpr const char KERNEL_VERSION[] =
    "FKernel version 0.1.0-dev (fkernel@localhost) (gcc 14.0.0) #1 SMP\n";

fk::core::Result<size_t, fk::core::Error> ProcVersionNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
  const size_t len = sizeof(KERNEL_VERSION) - 1;
  if (offset >= len) return static_cast<size_t>(0);
  size_t available = len - offset;
  size_t to_copy = (size < available) ? size : available;
  for (size_t i = 0; i < to_copy; ++i) buffer[i] = static_cast<uint8_t>(KERNEL_VERSION[offset + i]);
  return to_copy;
}

size_t ProcVersionNode::size() const { return sizeof(KERNEL_VERSION) - 1; }
