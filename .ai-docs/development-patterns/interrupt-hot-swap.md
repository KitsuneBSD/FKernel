# Interrupt Controller Hot-Swap

> AI-agent conceptual memory. Read before modifying interrupt handling code.

## The PIC→IOAPIC Transition

During boot, the interrupt controller switches from legacy 8259 PIC to IOAPIC. This happens when the memory manager initializes (because IOAPIC requires MMIO mapping).

```
Boot Phase 1: PIC8259 (no memory management)
  ↓ Memory Manager init
Boot Phase 2: IOAPIC (MMIO mapped)
```

## State Tracking

`HardwareInterruptManager` tracks unmasked IRQs in `m_unmasked_irqs` bitmask. When the controller switches:

1. PIC8259 is disabled
2. IOAPIC is enabled
3. All previously unmasked IRQs are re-applied on IOAPIC

This means IRQs unmasked during PIC phase (timer=0, keyboard=1, etc.) automatically work after the switch.

## Phase Guarding

The interrupt dispatch path runs on EVERY exception, including faults during early boot. Accessing APIC MMIO before it's mapped causes triple faults.

**Rule**: NEVER access hardware MMIO in the dispatch path without a phase guard:

```cpp
// CORRECT
if (SchedulerManager::the().is_initialized() &&
    SchedulerManager::the().is_need_resched()) {
  SchedulerManager::the().schedule();
}

// WRONG — unguarded APIC access
if (SchedulerManager::the().is_need_resched()) {
  SchedulerManager::the().schedule();
}
```

Same applies to `current_processor()` — falls back to `m_processors[0]` before APIC is ready.

## IST1 (Interrupt Stack Table 1)

Vector 8 (double fault) uses IST1: a dedicated 16 KiB stack in BSS. Prevents triple faults when the normal kernel stack is corrupted. All other vectors use the default RSP.

## Exceptions With Error Codes

Vectors 8, 10, 11, 12, 13, 14, 17 push error codes automatically. The ISR stub does NOT push a dummy error code for these. All other vectors get a dummy 0 error code.

## Key Files

| File | Role |
|------|------|
| `Src/Kernel/Arch/x86_64/Interrupt/interrupt_dispatch.cpp` | Central dispatch (phase-guarded) |
| `Src/Kernel/Arch/x86_64/Interrupt/interrupt_stub.asm` | ISR stubs (256 vectors) |
| `Src/Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt.cpp` | PIC/IOAPIC management, unmask tracking |
| `Src/Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/8259_pic.cpp` | Legacy PIC |
| `Src/Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.cpp` | Local APIC |
| `Src/Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/ioapic.cpp` | I/O APIC |
| `Src/Kernel/Arch/x86_64/Interrupt/interrupt_controller.cpp` | IDT setup, IST1 wiring |
| `Src/Kernel/Init/init.cpp` | Phase 3 init, enables interrupts |

## When Modifying

- Always check `is_initialized()` before accessing hardware in interrupt context
- `kerror()` halts — never call from interrupt context unless fatal
- `kexception()` does NOT halt — used for logging before halt_forever()
- New interrupt vectors must be added to IDT in `interrupt_controller.cpp`
- IST1 is ONLY for vector 8 — don't use for other vectors
