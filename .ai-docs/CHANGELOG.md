# FKernel — Changelog (Completed Work)

> Everything listed here is verified complete in the source tree. For pending work see `TODO.md`. For future roadmap see `ROADMAP.md`. For audit findings see `AUDITS.md`.

---

## Phase 40a #3 — PCI Config Space ioctl ✅ (session 18)

- `Include/Kernel/Hardware/Pci/pci_node.h`: Added `PIOC_READ_CONFIG = 0x5001`, `PIOC_WRITE_CONFIG = 0x5002` constants; `PiocConfigOp` struct `{bus, dev, fn, width, offset, value}`; `ioctl()` override declaration
- `Src/Kernel/Hardware/Pci/pci_node.cpp`: `PCIDeviceNode::ioctl()` — copies `PiocConfigOp` from userspace via `fkernel::memory::copy_from_user`, validates width (1/2/4) and offset (0–255), dispatches to `PciManager::read/write_config_{byte,word,dword}`, writes result back for reads; non-PCI requests return `NotImplemented`
- Userspace interface: open `/dev/pci`, call `ioctl(fd, PIOC_READ_CONFIG, &op)` with BDF + offset to read any config register; `PIOC_WRITE_CONFIG` to modify

---

## Phase 43d — ELF Header Validation Tests ✅ (session 18)

- `Include/Kernel/Loader/elf_validation.h` (NEW): `elf_check_header(const Elf64_Ehdr&)` inline function — pure validation with no I/O, no hardware, no Node dependency; checks magic, endian, class, machine, phnum limit, phoff bounds
- `Src/Kernel/Loader/Domains/parser_domain.cpp`: `validate_header()` now delegates field-level checks to `elf_check_header()`; read path unchanged
- `tests/Kernel/test_elf_header.cpp` (NEW): 15 tests — valid EXEC/DYN, wrong magic (all 4 bytes), big-endian, 32-bit class, wrong machine, phnum at/above limit, phoff overlap with header, phoff=0 with no phdrs, phoff exact boundary, return value preserves all fields

---

## Phase 39b — Sleep Queue O(S)→O(1) ✅ (session 18)

- `Include/LibFK/Container/intrusive_list.h`:
  - `remove()` now guards against double-remove: `if (prev==null && next==null && head!=obj) return;` — prevents head/tail corruption and m_size underflow on duplicate remove (pre-existing bug fixed)
  - `insert_before(T* position, T* obj)` added — O(1) splice before a known node
  - `insert_sorted(T* obj, Cmp&& cmp)` template method added — walks list once to find sorted position, then calls `insert_before`
- `Src/Kernel/Scheduler/scheduler_lifecycle.cpp`:
  - `sleep_current()` uses `insert_sorted` with `wake_up_time_ticks` comparator — sleep queue now always sorted earliest-first
  - `on_tick()` sleep scan changed from full iteration to front-check loop: stops at the first task not yet due, making average cost O(1) per tick (was O(S))
  - Old O(S) per-tick worst case replaced by O(W) where W = tasks waking up this tick (usually 0)

---

## Phase 43a — Kernel Test Harness: Infrastructure ✅ (session 18)

### 43a-1 Mock infrastructure ✅
- `tests/Kernel/mocks/mock_page_allocator.h` — `posix_memalign`-based 4 KiB page allocator stub
- `tests/Kernel/mocks/mock_timer.h` — manual-tick `MockTimer` singleton
- `tests/Kernel/mocks/mock_interrupt_controller.h` — mask/EOI no-ops with assertion counters

### 43a-2 Host-side kernel tests ✅
- `Include/LibFK/Synchronization/spinlock.h` — `ScopedLockIRQ` aliased to `ScopedLock` on non-`__fkernel__` builds, enabling kernel `.cpp` files to compile on the host
- `tests/Kernel/test_file_lock.cpp` — 11 tests for `FileLockList`: RDLCK/WRLCK semantics, conflict detection, `release()`, `release_all_for_process()` swap-and-pop correctness, boundary / non-overlapping ranges, `test_conflict()` idempotency
- `tests/Kernel/test_cspace.cpp` — 12 tests for `CSpace`: install/get, invalid handle, remove, `contains`, `find_by_object`, `remove_by_object`, `grant`, `transfer`, `grant_all_to` with type filter, `size()` tracking, free-list slot reuse
- `Src/Kernel/Fs/Vfs/FileLock/file_lock_list.cpp` added to `Test` xmake target

