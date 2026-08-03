#include <Kernel/Fs/Virtual/ProcFs/proc_partitions_node.h>
#include <Kernel/Driver/Storage/Partitions/partition_manager.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Text/string.h>

using namespace fk::core;

fk::core::Result<size_t, fk::core::Error> ProcPartitionsNode::read(uint64_t offset, size_t size, uint8_t* buffer) {
  ensure_cached();
  if (offset >= m_cached.size()) return static_cast<size_t>(0);
  size_t available = m_cached.size() - offset;
  size_t to_copy = (size < available) ? size : available;
  for (size_t i = 0; i < to_copy; ++i) buffer[i] = m_cached[offset + i];
  return to_copy;
}

void ProcPartitionsNode::ensure_cached() {
  if (!m_cached.is_empty()) return;
  
  fk::text::String buf = "major minor  #blocks  name\n\n";
  auto& partitions = PartitionManager::the().partitions().all();
  
  for (auto& part : partitions) {
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "   1     %d   %llu %s\n", 
             0, part->sector_count().value(), part->name().c_str());
    buf = buf + fk::text::String(tmp);
  }

  m_cached.clear();
  for (size_t i = 0; i < buf.length(); ++i) m_cached.push_back(static_cast<uint8_t>(buf[i]));
}
