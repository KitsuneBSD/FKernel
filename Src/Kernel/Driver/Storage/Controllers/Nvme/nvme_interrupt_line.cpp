#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_interrupt_line.h>

namespace fkernel {

NvmeInterruptLine::NvmeInterruptLine(uint32_t line) : m_line(line) {}

uint32_t NvmeInterruptLine::to_uint32() const {
  return m_line;
}

bool NvmeInterruptLine::is_configured() const {
  return m_line > 0;
}

void NvmeInterruptLine::configure(uint32_t line) {
  m_line = line;
}

} // namespace fkernel