### 43a-3 CI integration ✅
- `xmake run Test` now covers 23 kernel unit tests (FileLockList + CSpace) in addition to existing LibFK/LibC tests (all pass)

---

## Phase 38 — Kernel Hot-Path Performance ✅ (session 16)

### 38a — memcpy/memmove optimisation ✅
- `memcpy`/`memset` already used `rep movsb`/`rep stosb`
- `memmove` forward case updated to `rep movsb`; backward case: `std; rep movsb; cld`
- ERMSB detection via `g_has_ermsb` global exported from `cpu_ops.cpp`

### 38b — Lazy FPU save via CR0.TS + #NM handler ✅
- `Processor.last_fpu_task` added to per-CPU struct
- `context_switch.asm` no longer saves/restores FPU; sets `CR0.TS=1` on switch
- `schedule()` saves FPU only if `prev_task == last_fpu_task`
- `#NM` handler: loads current task's FPU, clears TS, updates `last_fpu_task`
- `initialize_task()` pre-initialises `fx_state` with FCW=0x037F / MXCSR=0x1F80

### 38c — Fast syscall path ⏭ DEFERRED
High risk; deferred.

### 38d — Slab caches 4KB/8KB ✅
- `SlabCache.pages_order` field added
- CACHE_COUNT expanded from 8 to 10 (adds 4096 and 8192 size classes)
- `grow_slab()` uses `alloc_contiguous(order)` for multi-page slabs
- Order computed dynamically: smallest 2^n pages fitting header + one object

### 38e — KQueue event-driven ✅ (already done in Phase 37)
`deliver_event()` handles EVFILT_PROC/SIGNAL/TIMER via event-driven `pending_fflags`.

---

## Phase 37 — KQueue Completeness ✅ (session 16)

### 37a — EVFILT_PROC ✅
- `KNoteHook::pending_fflags` added to `KNoteHook` struct (`node.h`)
- `TaskIpc::proc_knotes` + `proc_knotes_lock` added (`task.h`)
- `notify_proc_kqueue()` implemented (`kqueue.cpp`)
- Hooked: `terminate_current()` (NOTE_EXIT), `sys_execve()` (NOTE_EXEC), `fork()`/`clone()` (NOTE_FORK|child_pid)

### 37b — EVFILT_SIGNAL ✅
- `TaskIpc::signal_knotes` + `signal_knotes_lock` added (`task.h`)
- `notify_signal_kqueue()` implemented (`kqueue.cpp`)
- Hooked into `SignalDelivery::send_signal()` (`signal_delivery.cpp`)

### 37c — EVFILT_TIMER ✅
- `RegisteredEvent::timer_deadline_ticks` added (`kqueue.h`)
- `compute_timer_deadline()` converts NOTE_SECONDS/NOTE_MSECONDS to absolute ticks
- `nearest_timer_deadline()` drives smart wait in `kevent()` loop
- `deliver_event()` handles EVFILT_TIMER with periodic reload

### Task non-copyable refactor ✅
- `create_a_new_task()` → `void initialize_task(Task*, ...)` (in-place init)
- Updated: `task.cpp`, `idle_task.cpp`, `scheduler_manager.cpp`

---

## Phase 36 — Desktop IPC: SCM_RIGHTS & SCM_CREDENTIALS ✅ (session 16)

### 36a — SCM_RIGHTS (FD passing via Unix sockets) ✅
- `sendmsg()` parses `msg_control` cmsgs; SCM_RIGHTS → sender's fds → `send_fds()` into peer's `m_pending_fds[]`
- `recvmsg()` drains `recv_fds()`, installs each via `task->add_file_descriptor()`, writes SCM_RIGHTS cmsg back
- `m_pending_fds[MAX_PENDING_FDS=64]` + `m_pending_fd_count` on UnixSocket

