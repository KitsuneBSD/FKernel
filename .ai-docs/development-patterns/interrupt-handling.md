# Interrupt Handling Conventions

> This file is AI-agent conceptual memory. Read before modifying interrupt/exception handling code.

## Boot Sequence: Interrupt Lifecycle

```
long_mode_start (asm)
  cli                          <- Explicit IF=0
  call kmain

early_init()
  GDT/TSS (IST1 stack allocated)
  Heap
  InterruptController::initialize()
    cli                        <- Redundant but safe
    IDT setup (all 256 gates)
    IST1 -> vector 8 (double fault)
    IDT loaded (lidt)
    TimerManager::initialize(1000)  <- PIT configured, but IRQs masked
    NMI enabled
    HardwareInterruptManager::initialize() -> PIC8259 (m_has_memory_manager=false)
    ClockManager::initialize()
    *** NO enable_interrupt() ***
    *** NO unmask_interrupt() ***
  MemoryManager::initialize()
    PhysicalMemory + VirtualMemory
    HardwareInterruptManager::set_memory_manager(true)
      -> PIC8259 disable, IOAPIC enable
      -> Re-apply m_unmasked_irqs on new controller
    TimerManager::set_memory_manager(true) -> APIC timer if available
  ACPI, CPU features

init()
  kernel_puts hook (kprintf now routes to serial/VGA)
  PCI, VFS, drivers, keyboard, mouse
  SchedulerManager::initialize()
  SyscallManager::initialize()
  *** Unmask IRQs (0,1,8,12,14,15) ***
  *** enable_interrupt() (sti) ***
  SchedulerManager::schedule()
```

## Key Rule: Phase Guarding

The interrupt dispatch path (`interrupt_dispatch`) runs on EVERY exception,
including faults that occur during early boot before hardware is initialized.

**NEVER** access hardware MMIO in the dispatch path without a phase guard:

```cpp
// CORRECT: guard with is_initialized()
if (SchedulerManager::the().is_initialized() &&
    SchedulerManager::the().is_need_resched()) {
  SchedulerManager::the().schedule();
}

// WRONG: unguarded access to APIC MMIO
if (SchedulerManager::the().is_need_resched()) {
  SchedulerManager::the().schedule();
}
```

The same applies to `current_processor()`:

```cpp
// CORRECT: guard APIC access
fkernel::Processor& SchedulerManager::current_processor() {
  if (!m_is_initialized)
    return m_processors[0];   // Safe fallback
  uint32_t id = APIC::the().get_id();
  ...
}
```

## Key Files

| File | Role |
|------|------|
| `Src/Kernel/Arch/x86_64/Boot/long_mode_start.asm` | Entry point, cli |
| `Src/Kernel/Arch/x86_64/Init/early_init.cpp` | Phase 1+2 init |
| `Src/Kernel/Init/init.cpp` | Phase 3 init, enables interrupts |
| `Src/Kernel/Arch/x86_64/Interrupt/interrupt_controller.cpp` | IDT setup, IST1 wiring |
| `Src/Kernel/Arch/x86_64/Interrupt/interrupt_dispatch.cpp` | Central dispatch (phase-guarded) |
| `Src/Kernel/Arch/x86_64/Interrupt/interrupt_stub.asm` | ISR stubs (256 vectors) |
| `Src/Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt.cpp` | PIC/IOAPIC management, unmask tracking |
| `Src/Kernel/Scheduler/Core/scheduler_manager.cpp` | current_processor() with phase guard |
| `Include/Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h` | CPU frame layout |
| `Include/Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h` | Exception handler macros |

## PIC -> IOAPIC Hot-Swap

`HardwareInterruptManager` tracks unmasked IRQs in `m_unmasked_irqs` bitmask.
When the controller switches from PIC8259 to IOAPIC (after memory manager init),
all previously unmasked IRQs are re-applied on the new controller automatically.

## IST1 (Interrupt Stack Table 1)

Vector 8 (double fault) uses IST1: a dedicated 16 KiB stack in BSS.
This prevents triple faults when the normal kernel stack is corrupted.
All other vectors use the default RSP (current stack).

## Exceptions With Error Codes

Vectors 8, 10, 11, 12, 13, 14, 17 push error codes automatically.
The ISR stub does NOT push a dummy error code for these vectors.
All other vectors get a dummy 0 error code pushed by the ISR stub.

## Conventions

- Interrupts are DISABLED throughout early_init and most of init
- `enable_interrupt()` is called ONLY at the end of init(), after scheduler
- `kerror()` halts (cli;hlt) -- never call from interrupt context unless fatal
- `kexception()` does NOT halt -- used for exception logging before halt_forever()
- Panic handler (`panic.cpp`) is allowed to include LibC directly (exception file)
- `kernel_puts.cpp` is allowed to include LibC directly (exception file)
