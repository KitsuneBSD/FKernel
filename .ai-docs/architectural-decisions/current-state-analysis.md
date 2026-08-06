# Current Project State Analysis (July 2026)

*Updated: 2026-07-27 -- source-code audit refreshed all subsystem states*

## Executive Summary

FKernel is a hobby kernel at ~70% kernel completeness -- boots successfully to MockOS test harness with BusyBox 1.36.1 in QEMU. The codebase contains ~400 source files (362 Kernel .cpp + 324 Kernel headers + 39 LibC .c + 12 LibFK .cpp). All major kernel subsystems functional: PCI, VFS (BSD-style), drivers (AHCI/NVMe/E1000/ATA/PS2/PTY), QoS+MLFQ scheduler with turnstile priority inheritance + SMP, networking (full TCP/IP), ELF loader (DT_NEEDED dynamic linking + ASLR + W^X + RELRO), and IPC (capability-based with POSIX wrappers). ~5 bugs open. Phases 1-31b complete.

## Completed Milestones

### Phase 1-9: Foundation (Complete)
- All compilation blockers and critical bugs fixed
- Security: SMEP, SMAP, NX, ASLR, RELRO, atomic refcounts, SMAP-aware user access
- Networking: full TCP/IP stack

### Phase 10-12: MockOS + BusyBox Integration (Complete)
- Minimal init + shell, FAT32 rewritten (lookup, list_dir, subdirectory, LFN, metadata write)
- /dev/null, /dev/zero, /dev/urandom, /dev/ptmx registered
- PTY blocking reads, select/poll blocking, TCP connect/accept
- *at() syscall family (12 syscalls)

### Phase 13: Kernel -> LibFK Migration (Complete)
- byte_order.h, io.h, syscall_numbers.h moved to LibFK
- ~15 duplicated algorithms consolidated (DJB2, internet checksum, FAT name formatting, binary search)

### Phases 24-26: QoS + MLFQ + Turnstiles (Complete)
- XNU-inspired 6-class QoS scheduler with 4-level MLFQ, periodic priority boost, work stealing
- Turnstile-based priority inheritance for IPC (Endpoint boost/unboost)

### Phases 27-28: Memory Improvements (Complete)
- Bitmap+buddy reconciliation, CoW fork with per-frame refcount arrays
- Direct map at KERNEL_VIRT_BASE with 2MB huge pages
- Embedded FreeBlock buddy metadata in free pages (saves ~1MB BSS)
- SlabAllocator: 10 caches (16B-8192B)
- Anonymous demand paging: lazy zero-fill on first access

### Phases 30-30b: ELF Loader (Complete)
- DT_NEEDED shared library loading via VFS, ld.so self-relocation
- 10 relocation types (NONE, RELATIVE, 64, GLOB_DAT, JUMP_SLOT, COPY, IRELATIVE, TPOFF64, DTPMOD64, DTPOFF64)
- Cross-object symbol resolution via global library registry
- ASLR: ChaCha20PRNG with 30-bit entropy, randomized ld.so base
- W^X enforcement, full RELRO (all segments, correct alignment, interpreter RELRO)
- SMAP STAC/CLAC in all user-memory write paths

### Phase 31a-31b: Verification + FAT32 Metadata (Complete)
- CoW fork + demand paging verified complete
- FAT32 truncate (shrink + extend), rmdir emptiness check

### IPC/POSIX Phases 0-10 (Complete)
- Enhanced Notification (wait_timeout, signal_with_payload), Endpoint (call, send_timeout, receive_timeout)
- SharedMemory page-level sharing, cap_transfer/grant syscalls
- Signals with full siginfo_t, altstack, SA_RESETHAND, SA_NODEFER, SA_RESTART
- Pipes, FIFOs, eventfd, signalfd, timerfd, epoll, futex, semaphores, message queues, shared memory
- PTY discipline, TCP retransmission timer, KQueue unified event backend

## Current State by Subsystem