### 36b — SCM_CREDENTIALS (peer authentication) ✅
- `PeerCredentials` struct (pid/uid/gid) added to `unix_socket.h`
- `connect()` captures caller's `identity.id/uid/gid` into `m_peer_creds`
- `getsockopt(SOL_SOCKET=1, SO_PEERCRED=17)` returns `m_peer_creds` to caller

### 36c — siginfo_t truncation fix ✅
- `NOTIFICATION_PAYLOAD_SIZE` increased from 64 → 128 bytes (`notification.h`)

---

## Phase 35b — Real-Time Scheduling ✅ (session 16)

- `pick_next()` FIFO: skip demotion (`scheduler_lifecycle.cpp`)
- `on_tick()` skips demotion for FIFO/RoundRobin
- RoundRobin re-enqueues at same MLFQ level
- `pick_next()` filters by `cpu_affinity`
- `steal_task()` respects `cpu_affinity`

---

## Phase 34a — Critical x86_64 Fixes ✅ (sessions 15-16)

| Fix | Detail |
|-----|--------|
| `g_cpu_block` → `g_cpu_blocks[MAX_CPUS]` | Each AP sets own MSR_GS_BASE; `get_current_cpu_id()` via `gs:32` |
| Boot page tables PWT+PCD fix | `setup_page_tables.asm` flag `0b10011011` → `0b10000011` (WB cache) |
| CR0.WP set | `arch_enable_cpu_features()` sets `cr0 |= (1<<16)` |
| CR4.OSXSAVE + XCR0 | OSXSAVE set in CR4 when `has_xsave`; `xsetbv(0, x87|SSE|AVX)` called |
| XSAVE/XRSTOR context switch | `context_switch.asm` uses `xsave64`/`xrstor64` when available; `g_use_xsave`/`g_xsave_area_size` set in `cpu_ops.cpp` |

## Phase 34b — Important x86_64 Fixes (partial) ✅ (session 16)

| Fix | Status |
|-----|--------|
| PCID (CR4.PCIDE) | ✅ Enabled via CPUID detection |
| KPTI (Meltdown) | ⏭ Deferred — two PML4 roots too invasive |
| MCA handling | ✅ `machine_check.cpp` reads MCG_CAP banks, dumps MCi_STATUS/ADDR/MISC before halt |
| IA32_MISC_ENABLE | ✅ Enables Fast Strings (bit 0) + detects ERMSB via CPUID[7].EBX[9] |
| MSR_SFMASK = 0x4700 | ✅ Clears IF, TF, DF, AC, NT on syscall entry |
| MCFG/ECAM | ✅ Already done in `pci.cpp` (reads MCFG, maps ECAM range) |
| HPET | ✅ Already done in `timer_interrupt.cpp` |
| Early serial fallback | ⏭ Deferred (low impact for QEMU) |

---

## Phase 32 — New Filesystem Drivers ✅ (session 17)

### 32a — MinixFS ✅
- `minix_super.h`, `minix_fs.h/cpp`, `minix_node.h/cpp`
- Magic 0x137F/0x138F; direct+indirect+double-indirect block traversal
- Full read/write: bitmap alloc/free for inodes and zones, `create_in_inode`, `remove_from_inode`, `truncate_inode`
- Registered in `AutoMounter` as `"minix"`

### 32b — ExFAT ✅
- `exfat_bpb.h`, `exfat_fs.h/cpp`, `exfat_node.h/cpp`
- OEM name "EXFAT   " validation; allocation bitmap; cluster chain I/O
- Entry type state machine: File+StreamExt+FileName sets; UCS-2 LE → ASCII name
- `create_entry`, `delete_entry`, `update_stream_ext`; case-insensitive ASCII lookup
- Registered in `AutoMounter` as `"exfat"`

### 32e — ISO9660 ✅
- `iso9660_vd.h`, `iso9660_fs.h/cpp`, `iso9660_node.h/cpp`
- PVD (type 1) + Joliet SVD (type 2) + Rock Ridge SUSP detection
- DR chain walker, Rock Ridge NM/SL, Joliet UCS-2 BE → ASCII
- Read-only; all write ops return `NotImplemented`
- Registered in `AutoMounter` as `"iso9660"`

