# FKernel Development Instructions (GEMINI-ready)

## 1. Project Overview

FKernel is a **modern C++ kernel**, inspired by SerenityOS, BSD, and XNU.  

- **Goal:** Production-ready kernel, not hobby OS.  
- **Differentiators:**
  - **DAL:** Recompiles Linux/Windows drivers to FKernel userspace.
  - **IPUK:** Isolated kernel subsystems in userspace, security-first.
  - **Pragmatic POSIX:** Only a mask for internal use, no legacy complexity.
  - **SystemV + BSD ABI:** Native compatibility for modern x86_64.

---

## 2. Mandatory Guidelines

- Code is **freestanding** and **baremetal**.  
- No support for exceptions or RTTI.  
- **Error handling:** C-style or `Result<T, Error>`.  
- Directory `.ai-docs` must reflect **code modifications** and processes.

---

## 3. Architecture Layers (Conceptual)

┌─────────────────────────┐
│ Userspace │
│ (Applications, Shell) │
└─────┬───────────────────┘
│ syscalls
┌─────▼───────────────────┐
│ Kernel │
│ (Uses ONLY LibFK) │
└─────┬───────────────────┘
│
┌─────▼───────────────────┐
│ LibFK │
│ (STL-like, uses LibC) │
└─────┬───────────────────┘
│
┌─────▼───────────────────┐
│ LibC │
│ (Minimal, freestanding) │
└─────────────────────────┘

**Rules:**  

1. LibC: strings, memory, types (freestanding).  
2. LibFK: uses LibC + own functions.  
3. Kernel: uses ONLY LibFK.  

---

## 4. Namespaces (LibFK)

**LibFK:**

- `containers`: Vector, HashMap, List  
- `text`: String, StringBuilder  
- `memory`: OwnPtr, RefPtr  
- `core`: Result, Optional  

---

## 5. Object Calisthenics

**Mandatory 9 Rules:**  

1. **One indentation level per method** – extract short functions.  
2. **Avoid `else`** – use early returns.  
3. **Wrap primitives and strings** – type-safe wrappers.  
4. **First-class collections** – encapsulate vectors/arrays.  
5. **One dot per line** – Law of Demeter.  
6. **No abbreviations** – full descriptive names.  
7. **Keep entities small** – classes ≤200 lines, methods ≤20 lines.  
8. **Max 2 instance variables per class** – prefer composition.  
9. **No getters/setters** – use rich domain model methods.

---

## 6. Error Handling

- Use **Result<T, Error>** for fallible operations.  
- Use **Optional<T>** for nullable values.  
- Use **TRY macro** for error propagation.

```cpp
auto page = TRY(Physical::allocate_page());
if (auto proc = find_process(pid); proc.has_value())
    proc.value()->resume();
```

### 7. Performance & Memory

- RAII whenever possible.
- Prefer stack allocation, heap only when needed.
- Inline hot paths, avoid virtual calls (use CRTP).
- No exceptions, no RTTI, no STL.
- Manual memory barriers when interacting with hardware.

### 8. Hardware Guidelines

- Always validate against official specs (AHCI, APIC, PCI).
- Use volatile for MMIO registers.
- Use memory barriers (__sync_synchronize()) to enforce ordering.

### 9. Documentation

- Every public API must have: @brief, @return, @note, @warning, @example.

```cpp
/// @brief Allocates a physical page frame
/// @return Result<Page, Error::OutOfMemory>
/// @note Thread-safe, caller must free
auto page = TRY(Physical::allocate_page());
Physical::free_page(page);
```

### 10. GEMINI Notes / AI Context

- Architecture topology-aware: CPU cores & RAM mapped physically.
- DAL: drivers recompiled for FKernel userspace abstraction.
- IPUK: full isolation of userspace kernel subsystems.
- Flag O_HPC: physical threads, 1:1 mapping to cores.
- Green threads via MLFQ by default, bypass with O_HPC.
- Memory management hybrid: Buddy + Zones + Bitmap.

### 11. File & Directory Naming Conventions - SECRET RULE

- **🔑 SECRET RULE: ONE STRUCT/CLASS PER FILE** - This is non-negotiable and must be enforced
- **Directories:** PascalCase representing **domains**  
  - Examples: `Kernel`, `Driver`, `PhysicalMemory`, `VirtualMemory`, `Syscall`  
