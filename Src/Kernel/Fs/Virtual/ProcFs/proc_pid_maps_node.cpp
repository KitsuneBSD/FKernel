#include <Kernel/Fs/Virtual/ProcFs/proc_pid_maps_node.h>
#include <Kernel/Memory/VirtualMemory/memory_region.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <LibFK/Synchronization/spinlock.h>

fk::core::Result<size_t, fk::core::Error> ProcPidMapsNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
  ensure_cached();
  if (offset >= m_cached.size()) return static_cast<size_t>(0);
  size_t available = m_cached.size() - (size_t)offset;
  size_t to_copy = (size < available) ? size : available;
  for (size_t i = 0; i < to_copy; ++i) buffer[i] = m_cached[(size_t)offset + i];
  return to_copy;
}

static void append_str(fk::containers::Vector<uint8_t>& vec, const char* s) {
  for (; *s; ++s) vec.push_back(static_cast<uint8_t>(*s));
}

static void append_hex(fk::containers::Vector<uint8_t>& vec, uintptr_t val, int width = 0) {
  char buf[20];
  int i = 0;
  if (val == 0) {
    buf[i++] = '0';
  } else {
    uintptr_t tmp = val;
    int digits = 0;
    while (tmp) { digits++; tmp >>= 4; }
    for (int d = digits - 1; d >= 0; --d) {
      int nibble = (val >> (d * 4)) & 0xf;
      buf[i++] = nibble < 10 ? '0' + nibble : 'a' + nibble - 10;
    }
  }
  buf[i] = '\0';
  int len = i;
  for (int p = len; p < width; ++p) vec.push_back(static_cast<uint8_t>('0'));
  append_str(vec, buf);
}

static void append_dec(fk::containers::Vector<uint8_t>& vec, unsigned long long val) {
  char buf[24];
  if (val == 0) { append_str(vec, "0"); return; }
  int i = 0;
  unsigned long long tmp = val;
  while (tmp) { buf[i++] = '0' + (tmp % 10); tmp /= 10; }
  for (int j = i - 1; j >= 0; --j) vec.push_back(static_cast<uint8_t>(buf[j]));
}

void ProcPidMapsNode::ensure_cached() {
  if (!m_cached.is_empty()) return;

  auto t = SchedulerManager::the().find_task(m_pid);
  if (!t) return;

  fk::synchronization::ScopedLock lock(t->lock);
  for (auto& region : t->resources.memory.regions.list) {
    bool readable   = (region.flags & PageFlags::Present)        != 0;
    bool writable   = (region.flags & PageFlags::Writable)       != 0;
    bool executable = (region.flags & PageFlags::ExecuteDisable) == 0;

    append_hex(m_cached, region.start);
    m_cached.push_back('-');
    append_hex(m_cached, region.end);
    m_cached.push_back(' ');
    m_cached.push_back(readable   ? 'r' : '-');
    m_cached.push_back(writable   ? 'w' : '-');
    m_cached.push_back(executable ? 'x' : '-');
    m_cached.push_back('p');
    m_cached.push_back(' ');
    append_hex(m_cached, 0, 8);
    append_str(m_cached, " 00:00 ");
    append_dec(m_cached, 0);
    m_cached.push_back(' ');
    if (region.name && region.name[0]) {
      m_cached.push_back(' ');
      append_str(m_cached, region.name);
    }
    m_cached.push_back('\n');
  }
}
