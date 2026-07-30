# FKernel Development Guide

## Project Identity

FKernel is a **POSIX-compatible x86_64 hobby kernel** with BSD-inspired internals and seL4-style capability IPC. MockOS is a test harness (BusyBox + musl ISO) for validating syscall compatibility — it is NOT an operating system. BusyBox and musl are validation tools, not the project's "userspace."

## Build Commands

```bash
xmake                           # Build kernel
xmake run                      # Run in QEMU (MockOS test ISO)
xmake clean                    # Clean build artifacts
xmake -bv Test                 # Build tests (verbose)
xmake run Test                 # Run all tests (no single test target)
xmake setup-hda                # Create disk image
xmake build-initrd             # Build initrd
xmake config-initrd            # Configure initrd interactively
xmake analyze                  # Analyze kernel runtime
```

Build outputs: `build/FKernel.bin`, `build/FKernel-MockOS.iso`, `build/FKernel-HDA.qcow2`.

## Layer Separation (STRICT)

1. **LibC** - Freestanding C library (strings, memory, types)
2. **LibFK** - STL-like library, depends on LibC + self only
3. **Kernel** - Depends on LibFK only, NEVER LibC directly

```
LibC (std types) → LibFK (std containers) → Kernel (drivers, scheduling, VFS, IPC)
```

Violations are build errors. LibFK defines the boundary between LibC and Kernel.

### Allowed LibC includes in Kernel (Exceptions)

Only these two files may include LibC directly:
- `Src/Kernel/Io/kernel_puts.cpp` — implements kprintf output routing
- `Src/Kernel/Arch/x86_64/Panic/Panic.cpp` — panic handler (runs before logging init)

### LibFK ↔ Kernel Boundary

LibFK MUST NOT include Kernel headers. Use the allocator backend pattern
(`LibFK/Memory/allocator_backend.h`) for Kernel-injected callbacks.

### Enforcement

```bash
xmake check-layers    # Verify layer separation
```

### LibFK Equivalents for LibC

| LibC | LibFK Equivalent |
|------|-----------------|
| `<LibC/stdint.h>` | `<LibFK/Types/types.h>` |
| `<LibC/stddef.h>` | `<LibFK/Types/types.h>` |
| `<LibC/string.h>` | `<LibFK/Utilities/Memory.h>` |
| `<LibC/stdio.h>` | `<LibFK/Algorithms/log.h>` |
| `<LibC/stdarg.h>` | `<LibFK/Algorithms/log.h>` |
| `memcpy/memset/memmove/memcmp` | `fk::memory::copy/set/compare/move` |
| `strlen/strcmp/strcpy/strcat` | `fk::memory::length/compare/copy_string/concatenate` |
| `strncpy/strncmp/strncat` | `fk::memory::copy_n/compare_n/concatenate_n` |
| `kprintf()` | `fk::algorithms::klog/kwarn/kerror` |
| `snprintf()` | `fk::algorithms::format()` |

## Namespace Structure

```cpp
// LibFK
namespace fk::containers { }   // Vector, HashMap, List
namespace fk::text { }         // String, StringBuilder
namespace fk::memory { }       // Smart pointers
namespace fk::core { }         // Result, Error
namespace fk::algorithms { }   // CRC32, DJB2

// Kernel — most types are in global scope or `namespace fkernel { }`
// without sub-namespace. Only structured sub-namespaces listed below.
namespace boot { }             // BootInfo
namespace fkernel { }          // PCI, ACPI, VFS, Loader, Syscall, IOMMU...
namespace fkernel::io { }      // kernel_puts, serial
namespace fkernel::ipc { }     // endpoint, capability, cspace
namespace fkernel::scheduler { } // turnstile, mlfq_queue, qos
namespace fkernel::net { }     // network_stack
namespace fkernel::terminal { }// terminal
namespace fkernel::elf_domains { } // dynamic_domain
namespace fkernel::vtd { }     // Intel IOMMU (arch/)
```

## Object Calisthenics (Mandatory)

