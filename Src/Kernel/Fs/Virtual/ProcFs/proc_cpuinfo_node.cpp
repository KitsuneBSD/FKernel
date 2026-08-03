#include <Kernel/Fs/Virtual/ProcFs/proc_cpuinfo_node.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

using namespace fk::core;

void ProcCpuinfoNode::ensure_cached() {
  if (!m_cached.is_empty()) return;
  static constexpr const char CPUINFO[] =
    "processor\t: 0\n"
    "vendor_id\t: FKernel\n"
    "model name\t: FKernel Virtual CPU\n"
    "cpu MHz\t\t: 2000.000\n"
    "cache size\t: 0 KB\n"
    "bogomips\t: 4000.00\n"
    "flags\t\t: fpu vme de pse tsc msr pae mce cx8 apic sep "
               "mtrr pge mca cmov pat pse36 clflush mmx fxsr sse "
               "sse2 ht syscall nx rdtscp lm\n\n";
  m_cached.clear();
  for (size_t i = 0; CPUINFO[i]; ++i) m_cached.push_back(static_cast<uint8_t>(CPUINFO[i]));
}

fk::core::Result<size_t, fk::core::Error> ProcCpuinfoNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
  ensure_cached();
  if (offset >= m_cached.size()) return static_cast<size_t>(0);
  size_t available = m_cached.size() - (size_t)offset;
  size_t to_copy = (size < available) ? size : available;
  for (size_t i = 0; i < to_copy; ++i) buffer[i] = m_cached[(size_t)offset + i];
  return to_copy;
}
