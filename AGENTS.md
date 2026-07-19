# FKernel Development Guide

## Build Commands

```bash
xmake                           # Build kernel
xmake run                      # Run in QEMU
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

## Namespace Structure

```cpp
// LibFK
namespace fk::containers { }   // Vector, HashMap, List
namespace fk::text { }         // String, StringBuilder
namespace fk::memory { }       // Smart pointers
namespace fk::core { }         // Result, Error
namespace fk::algorithms { }   // CRC32, DJB2

// Kernel
namespace fkernel::boot { }
namespace fkernel::cpu { }
namespace fkernel::memory { }
namespace fkernel::process { }
namespace fkernel::fs { }
namespace fkernel::drivers { }
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
- Kernel critical paths: 75%+ coverage
- All public APIs must have tests
- New LibFK/LibC PRs require test additions
- Framework: custom (see `tests/test_framework.h`)

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

## AI Agent Instructions

- Read `.ai-docs/` before making changes; update it for significant modifications
- Layer separation is non-negotiable — never add LibC calls in Kernel code
- Scheduler runs at CPL 0 with interrupts disabled; shared data needs interrupt-safe locking
- Every struct/class goes in its own file (SECRET RULE)
