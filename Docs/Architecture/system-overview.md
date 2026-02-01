# FKernel Architecture Overview

## System Architecture

FKernel follows a **layered architecture** inspired by BSD/XNU with strict separation of concerns:

```
┌─────────────────────────┐
│ Userspace              │  Applications, Shell, Userspace Drivers (DAL)
└─────┬───────────────────┘
│ syscalls
┌─────▼───────────────────┐
│ Kernel                 │  Core kernel functionality (LibFK only)
└─────┬───────────────────┘
│
┌─────▼───────────────────┐
│ LibFK                  │  STL-like library (uses LibC + self)
└─────┬───────────────────┘
│
┌─────▼───────────────────┐
│ LibC                   │  Minimal freestanding C library
└─────────────────────────┘
```

## Architectural Principles

### 1. Strict Layer Separation
- **LibC**: Minimal C standard library (strings, memory, types)
- **LibFK**: STL-like containers and utilities (uses only LibC)
- **Kernel**: Core kernel functionality (uses only LibFK, NEVER LibC)

### 2. Domain-Based Organization
- Each directory represents a **cohesive domain**
- Files contain **exactly one** struct/class (SECRET RULE)
- Self-documenting hierarchy

### 3. Object Calisthenics
- Max 200 lines per class, 20 lines per method
- No `else` keyword, one dot per line
- Max 2 instance variables per class
- Rich domain models, no getters/setters

### 4. Production-Ready Design
- Security-first architecture
- DAL (Driver Abstraction Layer) for userspace drivers
- IPUK (Isolated Process User Kernel) for subsystem isolation
- Hardware compatibility focus (not QEMU-only)

## Key Domains

### Core Kernel Domains
- **Memory**: Physical (Buddy+Zones), Virtual (Paging), Object (Slab)
- **Process**: Task management, scheduling, IPC
- **Hardware**: CPU, ACPI, PCI, interrupts
- **Filesystem**: VFS, FAT12/16/32, ext2/3/4 (planned)
- **Drivers**: Storage (ATA/AHCI/NVMe), Network (E1000), USB
- **Syscalls**: POSIX-compatible interface

### Architecture-Specific
- **x86_64**: Boot, interrupts, memory management, syscall handling

### Security & Isolation
- **Capabilities**: Fine-grained permission system
- **IPC**: Secure inter-process communication
- **DAL**: Userspace driver framework
- **IPUK**: Isolated kernel subsystems

## Design Patterns

### Error Handling
- `Result<T, Error>` for fallible operations
- `Optional<T>` for nullable values
- `TRY` macro for error propagation

### Memory Management
- RAII with smart pointers (`OwnPtr`, `RefPtr`)
- Stack allocation preferred
- No exceptions, manual cleanup

### Hardware Interaction
- Strategy pattern for different hardware types
- Abstract interfaces with concrete implementations
- Volatile access for MMIO registers

## Development Philosophy

1. **Extensible over Rewritable**: Build on existing abstractions
2. **Strategy Pattern Consistency**: Maintain architectural coherence  
3. **Code Quality First**: Object Calisthenics non-negotiable
4. **Security by Design**: Capabilities, isolation, minimal trust
5. **Hardware Realism**: Support real hardware, not just emulation

## Technology Stack

- **Language**: C++20 (freestanding), C17 (LibC), NASM (assembly)
- **Build System**: XMake (Lua-based)
- **Compiler**: Clang/LLD with freestanding flags
- **Testing**: Custom framework with coverage requirements
- **Documentation**: Doxygen + comprehensive guides

---

This architecture enables FKernel to be **maintainable, secure, and production-ready** while supporting modern hardware and workloads.