- **Files:** camel-case, **exactly one** struct/class per file  
  - Examples: `cpuContext.h` (contains ONLY CpuContext), `ataController.cpp` (contains ONLY AtaController)  
- **Domain-based organization**: Each directory represents a **cohesive domain** with clear boundaries
- **Single responsibility**: Each file represents **exactly one concept/responsibility**
- **Deep autodocumentation**: Structure should be self-evident from directory/file names alone
- Keep **modification scope minimal**: changing a file should affect **only its domain**.

#### Domain Organization Principles

1. **Domain Separation**: Directories represent clear domain boundaries
2. **Conceptual Clarity**: File names should immediately reveal their purpose
3. **Architectural Intent**: Directory structure should document the system architecture
4. **Maintenance Isolation**: Changes to one domain should never impact unrelated domains
5. **Discovery**: Developers should be able to locate functionality by following the domain hierarchy

### 12. Directory Structure Enforcement

```
Include/
├── Kernel
│   ├── Arch
│   │   └── x86_64
│   │       ├── arch_defs.h
│   │       ├── Interrupt
│   │       │   ├── Handler
│   │       │   │   ├── exception_macros.h
│   │       │   │   ├── handlers.h
│   │       │   │   └── interrupt_frame.h
│   │       │   ├── HardwareInterrupts
│   │       │   │   ├── hardware_interrupt.h
│   │       │   │   ├── InterruptController
│   │       │   │   │   ├── 8259_pic.h
│   │       │   │   │   ├── apic.h
│   │       │   │   │   ├── ioapic.h
│   │       │   │   │   └── x2apic.h
│   │       │   │   ├── tick_manager.h
│   │       │   │   ├── TimerController
│   │       │   │   │   ├── apic_timer.h
│   │       │   │   │   ├── hpet.h
│   │       │   │   │   └── pit.h
│   │       │   │   └── timer_interrupt.h
│   │       │   ├── interrupt_controller.h
│   │       │   ├── interrupt_types.h
│   │       │   ├── isr_stubs.h
│   │       │   └── non_maskable_interrupt.h
│   │       ├── io.h
│   │       ├── Segments
│   │       │   ├── Gdt
│   │       │   │   ├── gdt_entry.h
│   │       │   │   ├── gdt_pointer.h
│   │       │   │   └── gdt_structures.h
│   │       │   ├── gdt.h
│   │       │   └── Tss
│   │       │       ├── tss_descriptor.h
│   │       │       ├── tss_stacks.h
│   │       │       └── tss_structures.h
│   │       └── Syscall
│   │           └── syscall_arch.h
│   ├── Boot
│   │   ├── boot_info.h
│   │   ├── kernel_entry.h
│   │   ├── Multiboot
│   │   │   ├── multiboot2.h
│   │   │   └── multiboot_interpreter.h
│   │   ├── Stages
│   │   │   ├── early_init.h
│   │   │   └── init.h
│   ├── Clock
│   │   ├── ClockController
│   │   │   ├── cmos.h
│   │   │   └── rtc.h
│   │   └── clock_interrupt.h
│   ├── Driver
│   │   ├── Keyboard
│   │   │   └── ps2_keyboard.h
│   │   ├── SerialPort
│   │   │   ├── serial_node.h
│   │   │   └── serial_port.h
│   │   ├── Storage
│   │   │   ├── Ata
│   │   │   │   ├── ata_controller.h
│   │   │   │   ├── ata_device.h
│   │   │   │   ├── ata_transfer_strategy.h
│   │   │   │   └── pio_strategy.h
│   │   │   ├── Partitions
│   │   │   │   ├── Gpt
│   │   │   │   │   ├── gpt.h
│   │   │   │   │   ├── gpt_header.h
│   │   │   │   │   └── gpt_partition_entry.h
│   │   │   │   ├── Mbr
│   │   │   │   │   ├── mbr.h
│   │   │   │   │   ├── mbr_header.h
│   │   │   │   │   └── mbr_partition_entry.h
│   │   │   │   ├── partition.h
│   │   │   │   ├── partition_list.h
│   │   │   │   └── partition_manager.h
│   │   │   ├── storage_cache.h
│   │   │   ├── storage_device.h
│   │   │   └── storage_device_name.h
│   │   └── Vga
│   │       ├── display.h
│   │       ├── font.h
│   │       ├── vga_adapter.h
│   │       └── vga_node.h
│   ├── Fs
│   │   ├── DebugFs
│   │   │   └── debug_fs.h
│   │   ├── DevFs
│   │   │   ├── dev_fs.h
│   │   │   └── tty.h
│   │   ├── Fat12
│   │   │   ├── bpb.h
│   │   │   ├── directory_entry.h
│   │   │   ├── fat_12_fs.h
│   │   │   └── fat_12_node.h
│   │   ├── Fat16
│   │   │   ├── bpb.h
│   │   │   ├── directory_entry.h
│   │   │   └── fat_16_fs.h
│   │   ├── Fat32
│   │   │   ├── bpb.h
│   │   │   ├── directory_entry.h
│   │   │   └── fat_32_fs.h
│   │   ├── ProcFs
│   │   │   └── proc_fs.h
│   │   ├── RamDisk
│   │   │   └── ram_disk.h
│   │   ├── TmpFs
│   │   │   └── tmp_fs.h
│   │   └── Vfs
│   │       ├── auto_mounter.h
│   │       ├── Compat
│   │       │   └── Linux
│   │       ├── Definitions
│   │       │   ├── file_descriptor.h
│   │       │   ├── inode_index.h
│   │       │   └── seek_mode.h
│   │       ├── definitions.h
│   │       ├── file_description.h
│   │       ├── fstab.h
│   │       ├── node.h
│   │       └── virtual_filesystem.h
│   ├── Hardware
│   │   ├── Acpi
│   │   │   ├── acpi.h
│   │   │   ├── rsdp.h
│   │   │   ├── rsdt.h
│   │   │   ├── sdt_header.h
│   │   │   └── xsdt.h
│   │   ├── Cpu
│   │   │   ├── cpu_block.h
│   │   │   ├── cpu_context.h
│   │   │   ├── cpu.h
│   │   │   ├── cpu_register.h
│   │   │   └── processor.h
│   │   ├── Fadt
│   │   │   ├── fadt.h
│   │   │   ├── fadt_manager.h
│   │   │   └── generic_address_structures.h
│   │   ├── Madt
│   │   │   ├── madt_entries.h
│   │   │   ├── madt.h
│   │   │   ├── madt_interrupt_source_override.h
│   │   │   ├── madt_ioapic.h
│   │   │   ├── madt_lapic.h
│   │   │   └── madt_lapic_override.h
│   │   └── Pci
│   │       ├── pci_address.h
│   │       ├── pci_device.h
│   │       └── pci.h
│   ├── Ipc
│   │   ├── badge.h
│   │   ├── capability.h
│   │   ├── cspace.h
│   │   ├── endpoint.h
│   │   ├── global_endpoint_manager.h
│   │   ├── message_info.h
│   │   ├── notification.h
│   │   ├── signal_delivery.h
│   │   └── system_labels.h
│   ├── Loader
│   │   └── elf_loader.h
│   ├── Memory
│   │   ├── memory_manager.h
│   │   ├── ObjectMemory
│   │   │   └── Zone
│   │   │       ├── zone_allocator.h
│   │   │       ├── zone_defs.h
│   │   │       └── zone_types.h
│   │   ├── PhysicalMemory
│   │   │   ├── Buddy
│   │   │   │   ├── buddy_allocator.h
│   │   │   │   ├── buddy_order.h
│   │   │   │   ├── buddy_state.h
│   │   │   │   └── free_blocks.h
│   │   │   ├── physical_memory_manager.h
│   │   │   └── physical_memory_zone.h
│   │   └── VirtualMemory
│   │       ├── Pages
│   │       │   ├── page_flags.h
│   │       │   └── page_table.h
│   │       └── virtual_memory_manager.h
│   ├── Posix
│   │   └── sys
│   │       ├── errno.h
│   │       ├── stat.h
│   │       ├── time.h
│   │       └── utsname.h
│   ├── Scheduler
│   │   ├── scheduler.h
│   │   └── Task
│   │       ├── task.h
│   │       └── task_state.h
│   └── Syscall
│       ├── syscall_arch.h
│       ├── syscall.h
│       ├── syscall_numbers.h
│       ├── syscall_types.h
│       └── syscall_utils.h
├── LibC
│   ├── assert.h
│   ├── ctype.h
│   ├── limits.h
│   ├── stdarg.h
│   ├── stdbool.h
│   ├── stddef.h
│   ├── stdint.h
│   ├── stdio.h
│   ├── stdlib.h
│   ├── string.h
│   ├── sys
│   │   └── syscall.h
│   └── unistd.h
└── LibFK
    ├── Algorithms
    │   ├── crc32.h
    │   ├── djb2.h
    │   ├── log.h
    │   └── math.h
    ├── Container
    │   ├── array.h
    │   ├── bitmap.h
    │   ├── hash_map.h
    │   ├── intrusive_list.h
    │   ├── list.h
    │   ├── queue.h
    │   ├── span.h
    │   ├── stack.h
    │   ├── static_vector.h
    │   └── vector.h
    ├── Core
    │   ├── Assertions.h
    │   ├── Error.h
    │   ├── Platform.h
    │   └── Result.h
    ├── Functional
    │   ├── Function.h
    │   └── Tuple.h
    ├── Memory
    │   ├── heap_malloc.h
    │   ├── new.h
    │   ├── optional.h
    │   ├── own_ptr.h
    │   ├── ref_counted.h
    │   ├── ref_ptr.h
    │   └── retain_ptr.h
    ├── Text
    │   ├── fixed_string.h
    │   ├── string_builder.h
    │   ├── string.h
    │   └── string_view.h
    ├── Traits
    │   ├── crtp.h
    │   ├── traits.h
    │   └── type_traits.h
    ├── Tree
    │   └── rb_tree.h
    ├── Types
    │   └── types.h
    └── Utilities
        ├── aligner.h
        ├── converter.h
        ├── Memory.h
        ├── pair.h
        └── size_checking.h
Src/
├── Kernel
│   ├── Arch
│   │   └── x86_64
│   │       ├── Boot
│   │       │   ├── check.asm
│   │       │   ├── error.asm
│   │       │   ├── long_mode_start.asm
│   │       │   ├── main.asm
│   │       │   └── setup_page_tables.asm
│   │       ├── Init
│   │       │   └── early_init.cpp
│   │       ├── Interrupt
│   │       │   ├── flush_idt.asm
│   │       │   ├── Handler
│   │       │   │   ├── Exception
│   │       │   │   │   ├── alignment_check.cpp
│   │       │   │   │   ├── bound_range_exceeded.cpp
│   │       │   │   │   ├── breakpoint.cpp
│   │       │   │   │   ├── debug.cpp
│   │       │   │   │   ├── default_handler.cpp
│   │       │   │   │   ├── device_not_available.cpp
│   │       │   │   │   ├── divide_by_zero.cpp
│   │       │   │   │   ├── double_fault.cpp
│   │       │   │   │   ├── gp_handler.cpp
│   │       │   │   │   ├── invalid_opcode.cpp
│   │       │   │   │   ├── invalid_tss.cpp
│   │       │   │   │   ├── machine_check.cpp
│   │       │   │   │   ├── nmi_handler.cpp
│   │       │   │   │   ├── overflow.cpp
│   │       │   │   │   ├── pf_handler.cpp
│   │       │   │   │   ├── segment_not_present.cpp
│   │       │   │   │   ├── simd_floating_point_exception.cpp
│   │       │   │   │   ├── stack_segment_fault.cpp
│   │       │   │   │   ├── virtualization_exception.cpp
│   │       │   │   │   └── x87_fpu_floating_point_error.cpp
│   │       │   │   └── Routine
│   │       │   │       ├── ata_handler.cpp
│   │       │   │       ├── clock_handler.cpp
│   │       │   │       ├── keyboard_handler.cpp
│   │       │   │       └── timer_handler.cpp
│   │       │   ├── HardwareInterrupts
│   │       │   │   ├── hardware_interrupt.cpp
│   │       │   │   ├── InterruptController
│   │       │   │   │   ├── 8259_pic.cpp
│   │       │   │   │   ├── apic.cpp
│   │       │   │   │   ├── ioapic.cpp
│   │       │   │   │   └── x2apic.cpp
│   │       │   │   ├── tick_manager.cpp
│   │       │   │   ├── TimerController
│   │       │   │   │   ├── apic_timer.cpp
│   │       │   │   │   ├── hpet.cpp
│   │       │   │   │   └── pit.cpp
│   │       │   │   └── timer_interrupt.cpp
│   │       │   ├── interrupt_controller.cpp
│   │       │   ├── interrupt_dispatch.cpp
│   │       │   └── interrupt_stub.asm
│   │       ├── Memory
│   │       │   ├── invalid_tlb.asm
│   │       │   ├── read_on_cr3.asm
│   │       │   └── write_on_cr3.asm
│   │       ├── Panic
│   │       │   └── Panic.cpp
│   │       ├── Scheduler
│   │       │   ├── context_switch.asm
│   │       │   └── enter_user_mode.asm
│   │       ├── Section
│   │       │   ├── bss.asm
│   │       │   ├── multiboot2.asm
│   │       │   └── rodata.asm
│   │       ├── Segments
│   │       │   ├── flush_gdt.asm
│   │       │   ├── flush_tss.asm
│   │       │   └── gdt.cpp
│   │       └── Syscall
│   │           ├── syscall_init.cpp
│   │           └── syscall_stub.asm
│   ├── Boot
│   │   ├── boot_info.cpp
│   │   ├── kernel_entry.cpp
│   │   └── Multiboot
│   │       └── kmain.cpp
│   ├── Clock
│   │   ├── ClockController
│   │   │   ├── cmos.cpp
│   │   │   ├── datetime.cpp
│   │   │   └── rtc.cpp
│   │   └── clock_manager.cpp
│   ├── Driver
│   │   ├── Graphics
│   │   ├── Keyboard
│   │   │   └── ps2_keyboard.cpp
│   │   ├── Serial
│   │   │   └── serial_port.cpp
│   │   ├── Storage
│   │   │   ├── Ata
│   │   │   │   ├── ata_controller.cpp
│   │   │   │   ├── ata_device.cpp
│   │   │   │   └── pio_strategy.cpp
│   │   │   ├── Partitions
│   │   │   │   ├── Gpt
│   │   │   │   │   └── gpt.cpp
│   │   │   │   ├── Mbr
│   │   │   │   │   └── mbr.cpp
│   │   │   │   ├── partition.cpp
│   │   │   │   └── partition_manager.cpp
│   │   │   ├── storage_cache.cpp
│   │   │   └── storage_device.cpp
│   │   └── Vga
│   │       ├── Display
│   │       │   ├── display_framebuffer.cpp
│   │       │   └── display_text.cpp
│   │       └── display.cpp
│   ├── Fs
│   │   ├── DebugFs
│   │   │   └── debug_fs.cpp
│   │   ├── DevFs
│   │   │   ├── dev_fs.cpp
│   │   │   └── tty.cpp
│   │   ├── Fat12
│   │   │   ├── fat_12_fs.cpp
│   │   │   └── fat_12_node.cpp
│   │   ├── Fat16
│   │   │   └── fat_16_fs.cpp
│   │   ├── Fat32
│   │   │   └── fat_32_fs.cpp
│   │   ├── ProcFs
│   │   │   └── proc_fs.cpp
│   │   ├── RamDisk
│   │   │   └── ram_disk.cpp
│   │   ├── TmpFs
│   │   │   └── tmp_fs.cpp
│   │   └── Vfs
│   │       ├── auto_mounter.cpp
│   │       ├── file_description.cpp
│   │       ├── Fstab.cpp
│   │       └── virtual_filesystem.cpp
│   ├── Hardware
│   │   ├── Acpi
│   │   │   └── acpi.cpp
│   │   ├── Cpu
│   │   │   ├── cpu_context.cpp
│   │   │   ├── cpu.cpp
│   │   │   └── cpu_register.cpp
│   │   ├── Fadt
│   │   │   └── fadt_manager.cpp
│   │   ├── Madt
│   │   │   └── madt.cpp
│   │   └── Pci
│   │       └── pci.cpp
│   ├── Init
│   │   └── init.cpp
│   ├── Ipc
│   │   ├── endpoint.cpp
│   │   ├── notification.cpp
│   │   └── signal_delivery.cpp
│   ├── Loader
│   │   └── elf_loader.cpp
│   ├── Memory
│   │   ├── memory_manager.cpp
│   │   ├── ObjectMemory
│   │   │   └── Zone
│   │   │       └── zone_allocator.cpp
│   │   ├── PhysicalMemory
│   │   │   ├── Buddy
│   │   │   │   ├── buddy_allocator.cpp
│   │   │   │   └── buddy_state.cpp
│   │   │   └── physical_memory_manager.cpp
│   │   └── VirtualMemory
│   │       └── virtual_memory_manager.cpp
│   ├── Posix
│   │   ├── errno.cpp
│   │   └── time.cpp
│   ├── Scheduler
│   │   ├── scheduler.cpp
│   │   ├── start_user_task.cpp
│   │   └── Task
│   │       └── task.cpp
│   └── Syscall
│       ├── syscall.cpp
│       └── SyscallList
│           ├── accept.cpp
│           ├── arch_prctl.cpp
│           ├── bind.cpp
│           ├── brk.cpp
│           ├── chdir.cpp
│           ├── close.cpp
│           ├── connect.cpp
│           ├── dup2.cpp
│           ├── execve.cpp
│           ├── exit.cpp
│           ├── fcntl.cpp
│           ├── fork.cpp
│           ├── fstat.cpp
│           ├── get_cwd.cpp
│           ├── get_dents64.cpp
│           ├── get_pid.cpp
│           ├── get_uid.cpp
│           ├── ioctl.cpp
│           ├── ipc_call.cpp
│           ├── ipc_receive.cpp
│           ├── ipc_send.cpp
│           ├── kill.cpp
│           ├── listen.cpp
│           ├── lseek.cpp
│           ├── lstat.cpp
│           ├── mkdir.cpp
│           ├── mmap.cpp
│           ├── newfstatat.cpp
│           ├── open.cpp
│           ├── read.cpp
│           ├── set_tid_address.cpp
│           ├── sig_action.cpp
│           ├── sig_proc_mask.cpp
│           ├── sig_rt_sigsuspend.cpp
│           ├── socket.cpp
│           ├── stat.cpp
│           ├── time.cpp
│           ├── uname.cpp
│           ├── wait4.cpp
│           ├── write.cpp
│           └── yield.cpp
├── LibC
│   ├── assert.c
│   ├── ctype.c
│   ├── stdio
│   │   ├── _impl
│   │   │   └── libc_putc.cpp
│   │   ├── kprintf.c
│   │   ├── snprintf.c
│   │   └── vsnprintf.c
│   └── string
│       ├── atoi.c
│       ├── itoa.c
│       ├── memcmp.c
│       ├── memcpy.c
│       ├── memmove.c
│       ├── memset.c
│       ├── stol.c
│       ├── strcat.c
│       ├── strchr.c
│       ├── strcmp.c
│       ├── strcpy.c
│       ├── string_data.c
│       ├── strlen.c
│       ├── strnchr.c
│       ├── strncmp.c
│       ├── strncpy.c
│       ├── strnlen.c
│       ├── strrchr.c
│       ├── strtok.c
│       └── ultoa.c
├── LibFK
│   ├── Algorithms
│   │   ├── crc32.cpp
│   │   └── djb2.cpp
│   ├── Container
│   │   └── intrusive_list.cpp
│   ├── cxxabi.cpp
│   ├── Memory
│   │   ├── heap_malloc.cpp
│   │   └── new.cpp
│   └── Text
│       ├── string_builder.cpp
│       └── string.cpp
├── Toolchain
│   ├── busybox
│   │   └── build.lua
│   └── musl
│       └── build.lua
└── Userland
    ├── Include
    │   └── sys
    ├── init
    │   └── main.asm
    ├── lib
    │   ├── crt0.asm
    │   ├── fk_user.h
    │   ├── syscalls.asm
    │   └── syscalls.inc
    ├── linker.ld
    └── shell
        ├── main.asm
        └── main.c

157 directories, 408 files
```

