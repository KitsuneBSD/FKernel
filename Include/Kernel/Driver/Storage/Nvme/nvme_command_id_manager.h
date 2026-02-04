#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/Container/array.h>

namespace fkernel {

class NvmeCommandIdManager {
private:
  static constexpr size_t MAX_COMMANDS = 256;
  fk::containers::array<bool, MAX_COMMANDS> m_used_ids;

public:
  NvmeCommandIdManager() {
    for (size_t i = 0; i < MAX_COMMANDS; ++i)
      m_used_ids[i] = false;
  }

  uint16_t allocate();
  void release(uint16_t id);
  bool is_allocated(uint16_t id) const;
};

} // namespace fkernel
