#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

class NvmeInterruptLine {
public:
  explicit NvmeInterruptLine(uint32_t line);

  uint32_t to_uint32() const;
  bool is_configured() const;
  void configure(uint32_t line);

private:
  uint32_t m_line;
};

} // namespace fkernel