- **One file per struct/class** ensures **high cohesion**, **low coupling**, and **auditability**.
- **Input/Output validation** must be present in every public API.
- Following PascalCase + camel-case makes the structure **intuitive and GEMINI-parsable**.

### 13. Validation Rules

- **Input validation**: All public methods must check arguments, range, and nullability.
- **Output validation**: Use `Result<T, Error>` or `Optional<T>` consistently.
- Example:

```cpp
auto proc = TRY(Process::find_by_pid(pid));
if (!proc.has_value())
    return Error::ProcessNotFound;

auto page = TRY(Physical::allocate_page());
```

- Enforces predictable behavior, early error propagation, and secure kernel state.

### 14. GEMINI Script Management & Memory System

- **Memory System**: GEMINI must maintain conceptual memory in `.ai-docs/` for all recent modifications and architectural decisions
- **Documentation Pipeline**: 
  - `.ai-docs/` → **Internal AI memory** (conceptual knowledge, recent changes, architectural decisions)
  - `Docs/` → **Human documentation** (comprehensive guides with descriptive names)
- **Continuous Learning**: GEMINI must read `.ai-docs/` as primary memory for understanding current state
- **Script Generation**: Generate Lua scripts automatically based on kernel, library, and userland analysis  
- **Integration**: Generated scripts integrated into kernel build, testing, and deployment pipelines  
- **Full Traceability**: All modifications documented in both `.ai-docs/` and `Docs/` for complete audit trail
- **Memory Updates**: Every significant change must update `.ai-docs/` with conceptual understanding
- **Documentation Creation**: Create comprehensive human documentation in `Docs/` with descriptive, non-README names

