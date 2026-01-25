#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Types/types.h>

#include <Kernel/Driver/Device/BlockDevice/sector_size.h>

namespace fkernel {

class SectorCount {
  uint64_t m_value;

public:
  explicit SectorCount(uint64_t count) : m_value(count) {}
  uint64_t value() const { return m_value; }
};

} // namespace fkernel
