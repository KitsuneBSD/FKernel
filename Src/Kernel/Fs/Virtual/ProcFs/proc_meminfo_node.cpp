#include <Kernel/Fs/Virtual/ProcFs/proc_meminfo_node.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <LibFK/Algorithms/log.h>

using namespace fk::core;

static size_t read_from_buf(const char* buf, size_t len, uint64_t offset, size_t size, uint8_t* buffer) {
  if (offset >= len) return 0;
  size_t available = len - (size_t)offset;
  size_t to_copy = (size < available) ? size : available;
  for (size_t i = 0; i < to_copy; ++i) buffer[i] = static_cast<uint8_t>(buf[(size_t)offset + i]);
  return to_copy;
}

fk::core::Result<size_t, fk::core::Error> ProcMeminfoNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
  size_t total_phys = PhysicalMemoryManager::the().total_memory();
  size_t heap_total = 0, heap_free = 0;
  MemoryManager::the().heap_stats(heap_total, heap_free);
  char buf[512];
  int len = snprintf(buf, sizeof(buf),
    "MemTotal:     %8zu kB\n"
    "MemFree:      %8zu kB\n"
    "MemAvailable: %8zu kB\n"
    "Buffers:             0 kB\n"
    "Cached:              0 kB\n"
    "SwapTotal:           0 kB\n"
    "SwapFree:            0 kB\n",
    total_phys / 1024,
    heap_free / 1024,
    heap_free / 1024);
  return read_from_buf(buf, (size_t)len, offset, size, buffer);
}
