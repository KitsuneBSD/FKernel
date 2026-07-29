# FKernel TODO

> **Lean version** — only open bugs and pending work. See also:
> - `.ai-docs/CHANGELOG.md` — everything completed
> - `.ai-docs/ROADMAP.md` — future phases with full detail
> - `.ai-docs/AUDITS.md` — ELF, IPC, distro-readiness, and x86_64 audit findings

---

## Open Bugs

All previously-listed bugs have been resolved. See `.ai-docs/AUDITS.md` for full history.

- **Bug 9** (CSPRNG unseeded before ASLR): ✅ Fixed — `init()` calls `set_entropy_callback` + `ChaCha20PRNG::initialize()` before any ELF load. `aslr_random_base()` now uses `ChaCha20PRNG::fill_buffer` instead of tick-counter arithmetic.
- **Bug 10** (`s_global_libraries` SMP-unsafe): ✅ Fixed — `s_library_lock` Spinlock guards all accesses in `dynamic_domain.cpp`.
- **Bug 18/19** (`Endpoint::wait/wait_timeout` data race): ✅ Fixed — both paths re-acquire `m_lock` before reading/clearing `m_pending_bits` after wakeup.
- **Bug 20** (`signal_with_payload` discards payload): ✅ Fixed — ring-buffer `m_payloads[NOTIFICATION_MAX_PAYLOADS]` in Endpoint stores payloads; `payload_push/payload_pop` exposed.

---

## LibC/LibFK Audit — Correctness & UB Issues

Found during static architecture audit. See also `.ai-docs/AUDITS.md`.

### UB / Bugs

| # | File | Line | Issue | Severity |
|---|------|------|-------|----------|
| 1 | `Include/LibFK/Container/vector.h` | 147 | Integer overflow in `new_capacity * sizeof(T)` with no overflow check before `allocate()` | ✅ **Fixed** — `reallocate_to()` now guards with `(~size_t(0)) / sizeof(T)` before multiply |
| 2 | `Include/LibFK/Traits/type_traits.h` | 179 | `is_base_of<void, int>` gives wrong result (non-class types not handled) | ✅ **Fixed** — `detail::is_base_of_impl` uses `__is_class` guard; non-class pairs always return false |

### Design / Performance

| # | File | Line | Issue | Severity |
|---|------|------|-------|----------|
| 3 | `Include/LibFK/Synchronization/spinlock.h` | 22–28 | `cpuid` instruction on every lock/unlock (~100 serializing cycles) | ✅ **Fixed** — replaced with `mov %gs:32` (single cycle read of CpuControlBlock::cpu_id) |
| 4 | `Include/LibFK/Core/result.h` | 29–38 | `value()` on error only warns, does not assert — silent invalid-state propagation | ✅ **Fixed** — `value()` now `ASSERT(false)` after warn when called on error |
| 5 | `Include/LibFK/Memory/retain_ptr.h` | 39 | Refcount allocated separately from object (`allocate<uint32_t>()`): fragmentation + leak if `allocate` fails after object constructed | Design — RetainPtr deprecated; no production callers |
| 6 | `Include/LibFK/Memory/ref_counted.h` | 17 | `virtual ~RefCounted()` adds vtable to every refcounted class | Overhead — accepted; RefCounted is the RTTI-bearing base |
| 7 | `Include/LibFK/Text/string.h` | — | SSO never shrinks back to inline after growth — permanent memory retention | Memory — not yet fixed |
| 8 | `Include/LibFK/Synchronization/interrupt_disabler.h` | 13–15 | `cli` without checking IF already = 0 (nesting edge case) | **False positive** — `m_previous_state` captures IF before cli; destructor only calls sti if IF was previously enabled. Nesting is handled correctly. |
| 9 | `Include/LibFK/Core/assertions.h` | 5 | `#define assert(x) ASSERT(x)` shadows the standard name — may conflict with `<cassert>` | Convention — low risk; FKernel never includes `<cassert>` |

---

## LibC/LibFK Complexity — Reduction (Done)

**Bottom-up analysis** revealed that all problematic functions/containers are dead code
or test-only. No kernel or LibFK production code calls them. Decision: deprecate, not rewrite.

| Lote | What | Verdict | Status |
|------|------|---------|--------|
| 1 | LibC string functions (`strstr`, `strpbrk`, `strspn`, `strcspn`, `strtok`) | **Dead code** — zero call sites in kernel or LibFK. Only used in tests. | **Won't fix** — leave as-is |
| 2 | `String::find` / `String::contains` | **Test-only** — zero production call sites. O(n·m) is harmless. | **Won't fix** — leave as-is |
| 3 | `Map`, `Set`, `MultiMap`, `MultiSet`, `Deque` | **Dead containers** — zero production call sites. Only used in container tests. | **Done** — `[[deprecated]]` |
| 3 | `Stack::clear()` leak | Only use is a `StaticArray` with POD (`char*`) — destructor loop for non-POD not needed yet | **Done** — fixed anyway |
| 4 | `StringBuilder::append(const char*)` char-by-char | **Actual hot spot** — used in ELF parsing, path resolution, kprintf. | **Done** — now `push_range` (block memcpy) |
| 4 | `StringBuilder::to_string()` char-by-char | Called once at end; buffer not null-terminated so can't use `append(const char*)`. Not worth adding API for. | **Won't fix** |
| 5 | `RetainPtr` single allocation | **Dead code** — no production call sites. Only `RetainPtr` references are in test code. | **Done** — `[[deprecated]]`, 26 kernel includes removed |
| 6 | `DefaultHasher` safety | Type-safe already — `static_cast<size_t>` only compiles for integrals/pointers/enums. Struct keys are compile error. | **Won't fix** — no issue |

Savings: ~2 weeks of engineering avoided by not rewriting dead code.

---

## ✅ Phase 27 — VFS + Capability Integration — DONE

All CSpace+FD wiring is implemented:
- `CapabilityType::FileDescription`, rights bitmask in `capability.h`
- `CSpace::install_fd/lookup_fd/revoke_fd/clone_fd_caps` in `cspace.h`
- `FileDescription::resolve_dentry()` called in all read/write/seek/ioctl paths
- All syscalls (open, close, dup, dup2, pipe, fork, execve cloexec) use CSpace via Task FD methods
- `TaskFiles::cap_handles` parallel array tracks CSpace handle per FD slot
- `fork.cpp` copies FD capabilities to child's CSpace with inherited rights
- `execve.cpp` revokes cloexec FDs from CSpace on exec

