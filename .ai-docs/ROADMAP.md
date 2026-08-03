# FKernel — Roadmap (Future Phases)

> All phases listed here are **not yet started** or **partially complete**. Completed work lives in `CHANGELOG.md`. Open bugs live in `TODO.md`. Audit findings live in `AUDITS.md`.

---

## Priority Legend

| Level | Meaning |
|-------|---------|
| **IMMEDIATE** | Blocking correct kernel operation (crash, corruption, security) |
| **HIGH** | Blocking real-world use or a planned phase |
| **MEDIUM** | Improves capability but kernel works without |
| **LOW** | Polish / long-term |

---

## Phase 27 — VFS + Capability Integration — ✅ COMPLETED (2026-07-31)

> **Implementado**: `CSpace::install_fd`/`revoke_fd` + `Task::add_file_descriptor`/`get_file_descriptor` com `fd_flags_to_rights()` em `Src/Kernel/Scheduler/Task/task.cpp`. FDs POSIX viram capabilities com rights por-FD; revoke em close/dup2; `cap_handles` rastreados na FdTable. Sub-fases 27a–27e concluídas. Detalhes em `.ai-docs/CHANGELOG.md`.

> O conteúdo abaixo (27a–27e, Key Design Decisions) é mantido como referência histórica do escopo original.

### 27a — Expand Capability Subsystem (1 day)

| # | Task | Files |
|---|------|-------|
| 1 | Add `CapabilityType::FileDescriptor` variant | `Include/Kernel/Ipc/Capabilities/capability.h` |
| 2 | Add rights bitmask: `cap_rights_t` with `CAP_READ`, `CAP_WRITE`, `CAP_SEEK`, `CAP_MMAP`, `CAP_IOCTL` | `capability.h` |
| 3 | `Capability<FileDescription>` with generation counter | `capability.h` |
| 4 | `CSpace::lookup_fd(cap_index)` → validates type + generation | `cspace.h`, `cspace.cpp` |
| 5 | `CSpace::revoke_fd(cap_index)` → invalidates generation | `cspace.h`, `cspace.cpp` |
| 6 | `CSpace::clone()` → copy all fd capabilities with same backing objects | `cspace.h`, `cspace.cpp` |

### 27b — Transition FileDescription (1.5 days)

| # | Task | Files |
|---|------|-------|
| 1 | `FileDescription` holds `Capability<Dentry>` instead of raw `RefPtr<Dentry>` | `file_description.h`, `file_description.cpp` |
| 2 | Add `resolve_dentry()` → does capability lookup + validates rights | `file_description.cpp` |
| 3 | `read()`/`write()`/`seek()` all call `resolve_dentry()` first | `file_description.cpp` |

### 27c — Transition Syscalls (2 days)

| File | Change |
|------|--------|
| `FileSystem/open.cpp` | Install capability into CSpace on open |
| `FileSystem/close.cpp` | Revoke capability from CSpace |
| `FileSystem/dup2.cpp` | Copy capability (independent revoke) |
| `FileSystem/dup3.cpp` | Copy capability + flags |
| `FileSystem/fcntl.cpp` | F_DUPFD via capability copy |
| `FileSystem/pipe.cpp` | Two capabilities (Read + Write) on same dentry |
| `Process/fork.cpp` | CSpace clone |
| `Process/execve.cpp` | FD_CLOEXEC via capability revoke |
| `Memory/mmap.cpp` | File capability for file-backed mmap |
| `Networking/socket.cpp` | Capability install on socket creation |

### 27d — Transition FdTable (1 day)

| # | Task | Files |
|---|------|-------|
| 1 | Task's `FdTable` becomes `Vector<CapabilityIndex>` (view into CSpace) | `task.h`, `task.cpp` |
| 2 | `get_file_description(fd)` → CSpace lookup | `task.cpp` |
| 3 | `add_file_descriptor(desc)` → CSpace install | `task.cpp` |

### 27e — Integration Testing (0.5 day)

| # | Task | Files |
|---|------|-------|
| 1 | Verify `open()`/`read()`/`write()`/`close()`/`dup2()` through new path | Manual QEMU boot |
| 2 | Verify CSpace clone on `fork()` | — |
| 3 | Verify FD_CLOEXEC via `execve()` | — |
| 4 | Verify `pipe()` read+write cap rights | — |
| 5 | Test BusyBox applets: `ls`, `cat`, `cp`, `mv`, `rm`, `grep`, `find` | — |

### Key Design Decisions

