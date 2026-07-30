#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

extern "C" uint8_t g_use_xsave;

static void fpu_save(void* area) {
  if (g_use_xsave) {
    uint32_t lo = 0xFFFFFFFFu, hi = 0xFFFFFFFFu;
    asm volatile("xsave64 %0" : "=m"(*static_cast<uint8_t*>(area))
                 : "a"(lo), "d"(hi) : "memory");
  } else {
    asm volatile("fxsave %0" : "=m"(*static_cast<uint8_t*>(area)) :: "memory");
  }
}

static void fpu_restore(void* area) {
  if (g_use_xsave) {
    uint32_t lo = 0xFFFFFFFFu, hi = 0xFFFFFFFFu;
    asm volatile("xrstor64 %0" :: "m"(*static_cast<uint8_t*>(area)),
                 "a"(lo), "d"(hi) : "memory");
  } else {
    asm volatile("fxrstor %0" :: "m"(*static_cast<uint8_t*>(area)) : "memory");
  }
}

void device_not_available_handler(uint8_t, InterruptFrame* frame) {
  // Clear CR0.TS so FPU is accessible for the rest of this handler
  asm volatile("clts");

  auto* current = SchedulerManager::the().current();
  if (!current) return;

  auto* last_fpu = SchedulerManager::the().last_fpu_task();

  // If a different task still owns the FPU registers, save its state first.
  // (Normally it was saved on context switch, but be defensive.)
  if (last_fpu && last_fpu != current) {
    fpu_save(get_fpu_save_area(last_fpu->resources.context));
  }

  // Restore current task's FPU state from its save area
  fpu_restore(get_fpu_save_area(current->resources.context));

  // Current task now owns the FPU on this CPU
  SchedulerManager::the().set_last_fpu_task(current);

  (void)frame;
}
