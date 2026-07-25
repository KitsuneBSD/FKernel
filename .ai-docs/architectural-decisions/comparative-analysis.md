# Comparative Analysis: FKernel vs Other Operating Systems

## Overview

This document captures architectural insights from comparing FKernel against Linux, FreeBSD, seL4, SerenityOS, and Windows NT. Use this when making design decisions — understand what's been tried elsewhere and why.

## Architectural Identity

FKernel is a **hybrid kernel** combining:
- **Linux x86_64 ABI** (syscall numbers, ELF loading) — pragmatic compatibility
- **BSD internals** (VFS vnode/dentry/mount, scheduler, process model) — cleaner design
- **seL4 capability model** (CSpace, Endpoints, revocation) — security primitives
- **SerenityOS C++ style** (smart pointers, containers, error handling) — modern practices

No other OS combines exactly these four influences. This is a deliberate design choice documented in `design-philosophy.md`.

## Key Architectural Insights

### 1. Dual Bitmap+Buddy Allocator (Unique to FKernel)

Per the analysis, FKernel uses **both** a bitmap (O(1) amortized single-page alloc) and buddy allocator (contiguous multi-page) per physical memory zone. No other known OS does this. Linux uses buddy only; FreeBSD uses buddy + UMA; seL4 uses simple buddy.

**Decision:** Keep this design. It gives optimal single-page allocation (bitmap) with contiguous fallback (buddy). The tradeoff is ~32MB bitmap overhead for 1TB support, which is acceptable.

### 2. seL4 Capabilities in a Monolithic Kernel (Unique to FKernel)

seL4 uses capabilities because it's a microkernel — all services run in userspace and must communicate via IPC. FKernel runs drivers in kernel space but still uses capability-based IPC for process-to-process communication.

**Decision:** Keep the hybrid model. Capabilities provide provable security properties (revocation, fine-grained rights) without the performance penalty of microkernel IPC for driver operations. The generation-counter revocation pattern is lightweight and effective.

### 3. No COW for fork() — Critical Gap

Linux, FreeBSD, and SerenityOS all implement copy-on-write for fork(). FKernel deep-copies the entire address space. This is a significant performance gap for:
- Server workloads with frequent fork()
- OpenRC service supervision (fork per service)
- Shell command execution (fork+exec)

**Recommendation:** Implement COW as a Phase 22 priority. Requires:
- Shared page table entries with read-only bit set
- Page fault handler that copies on write
- Reference counting for physical pages

### 4. Scheduler Simplicity vs Fairness

FKernel's scheduler is a simple priority+round-robin design (similar to 4.4BSD SVR4). Linux uses CFS/EEVDF (virtual runtime, red-black tree). FreeBSD uses decay-based priority. SerenityOS uses priority+round-robin with dynamic adjustment.

**Current gap:** `nice` values are stored but not used in scheduling decisions. No I/O boosting, no starvation prevention.

**Recommendation:** Wire `nice` into priority calculation as minimum viable improvement. Full CFS implementation is lower priority — the simple scheduler works for the current workload (BusyBox + ~40 applets).

### 5. Fixed 32MB Heap — Architectural Limitation

The kernel heap is statically defined in the linker script at 32MB. Linux has vmalloc + kmalloc with dynamic growth. FreeBSD has UMA zones. SerenityOS has a growing kernel heap.

**Risk:** Heap exhaustion under load (many open files, many processes) causes silent failure or kernel panic.

**Recommendation:** Implement vmalloc-style virtually-contiguous allocation, or at minimum make the heap size configurable at boot via Multiboot2 memory map.

### 6. VFS Mount Overlay — BSD-Inspired, DentryNodeStack

FKernel's `DentryNodeStack` pushes/pops filesystem nodes on a dentry for mount overlaying. This is different from Linux's mount-on-dentry approach. The key difference: FKernel merges directory listings from ALL layers with deduplication.

**Decision:** This is a clean design. Keep it. The merge behavior is more flexible than Linux's opaque mount overlay.

### 7. Event Notification Breadth

FKernel supports kqueue (BSD), epoll (Linux), select (POSIX), eventfd, timerfd, and signalfd — all as VFS nodes. This breadth is unusual even for production kernels.

**Decision:** Keep all mechanisms. They serve different userspace programs (BSD apps use kqueue, Linux apps use epoll, legacy apps use select).

### 8. Layer Separation Enforcement (Build-Time)