### 32f — ext2 ✅
- `ext2_super.h`, `ext2_fs.h/cpp`, `ext2_node.h/cpp`
- Magic 0xEF53; block groups; direct+single+double+triple-indirect blocks
- Bitmap alloc/free; `create_in_dir`/`remove_from_dir`; `truncate_inode`
- Short symlink inline path (`read_link()`)
- Registered in `AutoMounter` as `"ext2"`

### 32g — ext3 ✅
- `ext3_super.h`, `ext3_fs.h/cpp`
- JBD journal recovery on mount (reads inode 8, validates JBD magic 0xC03B3998 BE)
- Revoke block support; clears `s_start` after replay; delegates writes to Ext2FileSystem
- Registered in `AutoMounter` as `"ext3"`

### 32h — ext4 ✅
- `ext4_super.h`, `ext4_fs.h/cpp`, `ext4_node.h/cpp`
- Extent tree: recursive `walk_extent_node()` for depth 0 (leaf) and depth>0 (index)
- 48-bit block numbers via `ee_start_hi`/`ee_start_lo`; detection via `EXT4_INCOMPAT_EXTENTS`
- JBD2 journal recovery (same as ext3 path); delegates writes to Ext2FileSystem
- `EXT4_INCOMPAT_ACCEPTED` = 0x0002|0x0004|0x0040|0x0080|0x0200
- Registered in `AutoMounter` as `"ext4"` (probed before ext3)

---

## Phase 31 — Distro Readiness Gaps (partial) ✅

- **CoW fork**: verified complete (`clone_table_recursive` + `handle_write_protection` + PMM per-frame refcount)
- **Anonymous demand paging**: verified complete (`handle_demand_paging` zero-fill on page fault)
- **FAT32 truncate**: shrink (walk chain, mark EOC, free trailing clusters) + extend (allocate clusters)
- **FAT32 rmdir**: emptiness check before removal

---

## Phase 30 — ELF Loader Fixes ✅ (session 12)

### 30a — Dynamic Linking ✅
- `load_dependencies()` scans DT_NEEDED entries; `load_shared_library()` loads/relocates each .so
- `s_global_libraries` Vector for cross-object symbol resolution
- ld.so `PT_DYNAMIC` processed via `DynamicDomain::apply_relocations()`
- `R_X86_64_COPY`, `R_X86_64_IRELATIVE`, `R_X86_64_TPOFF64/DTPMOD64/DTPOFF64` all handled

### 30b — Security Hardening ✅
- SMAP-aware access (`arch_smap_begin/end`) in all user-memory write paths
- W^X enforcement: reject segments with PF_W + PF_X in `apply_final_permissions()`
- ASLR: ChaCha20PRNG with 30-bit entropy; ld.so base randomised (was hardcoded `0x70000000`)
- RELRO: all PT_GNU_RELRO segments processed (removed `break`); start rounded UP; interpreter RELRO applied
- Endianness check: `e_ident[EI_DATA] != ELFDATA2LSB` → reject
- File-size bounds: `p_offset + p_filesz > node->size()` → `InvalidParameter`
- `remap_page_with_permissions()` returns `Error::NotFound` when `translate()` returns 0

---

## Phase 29a — POSIX Nodes via Endpoint ✅ (session 12)

All 6 POSIX IPC nodes migrated from raw `ipc::Notification` to `ipc::Endpoint`:

| Node | Change |
|------|--------|
| PipeNode | 2 raw Notifications → 1 Endpoint (Send=write, Receive=read) |
| EventFdNode | 1 raw Notification → 1 Endpoint |
| SemNode | 1 raw Notification + own m_generation → 1 Endpoint (delegates generation) |
| MqueueNode | 2 raw Notifications + own m_generation → 1 Endpoint (delegates generation) |
| SignalFdNode | 1 raw Notification → 1 Endpoint |
| TimerFdNode | 1 raw Notification → 1 Endpoint |

### 29b — Epoll Event-Driven ✅ (Phase 11 / session 12)
`EpollNode` delegates to `KQueueNode` which uses `KNoteHook` attached to watched Nodes. I/O paths call `notify_kqueue_readers/writers()` to immediately wake `kevent()` callers.