---

## ✅ Phase 29b — CSpace Wiring — DONE

- `Task::get_file_descriptor_with_rights(fd, required)` added — checks CSpace cap rights before returning descriptor
- `CSpace::get_fd_rights(handle)` added — returns CapabilityRights for a FileDescription cap
- `sys_read`/`sys_readv`/`sys_pread64` check `CapabilityRights::Read`
- `sys_write`/`sys_writev`/`sys_pwrite64` check `CapabilityRights::Write`
- `sys_lseek` checks `CapabilityRights::Seek`
- `sys_ioctl` checks `CapabilityRights::Ioctl`
- Rights are set at open time via `fd_flags_to_rights(flags)` from O_RDONLY/O_WRONLY/O_RDWR

## ✅ Phase 29d — Unified Revocation — DONE

- SemNode/MqueueNode delegate generation to `m_endpoint.revoke()` (never had own `m_generation`)
- `close_file_descriptor()` already calls `revoke_fd(handle)` → CSpace entry cleared on every close
- All IPC close paths flow through `sys_close` → `close_file_descriptor` which handles CSpace revocation

---

## ✅ Phase 32c — UFS/UFS2 — DONE

All tasks complete: 5 headers + 2 sources in `Fs/Disk/Ufs/`, triple-indirect block traversal (UFS1 and UFS2), fragment-level addressing via `fsize`, AutoMounter registration as "ufs", short symlink inline support in `UfsNode::read_link()`.

## ✅ Phase 32d — HFS+ — DONE

All tasks complete:
- 7 headers in `Include/Kernel/Fs/Disk/HfsPlus/`: vh, btree, catalog, extents, unicode, node, fs
- 6 sources in `Src/Kernel/Fs/Disk/HfsPlus/`: btree, catalog, extents, unicode, node, fs
- B-tree engine: read_node + binary search in leaves, index traversal (`BTreeEngine::search`)
- Catalog lookup/iterate via B-tree key (parentCNID + UCS-2 name); case-insensitive (HFS+) and binary (HFSX)
- Unicode: UCS-2 BE ↔ UTF-8, case-folding for Latin-1 range, `hfsplus_strcmp`
- Fork I/O: 8 inline extents + extents overflow B-tree fallback (`resolve_logical_block`)
- Registered in `AutoMounter` as `"hfsplus"` (sig 0x482B 'H+' or 0x4858 'HX' at byte 1024)

---

## ✅ Phase 33 — Volume Layer — DONE

`StackableBlockDevice` base class, RAID 0/1 (`RaidDevice`), LVM linear (`LvmDevice`), and AES-256-XTS dm-crypt (`CryptoDevice`) with LUKS1 probe. All four sub-phases complete.

---

## ✅ Phase 34b — Remaining x86_64 Fixes — DONE

| # | Fix | Status |
|---|-----|--------|
| 7 | KPTI (Meltdown mitigation) | ✅ Two PML4 roots + CR3 swap on syscall entry/exit. Shadow PML4 per process. |
| 13 | Early serial fallback on COM1 | ✅ `serial::init()` first call in `kernel_entry()` |

## ✅ Phase 34c — Feature Detection — DONE

CPUID leaves for physical/virtual address width, 1GB pages, INVPCID, FSGSBASE, UMIP, AVX2/AVX-512, LA57, CET. All detected in `cpu.cpp::detect_cpu_features()`. FSGSBASE (CR4.FSGSBASE) and UMIP (CR4.UMIP) are now enabled via `arch_enable_cpu_features`. Also fixed XSAVE/AVX detection bug (was reading leaf 7 ECX instead of leaf 1 ECX).

## ✅ Phase 34d — SMP Hardening — DONE

- **APIC topology** (`read_apic_topology`): CPUID 0x0B leaf, fills `g_cpu_topology[]` with smt/core/socket IDs
- **MTRR sync** (`mtrr_read_all` / `mtrr_apply`): BSP snapshots all MTRRs; each AP restores them before going online
- **Microcode update** (`apply_microcode`): MSR 0x79 (IA32_BIOS_UPDT_TRIG) written on each AP with pre-loaded blob
- **IRQ affinity** (`setup_lapic_logical_dest`): LAPIC flat model (DFR=0xFFFFFFFF, LDR bit-per-CPU) on MMIO APICsL
- BSP calls `mtrr_read_all` + `read_apic_topology(0)` in `SyscallManager::initialize()`
- APs call all four routines in `ap_entry()` before setting `online_flag`

---

## ✅ Phase 35a — QoS Exposure in /proc — DONE

`/proc/<pid>/sched` exposes qos, nice, base_priority, mlfq_level, policy, cpu_affinity, cpu_time_consumed, boosted, turnstile_active. `/proc/<pid>/stat` includes qos/nice/policy/mlfq_level/cpu_affinity fields.

## ✅ Phase 35c — Transitive Turnstile Chain — DONE

Implemented in `Src/Kernel/Scheduler/turnstile.cpp`:
- `extend_chain()`: extends active_turnstile chain for re-boost on already-boosted holder
- `propagate_transitive()`: follows `holder->resources.ipc.pending_turnstile` chain to boost all intermediate holders (3+ participants)
- `remove_from_holder_chain()`: removes one waiter from a holder's chain on unboost
- `pending_turnstile` now set on the waiter in both the fresh-boost and chain-extend paths
- `unboost_task()` clears `pending_turnstile` on waiters and calls `remove_from_holder_chain` for transitive unboost

---

## Phase 39 — Complexity Hardening (10–14 days) — MEDIUM

Replace O(N) data structures with O(1) or O(N log N) equivalents throughout the kernel. See `.ai-docs/CHANGELOG.md#phase-39` for full spec.

### 39a — Physical Memory Manager

| Atual | Complexidade | Substituir por |
|-------|-------------|----------------|
| `Bitmap::alloc()` scan word-by-word | O(frames/64) | ✅ `m_hint` word-index added to `Bitmap<T>`. `alloc()` two-phase scan starting at hint. `free_hint(index)` called in `free_page()` and `free_contiguous()` to regress hint. O(1) amortized for sequential alloc/free patterns. |
| `BuddyState::remove()` linked-list search | O(L) | ✅ `UnorderedSet<uintptr_t> m_free_sets[NUM_ORDERS]` + doubly-linked `FreeBlock`. `remove()` = set lookup O(1) + prev/next unlink O(1). |
| `BuddyState::remove()` ×10 no coalesce | O(10·L) | ✅ Same fix — all coalesce removes now O(1) amortized. |

