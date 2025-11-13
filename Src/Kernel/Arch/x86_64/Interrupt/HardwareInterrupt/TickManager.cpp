#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TickManager.h>

void TickManager::sleep(uint64_t awaited_ticks) {
  while (get_ticks() < awaited_ticks) {
    asm volatile("hlt");
  }
}