FKernel's `xmake check-layers` script scans for forbidden include patterns (Kernel→LibC, LibFK→Kernel). Neither Linux, FreeBSD, nor SerenityOS has automated layer enforcement.

**Decision:** This is a strength. Keep and extend. Consider adding LibC→LibFK direction check.

### 9. Heap Corruption Detection (0xC0FFEE Magic)

Every heap block header in FKernel carries a magic number checked on every operation. Linux has CONFIG_DEBUG_HEAP but it's not always enabled. FKernel always checks.

**Decision:** Keep. The overhead is negligible and catches corruption early. Consider adding guard pages between heap blocks for stronger detection.

### 10. Three-Tier Smart Pointers

FKernel provides OwnPtr (unique), RefPtr (intrusive ref-counted), and RetainPtr (non-intrusive ref-counted). SerenityOS has NonnullRefPtr + OwnPtr. Linux has none. Windows has COM-style ref counting.

**Decision:** Keep the three tiers. RefPtr is used for kernel objects (Task, Node, FileDescription). RetainPtr is used for non-intrusive cases. OwnPtr is for exclusive ownership.

## Comparison Tables

### Memory Management

| Aspect | FKernel | Linux | FreeBSD | SerenityOS | seL4 |
|--------|---------|-------|---------|------------|------|
| Physical allocator | Bitmap+Buddy per zone | Buddy orders 0-10 | Buddy+UMA | Buddy | Simple buddy |
| COW | No (deep copy) | Yes | Yes | Yes | N/A |
| Slab/UMA | Not implemented | SLUB | UMA | Slab-like | None |
| Heap | 32MB fixed, first-fit | kmalloc+vmalloc | UMA zones | Growing heap | Static pool |
| Page tables | 4-level PML4 | 4/5-level | 4/5-level | 4-level | 4-level |
| NUMA | Basic zone selection | Full NUMA | Full NUMA | Basic | None |
| IOMMU | Interface stub | Full framework | Intel IOMMU | None | None |

### Scheduling

| Aspect | FKernel | Linux (EEVDF) | FreeBSD | SerenityOS |
|--------|---------|---------------|---------|------------|
| Algorithm | Priority + RR | Earliest Eligible VFD | Priority decay | Priority + RR |
| Fairness | FIFO within priority | Mathematically fair | Decay-based | Dynamic adjustment |
| Time slice | Fixed 5 ticks | Dynamic (weight-based) | Variable | Fixed |
| nice integration | Stored, unused | Weight-based | Decay modifier | Used |
| SMP balancing | Work stealing | Periodic load balance | Per-CPU + polling | Work stealing |
| Real-time | Stub (SCHED_OTHER only) | SCHED_FIFO/RR | SCHED_FIFO/RR | Not implemented |

### VFS & Filesystems

| Aspect | FKernel | Linux | FreeBSD | SerenityOS |
|--------|---------|-------|---------|------------|
| Core model | Node+Dentry+Stack | inode+dentry | vnode+namecache | Inode+dentry |
| Supported FS | FAT12/16/32, DevFs, ProcFs, TmpFs | ext4, Btrfs, XFS, FAT, NFS... | UFS, ZFS, FAT, NFS... | Ext2, FAT, TmpFs |
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

## Lessons Learned from Comparison

1. **Start simple, iterate** — FKernel's simple scheduler works for 40 applets. CFS complexity isn't needed until hundreds of concurrent processes.

2. **COW is not optional for production** — Every production kernel implements COW. The current deep-copy fork is a significant limitation.

3. **Slab/UMA is necessary for kernel longevity** — First-fit heap fragments badly. Linux's SLUB and FreeBSD's UMA solve this for kernels with many small allocations.

4. **Layer enforcement is rare and valuable** — FKernel's automated layer checking is ahead of most projects. Keep it.

5. **Capabilities in monolithic is novel** — No production kernel combines monolithic performance with seL4-style capabilities. FKernel is exploring new territory here.

6. **Test coverage is the biggest debt** — At ~10-15%, FKernel is far below production standards. Linux has kunit; SerenityOS has host-side tests. FKernel needs both.

## References

- Intel Software Developer Manual (SDM) Vol. 3 — x86 memory management, paging, protection
- Linux kernel documentation — scheduler/CFS design (docs.kernel.org/scheduler/)
- seL4 Reference Manual v16.0 — capability model, IPC, CSpace design
- FreeBSD Architecture Handbook — Newbus driver framework, VFS vnode model
- SerenityOS AK library — container and smart pointer design patterns
- cppreference.com — C++ standard library semantics for freestanding implementations