1. FDs stay FDs to userspace. Mapping `fd → Capability` is kernel-internal. POSIX ABI unchanged.
2. VFS NOT refactored. Dentry, Node, path resolution — zero changes.
3. Rights are per-capability, not per-resource.
4. Revoke does NOT free the resource; RefCounted backing object cleans up lazily.
5. CSpace clone on fork creates independent generation counters.
6. `FileDescription` wraps `Capability<Dentry>`. It is NOT a capability type.
7. Signals/Notifications use the SAME CSpace.

---

## Phase 29b — CSpace Wiring + Rights Enforcement (HIGH)

Completes the POSIX → Capability migration after Phase 27.

| # | Task | Files | Priority |
|---|------|-------|----------|
| 9 | Wire POSIX fd operations through CSpace capability lookup | All POSIX node types + syscall handlers | HIGH |
| 11 | Add rights enforcement at POSIX syscall boundary (cap_transfer/grant on fds) | Syscall handlers + CSpace | MEDIUM |

### Phase 29d — Unified Revocation (0.5 day)

| # | Task | Files |
|---|------|-------|
| 1 | Remove `SemNode::m_generation`, delegate to Endpoint/Notification generation | `sem_node.h/cpp` |
| 2 | Remove `MqueueNode::m_generation`, delegate to Endpoint/Notification generation | `mqueue_node.h/cpp` |
| 3 | Ensure all POSIX IPC close/release paths call CSpace revoke | All node types |

---

## Phase 32c — UFS/UFS2 (~4000 LOC, 5–7 days) — HIGH

BSD native filesystem. Inodes (128B UFS1 / 256B UFS2) with 12 direct + single/double/triple indirect blocks. Cylinder groups with per-CG bitmaps and superblock backup.

| # | Task | Files | Priority |
|---|------|-------|----------|
| 1 | Headers: `ufs_fs.h`, `ufs_node.h`, `ufs_super.h`, `ufs_dir.h`, `ufs_endian.h` | `Include/Kernel/Fs/Disk/Ufs/` | HIGH |
| 2 | Sources: `ufs_fs.cpp` (~1800 lines), `ufs_node.cpp` (~500 lines), `ufs_endian.cpp` (~50 lines) | `Src/Kernel/Fs/Disk/Ufs/` | HIGH |
| 3 | Triple-indirect block traversal (recursive `get_data_block()` to depth 3) | `ufs_fs.cpp` | HIGH |
| 4 | Fragment support: `di_blocks` counts fragments, not blocks | `ufs_fs.cpp` | MEDIUM |
| 5 | Register in `AutoMounter` as `"ufs"` (magic: UFS1=0x011954, UFS2=0x19540119) | `auto_mounter.cpp` | HIGH |
| 6 | Symlink support: short links (< 60 chars) inline in `di_shortlink` over `di_db` | `ufs_node.cpp` | MEDIUM |

---

## Phase 32d — HFS+ (~5000 LOC, 10–14 days) — HIGH

macOS native filesystem. B-trees for catalog and extents overflow, Unicode UCS-2 (NFD), case-insensitive lookup, fork-based I/O, 8 inline extents per fork.

| # | Task | Files | Priority |
|---|------|-------|----------|
| 1 | Headers: `hfsplus_fs.h`, `hfsplus_node.h`, `hfsplus_vh.h`, `hfsplus_catalog.h`, `hfsplus_btree.h`, `hfsplus_extents.h`, `hfsplus_unicode.h` | `Include/Kernel/Fs/Disk/HfsPlus/` | HIGH |
| 2 | Sources: `hfsplus_fs.cpp` (~1000L), `hfsplus_node.cpp` (~500L), `hfsplus_btree.cpp` (~2000L), `hfsplus_catalog.cpp` (~600L), `hfsplus_extents.cpp` (~300L), `hfsplus_unicode.cpp` (~200L) | `Src/Kernel/Fs/Disk/HfsPlus/` | HIGH |
| 3 | **B-tree**: search, insert (split with redistribution), delete (merge). Node cache with LRU eviction | `hfsplus_btree.cpp` | **CRITICAL** |
| 4 | Catalog: `lookup(parent_cnid, name)` via B-tree key `(parentCNID, nodeName Unicode NFD)` | `hfsplus_catalog.cpp` | HIGH |
| 5 | Unicode: UCS-2 BE ↔ UTF-8 (ASCII-only subset); case-insensitive via 256-byte folding table | `hfsplus_unicode.cpp` | MEDIUM |
| 6 | Fork I/O: 8 inline extents + B-tree overflow; allocate via allocation bitmap for extends | `hfsplus_fs.cpp` | HIGH |
| 7 | Hard links: follow indirect link chain to resolve CNID | `hfsplus_fs.cpp` | LOW |
| 8 | Register in `AutoMounter` as `"hfsplus"` (signature "H+" or "HX" at VolumeHeader, sector 2) | `auto_mounter.cpp` | HIGH |