### 15. Algorithm Consolidation Rule

**🔑 IMPORTANT**: Move all known algorithms used across multiple kernel domains to `LibFK/Algorithms/` for maximum reusability.

#### Algorithm Categories to Consolidate

- **Archive Algorithms**: TAR, ZIP, GZIP (used in Filesystem, Loader, Userland)
- **Compression**: LZ4, ZLIB, DEFLATE (used in Storage, Network, Filesystem)
- **Checksum/Hash**: CRC32, MD5, SHA256 (used in Storage, Network, Security)
- **Encoding**: Base64, Hex, URL encoding (used in Network, Filesystem, IPC)
- **Data Structures**: Priority queues, Bloom filters, LRU caches (used across all domains)
- **Parsing**: INI, JSON, ELF, PE (used in Loader, Config, Debug)

#### Consolidation Process

1. **Identify**: Find duplicate algorithms across kernel domains
2. **Extract**: Move algorithm implementations to `Include/LibFK/Algorithms/` and `Src/LibFK/Algorithms/`
3. **Generalize**: Make algorithms domain-agnostic with proper interfaces
4. **Update**: Replace domain-specific implementations with LibFK calls
5. **Test**: Ensure consolidated algorithms work across all use cases

#### Benefits

- **Code Reuse**: Single implementation for all domains
- **Maintenance**: Centralized bug fixes and optimizations
- **Consistency**: Same behavior across all kernel components
- **Testing**: One comprehensive test suite instead of scattered tests
- **Performance**: Shared optimizations benefit all domains

#### Examples

```cpp
// ❌ Before: Multiple TAR implementations
// Src/Kernel/Fs/Vfs/tar_parser.cpp     // VFS-specific
// Src/Kernel/Loader/elf_tar_extractor.cpp // Loader-specific
// Src/Userland/Shell/tar_command.cpp     // Userland-specific

// ✅ After: Single consolidated implementation
// Include/LibFK/Algorithms/tar.h          // Unified interface
// Src/LibFK/Algorithms/tar.cpp          // Single implementation
// All domains use: fk::Algorithms::Tar::extract(...)
```

**This rule eliminates code duplication and ensures algorithm consistency across FKernel.**

#### Memory Structure Requirements

```
.ai-docs/
├── architectural-decisions/     # High-level design decisions
├── recent-modifications/        # Track recent code changes
├── conceptual-models/           # How the system works conceptually  
├── domain-knowledge/            # Per-domain understanding
└── development-patterns/        # Established patterns and conventions

Docs/
├── Architecture/               # System architecture documentation
├── Development/               # Development guides and workflows  
├── Domains/                   # Per-domain comprehensive guides
├── Patterns/                  # Design patterns and conventions
└── Tutorials/                 # Learning materials for developers
```
