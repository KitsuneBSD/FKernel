#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Fs/Virtual/ProcFs/proc_stat_node.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>

using namespace fk::core;

static size_t read_from_buf(const char* buf, size_t len, uint64_t offset, size_t size, uint8_t* buffer) {
  if (offset >= len) return 0;
  size_t available = len - (size_t)offset;
  size_t to_copy = (size < available) ? size : available;
  for (size_t i = 0; i < to_copy; ++i) buffer[i] = static_cast<uint8_t>(buf[(size_t)offset + i]);
  return to_copy;
}

fk::core::Result<size_t, fk::core::Error> ProcStatNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
  uint64_t ticks = TickManager::the().get_ticks();
  char buf[256];
  int len = snprintf(buf, sizeof(buf),
    "cpu  0 0 0 %llu 0 0 0 0 0 0\n"
    "cpu0 0 0 0 %llu 0 0 0 0 0 0\n"
    "intr 0\n"
    "ctxt 0\n"
    "btime 0\n"
    "processes %llu\n"
    "procs_running 1\n"
    "procs_blocked 0\n",
    (unsigned long long)ticks,
    (unsigned long long)ticks,
    (unsigned long long)SchedulerManager::the().process_count());
  return read_from_buf(buf, (size_t)len, offset, size, buffer);
}
