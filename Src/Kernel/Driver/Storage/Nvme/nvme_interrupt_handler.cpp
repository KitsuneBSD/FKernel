#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_types.h>
#include <Kernel/Driver/Storage/Nvme/interrupt_driven_nvme.h>
#include <Kernel/Driver/Storage/Nvme/nvme_interrupt_handler.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {

InterruptDrivenNvmeController* NvmeInterruptHandler::s_nvme_controllers[MAX_IDT_SIZE] = {nullptr};

void NvmeInterruptHandler::register_handler(fk::RefPtr<InterruptDrivenNvmeController> controller) {
  uint32_t irq = controller->interrupt_line();
  if (irq == 0 || irq >= 32) {
    fk::algorithms::kerror("NVMe-INT", "Invalid IRQ line: %d", irq);
    return;
  }

  uint8_t vector = static_cast<uint8_t>(irq + 32);
  s_nvme_controllers[vector] = controller.ptr();

  auto dispatch = [](uint8_t vec, InterruptFrame*) {
    if (s_nvme_controllers[vec]) {
      s_nvme_controllers[vec]->handle_interrupt();
    }
  };

  InterruptController::the().register_interrupt(dispatch, vector);

  fk::algorithms::klog("NVMe-INT", "Registered interrupt handler for IRQ %d (vector %d)", irq,
                       vector);
}

void NvmeInterruptHandler::unregister_handler(uint8_t vector) {
  s_nvme_controllers[vector] = nullptr;
}

} // namespace fkernel
