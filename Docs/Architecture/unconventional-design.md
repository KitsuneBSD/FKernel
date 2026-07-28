# Unconventional Design Decisions

FKernel deliberately cherry-picks ideas from multiple traditions: Rust's error handling, seL4's capability model, Smalltalk's coding discipline, BSD's event mechanism, and domain-driven design. None of these are typical in a hybrid C++ kernel. This document explains the non-obvious choices and why they exist.

## 1. Rust-Style Error Handling in a C++ Kernel

FKernel uses `Result<T, Error>` and `TRY` macro instead of C++ exceptions (disabled via `-fno-exceptions`) or raw error codes. This is inspired by Rust's `Result<T, E>` type.

- `Result<T, E>` uses `fk::memory::Optional<T>` internally for the value
- `TRY(expression)` macro propagates errors via GCC statement expressions
- `Optional<T>` for nullable values
- No exceptions, no RTTI, no hidden control flow
- Eliminates unchecked return values and null dereferences

Example:
```cpp
Result<Page*, Error> allocate_page();
auto page = TRY(allocate_page());  // Propagates error on failure
```

Key files: `Include/LibFK/Core/Result.h`, `Include/LibFK/Memory/optional.h`

## 2. seL4-Style Capability-Based IPC in a Hybrid Kernel

Most hybrid kernels use traditional Unix IPC (pipes, shared memory, signals). FKernel implements seL4-style capabilities with CSpace (capability space), endpoints, notifications, and rights-based access control.

- **Capability**: Typed (Endpoint/Notification/SharedMemory) + rights (Send/Receive/Manage)
- **CSpace**: Per-process capability space mapping slots to capabilities
- **Revocation**: Generation counter mechanism — when an IPC object is destroyed, its generation increments, invalidating all capabilities pointing to it
- **Rights decomposition**: `with_rights()` creates a derived capability with restricted rights

This provides fine-grained access control properties uncommon in traditional Unix IPC, even though it runs in a hybrid (not microkernel) architecture.

Key files: `Include/Kernel/Ipc/capability.h`, `Include/Kernel/Ipc/cspace.h`, `Include/Kernel/Ipc/endpoint.h`

## 3. Object Calisthenics Enforcement

The codebase follows strict Smalltalk-inspired coding rules — unconventional in kernel development:

- **No `else` keyword**: Early returns only
- **Max 2 instance variables per class**: Compose objects instead
- **Max 20 lines per method**: Extract helper methods
- **Max 200 lines per class**: Keep entities small
- **No getters/setters**: `is_running()` / `block()` not `state()` / `set_state()`
- **One dot per line**: `process->thread_name()` not `process->thread()->name()`
- **No abbreviations**: `ProcessManager` not `ProcMgr`
- **Wrap all primitives**: `ProcessId` not `int`, `BlockSize` not `u64`

This is non-negotiable and enforced by automated validators.

## 4. NVMe Hyper-Decomposition (One-Class-Per-File Extreme)

The NVMe driver is the most extreme application of the "one struct/class per file" rule: **19 header files and 14 source files** for a single storage controller.

Each concept gets its own class:
- `NvmeCommandIdManager` — tracks command IDs
- `NvmeInterruptLine` — manages a single interrupt line
- `NvmePendingOperations` — tracks in-flight operations
- `NvmeQueueSetup` — configures submission/completion queues
- `NvmeRegisterMapper` — maps MMIO registers
- `NvmeCompletionProcessor` — processes completion queue entries
- `NvmeInterruptConfigurator` — configures MSI-X
- `NvmeDeviceConfiguration` — stores device configuration
- `NvmeControllerState` — state machine

Each class is small (50-150 lines), single-responsibility, and independently testable.

Key files: `Include/Kernel/Driver/Storage/Nvme/` (all 19 headers)

## 5. Allocator Backend Injection Pattern

LibFK (the STL-like library) must remain independent of the Kernel. But it needs dynamic memory allocation. The solution: a callback-based allocator backend.

```cpp
// Include/LibFK/Memory/allocator_backend.h
struct AllocatorBackend {
  void *(*allocate)(size_t size);
  void *(*reallocate)(void *ptr, size_t size);
  void (*free)(void *ptr);
};
```

The Kernel sets the backend during early init:
```cpp
fk::memory::set_allocator_backend(&kernel_allocator);
```

LibFK containers and smart pointers call through the backend interface, never including Kernel headers. This maintains strict layer separation (LibC → LibFK → Kernel).

**Known violation**: `Src/LibFK/Memory/heap_malloc.cpp` directly includes Kernel headers, breaking this pattern. Tracked in TODO.md.

## 6. Domain-Driven ELF Loader

Most ELF loaders are monolithic functions. FKernel decomposes loading into 5 domain objects:

- **ParserDomain**: Parses ELF headers, program headers, section headers
- **LoadDomain**: Loads segments into memory at correct virtual addresses
- **DynamicDomain**: Handles PT_DYNAMIC, PLT/GOT relocation
- **MemoryDomain**: Manages memory region allocation and permission bits
- **InterpreterDomain**: Loads dynamic linker (ld-linux.so equivalent)

Each domain is in its own file (one class per file), making the loader independently testable and extensible. The `ElfLoader` class coordinates them via a pipeline.