### 39b — Scheduler

| Atual | Complexidade | Substituir por |
|-------|-------------|----------------|
| `m_sleep_queue` scan a cada tick | O(S) | ✅ Sorted insertion on `sleep_current()` (O(S) insert, O(W) on_tick where W=woken tasks). `IntrusiveList::insert_before()` added. `on_tick()` exits at first task not yet due. |
| `find_task(pid)` scan 6 filas | O(N_total) | ✅ `HashMap<uint64_t, Task*> m_task_map` in SchedulerManager. `add_task()` inserts, `reap_zombie()` removes, `find_task()` uses O(1) map lookup with CPU-running-task fallback |
| `pick_next()` affinity scan por nível | O(·N_lvl) | ✅ `MLFQQueue::affinity_bitmap` + `any_cpu_count`. `can_serve_cpu(id)` = O(1). `pick_next()` skips levels with no eligible task. `enqueue()`/`dequeue()`/`dequeue_front()` maintain accounting; `rebuild_affinity_bitmap()` called lazily when pinned task leaves. |

### 39c — IPC / CSpace

| Atual | Complexidade | Substituir por |
|-------|-------------|----------------|
| `cap_grant/transfer` via `find_task` | O(N_total) | Resolvido por HashMap em 39b |
| `CSpace::find_by_object/remove_by_object` | O(C) | ✅ `HashMap<uint64_t, uint32_t> m_object_index` em CSpace. `install()` insere, `remove()` apaga (guarded: só remove se idx == handle), `find_by_object/remove_by_object` fazem O(1) lookup |
| `CSpace::grant_all_to` | O(C) | ✅ `m_used_slots` Vector tracks valid slot indices. `install()` appends, `remove()` swap-with-last. `grant_all_to()` iterates only `m_used_slots` — O(valid). `clone_fd_caps()` also iterates `m_used_slots`. |

### 39d — VFS

| Atual | Complexidade | Substituir por |
|-------|-------------|----------------|
| Dentry child lookup `find_if(name)` | O(C) | ✅ `HashMap<fk::text::String, Dentry*> m_child_map` in Dentry. `lookup()` and `add_child()` maintain the map; raw ptr safe because lifetime owned by `m_children` Vector |
| `readdir()` dedup `insert_if_absent` | O(E²) | ✅ `HashMap<String, bool> seen` local to each readdir call; `add_directory_entry` accepts seen-set and does O(1) check |
| File lock `release_all_for_process` | O(L²) | ✅ Single-pass compaction (write-index pattern) — O(L) scan, no per-element shifts |

### 39e — Network

| Atual | Complexidade | Substituir por |
|-------|-------------|----------------|
| ARP lookup/update/expire | O(E) / O(E²) | ✅ `HashMap<uint32_t, ArpEntry> m_table` in ArpTable. Added `for_each` to HashMap for expiry iteration. `update/lookup/remove` are O(1) |
| TCP read `dequeue_front` shift left | O(B) | ✅ `CircularBuffer<uint8_t, 65536>` replaces `Vector<uint8_t>` for `recv_buf`. Added `drain_to()` + `push_range_checked()` to CircularBuffer |
| TCP accept `remove_at` left-shift | O(Q) | ✅ Swap-with-last + `pop_back()` = O(1) removal; order doesn't matter since any Established socket is accepted |

### 39f — KQueue

| Atual | Complexidade | Substituir por |
|-------|-------------|----------------|
| `nearest_timer_deadline()` brute force | O(R) | ✅ `m_min_timer_deadline` cached in KQueueNode. Updated on EV_ADD/EV_ENABLE timer events; `recompute_min_timer()` (scans only on delete/disable/fire) |
| `EV_ENABLE/DISABLE` scan total | O(R) | ✅ `HashMap<uint64_t, size_t> m_event_index` keyed by `(ident << 16 \| filter)`. EV_DELETE maintains index with swap-with-last + moved-element re-index |

---

## Phase 40 — Userspace Complex Subsystems (UDI) — MEDIUM

**Princípio**: Kernel fornece interfaces **genéricas** (PCI bus, BlockDevice, VFS Node, DRM,
HCI socket); userspace implementa a **complexidade específica** (OpenZFS, XFS, GPU compute,
wireless stack). Tudo que já está no kernel **permanece** no kernel — não há migração.

Exceptions ao princípio:
- SquashFS fica no kernel (simples + boot-critical — decompress + block index)
- Qualquer subsistema userspace complexo precisa de uma interface genérica no kernel primeiro
  (ex: sem VFS/UserFsNode, não há filesystem userspace; sem DRM/GEM, não há GPU userspace)

### 40a — Hardware Access Primitives (4 itens, ~1–2 weeks) — HIGH

Pré-requisito para qualquer driver/filesystem userspace. Adicionar ao kernel as 4 primitivas
que faltam para delegar hardware a um processo.

| # | Primitiva | Mecanismo | Arquivos | Days |
|---|-----------|-----------|----------|------|
| 1 | Interrupção → Endpoint | ✅ `IrqBinding` (RefCounted): ISR calls `ep->signal(SIGNAL_BIT=1)`. `s_bindings[256]` holds owning RefPtrs. `sys_bind_irq(vector, ep_handle)` → installs ISR + unmasks IRQ + returns `CapabilityType::Irq` handle. SYS_BIND_IRQ=406 registered | `irq_binding.h/cpp`, `capability.h`, `bind_irq.cpp`, `syscall.cpp` | — |
| 2 | mmap físico userspace | ✅ `MAP_PHYSICAL = 0x400` in `mmap.cpp`. `offset` = physical base address; maps contiguously with `PageFlags::User | CacheDisabled`; honours PROT_WRITE and MAP_FIXED | `mmap.cpp` | — |
| 3 | PCI config space userspace | ✅ `PIOC_READ_CONFIG`/`PIOC_WRITE_CONFIG` ioctl on `/dev/pci`. `PciConfigRequest` struct holds bus/device/function/offset/value. Uses `PciManager::read_config_dword/write_config_dword` | `pci_node.h/cpp` | — |
| 4 | DMA compartilhável | ✅ `DmaShm`: alocação com `PMM::alloc_contiguous(order)` (buddy allocator), pages mapeadas com `PageFlags::User | Writable | CacheDisabled`. `sys_dma_alloc(size)` → instala `CapabilityType::DmaShm` + retorna handle; `sys_dma_map(handle, vaddr_hint)` → mapeia buffer no espaço do processo. SYS_DMA_ALLOC=407, SYS_DMA_MAP=408 registrados. | `dma_shm.h/cpp`, `dma_shm.cpp` (syscall), `syscall.cpp`, `capability.h`, `numbers.h` | — |

