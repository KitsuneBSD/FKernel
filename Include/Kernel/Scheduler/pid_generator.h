#pragma once

#include <LibC/stdint.h>
#include <LibFK/Types/processId.h>

namespace fkernel {

class PidGenerator {
private:
  uint64_t m_next_pid = 1;
  uint64_t m_max_pid = UINT64_MAX;

public:
  explicit PidGenerator(uint64_t start_pid = 1) : m_next_pid(start_pid) {}

  fk::ProcessId generate_pid() {
    if (m_next_pid >= m_max_pid) {
      return fk::ProcessId(0); // Invalid PID indicates exhaustion
    }
    return fk::ProcessId(m_next_pid++);
  }

  bool is_valid(fk::ProcessId id) const { return id.value() > 0 && id.value() < m_next_pid; }

  void reset(uint64_t new_start = 1) { m_next_pid = new_start; }
};

} // namespace fkernel