---

## Phase 33 — Volume Layer: LVM, RAID, dm-crypt (~5.5–8.5 days) — MEDIUM

Block device transformations sitting between filesystem and hardware. Zero VFS changes.

```
Filesystem (FAT32/ExFAT/UFS/HFS+/ISO9660)
  └── BlockDevice::read_sectors() / write_sectors()
        └── LvmDevice      → LV offset → (PV, PV offset)
              └── RaidDevice  → stripe/mirror calculation
                    └── CryptoDevice → AES-XTS encrypt/decrypt
                          └── StorageDevice → Hardware (AHCI/NVMe)
```

### 33a — StackableBlockDevice Base Class (~200 LOC, 0.5 day)

| # | Task | Files | Priority |
|---|------|-------|----------|
| 1 | `StackableBlockDevice` holding `Vector<RefPtr<BlockDevice>> m_children` | `Include/Kernel/Driver/Device/BlockDevice/stackable_block_device.h` | HIGH |
| 2 | Subclasses implement `read_sectors()`, `write_sectors()`, `sector_size()`, `sector_count()` | — | HIGH |

### 33b — dm-crypt / AES-XTS (~800 LOC, 2–3 days)

| # | Task | Files | Priority |
|---|------|-------|----------|
| 1-2 | `crypto_device.h/cpp` with AES-XTS via AES-NI (`AESENC`/`AESDEC`/`AESKEYGENASSIST`) | `CryptoDevice` files | HIGH |
| 3 | Per-sector XTS tweak (sector number as tweak; no two sectors encrypt identically) | `crypto_device.cpp` | HIGH |
| 4 | LUKS1/LUKS2 header parser: magic `LUKS\xBA\xBE`, cipher name, key size, PBKDF2 params, key slots | `crypto_device.cpp` | HIGH |
| 5 | PBKDF2-HMAC-SHA256 for key derivation (~200 lines) | `crypto_device.cpp` | MEDIUM |
| 6 | `CryptoDevice::create(child, luks_header)` factory | `crypto_device.cpp` | HIGH |

### 33c — RAID 0/1 (~600 LOC, 1–2 days)

**RAID 0**: `sector_count()` = min(all) × num_disks; chunk-based stripe mapping.  
**RAID 1**: `sector_count()` = min(all); read round-robin; write to ALL disks; degraded mode.

| # | Task | Files | Priority |
|---|------|-------|----------|
| 1-2 | `raid_device.h/cpp` | — | HIGH |
| 3 | Linux mdadm superblock parser (magic `0xa92b4efc` at 4K from end) | `raid_device.cpp` | HIGH |
| 4 | RAID 0 stripe read/write with chunk boundary splitting | `raid_device.cpp` | HIGH |
| 5 | RAID 1 mirror write + round-robin read; degraded mode | `raid_device.cpp` | MEDIUM |

### 33d — LVM: Logical Volume Manager (~1000 LOC, 2–3 days)

| # | Task | Files | Priority |
|---|------|-------|----------|
| 1-2 | `lvm_device.h/cpp` | — | HIGH |
| 3 | PV header parser (sector 0; UUID + metadata area offsets) | `lvm_device.cpp` | HIGH |
| 4 | VG/LV text metadata parser → segment table: `Vector<Segment>` mapping LV extents → (PV, PV extent) | `lvm_device.cpp` | **CRITICAL** |
| 5 | `read_sectors`/`write_sectors` with O(log n) segment table lookup; split I/O on extent boundaries | `lvm_device.cpp` | HIGH |
| 6 | Striped LV: round-robin extent distribution across PVs | `lvm_device.cpp` | MEDIUM |

**Future sub-phases (not planned)**: RAID 5/6 (~1500L parity), LVM snapshots (~800L block-level CoW), dm-integrity/dm-verity.

---

## Phase 34c — Feature Detection (1 day) — MEDIUM