### 40b — Userspace Driver Protocol (3–5 days) — MEDIUM

Protocolo e biblioteca para um processo userspace agir como driver de dispositivo.

| # | Task | Detalhe | Days |
|---|------|---------|------|
| 1 | `libdriver.so` | Lib userspace: `udi_claim_pci(class, subclass)` → recebe capability do BAR + IrqBinding; `udi_recv_irq()` → `ep->wait()`; `udi_map_bar(n)` → `MAP_PHYSICAL`; `udi_dma_alloc(size)` → kernel alloc contíguo mapeado no user | 2 |
| 2 | `/dev/udi` device events | ✅ `UdiNode` (`Include/Kernel/Driver/Udi/udi_node.h/cpp`): `CircularBuffer<UdiEvent, 64>` + blocking `read()` via `Endpoint::wait()`. `UdiEvent` carries bdf, class/subclass, vendor/device_id, and `UdiBarInfo[6]` (base, size, flags). Registered as `/dev/udi` by PciManager::initialize(). | — |
| 3 | PCI device fallback | ✅ `instantiate_drivers()` and `handle_hotplug_event()` call `m_udi_node->push_attach(device)` when no kernel driver claims a device. `handle_hotplug_event()` calls `push_detach()` on removal of userspace-held devices. | — |
| 4 | Initrd com drivers userspace | Script de build empacota `libdriver.so` + drivers userspace na initrd | 1 |

### 40c — FUSE-like Filesystem Protocol (5–7 days) — MEDIUM

Interface **genérica** do kernel para filesystems userspace.

| # | Task | Detalhe | Days |
|---|------|---------|------|
| 1 | `UserFsNode` | ✅ `UserFsNode` (`Include/Kernel/Fs/UserFs/user_fs_node.h/cpp`): VFS Node that serialises `read/write/lookup/mkdir/readdir/unlink/rmdir/truncate/fsync` as `FsRequest`/`FsResponse` pairs sent through a parent `UserFsServer`. `mount("", target, "userfs")` allocates a server, registers it as `/dev/userfs<N>`, mounts a root `UserFsNode(ino=1)`. One request in flight per mount at a time (Endpoint blocking). | — |
| 2 | Protocolo `user_fs.h` | ✅ `Include/Kernel/Fs/UserFs/user_fs_protocol.h`: `FsRequest`/`FsResponse`/`FsDirent`/`FsAttr`; opcodes `Lookup/Read/Write/Mkdir/Readdir/Unlink/Rmdir/Getattr/Truncate/Fsync`; `FS_NAME_MAX=256`, `FS_DATA_MAX=4096`. `UserFsServer` (`user_fs_server.h/cpp`): CharacterDevice at `/dev/userfs<N>`; daemon `read()` blocks on pending request, `write()` delivers response and signals VFS caller. | — |
| 3 | `libuserfs.so` | Lib userspace: `ufs_register(endpoint)` → faz `ep->recv()` loop, dispatches para callbacks do driver | 2 |
| 4 | Exemplo: Ext4 userspace | `ext4_ufs` linked com `libuserfs.so`, faz I/O via `open("/dev/nvme0n1")` no userspace | 2 |
| 5 | Initrd com FS userspace | Root fs pode ser userspace — kernel monta tmpfs, init carrega ex: `openzfs_ufs`, monta root real | 1 |

### 40d — New Complex Subsystems in Userspace (ongoing) — LOW

Tudo abaixo é **novo** — nada existente no kernel é migrado. Cada entrada requer a interface
genérica no kernel primeiro.

#### Filesystems

| FS | Kernel interface | Why userspace | Est. lines |
|----|-----------------|---------------|------------|
| **OpenZFS** | UserFsNode (40c) | 200KLOC, ARC, RAID-Z, snapshots, dedup | ~200K |
| **XFS** | UserFsNode (40c) | libxfs já existe userspace-ready, B+tree dirs | ~80K |
| **Btrfs** | UserFsNode (40c) | CoW, subvolumes, checksums, RAID | ~80K |
| **APFS** | UserFsNode (40c) | B-tree, crypto, fusion, snapshots | ~? |
| **NTFS-3G** | UserFsNode (40c) | Já é FUSE no Linux, reuso direto | ~30K |

#### Drivers

| Driver | Kernel interface | Why userspace |
|--------|-----------------|---------------|
| **GPU 3D/compute** | DRM/GEM (futuro, scanout + buffer mgmt) | Vulkan compiler, command scheduling, shaders |
| **Audio stack** | Audio DMA engine (futuro) | Codec, mixer, policy, Bluetooth A2DP |
| **Wireless (802.11)** | HCI/netlink socket (futuro) | MAC firmware, crypto, scanning, mesh |
| **Bluetooth** | HCI socket (futuro) | L2CAP, RFCOMM, AVDTP, profiles, pairing |
| **TPM 2.0** | /dev/tpm character device | tpm2-tss já userspace, crypto policy |

### Boot Boundary

```
KERNEL-SPACE (tudo que já existe permanece):
  NVMe, AHCI, ATA PIO        — drivers de armazenamento atuais
  E1000                       — driver de rede atual
  Ext2/3/4, FAT32, ExFAT     — filesystems de disco atuais
  HFS+, UFS, ISO9660          — filesystems de disco atuais
  tmpfs, devfs, procfs        — filesystems virtuais atuais
  SquashFS                    — novo, simples, boot-critical
  PipeFs, PTY, eventfd, ...   — IPC mechanisms atuais
  PS/2, serial, VGA/VESA      — console de boot atual
  TCP/IP, ARP, DHCP, DNS      — rede atual
  /dev/udi                    — ponte genérica para userspace drivers

USERSPACE (apenas novos desenvolvimentos):
  OpenZFS, XFS, Btrfs, APFS, NTFS-3G
  GPU 3D/compute, audio, wireless, Bluetooth, TPM
```

---

## Phase 41 — Distro Boot Blockers — HIGH

