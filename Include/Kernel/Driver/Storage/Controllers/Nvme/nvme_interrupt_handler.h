#pragma once

#include <LibFK/Container/Sequence/array.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>

namespace fkernel {

class InterruptDrivenNvmeController;

class NvmeInterruptHandler {
private:
  static constexpr size_t MAX_IDT_SIZE = 256;
  static InterruptDrivenNvmeController* s_nvme_controllers[MAX_IDT_SIZE];

  fk::RefPtr<InterruptDrivenNvmeController> m_controller;
  uint32_t m_irq_vector;

public:
  NvmeInterruptHandler(fk::RefPtr<InterruptDrivenNvmeController> controller, uint32_t irq)
      : m_controller(controller), m_irq_vector(irq) {}

  void handle_interrupt();
  uint32_t get_irq_vector() const { return m_irq_vector; }

  static void register_handler(fk::RefPtr<InterruptDrivenNvmeController> controller);
  static void unregister_handler(uint8_t vector);
  static void dispatch_interrupt(uint8_t vector, void* frame);
};

} // namespace fkernel
