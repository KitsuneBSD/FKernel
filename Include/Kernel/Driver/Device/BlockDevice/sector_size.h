#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class SectorSize {
  size_t m_value;

public:
  explicit SectorSize(size_t size) : m_value(size) {}
  size_t value() const { return m_value; }
};

} // namespace fkernel
