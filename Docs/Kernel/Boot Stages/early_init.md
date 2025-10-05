# FKernel Early Initialization (early_init)

## Overview
The **early initialization stage** (`early_init`) is the very first phase executed after `kmain` is invoked.  
Its purpose is to establish the fundamental environment required for the kernel to operate in a stable and isolated context before any complex subsystems or drivers are loaded.

At this stage, the kernel prepares the CPU, memory management, and interrupt systems to ensure that all subsequent initialization phases can execute safely and deterministically.
---

## Responsibilities

The following components are initialized during `early_init`:

- [x] **[Global Descriptor Table (GDT)](https://en.wikipedia.org/wiki/Global_Descriptor_Table)**  
  Sets up code, data, and system segments. Ensures proper memory segmentation and privilege levels.

- [x] **[Task State Segment (TSS)](https://en.wikipedia.org/wiki/Task_state_segment)**  
  Configures the task state structure for stack switching and interrupt handling on x86_64.

- [x] **[Programmable Interrupt Controller (PIC)](https://en.wikipedia.org/wiki/Programmable_interrupt_controller)**  
  Initializes and masks legacy IRQ lines to ensure clean interrupt routing before the APIC takes over.

- [x] **[Non-Maskable Interrupt (NMI)](https://en.wikipedia.org/wiki/Non-maskable_interrupt)**  
  Enables the NMI to catch critical hardware or CPU faults during early boot.

- [x] **[Programmable Interval Timer (PIT)](https://en.wikipedia.org/wiki/Programmable_interval_timer)**  
  Initializes the base timer at 100 Hz. Provides a fallback timing source before the APIC timer is calibrated.

- [x] **[Physical Memory Manager (PMM)](https://en.wikipedia.org/wiki/Memory_paging)**  
  Parses the Multiboot2 memory map and marks usable/reserved memory regions.

- [x] **[Virtual Memory Manager (VMM)](https://en.wikipedia.org/wiki/Virtual_memory)**  
  Sets up virtual memory mappings and enables paging for kernel-space memory management.

- [x] **[Advanced Programmable Interrupt Controller (APIC)](https://en.wikipedia.org/wiki/Advanced_Programmable_Interrupt_Controller)**  
  Maps and initializes the Local APIC, calibrates the high-precision timer, and sets a 1 ms tick interval.

---

## Design Philosophy

> **"Expand only when there’s a real reason to expand."**

The early initialization layer is intentionally minimalistic.  
Its job is not to be feature-rich — it is to be *rock-solid*.  
Each additional feature added here must be **justified by necessity**, not convenience.

This pragmatic approach ensures:
- Minimal coupling between the early boot code and higher-level subsystems.  
- Simplicity in debugging early-stage faults.  
- Clear separation of responsibilities between `early_init`, `init`, and `late_init`.

---

## Transition to Next Stage

After `early_init` completes successfully, the kernel enters the [`init`](../kernel/init.md) stage.

In `init`, the system begins loading:
- Basic device drivers (console, storage, input)
- Hardware abstraction layers
- Logging and debugging facilities
- Filesystem and device management

Once those are operational, the `late_init` stage will initialize:
- The scheduler and multitasking environment
- Kernel subsystems and services

---

## Current Limitations

While the current `early_init` implementation is functional, it does not yet:
- Handle SMP (Symmetric Multiprocessing)
- Support NUMA-aware memory layouts
- Implement advanced paging features (huge pages, copy-on-write)
- Provide fault recovery for failed early subsystem initialization

These features will be considered **only when they become necessary** for FKernel’s evolution.

---

## Summary

| Component | Status | Description |
|------------|--------|-------------|
| GDT / TSS | ✅ | CPU segmentation and task state |
| IDT / PIC | ✅ | Basic interrupt handling setup |
| NMI | ✅ | Enabled for critical fault detection |
| PIT | ✅ | Fallback timer initialized |
| PMM | ✅ | Physical memory mapping ready |
| VMM | ✅ | Paging and virtual memory enabled |
| APIC | ✅ | High-precision timer calibrated |
| Drivers | ❌ | Deferred to `init` |
| Filesystems | ❌ | Deferred to `init` | 
| Scheduler | ❌ | Deferred to `late_init` |


---

## Conclusion

The **`early_init` stage is considered complete** and stable.  
It provides a clean, consistent, and predictable foundation for the kernel to continue bootstrapping higher-level systems.  
No further expansion is planned at this time — **by design**.

> *Simplicity is a feature when you’re this close to the metal.*