### 29c — UnixSocket Migration ✅
`UnixSocket::accept()` now uses `ipc::Endpoint` (was raw `SchedulerManager::block_current()`).

---

## Phase 28 — Memory Improvements ✅

- DMA vaddr free-list (replaces leaky bump allocator)
- Embedded `FreeBlock` in free pages via `KERNEL_VIRT_BASE` (1MB BSS savings)
- `-ENOSYS` stubs for missing syscalls
- **Slab allocator**: 8 caches 16B–2048B
- **Anonymous demand paging**: `mmap MAP_ANONYMOUS` lazy + `handle_demand_paging` zero-fill
- `extend_direct_map()` with 2MB huge pages
- Init flow restructured

## Phase 27 (Memory) — Bug Fixes ✅

- Bitmap↔buddy reconciliation; `alloc_page` bitmap-only; `free_page` dead code removal
- `alloc_contiguous`/`free_contiguous` bitmap sync
- `heap_stats` lock

---

## Phase 26 — QoS/MLFQ/Turnstiles ✅

- 6-class QoS scheduler (`UserInteractive`, `UserInitiated`, `Default`, `Utility`, `Background`, `Maintenance`)
- MLFQ demotion on allotment expiry; priority boost for interactive tasks
- Turnstile priority inheritance: `boost_qos_if_needed()` / `unboost_task()`
- Work-stealing across CPUs with least-loaded-CPU selection

---

## Phase 25 — Boot Optimisation (partial) ✅

- NMI/MCE IST stacks (IST2→NMI, IST3→MCE)
- IOAPIC destination field from `CPU::lapic_id()` via CPUID.01h:EBX[31:24]
- MSR_CSTAR removed (Intel-only dead code)
- sys_kill negative PIDs → `send_signal_to_pgrp()`
- `send_signal` UAF guard: checks `is_valid()` + terminated before access
- SA_SIGINFO: `rdx` now points to `saved_regs` (was NULL)

## Phase 24 — LibFK/LibC Improvements ✅

| Component | Change |
|-----------|--------|
| Robin Hood HashMap | Replaces linear-probing+tombstones; 80% load factor |
| String SSO | 16-byte inline buffer; heap only for > 16 chars |
| NonnullOwnPtr / NonnullRefPtr | Null-safety wrappers |
| WeakPtr | Weak reference implementation |
| BumpAllocator | For scoped temporary allocations |
| Lock rank checking | Deadlock detection via compile-time rank ordering |
| memcpy/memset | Already use `rep movsb`/`rep stosb` |
| LibC stdio | fopen/fclose/fread/fwrite/fgets fully implemented |
| LibC strtol | endptr logic fixed; strtoll/strtoull correct unsigned parse |

---

## Phase 23 — Manager Pattern (partial) ✅

Most kernel subsystem managers converted to canonical singleton form:
- Private default constructor + deleted copy/move
- `is_initialized()` accessor
- `fkernel::` namespace; `using` alias at bottom
- Double-init guard; `m_is_initialized = true` at end of `initialize()`

---

## Phase 22 — File Naming Cleanup ✅

All source/header files renamed to `snake_case`. `git mv` used throughout. All `#include` references updated.

---

## Phase 18 — TCP/UDP Checksums ✅

TX+RX checksums computed via RFC 793/768 pseudo-header in `tcp_socket.cpp`, `udp_socket.cpp`, `network_stack.cpp`.

## Phase 17 — Security & Concurrency ✅

- Triple fault IST stack
- `kcalloc` overflow guard
- VMM lock in `switch_address_space()`
- `copy_from/to_user` with SMAP STAC/CLAC
- E1000 interrupt-driven TX
- DNS/DHCP deadline-based timeout (was busy-wait)

---

## IPC/POSIX Phases 0–11 ✅ (2026-07-26)

All 10 POSIX IPC phases complete. ~81 files created/modified.