Bloqueadores críticos para bootar uma distro real (Alpine, Gentoo) no FKernel.
Pré-requisito: pivot_root + filesystems básicos (já existentes).

### 41a — Signal & Threading (NSIG + robust_list) — HIGH

| # | Item | Detalhe | Arquivos | Dias |
|---|------|---------|----------|------|
| 1 | Expandir NSIG de 32 para 65+ | ✅ NSIG=65, SIGRTMIN=34, SIGRTMAX=64. pending/blocked → uint64_t in task.h. signalfd, sigsuspend, rt_sigtimedwait updated. | — | — |
| 2 | Implementar `set_robust_list`/`get_robust_list` | ✅ Implemented in `robust_list.cpp`, registered in syscall.cpp, robust_list_head in task.h | — | — |
| 3 | Implementar `personality` syscall | ✅ Stub returning 0 (PER_LINUX) — `personality.cpp` registered in syscall.cpp | — | — |
| 4 | Implementar `prctl` syscall | ✅ PR_SET/GET_NAME, PR_GET/SET_DUMPABLE, PR_SET/GET_PDEATHSIG, PR_SET/GET_NO_NEW_PRIVS — `prctl.cpp` registered | — | — |
| 5 | Thread group (tgid) e CLONE_THREAD | ✅ `tgid` field added to TaskIdentity. CLONE_THREAD sets child tgid=parent tgid. exit_group kills all tasks with same tgid. `send_signal_to_tgid()`: SIGKILL/SIGSTOP broadcast to all threads; others go to first non-blocking thread. `sys_kill` uses `send_signal_to_tgid`. | `task.h`, `clone.cpp`, `scheduler_introspection.cpp`, `signal.cpp` | — |

### ✅ 41b — sysfs (minimal) — DONE

| # | Item | Detalhe | Arquivos | Dias |
|---|------|---------|----------|------|
| 1 | sysfs mínimo | ✅ `/sys` com: `/sys/block/`, `/sys/class/`, `/sys/devices/`, `/sys/fs/`. Mounted at boot | `Include/Kernel/Fs/Virtual/SysFs/`, `Src/Kernel/Fs/Virtual/SysFs/` | — |
| 2 | Block devices em `/sys/block/` | ✅ Cada StorageDevice em `/sys/block/<name>/` com `size`, `sector_size` via DriverManager | `sys_block_dir_node.h/cpp`, `sys_block_dev_dir_node.h/cpp` | — |
| 3 | PCI devices em `/sys/devices/` | ✅ Cada PciDevice em `/sys/devices/pci/<BDF>/` com vendor, device, class | `sys_pci_dir_node.h/cpp`, `sys_pci_dev_dir_node.h/cpp` | — |

### 41c — /proc/self/ completeness — HIGH

| # | Item | Detalhe | Arquivos | Dias |
|---|------|---------|----------|------|
| 1 | `/proc/self/fd/` | ✅ Implemented: `proc_pid_fd_node.h/cpp` wired into `proc_pid_dir_node.cpp` | `proc_pid_fd_node.h/cpp` | — |
| 2 | `/proc/self/maps` | ✅ Implemented: `proc_pid_maps_node.h/cpp` — reads from `task.resources.memory.regions.list` | `proc_pid_maps_node.h/cpp` | — |
| 3 | `/proc/self/exe` | ✅ Implemented: `proc_pid_exe_node.h/cpp`, exe_path set in execve.cpp | `proc_pid_exe_node.h/cpp` | — |
| 4 | `/proc/filesystems` | ✅ Implemented: `proc_filesystems_node.h/cpp` wired into `proc_fs_node.cpp` | `proc_fs.cpp` | — |

### 41d — Device Model Fixes — MEDIUM

| # | Item | Detalhe | Arquivos | Dias |
|---|------|---------|----------|------|
| 1 | Block devices em `/dev/` | ✅ ATA devices registered via `DriverManager::register_device()` which auto-routes to DevFs. Accessible as `/dev/<name>` | `ata_controller.cpp`, `driver_manager.cpp` | — |
| 2 | Partições em `/dev/` | ✅ `PartitionManager::add_partition()` calls `DriverManager::register_device(partition)`. Partition naming updated to `<disk>p<N>` format (e.g., `ad0p1`) via `StorageDeviceName::partition()` | `storage_device_name.h`, `mbr.cpp`, `gpt.cpp` | — |
| 3 | Kernel cmdline parsing | ✅ Parses `init=`, `root=`, `rootfstype=`, `quiet` from Multiboot2 TagType::Cmdline. `/proc/cmdline` exposed. `init_task.cpp` uses `get_init_path()` | `boot_info.h/cpp`, `proc_cmdline_node.h/cpp` | — |
| 4 | Corrigir `DT_type` no getdents64 | ✅ `definitions.h` uses Linux DT_DIR=4, DT_BLK=6, DT_REG=8, DT_LNK=10. `vfs_directory.cpp` maps `entry.type` to correct DT_ values. | `definitions.h`, `vfs_directory.cpp` | — |

### Expected Result

Com 41a-41d implementados, um initrd TAR contendo Alpine's `/sbin/init` + musl + BusyBox
deve bootar até um shell funcional. Sem sysfs (41b) completo, usar mdev no lugar de udev
ou initrd com device nodes pré-criados.

```
Before: Kernel boota, init rodando, mas sem threading, sem /sys, sem /dev/block, sem /proc/self/fd
After:  Alpine boota, getty no tty0, shell com pipe/job control, apt-get (se networking OK)
```

Run FKernel with Alpine initrd + OpenRC; fix blocking syscall stubs.

## Phase 42 — Architecture Portability Layer (5–7 weeks) — MEDIUM

Extrair dependências x86_64 do kernel genérico para interfaces abstratas,
desbloqueando suporte a AArch64 e RISC-V no futuro.

### 42a — CPU/Hardware Abstractions (1 week)

| # | Task | Files | Effort |
|---|------|-------|--------|
| 1 | Generic `VirtualMemoryManager` interface — `map_page()`, `unmap_page()`, `translate()`, `switch_address_space()`, `create/clone/free_address_space()` | `Include/Kernel/Memory/virtual_memory_manager.h` (new), `Src/Kernel/Arch/x86_64/Memory/virtual_memory_manager.cpp` (extract) | 3d |
| 2 | `PageFlags` neutral (arch → flag translation via callback/enum) + x86_64 translator | `Include/Kernel/Memory/page_flags.h` | 1d |
| 3 | `cpu_irq_enable()`, `cpu_irq_disable()`, `cpu_idle()` — extract `cli`/`sti`/`hlt` from init.cpp and Scheduler | `Include/Kernel/Arch/cpu_irq.h`, `Src/Kernel/Arch/x86_64/cpu_irq.cpp` | 1d |

