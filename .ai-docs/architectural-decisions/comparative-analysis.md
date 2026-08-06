# Comparative Analysis: FKernel vs Other Kernels

## Overview

This document captures architectural insights from comparing FKernel against Linux, FreeBSD, seL4, SerenityOS, and Windows NT. Use this when making design decisions — understand what's been tried elsewhere and why.

## Architectural Identity

FKernel is a **hybrid kernel** combining:
- **Linux x86_64 ABI** (syscall numbers, ELF loading) — pragmatic compatibility for test tooling
- **BSD internals** (VFS vnode/dentry/mount, scheduler, process model) — cleaner design
- **seL4 capability model** (CSpace, Endpoints, revocation) — security primitives
- **SerenityOS C++ style** (smart pointers, containers, error handling) — modern practices

This combination is unusual and a deliberate design choice documented in `design-philosophy.md`.

## Key Architectural Insights

### 1. Dual Bitmap+Buddy Allocator

FKernel uses **both** a bitmap (O(1) amortized single-page alloc) and buddy allocator (contiguous multi-page) per physical memory zone. Linux uses buddy only; FreeBSD uses buddy + UMA; seL4 uses simple buddy.

**Decision:** Keep this design. It gives optimal single-page allocation (bitmap) with contiguous fallback (buddy). The tradeoff is ~32MB bitmap overhead for 1TB support, which is acceptable.

### 2. seL4 Capabilities in a Monolithic Kernel

seL4 uses capabilities because it's a microkernel — all services run in userspace and must communicate via IPC. FKernel runs drivers in kernel space but still uses capability-based IPC for process-to-process communication.

**Decision:** Keep the hybrid model. Capabilities provide fine-grained security properties (revocation, rights decomposition) without the performance penalty of microkernel IPC for driver operations.

### 3. No COW for fork() — Critical Gap (Fixed)

CoW fork is now implemented (`clone_table_recursive()` with per-frame refcount arrays). Previously a deep-copy bottleneck.

### 4. Scheduler Simplicity vs Fairness

FKernel's scheduler is a priority+round-robin design (similar to 4.4BSD SVR4) with XNU-inspired QoS (6 classes) + MLFQ (4 levels) + turnstile priority inheritance. SMP with work stealing.

**Current gap:** `nice` values are stored but not used in scheduling decisions.

**Recommendation:** Wire `nice` into priority calculation.

### 5. Fixed 32MB Heap — Architectural Limitation

The kernel heap is statically defined in the linker script at 32MB. Linux has vmalloc + kmalloc with dynamic growth. FreeBSD has UMA zones.

**Risk:** Heap exhaustion under load (many open files, many processes) causes silent failure or kernel panic.

**Recommendation:** Implement vmalloc-style virtually-contiguous allocation, or make heap size configurable at boot.

### 6. VFS Mount Overlay — BSD-Inspired DentryNodeStack

FKernel's `DentryNodeStack` pushes/pops filesystem nodes on a dentry for mount overlaying. Merges directory listings from ALL layers with deduplication.

**Decision:** Keep. The merge behavior is more flexible than Linux's opaque mount overlay.

### 7. Event Notification Breadth

FKernel supports kqueue (BSD), epoll (Linux), select (POSIX), eventfd, timerfd, and signalfd — all as VFS nodes. Deliberate breadth-over-simplicity choice.

**Decision:** Keep all mechanisms. They serve different userspace programs (BSD apps use kqueue, Linux apps use epoll, legacy apps use select).

### 8. Layer Separation Enforcement (Build-Time)

FKernel's `xmake check-layers` script scans for forbidden include patterns. Neither Linux, FreeBSD, nor SerenityOS has automated layer enforcement.

**Decision:** This is a strength. Keep and extend. Consider adding LibC→LibFK direction check.

### 9. Heap Corruption Detection (0xC0FFEE Magic)

Every heap block header carries a magic number checked on every operation.

**Decision:** Keep. Consider adding guard pages between heap blocks.

### 10. Three-Tier Smart Pointers

OwnPtr (unique), RefPtr (intrusive ref-counted), and RetainPtr (non-intrusive, deprecated). SerenityOS has NonnullRefPtr + OwnPtr. Linux has none.

**Decision:** Keep OwnPtr + RefPtr. RetainPtr is deprecated (zero production call sites).

## Comparison Tables

### Memory Management