| Phase | Features |
|-------|----------|
| 0. IPC Primitives | wait_timeout, signal_with_payload, Endpoint::call/timeout, SharedMemory, cap_transfer/grant |
| 1. Signals | SA_SIGINFO, SA_ONSTACK, SA_RESETHAND, siginfo_t (128B), SIGSTOP/CONT, sigreturn trampoline |
| 2. Pipes+Named | O_NONBLOCK, mkfifo via VFS, mknod S_IFIFO |
| 3. Eventfd/Signalfd/Timerfd | O_NONBLOCK via wait_timeout(0) |
| 4. Epoll | Event-driven via KQueueNode + KNoteHook |
| 5. Futex | Notification[256] replaces hash table; FUTEX_REQUEUE |
| 6. Semaphores | SemNode, /dev/sem/, sem_open/wait/post/getvalue/unlink |
| 7. Msg Queues | MqueueNode priority queue; mq_open/send/receive/unlink |
| 8. Shared Memory | ShmNode, /dev/shm/, mmap MAP_SHARED |
| 9. PTY | Termios, PtyLineDiscipline (^C/^\/^Z), TCSETS/TCGETS ioctls |
| 10. TCP | Retransmission timer, exponential backoff, socket registry |
| 11. KQueue | Unified backend: epoll/poll/select; EVFILT_TIMER/VNODE/PROC/SIGNAL/USER; EV_ONESHOT/EV_CLEAR/EV_DISPATCH |

---

## Phases 1–14 — Foundation ✅