### 42b — Interrupt Interface (1 week)

| # | Task | Files | Effort |
|---|------|-------|--------|
| 4 | Generic `HardwareInterruptController` interface — `register_handler()`, `mask/unmask()`, `send_eoi()`, `enable_irq()` | `Include/Kernel/Interrupt/hardware_interrupt_controller.h` (new) | 2d |
| 5 | Move `Timer` abstract class to generic path | `Include/Kernel/Clock/timer.h` | 1d |
| 6 | Port x86_64 `HardwareInterruptManager` + `APIC`/`IOAPIC` to new interface | `Src/Kernel/Arch/x86_64/Interrupt/` | 2d |

### 42c — Scheduler refactor (1.5 weeks)

| # | Task | Files | Effort |
|---|------|-------|--------|
| 7 | Extract `start_aps()` → `Arch::start_secondary_cpus()` | `Src/Kernel/Arch/x86_64/Smp/smp_start.cpp` | 1d |
| 8 | Extract `idle_loop()` → `arch_cpu_idle()` | `Src/Kernel/Arch/x86_64/cpu_idle.cpp` | 0.5d |
| 9 | Extract context switch → `arch_switch_context(CpuContext**)` with void* layout | `Src/Kernel/Arch/x86_64/switch_context.S` | 2d |
| 10 | Generic `CpuContext` (union/variant) + x86_64 impl | `Include/Kernel/Arch/cpu_context.h` | 2d |
| 11 | Per-CPU data abstraction (GS segment ↔ TPIDR_EL1) | `Include/Kernel/Arch/percpu.h` | 1d |

### 42d — Syscall refactor (1 week)

| # | Task | Files | Effort |
|---|------|-------|--------|
| 12 | `PtRegs` → union per arch, generic code uses getter/setter | `Include/Kernel/Arch/ptregs.h` | 2d |
| 13 | Extract `init_syscalls()` arch dispatch setup | `Src/Kernel/Arch/x86_64/Syscall/syscall_entry.cpp` | 2d |
| 14 | Remove arch includes from `init.cpp` — inject via functor/interface | `Src/Kernel/Init/init.cpp` | 1d |

### 42e — Init refactor (1 week)

| # | Task | Files | Effort |
|---|------|-------|--------|
| 15 | `ArchEarlyInit` interface — GDT, IDT, CPU features, TSC detection | `Include/Kernel/Arch/early_init.h` (new) | 2d |
| 16 | Remove arch includes from `init.cpp` | `Src/Kernel/Init/init.cpp` | 1d |
| 17 | `KernelImage` struct — abstract boot entry (multiboot vs EFI vs DTB) | `Include/Kernel/Arch/kernel_image.h` | 2d |

### Dependencies

42a → 42b (VMM needs IRQ for TLB shootdown)
42a → 42c (scheduler needs cpu_idle)
42b → 42d (syscall entry needs IRQ enable/disable)
42d → 42e (init.cpp calls syscall init)

## Phase 20 — POSIX Networking Syscalls (~25 remaining) — LOW

Advanced socket options, multicast, `MSG_DONTWAIT`, `sendmmsg`/`recvmmsg`, `TCP_NODELAY`, etc. See `.ai-docs/ROADMAP.md#phase-20` for details.

## Phase 21 — Test Coverage — LOW

VFS tests, buddy allocator tests, ELF loader tests, TCP state machine tests. See `.ai-docs/ROADMAP.md#phase-21` for details.

---

## Distro Readiness — Remaining Gaps

Items from Phase 31 not yet implemented. Full context in `.ai-docs/AUDITS.md#distro-readiness-audit`.

### Runtime Blockers (31b)

| # | Gap | Files | Fix |
|---|-----|-------|-----|
| 4 | No permission check in `open()` | `FileSystem/open.cpp` | ✅ Already implemented |
| 5 | `MAX_OPEN_FILES = 128` hardcoded | `task.h` | ✅ Raised to 1024 |
| 6 | `exit_group` == `exit` (single-thread only) | `Process/exit_group.cpp` | ✅ Kills all tasks in tgid (tgid field added to TaskIdentity) |
| 7 | `TIOCGWINSZ` missing on PtyMaster | `pty_master.cpp` | ✅ Implemented (80x24 default) |
| 8 | No SIGTTIN/SIGTTOU | `vga_terminal.cpp`, `signal_delivery.cpp` | ✅ Both implemented |

### Bugs (31c)

| # | Gap | Files | Fix |
|---|-----|-------|-----|
| 9 | `stat`/`chdir`/`mkdir` unsafe user pointer | `stat.cpp`, `chdir.cpp`, `mkdir.cpp` | ✅ All use `copy_from_user()` |
| 10 | `utimensat` not registered | `syscall_numbers.h`, `syscall.cpp` | ✅ Registered as SYS_UTIMENSAT=280 |
| 11 | fcntl advisory locks are no-ops | `FileSystem/fcntl.cpp` | ✅ `FileLockList` per-node with pid, type, start, end. F_GETLK/F_SETLK/F_SETLKW implemented. Locks released on exit via `release_all_file_locks()` |
| 12 | `getrandom` uses xorshift64 (not cryptographic) | `System/getrandom.cpp`, `urandom_device.cpp` | ✅ ChaCha20PRNG with RDTSC + interrupt-jitter pool (`g_interrupt_entropy_pool` updated each tick via `tick_manager.cpp::increment_ticks()`; XORed into entropy callback) |
| 13 | `close()` doesn't call `node->close()` | `FileSystem/close.cpp` | ✅ `on_close()` called in `close_file_descriptor()` |

### Missing Subsystems (31d)