| Rule | Example |
|---|---|
| 1 indentation level per method | Extract helper methods, no nested `if` in loops |
| No `else` | Early returns instead |
| Wrap all primitives | `ProcessId` not `int`, `BlockSize` not `u64` |
| First-class collections | `ProcessList` class, not raw `Vector` |
| 1 dot per line | `process->thread_name()` not `process->thread()->name()` |
| No abbreviations | `ProcessManager` not `ProcMgr` |
| Keep entities small | Class ≤200 lines, method ≤20 lines, file ≤500 lines |
| Max 2 instance variables | Compose objects instead |
| No getters/setters | `is_running()` / `block()` not `state()` / `set_state()` |

**SECRET RULE**: One struct/class per file. File name matches class name.

**Nested classes, structs, and enums are FORBIDDEN.** Each type must
live in its own file. Known tech debt: `task.h` (6+ types in one file,
including nested `Control`/`Resources`), `dynamic_domain.h` (nested
`RelaTable`/`SymbolContext`), `boot_timer.h` (nested `Mark`).

## Subsystem Manager Pattern (Mandatory)

All kernel subsystem singletons must follow this canonical form:

```cpp
namespace fkernel {

class FooManager {
  bool m_is_initialized{false};

  FooManager() = default;
  FooManager(const FooManager &) = delete;
  FooManager &operator=(const FooManager &) = delete;

public:
  static FooManager &the() {
    static FooManager inst;
    return inst;
  }

  bool is_initialized() const { return m_is_initialized; }
  void initialize(); // must set m_is_initialized = true at end
};

} // namespace fkernel
using fkernel::FooManager; // backward-compat alias
```

Rules:
- Private default constructor + deleted copy/move (no external instantiation)
- `is_initialized()` lets boot code assert on success
- All managers live in `fkernel::` namespace
- `initialize()` must guard against double-init and set `m_is_initialized = true` at end
- Boot flow (`init.cpp`) must assert `is_initialized()` after each `initialize()` call

## Architecture Portability

FKernel uses `extern "C"` functions with the `arch_` prefix as its
portability layer, declared in `<Kernel/Arch/<arch>/Hardware/Cpu/cpu_ops.h>`.

### Available arch_* Functions

| Function | Purpose |
|----------|---------|
| `arch_disable_interrupts()` | CLI — generic code MUST NOT use `asm("cli")` |
| `arch_enable_interrupts()` | STI — generic code MUST NOT use `asm("sti")` |
| `arch_save_flags_and_disable()` | PUSHFQ + CLI |
| `arch_restore_flags(flags)` | POPFQ |
| `arch_halt_loop()` | HLT loop ([[noreturn]]) |
| `arch_cpu_relax()` | PAUSE |
| `arch_cpuid()` | CPUID instruction |
| `arch_read_msr()` / `arch_write_msr()` | RDMSR / WRMSR |
| `arch_enable_cpu_features()` | CR0/CR4, NX, SMEP/SMAP, XSAVE, ERMSB |
| `arch_smap_begin()` / `arch_smap_end()` | STAC / CLAC |
| `arch_triple_fault()` | LIDT + INT3 panic |
| `detect_tsc_frequency()` | CPUID 0x15/0x16 TSC calibration |
| `arch_cpu_idle()` _(Phase 42)_ | TODO: extract `sti; hlt` from scheduler |

### Rules

1. Generic code MUST call `arch_*()` functions, NOT inline `asm()`.
2. New arch-specific primitives MUST follow `arch_` + `extern "C"` pattern.
3. Implementations live in `Src/Kernel/Arch/<arch>/<subsystem>/`.
4. See TODO.md Phase 42 for remaining extractions (scheduler, VMM, init.cpp).

## Architecture Change Policy (Mandatory)

Before changing existing architecture:

1. **Search for an existing abstraction** — grep the codebase for similar patterns, interfaces, or utilities.
2. **Reuse it if possible** — prefer composition or extension over creating new constructs.
3. **If introducing a new abstraction, justify**:
   - Why existing abstractions are insufficient.
   - Trade-offs (complexity, performance, maintenance burden).
   - Impact on layering (LibC/LibFK/Kernel boundaries).
   - Migration path from current code to the new abstraction.

Violations of this policy must be called out in code review.

## Error Handling

