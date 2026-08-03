#include <Kernel/Fs/Virtual/ProcFs/proc_uptime_node.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <LibFK/Algorithms/Logging/log.h>

using namespace fk::core;

static size_t read_from_buf(const char* buf, size_t len, uint64_t offset, size_t size, uint8_t* buffer) {
  if (offset >= len) return 0;
  size_t available = len - (size_t)offset;
  size_t to_copy = (size < available) ? size : available;
  for (size_t i = 0; i < to_copy; ++i) buffer[i] = static_cast<uint8_t>(buf[(size_t)offset + i]);
  return to_copy;
}

fk::core::Result<size_t, fk::core::Error> ProcUptimeNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
  uint64_t ticks = TickManager::the().get_ticks();
  uint32_t freq = TickManager::the().get_frequency();
  uint64_t seconds = (freq > 0) ? (ticks / freq) : 0;
  uint64_t centis = (freq > 0) ? ((ticks % freq) * 100 / freq) : 0;
  char buf[64];
  int len = snprintf(buf, sizeof(buf), "%llu.%02llu %llu.00\n",
                     (unsigned long long)seconds, (unsigned long long)centis,
                     (unsigned long long)(seconds / 2));
  return read_from_buf(buf, (size_t)len, offset, size, buffer);
}