| # | Gap | CPUID Leaf | Priority |
|---|-----|-----------|----------|
| 14 | Physical/virtual address width | `0x80000008 EAX[7:0]/[15:8]` | MEDIUM |
| 15 | 1GB page support | `0x80000001.EDX[26]` | LOW |
| 16 | INVPCID | `0x07.EBX[10]` | LOW |
| 17 | FSGSBASE | `0x07.EBX[0]` | LOW |
| 18 | UMIP | `0x07.EBX[2]` | LOW |
| 19 | AVX2/AVX-512/FMA/BMI/RDRAND detection | `0x07.EBX`, `0x01.ECX` | LOW |
| 20 | LA57 (5-level paging) | `0x07.ECX[16]` | LOW |
| 21 | CET (Shadow Stack + IBT) | `0x07.ECX[7]` | LOW |

## Phase 34d — SMP Hardening (1–2 days) — MEDIUM

| # | Gap | Fix | Priority |
|---|-----|-----|----------|
| 22 | No IRQ affinity / load balancing | Logical destination mode or APIC flat cluster | MEDIUM |
| 23 | No microcode update on AP | Load `IA32_BIOS_UPDT_TRIG` on each AP before `online_flag = 1` | MEDIUM |
| 24 | No MTRR synchronisation | Read BSP MTRRs; program identically on AP | MEDIUM |
| 25 | Trampoline at 0x8000 may conflict with SMM | Relocate to 0x10000 if SMM detected | LOW |
| 26 | No APIC ID → topology mapping | Parse CPUID 0x0B or 0x1F; build `CpuTopology` struct | LOW |

---

## Phase 35a — QoS Exposure in /proc (0.5 day) — MEDIUM

| # | Task | Files |
|---|------|-------|
| 1 | Add QoSClass, nice, SchedulingPolicy, mlfq_level, cpu_affinity to `/proc/<pid>/stat` | `proc_pid_stat_node.cpp` |
| 2 | Add `QoS:`, `Nice:`, `Policy:`, `MLFQ:`, `Cpus_allowed:` to `/proc/<pid>/status` | `proc_process_node.cpp` |
| 3 | New `/proc/<pid>/sched` node | `proc_pid_sched_node.h/cpp` |
| 4 | `/proc/sys/kernel/sched_qos_stats` showing per-QoS-class task counts | `proc_sys_kernel_node.cpp` |

**Impact**: `ps -eo pid,qos,nice,policy` becomes possible. `top`/`htop` show real scheduling state.

## Phase 35c — Transitive Turnstile Chain (1 day) — MEDIUM

| # | Task | Files |
|---|------|-------|
| 1 | Walk `holder->active_turnstile->chain` to boost waiter's QoS transitively | `turnstile.cpp:25-56` |
| 2 | `unboost_task()`: walk chain, restore all intermediate tasks' original QoS | `turnstile.cpp:58-76` |
| 3 | `MAX_CHAIN_DEPTH = 8` enforcement (already declared in `turnstile.h`) | `turnstile.h` |
| 4 | Test: 3 tasks A→B→C, verify C gets A's QoS through chain | `tests/Scheduler/test_turnstile.cpp` |

**Impact**: Priority inversion with 3+ participants (proxies, middleware, notification chains) solved transitively.

---

---

## Phase 20 — POSIX Networking Syscalls — MEDIUM

~25 advanced networking syscalls still missing:

| Group | Syscalls |
|-------|---------|
| Advanced socket opts | `SO_RCVBUF`, `SO_SNDBUF`, `SO_KEEPALIVE`, `SO_LINGER`, `SO_REUSEADDR`, `SO_REUSEPORT` |
| Multicast | `IP_ADD_MEMBERSHIP`, `IP_DROP_MEMBERSHIP`, `IP_MULTICAST_IF` |
| Non-blocking I/O | `MSG_DONTWAIT` in send/recv; `O_NONBLOCK` on sockets |
| Address info | `getaddrinfo` (requires resolver integration) |
| Advanced TCP | `TCP_NODELAY`, `TCP_KEEPIDLE`, `TCP_KEEPINTVL`, `TCP_KEEPCNT` |
| Ancillary data | `sendmmsg(307)`, `recvmmsg(299)` |

---

## Phase 43 — Kernel Test Harness (Phase 21 reborn) — HIGH

Target: Kernel critical paths at 75%.

### 43a — Test Infrastructure (2 days)
| # | Task | Files |
|---|------|-------|
| 1 | Kernel test runner — run in host context with mocked hardware | `tests/Kernel/test_runner.cpp` |
| 2 | Mock page allocator, mock timer, mock interrupt controller | `tests/Kernel/mocks/` |
| 3 | CI integration — `xmake run Test` covers kernel tests | `xmake.lua` |