| Subsystem | Status | Files | Notes |
|-----------|--------|-------|-------|
| LibFK | ~75% | ~78 | Containers, text, core, algorithms solid |
| LibC | ~65% | ~37 | Strings/stdio/ctype complete |
| Memory | ~90% | ~19 | Buddy+zones+CoW; VMM with demand paging; SlabAllocator (10 caches); 2MB huge page direct map |
| Scheduler | ~90% | ~12 | QoS (6 classes) + MLFQ (4 levels) + Turnstiles; SMP with work stealing |
| VFS | ~85% | ~24 | BSD-style dentry/vnode/mount; FAT12/16/32 LFN+write; mount namespaces; pivot_root; KQueue |
| Drivers | ~70% | ~53 | ATA/AHCI/NVMe/E1000 + PS/2 + PTY + Serial; USB headers only |
| Networking | ~85% | ~12 | Full TCP/IP: ARP, ICMP, IP, TCP (handshake+window+retransmit), UDP, DHCP, DNS, routing |
| ELF Loader | ~85% | ~12 | DT_NEEDED + 10 reloc types + cross-object symbols + ASLR + W^X + RELRO + SMAP |
| IPC | ~75% | ~8 | seL4-style CSpace/Endpoint/Notification; POSIX wrappers use Notification directly |
| Syscalls | ~80% | ~144 | ~206 registered handlers across 11 domain directories |
| Arch/x86_64 | ~85% | ~77 | GDT/IDT/TSS, page tables, context switch with FPU, syscall entry, SMP AP startup |

## Key Architecture Insights (from Source Audit)

### What Actually Works (verified in source code)

- CoW fork: clone_table_recursive() with per-zone uint16_t refcount arrays
- Anonymous demand paging: pf_handler allocates + zero-fills on first access
- SlabAllocator: 10 caches, tried first in kernel heap allocate()
- 2MB huge pages: extend_direct_map() maps all RAM at KERNEL_VIRT_BASE
- Embedded buddy FreeBlock: metadata in free pages via direct map, no BSS allocation
- Full POSIX signal delivery: SA_SIGINFO, SA_RESTART (rip -= 2), SA_ONSTACK, SA_RESETHAND, SA_NODEFER, builtin restorer trampoline
- TCP checksums: RFC 793 pseudo-header computation in tcp_socket.cpp
- TCP retransmission: exponential backoff (RTO * (1 << attempt)), max 4 retries
- Mount namespaces: per-process isolation via MountNamespace + dentry stack overrides
- pivot_root: full implementation with mount record updates
- MLFQ demotion: cpu_time_consumed >= allotment_ticks triggers level demotion
- Stopped state: wired through signal_delivery.cpp -> TaskState::Stopped

### What Still Has Gaps

| Gap | Detail |
|-----|--------|
| IPC fragmentation | POSIX mechanisms use Notification directly; CSpace/Endpoint is parallel subsystem (Phase 27) |
| Thread group signal delivery | CLONE_THREAD tgid set, but signal delivery to thread groups not implemented |
| TCP out-of-order | process_data() only accepts in-order segments (seq must match recv_next exactly) |
| Kernel tests | Phase 43 started -- 10 kernel suites / 99 tests (kernel coverage target 75%) |
| CSPRNG | init.cpp lines 105-107 commented out; ASLR may use unseeded PRNG |
| POSIX networking syscalls | ~25 advanced socket options still missing |

## Test Coverage

| Library | Tests | Coverage |
|---------|-------|----------|
| LibC (string/memory/stdio) | ~65 | ~60% |
| LibFK containers | ~110 | ~75% |
| LibFK text/algos/core/memory | ~55 | ~60-80% |
| Kernel | 10 suites / 99 | 0% (target 75%) |
| Total | ~330 test cases (incl. 99 kernel) | ~40-50% |

## Strategic Recommendations

1. Phase 27 (IPC Capability Integration) -- route POSIX mechanisms through CSpace/Endpoint for unified security model
2. Phase 43 (Kernel Test Harness) -- VFS, scheduler, and memory manager test coverage
3. Phase 44 (Signal Completion) -- thread group signal delivery for multi-threaded POSIX compliance
4. Fix remaining ELF gaps -- endianness check, file-size bounds, symbol versioning
5. Enable CSPRNG seeding -- uncomment init.cpp:105-107 for real ASLR entropy
6. Complete POSIX networking syscalls (~25 remaining)
