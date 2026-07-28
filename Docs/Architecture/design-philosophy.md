# FKernel Design Philosophy

> *"Linux ABI compatibility with BSD-inspired internals, built with modern software engineering practices."*

## Core Identity

FKernel is a **hybrid kernel** that deliberately cherry-picks the best ideas from multiple operating system traditions:

| Aspect | Inspiration | Rationale |
|--------|-------------|-----------|
| **Syscall ABI** | Linux x86_64 | Run existing binaries (BusyBox, musl, OpenRC) without modification |
| **VFS Architecture** | BSD (vnode/dentry/mount) | Clean layered design, everything-is-a-file, composable |
| **Process Model** | BSD (session/group/tty) | Job control, terminal management, process groups |
| **Scheduling** | BSD (priority queues) + Linux (load balancing) | Fairness with performance |
| **IPC** | seL4 (capabilities) | Fine-grained rights, revocation, secure by design |
| **Memory Management** | BSD (buddy+slab+zones) | Proven, NUMA-aware, minimal fragmentation |
| **Event Notification** | BSD (kqueue) | Scalable, unified, avoids Linux epoll complexity |
| **Driver Model** | BSD (Newbus-style) | PCI class-based matching, modular, self-describing |
| **Error Handling** | Rust (`Result<T, Error>`, `TRY`) | No exceptions, no hidden control flow, explicit fallibility |
| **Code Style** | Smalltalk (Object Calisthenics) | Maintainability, readability, enforced discipline |
| **Layered Architecture** | BSD/XNU (LibC -> LibFK -> Kernel) | Strict separation, testability, clear boundaries |

## Why This Hybrid?

### Why Linux ABI?

- **Ecosystem**: Instantly compatible with thousands of POSIX applications compiled for Linux
- **Toolchain**: Reuse GCC/Clang targeting `linux-gnu` or `linux-musl`
- **No fork hell**: Standard syscall numbers, ELF format, signal semantics -- userspace doesn't know it's not Linux
- **Pragmatism**: Building a compatible ABI layer is far cheaper than porting every application

### Why BSD Internals?

- **Cleaner design**: BSD subsystems (VFS, scheduler, process model) have decades of refinement with less historical baggage than Linux equivalents
- **Documentation**: BSDs document *interfaces and architectures*, not just implementation
- **Coherence**: BSD's layered VFS, kqueue, session model, and device framework compose naturally
- **Simplicity**: BSD kernel APIs tend toward clarity over performance-at-all-costs

### Why Modern Practices?

- **Rust-style errors** (`Result`, `TRY`, `Optional`): Eliminate entire classes of bugs (unchecked returns, null dereferences)
- **seL4 capabilities**: Capability-based IPC provides fine-grained access control absent in traditional Unix IPC
- **Object Calisthenics**: Enforced small entities, no `else`, one dot per line -- code that is easy to read, refactor, and verify
- **Strict layering**: LibC -> LibFK -> Kernel prevents dependency spaghetti

## Decision Matrix for Future Development

When adding a new subsystem or feature, use this table to determine which reference to follow:

| If adding... | Look at... | Because... |
|---|---|---|
| A new syscall | Linux x86_64 syscall table | Must match Linux ABI |
| A VFS operation | FreeBSD VFS | Cleanest layered VFS design |
| A scheduling policy | FreeBSD ULE / Linux CFS | Priority queues for fairness |
| An IPC mechanism | seL4 cap model | Security + formal verification |
| A driver interface | FreeBSD Newbus | Class-based matching, self-describing |
| A memory allocator | FreeBSD slab allocator | Proven, NUMA-aware, O(1) for objects |
| An event mechanism | FreeBSD kqueue | Scalable, unified, native to BSD |
| Error handling | Rust `Result` pattern | Explicitness without exceptions |
| A container type | Existing LibFK patterns | Consistency, RAII, no STL |
| A filesystem driver | BSD VFS node interface | vnode operations vector |

## What FKernel Is NOT

- **Not a Linux kernel**: We implement only the Linux *ABI*, not the internal architecture
- **Not a microkernel**: Despite seL4-inspired IPC, we run drivers and core services in kernel space for performance
- **A personal kernel**: Built for a single developer's machine and workflow; not aiming to replace Linux or run arbitrary hardware
- **Not a BSD kernel**: We don't use BSD syscall numbers or binary compat layers -- the ABI is Linux

## Key Non-Negotiables

1. **Layer separation**: Kernel never calls LibC directly. LibFK is the only bridge.
2. **ABI compatibility**: Syscall numbers match Linux x86_64. ELF format matches Linux.
3. **Result-based errors**: No C++ exceptions, no RTTI, no hidden error paths.
4. **Object Calisthenics**: All 9 rules enforced. One class per file. No exceptions.
5. **Capability IPC**: seL4-style rights for all inter-process communication.