### 43b — VFS Tests (3 days)
- Path resolution (absolute, relative, symlink chains, mount point crossing)
- Dentry caching (insert, evict, concurrent access)
- File description offset, seek, concurrent read/write

### 43c — Memory Manager Tests (2 days)
- Buddy allocator: alloc/free at each order (0–10)
- Fragmentation scenario: alloc N pages of order 0, free alternating, alloc order 1
- Multi-zone: alloc from NORMAL zone, exhaust, verify DMA zone not touched
- SlabAllocator: alloc/free from each cache size

### 43d — ELF Loader Tests (2 days)
- Header validation: wrong magic, wrong class, wrong machine
- Relocation application: R_X86_64_64, R_X86_64_RELATIVE, R_X86_64_GLOB_DAT
- Segment loading: PT_LOAD with gap, overlapping segments (should reject), file-size bounds

### 43e — Scheduler Tests (2 days)
- MLFQ level demotion on allotment expiry
- QoS class priority ordering
- Turnstile chain boost/unboost (priority inheritance)
- Work-stealing between CPU queues

### 43f — TCP State Machine Tests (2 days)
- SYN → SYN-ACK → ACK (connect)
- Data exchange + sliding window
- FIN → FIN-ACK → ACK (close)
- Retransmit timer: send packet, drop ACK, verify retransmit

---

## ELF Loader — Remaining Low-Priority Items

| # | Task | Files | Priority |
|---|------|-------|----------|
| 13 | Cache program headers — parse once, pass `Vector<Elf64_Phdr>` by const ref | `elf_loader_core.cpp` | LOW (was reverted: caused Error 0 on init loading; needs investigation) |
| 16 | Unify TLS setup — move FS_BASE write into loader; init_task.cpp has no TLS | `elf_loader_core.cpp`, `execve.cpp`, `init_task.cpp` | LOW |
| 17 | ELF loader tests | `tests/Loader/` | LOW |
| Symbol versioning | DT_VERSYM/VERNEED parsing | `dynamic_domain.cpp` | LOW |

---

## Phase 44 — Thread Group Signal Delivery — HIMMEDIATE

Signal delivery currently targets individual threads, not thread groups. POSIX requires signals to be deliverable to any thread in the group (with specific rules for SIGCHLD, SIGSTOP, etc.). CLONE_THREAD and tgid tracking exist, but signal routing is incomplete.

### 44a — Signal Delivery to Thread Groups (3 days)

| # | Task | Files |
|---|------|-------|
| 1 | `tgkill()` syscall — signal specific thread within tgid | `Process/signal_tgkill.cpp`, `syscall.cpp` |
| 2 | `SignalManager::deliver_to_group(sig, tgid)` — pick target thread via priority/fallback | `signal_delivery.cpp` |
| 3 | Handle `SIGCHLD` for parent's thread group | `Process/exit.cpp` |
| 4 | `exit_group()` properly signals all threads in tgid | `Process/exit_group.cpp` |

### 44b — Signal Mask Inheritance (1 day)

| # | Task | Files |
|---|------|-------|
| 1 | CLONE_THREAD inherits parent's signal mask | `clone.cpp` |
| 2 | execve resets signal masks for all threads | `execve.cpp` |
| 3 | sigsuspend/rt_sigtimedwait work per-thread within group | `signal_syscalls.cpp` |

---

## Phase 45 — Security Hardening — MEDIUM

### 45a — CSPRNG Seeding (0.5 day)

| # | Task | Files |
|---|------|-------|
| 1 | Seed ChaCha20 from RDTSC + RDRAND early in init() | `init.cpp`, `chacha20.cpp` |
| 2 | Uncomment seed path (currently lines 105-107) | `init.cpp` |
| 3 | Verify /dev/urandom produces non-deterministic output | `urandom_device.cpp` |

### 45b — KPTI / Meltdown Mitigation (2 days)

| # | Task | Files |
|---|------|-------|
| 1 | Two PML4 roots: kernel root + user root (kernel unmapped in user mode) | `virtual_memory_manager.cpp` |
| 2 | CR3 swap on syscall entry/exit and interrupt entry/exit | `syscall_entry.cpp`, `interrupt_controller.cpp` |
| 3 | Trampoline pages (kernel mappings in user page table for entry/exit) | `trampoline.S` |

### 45c — Address Space Layout Randomisation Hardening (1 day)

| # | Task | Files |
|---|------|-------|
| 1 | Randomise mmap base address (currently fixed) | `mmap.cpp` |
| 2 | Randomise stack base on execve | `execve.cpp` |
| 3 | Add guard page below stack | `execve.cpp` |