| Phase | What was done |
|-------|--------------|
| 1 — Compilation Blockers | List/Queue/HashMap/Optional/Result all fixed |
| 2 — Critical Bugs | Memory, scheduler, VFS, IPC, containers |
| 3 — Security | SMEP/SMAP enabled, atomic refcounts, TLB fence |
| 4 — Architecture | Layer violations fixed, Error enum unique values |
| 5 — POSIX Foundation | LibFK Text/Containers, LibC headers + functions |
| 6 — Core Features | VFS truncate/fsync/O_CREAT, IPC caps, ELF validation |
| 7 — Networking | ARP, IPv4, ICMP, UDP, TCP, AF_INET, routing table, DNS, DHCP |
| 8 — USB/Drivers (partial) | PS/2 Mouse, PTY, Serial /dev/ttyS0 |
| 9 — Code Quality | Dead code removed, type wrappers, 45 tests |
| 10 — BusyBox | PID 1 init, shell, ls/cat/uname/clear, xmake setup-hda |
| 12 — BusyBox ~60 applets | pipe2/dup3/mprotect/*at() family, signal defaults, device nodes |
| 14 — BusyBox job control | Process groups/sessions, readv, pread64/pwrite64, flock/fcntl |

---

## All P0–P3 Bugs ✅

All critical, high, and medium bugs resolved:

- P0 Compilation Blockers: 7 bugs — List, Queue, HashMap, Optional, Result (all ✅)
- P0 BusyBox Showstoppers: 22 bugs — syscall collisions, signal defaults, setsid/setpgid, pipe2/dup3, PTY blocking, at() family (all ✅)
- P0 Boot Blockers: 7 bugs — initrd, userspace binaries, disk partitioning (all ✅)
- P0 Source Code Bugs: 33 bugs — 29 ✅ fixed, **4 OPEN** (see TODO.md)
- P0 Comprehensive Audit: 60+ bugs across LibC, LibFK, Scheduler, VFS, IPC, Drivers (all ✅)
- P1: Boot failures, filesystem gaps, syscall stubs, hardware gaps (all ✅)
- P2: Security — NX, SMEP, SMAP, RefPtr atomicity (all ✅)
- P3: Architecture violations, layer separation (all ✅)

## P6 — LibFK Migration ✅

- `byte_order.h`, `io.h`, `syscall_numbers.h` moved to LibFK
- Algorithm consolidation: case-insensitive compare, RFC 1071 checksum, queue dequeue-N, FAT 8.3 name formatting, dedup-on-insert, binary search (all ✅)
- DJB2 deduplication, base-N formatting shared helper (all ✅)

---

## Session 20 — 2026-07-30 ✅

### Phase 27 — VFS + Capability Integration

All POSIX FDs routed through CSpace capabilities. `CapabilityType::FileDescriptor`, `CapabilityRights` (Read/Write/Seek/Ioctl), `CSpace::install_fd/lookup_fd/revoke_fd/clone_fd` implemented. `TaskFiles` parallel `cap_handles` vector wired throughout task fd lifecycle. Rights enforced in `FileDescription::read()/write()` via `O_ACCMODE` check. Pipe creates separate `O_RDONLY`/`O_WRONLY` descriptions with correct rights. Fork uses new `CSpace::clone_fd()`. Execve revokes `FD_CLOEXEC` caps. Mmap and socket use validated `get/add_file_descriptor`.

### Phase 29b — CSpace Wiring + Phase 29d — Unified Revocation

All POSIX syscall handlers go through CSpace. `SemNode`/`MqueueNode` already delegated generation to `ipc::Endpoint` — no separate `m_generation` to remove. CSpace revoke called from `close_file_descriptor`.

### Bugs 9, 10, 18, 19, 20 ✅

- Bug 9 (CSPRNG): Already seeded via `arch_read_tsc()` at init.cpp:30–32
- Bug 10 (`s_global_libraries`): Already guarded by `s_library_lock` (ScopedLockIRQ) at all call sites
- Bugs 18/19 (Endpoint wait data race): Fixed in prior session (noted in session 19)
- Bug 20 (signal_with_payload): Fixed in prior session

### P1 Manager Pattern + P1 Arch Portability ✅

All 13 managers converted (session 19). All inline x86_64 asm extracted to `arch_*` functions (session 19).

### Phase 32d — HFS+ / HFSX

7 headers + 6 sources in `Include/Kernel/Fs/Disk/HfsPlus/` and `Src/Kernel/Fs/Disk/HfsPlus/`:
- `hfsplus_vh.h` — all on-disk structures (Volume Header, B-tree nodes, Catalog records, Extents)
- `hfsplus_unicode.h/cpp` — UCS-2 BE ↔ UTF-8, 256-entry case-folding table, case-sensitive compare
- `hfsplus_btree.h/cpp` — `BTreeNode`, `BTreeFile` (fork-backed B-tree I/O), B-tree descent for catalog lookup, catalog list (enumeration by parentID across leaf chain), extents overflow lookup
- `hfsplus_catalog.h/cpp` — `make_catalog_key()` helper
- `hfsplus_extents.h/cpp` — `HFSPlusForkReader`: 8 inline extents + overflow B-tree for large files; partial-block reads
- `hfsplus_fs.h/cpp` — `HFSPlusFileSystem` (VFS Node, `create()` factory, `lookup()`/`list_dir()`, HFSX case-sensitive support)
- `hfsplus_node.h/cpp` — `HFSPlusNode` (file/dir/symlink VFS Node, reads via `HFSPlusForkReader`)
- Registered in `AutoMounter::try_mount()` and `try_mount_at()` as `"hfsplus"`

## Session 21 (2026-07-30)

### Phase 44 — Thread Group Signal Delivery

**44a — Signal Delivery to Thread Groups:**
- `SignalDelivery::deliver_to_group(sig, tgid, info)` added to `signal_delivery.h/cpp`:
  - Iterates all tasks via `last_pid()` + `find_task()` loop
  - Picks first thread in group where signal is not blocked
  - Falls back to tgid leader (thread with `id == tgid`) if all threads block the signal
- `sys_tgkill` fixed: was finding task by `tgid` value (wrong); now finds by `tid`, verifies `task->tgid == tgid`
- `sys_kill(pid > 0)`: replaced `find_task(pid)` + `send_signal` with `deliver_to_group(sig, ProcessId(pid))` — correct for multi-threaded processes
- `scheduler_lifecycle.cpp` SIGCHLD: replaced `send_signal(parent)` with `deliver_to_group(SIGCHLD, parent->tgid)` — delivers to any thread in parent group

**44b — Signal Mask Inheritance:**
- CLONE_THREAD signal mask inheritance already done (clone.cpp:84 `blocked = parent->blocked`)
- execve now kills sibling threads (SIGKILL loop before address space switch) — POSIX multi-thread exec semantics
- `execve.cpp`: removed incorrect `signals.blocked = 0` (POSIX: signal mask preserved across exec); replaced with `signals.pending = 0` (clear pending signals on exec, correct per POSIX)
- sigsuspend/rt_sigtimedwait already per-task — no changes needed