Key files: `Include/Kernel/Loader/Domains/`, `Src/Kernel/Loader/Domains/`

## 7. PIC→IOAPIC Hot-Swap with IRQ State Tracking

During boot, the interrupt controller switches from legacy 8259 PIC to IOAPIC. `HardwareInterruptManager` tracks unmasked IRQs in a `m_unmasked_irqs` bitmask. When the controller switches (after memory manager init), all previously unmasked IRQs are re-applied automatically.

This allows:
- Boot with PIC (no memory management needed)
- Seamless transition to IOAPIC (after paging enabled)
- No lost IRQ state during the transition

## 8. Phase-Guarded Interrupt Dispatch

The interrupt dispatch path runs on EVERY exception, including faults during early boot before hardware is initialized. Accessing APIC MMIO before it's mapped causes triple faults.

Solution: Every hardware access in the dispatch path is guarded:
```cpp
if (SchedulerManager::the().is_initialized() &&
    SchedulerManager::the().is_need_resched()) {
  SchedulerManager::the().schedule();
}
```

The same applies to `current_processor()` — falls back to `m_processors[0]` before APIC is ready.

## 9. kqueue Over epoll

Despite using Linux syscall ABI, the kernel implements BSD kqueue instead of Linux epoll. This provides:
- Unified event mechanism (files, signals, timers, processes in one call)
- Better scalability (O(1) notification vs O(n) polling)
- Simpler kernel implementation

The trade-off: musl/BusyBox expect epoll. A compatibility shim maps epoll syscalls to kqueue internally.

Key files: `Include/Kernel/Fs/Vfs/kqueue.h`, `Src/Kernel/Fs/Vfs/kqueue.cpp`

## 10. IntrusiveList for Zero-Overhead Scheduler Queues

Scheduler queues (wait, sleep, zombie) use `fk::containers::IntrusiveList` — a linked list where the next/prev pointers are embedded directly in the `Task` struct via pointer-to-member.

```cpp
IntrusiveList<Task, &Task::wait_node> m_wait_queue;
```

This means:
- No allocation needed for queue operations
- No separate node objects
- Cache-friendly traversal
- O(1) insert/remove

## 11. Type Wrappers for Preventing Accidental Swaps

Every domain-specific integer is wrapped in a strong type:
- `ProcessId` (not `uint64_t`)
- `PhysicalAddress` / `VirtualAddress` (not `uintptr_t`)
- `SectorSize` / `SectorCount` (not `size_t`)
- `BuddyOrder` (not `uint8_t`)
- `FrameIndex` (not `uint64_t`)

These compile to zero overhead but prevent accidental type mixing at compile time.

Key files: `Include/LibFK/Types/` (11 type wrapper headers)

## 12. 4-Layer Logging Pipeline

The logging system has 4 distinct layers:

1. **klog/kwarn/kerror** (LibFK) — formatted log with level and prefix
2. **kprintf** (LibC) — vsprintf to 512-byte stack buffer
3. **libc_puts** (LibC) — hook-based dispatch with SpinlockIRQ protection
4. **kernel_puts_impl** (Kernel) — fan-out to 3 targets: Serial (COM1), Display (VGA/framebuffer), DebugFS (ring buffer)

Log targets are controlled by a bitmask and can be changed at runtime. This extreme decomposition allows:
- Serial-only logging during early boot
- Display logging after framebuffer init
- DebugFS ring buffer for dmesg
- SpinlockIRQ protection for concurrent output

## 13. Dual-Inheritance Storage Drivers

Storage controllers like NVMe and AHCI inherit from both `Driver` and `StorageDevice`:

```cpp
class NVMeController final : public Driver, public StorageDevice { ... };
```

This means a single object serves as both:
- A PCI-matched driver (probed via driver registry)
- A block device (registered in VFS)

The alternative (separate driver and device objects) would require indirection and lifetime management complexity.

## 14. Syscall Organization by Domain

Rather than one monolithic syscall dispatch file, each syscall family lives in its own subdirectory:

| Directory | Count | Examples |
|-----------|-------|---------|
| FileSystem/ | 52 | open, read, write, mount, epoll, kqueue |
| Process/ | 35 | fork, execve, clone, wait4, setpgid |
| Networking/ | 16 | socket, bind, connect, sendmsg |
| Memory/ | 6 | mmap, mprotect, brk |
| Time/ | 7 | clock_gettime, nanosleep |
| Signals/ | 5 | tgkill, sigaltstack |
| Posix/ | 3 | futex, openpty |
| System/ | 4 | uname, reboot, getrandom |
| Ipc/ | 4 | ipc_call, ipc_send |
| Terminal/ | 3 | tty_create, tty_list |

Each file is self-contained with a single handler function.

## 15. Dual C/C++ in LibC

LibC is predominantly `.c` files but has one `.cpp` file: `Src/LibC/stdio/_impl/libc_putc.cpp`. This file bridges LibC output to LibFK's logging system. It's a deliberate exception to the "LibC is pure C" rule, allowed because it's the only way to route printf output without duplicating the logging infrastructure.

Similarly, `Src/Kernel/Io/kernel_puts.cpp` and `Src/Kernel/Arch/x86_64/Panic/Panic.cpp` are the only two Kernel files allowed to include LibC directly.