```cpp
Result<Page*, Error> allocate_page();        // Fallible operations
auto page = TRY(allocate_page());            // Propagate errors
fk::Optional<Process*> find_process(ProcessId pid);  // Nullable values
```

## Coding Style

- **Classes**: PascalCase (`ProcessManager`)
- **Methods**: snake_case (`initialize_processes()`)
- **Members**: `m_` prefix (`m_process_count`)
- **Constants**: UPPER_SNAKE_CASE (`MAX_PROCESSES`)
- **Namespaces**: snake_case (`fkernel::memory`)
- **Directories**: PascalCase (`Src/Kernel/Posix/`)
- **Files**: snake_case (`topology_manager.cpp`)
- 2-space indentation, opening braces on same line
- No `<iostream>`, `<vector>`, `<string>` — use LibFK equivalents
- No exceptions, no RTTI. Use `Result<T, Error>` for errors
- Use `volatile` for hardware registers; memory barriers when needed
- RAII preferred, stack allocation preferred, `OwnPtr`/`RefPtr` for heap

### File Naming (ENFORCED)

All source and header files MUST use `snake_case` naming. No exceptions.

| Convention | Examples | Wrong |
|------------|----------|-------|
| Headers | `error.h`, `string_builder.h`, `process_id.h` | `Error.h`, `StringBuilder.h`, `processId.h` |
| Sources | `scheduler_manager.cpp`, `tcp_socket.cpp` | `SchedulerManager.cpp`, `TcpSocket.cpp` |

- Use `git mv` for renames to preserve history
- After renaming, update ALL `#include` directives that reference the old name
- One struct/class per file. File name matches class name in snake_case
- Directory cleanup tracked in TODO.md Phase 22

## Include Order

1. System headers (LibC)
2. LibFK headers
3. Kernel headers
4. Local headers

## Project Structure

```
Src/Kernel/        # Kernel implementation
Src/LibC/          # Freestanding C library
Src/LibFK/         # STL-like library
Include/           # All headers (mirrors Src/)
Meta/              # Build tools and scripts
Docs/              # Documentation
tests/             # Unit tests (pattern: tests/[component]/test_[module].cpp)
.ai-docs/          # AI conceptual memory
.gemini/           # Validation scripts (tooling, not docs)
```

Full directory tree: `Docs/directory-structure.md`.

## Testing

- LibC: 90%+ coverage required
- LibFK: 85%+ coverage required
- Kernel: **0% coverage today** — Phase 43 target: 75%+ for critical paths
- All public APIs must have tests
- New LibFK/LibC PRs require test additions
- Kernel changes MUST include regression tests for the modified subsystem (when test infrastructure exists)
- Test for edge cases: null inputs, boundary values, error paths, concurrent access
- Framework: custom (see `tests/test_framework.h`)
- MockOS (BusyBox + musl ISO) validates syscall correctness at integration level

## Algorithm Consolidation

All algorithms used across multiple domains belong in `LibFK/Algorithms/`:
- Archive: TAR, ZIP, GZIP
- Compression: LZ4, ZLIB, DEFLATE
- Checksum/Hash: CRC32, MD5, SHA256
- Encoding: Base64, Hex, URL
- Parsing: INI, JSON, ELF, PE

## Hardware Notes

- Compiler flags: `-ffreestanding`, `-fno-exceptions`, `-fno-rtti`, `-mcmodel=kernel`, `-mno-sse`
- Use `volatile` for MMIO, `__sync_synchronize()` for memory barriers
- Document against hardware specifications

## Kernel Logging

### Log Functions

| Function | Use Case | Behavior |
|----------|----------|----------|
| `fk::algorithms::klog(PREFIX, fmt, ...)` | Normal operations, init | Returns |
| `fk::algorithms::kwarn(PREFIX, fmt, ...)` | Warnings, degraded ops | Returns |
| `fk::algorithms::kerror(PREFIX, fmt, ...)` | Unrecoverable errors | **Halts CPU** (proposed: split into `kfatal()` halt + `kerror()` non-halting) |
| `fk::algorithms::kdebug(PREFIX, fmt, ...)` | Debug diagnostics | Returns |
| `fk::algorithms::kexception(PREFIX, fmt, ...)` | Exception handlers | Returns |