| # | Gap | Priority |
|---|-----|----------|
| 14 | `mmap MAP_SHARED` file-backed | ✅ MEDIUM — mmap.cpp: MAP_SHARED+file fd calls mmap_file() and registers FileMmapRecord in task->resources.memory.file_mmaps for writeback tracking |
| 15 | `mmap MAP_FIXED` | ✅ HIGH — Fixed: file-backed and shm paths now respect MAP_FIXED |
| 16 | File-backed mmap with `PROT_WRITE` | ✅ HIGH — Already worked via prot_to_page_flags |
| 17 | mmap shared mapping writeback / `msync` | ✅ MEDIUM — msync.cpp: walks file_mmaps, checks x86_64 PTE dirty bit (bit 6), writes dirty pages back via node->write(), clears dirty bit + invlpg |
| 18 | inotify | LOW |
| 19 | `/proc/sys/` writable nodes | LOW |
| 20 | Coredumps | LOW |

### PTY Completeness (31e)

| # | Gap | Files |
|---|-----|-------|
| 21 | `TIOCSCTTY` on PtyMaster | ✅ `pty_master.cpp` |
| 22 | `TIOCGPGRP`/`TIOCSPGRP` on PtyMaster | ✅ `pty_master.cpp` |
| 23 | ICANON line editing in PtyLineDiscipline | ✅ `pty_line_discipline.cpp` — VERASE (BS-SP-BS echo), VKILL (visual erase), VEOF, canonical line buffering with line-ready flag, ECHO/ECHOE/ECHONL |
| 24 | OPOST output processing (`\n` → `\r\n`) | ✅ `pty_slave.cpp` — ONLCR applied in slave write |
| 25 | Userspace terminal emulator | New program |

---

## ELF Loader — Remaining Items

| # | Task | Files | Priority |
|---|------|-------|----------|
| 13 | Cache program headers (reverted — caused Error 0; needs investigation) | `elf_loader_core.cpp` | LOW |
| 16 | Unify TLS setup in loader (split across execve.cpp + init_task.cpp; init_task has no TLS) | `elf_loader_core.cpp`, `execve.cpp`, `init_task.cpp` | LOW |
| 17 | ELF loader tests | `tests/Loader/` | LOW |
| — | Symbol versioning parsing (DT_VERSYM/VERNEED macros defined; parsing not implemented) | `dynamic_domain.cpp` | LOW |

---

---

## AGENTS.md Compliance — Code Audit Findings

Audit of the entire codebase against rules in `AGENTS.md`. Each entry includes which rule is violated, severity, and affected files.

### ✅ P1 — Manager Pattern (AGENTS.md:107–140) — DONE

All 13 managers now follow the canonical pattern:
- Wrapped in `namespace fkernel` (or acceptable sub-namespace) + `using` alias at global scope
- Private constructor + deleted copy AND move
- `m_is_initialized{false}` + `is_initialized()` + double-init guard in `initialize()`
- `m_is_initialized = true` set at END of `initialize()`
- `DriverManager::initialize()` wired into boot flow with `kfatal` assert
- `early_init.cpp` now asserts after `MemoryManager` heap + initialize
- `TickManager::initialize()` implemented
- `FadtManager` and `ClockManager` track `m_is_initialized`
- Linker symbols in PMM moved to file-scope `extern "C"` block

### 🔴 P1 — Architecture Portability (AGENTS.md:142–170)

| # | File | Issue | Fix |
|---|------|-------|-----|
| 1 | `Src/Kernel/Init/init.cpp:25` | ✅ `arch_read_tsc()` added to `cpu_ops.h/cpp`. `rdtsc_entropy_source()` uses `arch_read_tsc()`. | — |
| 2 | `Src/Kernel/Init/init.cpp:127` | ✅ `asm volatile("cli")` replaced with `arch_disable_interrupts()`. | — |
| 3 | `Src/Kernel/Scheduler/scheduler_manager.cpp:196,199` | `asm volatile("xsave64")` / `asm volatile("fxsave")` | Extract to `arch_fpu_save()` / `arch_fpu_restore()` (Phase 42c-9) |
| 4 | `Src/Kernel/Scheduler/scheduler_manager.cpp:275` | `asm volatile("sti; hlt")` in `idle_loop()` | Already marked Phase 42c-8 — use `arch_cpu_idle()` |
| 5 | `Src/Kernel/Scheduler/scheduler_manager.cpp:240` | `asm volatile("lea ap_entry")` | Extract to `arch_start_secondary_cpus()` (Phase 42c-7) |
| 6 | `Src/Kernel/Boot/boot_timer.cpp:8` | ✅ `arch_read_tsc_serialized()` added to `cpu_ops.h/cpp` (LFENCE+RDTSC). `boot_timer.cpp` uses `arch_read_tsc_serialized()`. `tick_manager.cpp` uses `arch_read_tsc()` and `arch_cpu_relax()`. | — |
| 7 | `Src/Kernel/Syscall/System/reboot.cpp:54,74,89` | `asm volatile("pause")` × 3 | ✅ Replaced with `arch_cpu_relax()` — already declared in `cpu_ops.h`, included via existing include |
| 8 | 13 generic files | `inb`/`outb`/`inw`/`outw`/`inl`/`outl` (93+ calls) — port I/O without `arch_` abstraction | Rename to `arch_inb()`/`arch_outb()` or add abstraction layer |
| 9 | `Include/Kernel/Memory/.../virtual_memory_manager.h:10-12` | `extern "C"` functions `invalid_tlb`, `write_on_cr3`, `read_on_cr3` without `arch_` prefix | Rename to `arch_invalid_tlb`, `arch_write_cr3`, `arch_read_cr3` |
| 10 | `Src/Kernel/Arch/x86_64/Segments/gdt.cpp` | `extern "C"` `flush_tss`, `flush_gdt` without `arch_` prefix | Rename to `arch_flush_tss`, `arch_flush_gdt` |
| 11 | `Src/Kernel/Arch/x86_64/Interrupt/interrupt_controller.cpp` | `extern "C"` `flush_idt` without `arch_` prefix | Rename to `arch_flush_idt` |
| 12 | `Include/Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h` | `arch_cpu_idle()` **missing** (declared in AGENTS.md but not implemented) | Implement (Phase 42c-8) |

### 🔴 P1 — Secret Rule: One Class/File + No Nested Types (AGENTS.md:100–105)