| Aspect | FKernel | Linux | FreeBSD | SerenityOS | seL4 |
|--------|---------|-------|---------|------------|------|
| Physical allocator | Bitmap+Buddy per zone | Buddy orders 0-10 | Buddy+UMA | Buddy | Simple buddy |
| COW | Yes (fixed) | Yes | Yes | Yes | N/A |
| Slab/UMA | Slab (10 caches, 16B-8192B) | SLUB | UMA | Slab-like | None |
| Heap | 32MB fixed, first-fit | kmalloc+vmalloc | UMA zones | Growing heap | Static pool |
| Page tables | 4-level PML4 | 4/5-level | 4/5-level | 4-level | 4-level |
| NUMA | Basic zone selection | Full NUMA | Full NUMA | Basic | None |
| IOMMU | Interface stub | Full framework | Intel IOMMU | None | None |

### Scheduling

| Aspect | FKernel | Linux (EEVDF) | FreeBSD | SerenityOS |
|--------|---------|---------------|---------|------------|
| Algorithm | QoS + MLFQ + RR | Earliest Eligible VFD | Priority decay | Priority + RR |
| QoS classes | 6 (XNU-inspired) | cgroups | None | None |
| Priority inheritance | Turnstile chain (depth 8) | PI-futex | Priority propagation | None |
| Time slice | Fixed 5 ticks | Dynamic (weight-based) | Variable | Fixed |
| nice integration | Stored, unused | Weight-based | Decay modifier | Used |
| SMP balancing | Work stealing | Periodic load balance | Per-CPU + polling | Work stealing |

### VFS & Filesystems

| Aspect | FKernel | Linux | FreeBSD | SerenityOS |
|--------|---------|-------|---------|------------|
| Core model | Node+Dentry+Stack | inode+dentry | vnode+namecache | Inode+dentry |
| Supported FS | 13 FS: Ext2/3/4, FAT12/16/32, ExFAT, ISO9660, MinixFS, TmpFs, DevFs, ProcFs, DebugFs, PtsFs, SemFs, MqueueFs, ShmFs, PipeFs, Epoll, EventFd, SignalFd, TimerFd | ext4, Btrfs, XFS, FAT, NFS... | UFS, ZFS, FAT, NFS... | Ext2, FAT, TmpFs |
| Event polling | kqueue+epoll+select+eventfd+timerfd+signalfd | epoll+select+poll+signalfd | kqueue+poll | kqueue+select |
| Mount overlay | DentryNodeStack (merge) | mount-on-dentry | mount-on-vnode | mount-on-vnode |

### IPC

| Aspect | FKernel | seL4 | Linux | SerenityOS |
|--------|---------|------|-------|------------|
| Model | POSIX signals + Capabilities | Pure capabilities | Signals + pipes + sockets | Custom LibIPC |
| Endpoints | Synchronous rendezvous | Synchronous rendezvous | N/A (sockets) | Synchronous rendezvous |
| Notifications | Bitfield async | Async endpoints | Signals | Custom |
| Revocation | Generation counter | CSpace deletion | N/A | N/A |
| Futex | Linux-compatible | None | None | None |

### Drivers & Hardware

| Aspect | FKernel | Linux | FreeBSD | SerenityOS |
|--------|---------|-------|---------|------------|
| Driver matching | Class-based (simplified Newbus) | OF-style matching | Full Newbus hierarchy | Simple flat |
| PCI | ECAM+legacy+hotplug | Full PCI subsystem | PCI+PCIe hotplug | PCI |
| ACPI | MADT+HPET+MCFG (no AML) | Full AML | Full AML | MADT+HPET |
| Storage | ATA+AHCI+NVMe | Hundreds | Dozens | AHCI+NVMe+VirtIO |
| Network | E1000 | Thousands | Hundreds | E1000+VirtIO+RTL8168 |

## Lessons Learned

1. **Start simple, iterate** — FKernel's scheduler works for 40 applets. CFS complexity not needed yet.

2. **COW is not optional for production** — Now fixed. CoW fork with per-frame refcounts.

3. **Slab/UMA is necessary for kernel longevity** — First-fit heap fragments badly. SlabAllocator (10 caches) implemented.

4. **Layer enforcement is rare and valuable** — FKernel's automated layer checking is ahead of most projects.

5. **Capabilities in monolithic is unusual** — seL4-style capabilities normally only in microkernels. FKernel explores hybrid context.

6. **Test coverage is the biggest debt** — At 0% kernel tests, FKernel needs a dedicated test infrastructure (Phase 43).

## References

- Intel SDM Vol. 3 — x86 memory management, paging, protection
- Linux kernel documentation — scheduler design (docs.kernel.org/scheduler/)
- seL4 Reference Manual v16.0 — capability model, IPC, CSpace design
- FreeBSD Architecture Handbook — Newbus driver framework, VFS vnode model
- SerenityOS AK library — container and smart pointer design patterns