### Prefix Convention

```cpp
// ✅ Good — UPPER_SNAKE_CASE, max 20 chars
fk::algorithms::klog("SCHEDULER", "Task %d migrated to CPU %d", pid, cpu);
fk::algorithms::kwarn("NVME", "Sector size mismatch: expected %u", expected);
fk::algorithms::kerror("MEMORY", "Page alloc failed: order=%u", order);

// ❌ Bad — mixed case, hyphens, lowercase
fk::algorithms::klog("Scheduler Manager", ...);  // mixed case
fk::algorithms::klog("NVMe-REG", ...);           // hyphen
fk::algorithms::klog("sys_reboot", ...);          // lowercase
```

### Rules

1. **Never use raw `kprintf()` in kernel code** — always use `fk::algorithms::klog/kwarn/kerror`
2. **Never use `kerror()` for recoverable errors** — it halts; use `kwarn()` instead
3. **Include context in error messages** — error code, path, relevant values
4. **Log at INFO level during subsystem init** — helps debug boot order issues
5. **Use `kdebug()` for hot paths** — may be stripped in release builds
6. **Route panic through logging** — ensure panic messages reach dmesg ring buffer

### Log Targets

Controlled by bitmask: `Serial` (COM1), `Display` (VGA), `DebugFS` (ring buffer).

### References

- `Include/LibFK/Algorithms/log.h` — log function definitions
- `Docs/Kernel/Logging/README.md` — full architecture
- `.ai-docs/development-patterns/kernel-logging.md` — AI agent conventions

## Documentation Maintenance

### Docs/ Structure
```
Docs/
├── Architecture/     # system-overview.md, design-philosophy.md
├── Domains/          # Per-subsystem guides (vfs, memory, scheduling, etc.)
├── Kernel/           # Per-subsystem README stubs (Boot, Process, Syscalls, VFS)
└── directory-structure.md
```

### Rules for AI Agents
- **Diagrams**: Use Mermaid for ALL architecture/flow diagrams. No ASCII art in Docs/.
- **Math**: Use MathJax (`$$...$$`) for algorithms with mathematical formulas.
- **Accuracy**: Docs must match actual source code. When in doubt, read the source first.
- **Staleness**: When modifying kernel subsystems, update the corresponding Docs/ file in the same session.
- **Cross-references**: Link between related Docs/ files using relative markdown links.
- **Completeness**: Fill empty README stubs with actual content (architecture, key files, data flow).

### Docs/ Domain Files
| File | Subsystem | Key Diagrams |
|------|-----------|-------------|
| `process-scheduling.md` | Scheduler | State machine, work-stealing, context switch |
| `vfs-architecture.md` | VFS | Layer architecture, path resolution, mount overlay |
| `ipc-capabilities.md` | IPC | Capability model, revocation, signal delivery |
| `memory-management-guide.md` | Memory | Init flow, buddy allocator, page tables |
| `drivers-framework.md` | Drivers | PCI matching, storage stack, hardware discovery |
| `networking.md` | Network | TCP/IP stack, TCP state machine |
| `elf-loader.md` | ELF | Loading pipeline, security features |

### When Updating Docs/
1. Read the corresponding source files first
2. Replace ASCII diagrams with Mermaid
3. Add MathJax for mathematical algorithms
4. Update status/percentages to match reality
5. Add new components discovered in source
6. Remove stale information

## AI Agent Instructions

- `CLAUDE.md` is a proxy that loads `AGENTS.md` — always read `AGENTS.md` first
- Read `.ai-docs/` before making changes; update it for significant modifications
- Layer separation is non-negotiable — never add LibC calls in Kernel code
- **Architecture changes require justification** — search existing abstractions first (see "Architecture Change Policy")
- Scheduler runs at CPL 0 with interrupts disabled; shared data needs interrupt-safe locking
- Every struct/class goes in its own file (SECRET RULE)
- **Always update Docs/** when modifying kernel subsystems — keep documentation in sync
- **Always write tests** when modifying kernel code — regression tests for the modified subsystem
- FKernel is a **kernel**, not an operating system. MockOS is a test harness, not "userspace." Do not use OS framing in new documentation.