| # | File | Types | Fix |
|---|------|-------|-----|
| 1 | `Include/Kernel/Scheduler/Task/task.h` | **8 top-level** + **2 nested** + **4 anonymous nested** — worst violator | Extract each `Task*` struct to own file; remove nested `Control`/`Resources` |
| 2 | `Include/Kernel/Boot/boot_info.h` | **7 top-level** types | Extract `ModuleInfo`, `BootMode`, `MemoryMapEntry`, `FramebufferInfo`, `AcpiTableInfo`, `MemoryMapIterator` |
| 3 | `Include/Kernel/Hardware/Acpi/srat.h` | **5 types** | Extract SRAT entry structs |
| 4 | `Include/Kernel/Fs/Disk/Exfat/exfat_bpb.h` | **5 types** | Extract on-disk entry structs |
| 5 | `Include/Kernel/Fs/Disk/MinixFs/minix_super.h` | **4 types** | Extract on-disk structs |
| 6 | `Include/Kernel/Fs/Disk/Ext2/ext2_super.h` | **4 types** | Extract on-disk structs |
| 7 | `Include/Kernel/Driver/Storage/Nvme/nvme_utilities.h` | **4 types** | Extract NvmeQueueManager, etc. |
| 8 | `Include/Kernel/Driver/Storage/Nvme/nvme_command.h` | **4 types** | Extract NvmeCompletion, queues |
| 9 | `Include/Kernel/Loader/Domains/dynamic_domain.h` | **2 top-level** + **2 nested** (known tech debt) | Extract `RelaTable`, `SymbolContext`, `LibraryContext` |
| 10 | `Include/Kernel/Boot/boot_timer.h` | **1 nested** `Mark` (known tech debt) | Extract to `boot_timer_mark.h` |
| 11 | `Include/Kernel/Driver/Storage/Ahci/ahci_controller.h` | **6 nested** structs (HBA_PORT, PRDT, etc.) | Extract each to own header |
| 12 | `Include/Kernel/Driver/Storage/Nvme/nvme_controller.h` | **4 nested** (Namespace, QueuePair, etc.) | Extract each to own header |
| 13 | 40+ files with **2 top-level types** each | Helper struct + main class | Extract helper to own file |
| 14 | 30+ files with **nested types** in various drivers/containers | Various | Extract to own files |
| 15 | `Include/LibFK/Container/vector.h`, `list.h`, `forward_list.h`, `hash_map.h`, `intrusive_list.h`, `rb_tree.h` | Nested iterator/node types | Acceptable STL-like pattern, but document exemption |

### 🔴 P1 — Include Order Reversed (AGENTS.md:223–228)

All kernel `.cpp` files place **Kernel headers before LibFK headers**. Correct order: LibC → LibFK → Kernel → Local.

**Fix**: Bulk reorder includes across all `Src/Kernel/` files (Kernel after LibFK). Estimated ~200 files affected.

### 🟠 P2 — Object Calisthenics (AGENTS.md:86–105)

| Rule | Status | Details |
|------|--------|---------|
| 1 indentation level/method | 🟡 Not audited | Recommend static analysis |
| No `else` — early returns | **30+ files violate** | Worst: `display_framebuffer.cpp` (26 else), `ansi_parser.cpp` (25), `kqueue.cpp` (12), `ram_disk.cpp` (11), `rtc.cpp` (11) |
| Wrap all primitives | 🟡 Not audited | Recommend partial check |
| First-class collections | 🟡 Not audited | `Vector` used directly in many places |
| 1 dot per line | 🟡 Not audited | Recommend grep for `->.*->` chains |
| No abbreviations | **3 violations** | `BootInfo` (→ `BootInformation`), `MessageInfo` (→ `MessageInformation`), `DmaBuffer` (→ `DMABuffer`) |
| Keep entities small | **5 files >500 lines** | `ext2_fs.cpp` (752), `exfat_fs.cpp` (703), `virtual_memory_manager.cpp` (543), `minix_fs.cpp` (521), `syscall.cpp` (504) |
| Max 2 instance variables | **Virtually every class violates** | `DisplayFramebuffer` (18+ vars), `SchedulerManager` (12+), `VirtualMemoryManager` (10+) |
| No getters/setters | **Widespread** | VGA Display, VFS Node, APIC/IOAPIC, Scheduler use `get_*()`/`set_*()` instead of behavioral methods |

### 🟠 P2 — Coding Style Violations (AGENTS.md:194–221)

| Violation | Files | Fix |
|-----------|-------|-----|
| `m_` prefix **inconsistent** — worst offenders | `display_framebuffer.h` (13/18 without), `task.h` sub-structs (30+ without), `ps2_keyboard.h` (6 without), many NVMe structs | Add `m_` to all members across entire codebase |
| Lowercase class names (PascalCase violation) | `serial_port.h:16` (`class serial`), `vga_adapter.h:12` (`class vga`) | Rename to `Serial`, `VGA` |
| Directory naming (should be PascalCase) | `Src/Kernel/Syscall/syscall_list/`, `Include/Kernel/Syscall/syscall_list/` (snake_case) | Rename to `SyscallList/` |
| File `8259_pic.h/.cpp` starts with digit | `8259_pic.h`, `8259_pic.cpp` | Rename to `i8259_pic.h/.cpp` |

### 🟡 P3 — Kernel Logging Issues (AGENTS.md:272–315)

| # | Issue | Location | Fix |
|---|-------|----------|-----|
| 1 | `kerror()` used for **recoverable errors** (should be `kwarn()`) | ✅ Fixed: ATA PIO (2 sites), ATA device null strategy (2), E1000 init failures (5), AHCI create/init/IRQ (4), NVMe IRQ + large xfer (2), ELF interpreter not found (1), syscall validation (2) — 18 sites total | — |
| 2 | Prefix `"Page Fault"` (title case, not UPPER_SNAKE_CASE) | ✅ Changed to `"PF"` in `pf_handler.cpp`. | — |
| 3 | Prefix `"General Protection"` (title case, not UPPER_SNAKE_CASE) | ✅ Changed to `"GPF"` in `gp_handler.cpp`. | — |
| 4 | AGENTS.md says `kerror()` **halts CPU** — outdated | `AGENTS.md:280` | Update docs: `kfatal()` halts, `kerror()` returns |

---

## Build Verification

After kernel changes: `xmake` + `xmake check-layers`  
After LibFK/LibC changes: `xmake -bv Test && xmake run Test`  
Full boot test: `xmake run`  
After UDI changes: `xmake check-layers` + boots QEMU with initrd containing userspace drivers

## References

- `.ai-docs/CHANGELOG.md` — all completed phases
- `.ai-docs/ROADMAP.md` — full specs for future phases
- `.ai-docs/AUDITS.md` — audit reports and findings
- `Docs/` — architecture documentation
- `CLAUDE.md` / `AGENTS.md` — coding rules and conventions
