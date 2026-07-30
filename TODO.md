# FKernel TODO

> **Lean version** — only open bugs and pending work. See also:
> - `.ai-docs/CHANGELOG.md` — everything completed
> - `.ai-docs/ROADMAP.md` — future phases with full detail
> - `.ai-docs/AUDITS.md` — ELF, IPC, POSIX-readiness, and x86_64 audit findings

---

## Open Bugs

These are the only bugs known to be open. All other bugs from prior audits are fixed — see `.ai-docs/AUDITS.md`.

### ✅ Bug 9 — CSPRNG not seeded before ASLR — FIXED

`init.cpp:30-32` sets entropy callback to `arch_read_tsc()` and calls `ChaCha20PRNG::initialize()` before any ELF loading. ASLR is seeded.

### ✅ Bug 10 — `s_global_libraries` not SMP-safe — FIXED

`dynamic_domain.cpp:14` has `static Spinlock s_library_lock`; all accesses at lines 57, 72, 130, 347 use `ScopedLockIRQ`.

### ✅ Bug 18 — `Endpoint::wait()` data race on `m_pending_bits` — FIXED

Re-acquires `m_lock` in a second `ScopedLockIRQ` block after `block_current_noqueue()` returns before reading and clearing `m_pending_bits`. Added `wait_interruptible()` variant that also checks `has_pending_signals()` and propagated to 13 subsystems.

### ✅ Bug 19 — `Endpoint::wait_timeout()` data race on `m_pending_bits` — FIXED

Same fix as Bug 18 — second lock block after `sleep_current()` path. `wait_interruptible_timeout()` added and propagated.

### ✅ Bug 20 — `Endpoint::signal_with_payload()` discards payload — FIXED

Implemented `NotificationPayload` ring buffer (`m_payloads[NOTIFICATION_MAX_PAYLOADS]`, `payload_push`/`payload_pop`). `signal_with_payload()` now stores payload; exposed via `poll`/`wait` return path.

---

## LibC/LibFK Audit — Correctness & UB Issues

Found during static architecture audit. See also `.ai-docs/AUDITS.md`.

### UB / Bugs

| # | File | Line | Issue | Severity | Status |
|---|------|------|-------|----------|--------|
| 1 | `Include/LibFK/Container/vector.h` | 147 | Integer overflow in `new_capacity * sizeof(T)` with no overflow check before `allocate()` | **UB** | ✅ Fixed — guard `new_capacity > SIZE_MAX/sizeof(T)` in `reallocate_to()` |
| 2 | `Include/LibFK/Traits/type_traits.h` | 179 | `is_base_of<void, int>` gives wrong result (non-class types not handled) | **Bug** | ✅ Fixed — added `is_class<T>` helper + `is_base_of` now requires both types be class; `tests/LibFK/test_traits.cpp` added |

### Design / Performance

| # | File | Line | Issue | Severity |
|---|------|------|-------|----------|
| 3 | `Include/LibFK/Synchronization/spinlock.h` | 22–28 | `cpuid` instruction on every lock/unlock (~100 serializing cycles) | Performance |
| 4 | `Include/LibFK/Core/result.h` | 29–38 | `value()` on error only warns, does not assert — silent invalid-state propagation | Design |
| 5 | `Include/LibFK/Memory/retain_ptr.h` | 39 | Refcount allocated separately from object (`allocate<uint32_t>()`): fragmentation + leak if `allocate` fails after object constructed | Design |
| 6 | `Include/LibFK/Memory/ref_counted.h` | 17 | `virtual ~RefCounted()` adds vtable to every refcounted class | Overhead |
| 7 | `Include/LibFK/Text/string.h` | — | SSO never shrinks back to inline after growth — permanent memory retention | Memory |
| 8 | `Include/LibFK/Synchronization/interrupt_disabler.h` | 13–15 | `cli` without checking IF already = 0 (nesting edge case) | Correctness edge |
| 9 | `Include/LibFK/Core/assertions.h` | 5 | `#define assert(x) ASSERT(x)` shadows the standard name — may conflict with `<cassert>` | Convention |

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

All POSIX file descriptors routed through CSpace capabilities. `CapabilityType::FileDescriptor`, `CapabilityRights` (Read/Write/Seek/Ioctl), `CSpace::install_fd/lookup_fd/revoke_fd/clone_fd` all implemented. `TaskFiles` carries parallel `cap_handles` vector; `add_file_descriptor` installs, `get_file_descriptor` validates, `close_file_descriptor` revokes via CSpace. Rights enforced in `FileDescription::read()/write()` via `O_ACCMODE` check. Pipe creates separate `O_RDONLY`/`O_WRONLY` descriptions. Fork clones caps via `CSpace::clone_fd()`. Execve revokes `FD_CLOEXEC` caps. Mmap and socket use `get/add_file_descriptor`.

## ✅ Phase 29b — CSpace Wiring + Rights Enforcement — DONE

All POSIX fd syscalls (open, close, dup2, dup3, fcntl, pipe, fork, execve, mmap, socket) go through CSpace. Rights enforcement at `FileDescription` level via open-flags check.

## ✅ Phase 29d — Unified Revocation — DONE

`SemNode` and `MqueueNode` never had a separate `m_generation` — they already delegate `generation()`/`generation_ptr()`/`revoke()` to their internal `ipc::Endpoint`. CSpace revoke called from `close_file_descriptor` on every fd close.

---

## ✅ Phase 32c — UFS/UFS2 — DONE

All tasks complete: 5 headers + 2 sources in `Fs/Disk/Ufs/`, triple-indirect block traversal (UFS1 and UFS2), fragment-level addressing via `fsize`, AutoMounter registration as "ufs", short symlink inline support in `UfsNode::read_link()`.

## ✅ Phase 32d — HFS+ — DONE

7 headers + 6 sources in `Include/Kernel/Fs/Disk/HfsPlus/` and `Src/Kernel/Fs/Disk/HfsPlus/`. B-tree search (index + leaf descent, catalog + extents overflow), case-folding via 256-entry table, UCS-2 BE ↔ UTF-8, 8 inline extents + B-tree overflow for fork I/O, symlinks (data fork = UTF-8 target), HFSX case-sensitive mode. Registered in `AutoMounter` as `"hfsplus"`. Hard links deferred (LOW priority).

---

## Phase 33 — Volume Layer (5.5–8.5 days) — MEDIUM

LVM, RAID, dm-crypt. See `.ai-docs/ROADMAP.md#phase-33` for full spec.

| Sub-phase | Component | Days |
|-----------|-----------|------|
| 33a | `StackableBlockDevice` base class | 0.5 |
| 33b | dm-crypt (AES-XTS + LUKS + PBKDF2) | 2–3 |
| 33c | RAID 0/1 | 1–2 |
| 33d | LVM (linear + striped) | 2–3 |

---

## Phase 34b — Remaining x86_64 Fixes — MEDIUM

| # | Fix | Priority |
|---|-----|----------|
| 7 | KPTI (Meltdown mitigation) — two PML4 roots + CR3 swap on syscall entry/exit | MEDIUM |
| 13 | Early serial fallback on COM1 | LOW |

## ✅ Phase 34c — Feature Detection — DONE

CPUID leaves for physical/virtual address width, 1GB pages, INVPCID, FSGSBASE, UMIP, AVX2/AVX-512, LA57, CET. All detected in `cpu.cpp::detect_cpu_features()`. FSGSBASE (CR4.FSGSBASE) and UMIP (CR4.UMIP) are now enabled via `arch_enable_cpu_features`. Also fixed XSAVE/AVX detection bug (was reading leaf 7 ECX instead of leaf 1 ECX).

## Phase 34d — SMP Hardening (1–2 days) — MEDIUM

IRQ affinity, microcode update on APs, MTRR synchronisation, trampoline relocation, APIC topology. See `.ai-docs/ROADMAP.md#phase-34d` for details.

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
| `Bitmap::alloc()` scan word-by-word | O(frames/64) | Hint pointer + free-page-per-word counter |
| `BuddyState::remove()` linked-list search | O(L) | Hash set por endereço físico |
| `BuddyState::remove()` ×10 no coalesce | O(10·L) | Idem (hash set) |

### 39b — Scheduler

| Atual | Complexidade | Substituir por |
|-------|-------------|----------------|
| `m_sleep_queue` scan a cada tick | O(S) | Timer wheel (O(1)) ou binary heap (O(log S)) |
| `find_task(pid)` scan 6 filas | O(N_total) | `HashMap<ProcessId, Task*>` global |
| `pick_next()` affinity scan por nível | O(·N_lvl) | Per-CPU bitmap por afinidade |

### 39c — IPC / CSpace

| Atual | Complexidade | Substituir por |
|-------|-------------|----------------|
| `cap_grant/transfer` via `find_task` | O(N_total) | Resolvido por HashMap em 39b |
| `CSpace::find_by_object/remove_by_object` | O(C) | `HashMap<void*, cap_index>` reverso |
| `CSpace::grant_all_to` | O(C) | Iterar slots válidos via free-list |

### 39d — VFS

| Atual | Complexidade | Substituir por |
|-------|-------------|----------------|
| Dentry child lookup `find_if(name)` | O(C) | `HashMap<StringView, Dentry*>` por parent |
| `readdir()` dedup `insert_if_absent` | O(E²) | `HashSet<ino_t>` temporário |
| File lock `release_all_for_process` | O(L²) | IntrusiveList + O(1) remove |

### 39e — Network

| Atual | Complexidade | Substituir por |
|-------|-------------|----------------|
| ARP lookup/update/expire | O(E) / O(E²) | `HashMap<IPv4Address, ArpEntry>` |
| TCP read `dequeue_front` shift left | O(B) | `RingBuffer<u8>` circular |
| TCP accept `remove_at` left-shift | O(Q) | `IntrusiveList<TcpSocket>` |

### 39f — KQueue

| Atual | Complexidade | Substituir por |
|-------|-------------|----------------|
| `nearest_timer_deadline()` brute force | O(R) | Min-heap (priority queue style) |
| `EV_ENABLE/DISABLE` scan total | O(R) | Index por event ID + direct lookup |

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
| 1 | Interrupção → Endpoint | `IrqBinding` object: ISR faz `endpoint->signal(bits)` em vez de C-handler. `sys_bind_irq(vector, ep_handle)` + `CapabilityType::Irq` | `irq_binding.h/cpp`, `capability.h`, `endpoint.h`, syscall list | 3 |
| 2 | mmap físico userspace | Nova flag `MAP_PHYSICAL`: mapeia `(phys_addr, size)` com `PageFlags::User`, `CacheDisabled` para BARs PCI | `mmap.cpp`, `syscall_numbers.h` | 1 |
| 3 | PCI config space userspace | `ioctl(PIOC_READ_CONFIG/PIOC_WRITE_CONFIG)` em `/dev/pci` ou `/dev/pci/<BDF>` | `pci_node.h/cpp` | 1 |
| 4 | DMA compartilhável | `DmaShm` estende `SharedMemory` com alocação contígua (`alloc_contiguous`); mapeia no userspace com CacheDisabled | `dma_shm.h/cpp`, `shared_memory.h` | 2–3 |

### 40b — Userspace Driver Protocol (3–5 days) — MEDIUM

Protocolo e biblioteca para um processo userspace agir como driver de dispositivo.

| # | Task | Detalhe | Days |
|---|------|---------|------|
| 1 | `libdriver.so` | Lib userspace: `udi_claim_pci(class, subclass)` → recebe capability do BAR + IrqBinding; `udi_recv_irq()` → `ep->wait()`; `udi_map_bar(n)` → `MAP_PHYSICAL`; `udi_dma_alloc(size)` → kernel alloc contíguo mapeado no user | 2 |
| 2 | `/dev/udi` device events | Kernel emite evento `DeviceAttach(bdf, class, subclass, bar_info)` — userspace `read()` e decide se claim | 1 |
| 3 | PCI device fallback | Se nenhum driver kernel reivindicar um PCI device, kernel expõe via `/dev/udi` para userspace | 1 |
| 4 | Initrd com drivers userspace | Script de build empacota `libdriver.so` + drivers userspace na initrd | 1 |

### 40c — FUSE-like Filesystem Protocol (5–7 days) — MEDIUM

Interface **genérica** do kernel para filesystems userspace.

| # | Task | Detalhe | Days |
|---|------|---------|------|
| 1 | `UserFsNode` | Novo `Node` que traduz `read/write/lookup/mkdir/readdir` em mensagens Endpoint. Sem `m_device` de bloco — usa `Endpoint` handle | 2 |
| 2 | Protocolo `user_fs.h` | Estruturas `FsRequest`/`FsResponse` (opcode, ino, offset, size, name, data). Serialização simples | 1 |
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

## Phase 41 — POSIX Completeness Blockers — HIGH

Critical gaps blocking full POSIX compliance for the kernel. These are the remaining syscall-level and process-model gaps between FKernel and Linux uABI compatibility.

### 41a — Signal & Threading (NSIG + robust_list) — HIGH

| # | Item | Detalhe | Arquivos | Dias |
|---|------|---------|----------|------|
| 1 | Expandir NSIG de 32 para 65+ | ✅ NSIG=65, SIGRTMIN=34, SIGRTMAX=64. pending/blocked → uint64_t in task.h. signalfd, sigsuspend, rt_sigtimedwait updated. | — | — |
| 2 | Implementar `set_robust_list`/`get_robust_list` | ✅ Implemented in `robust_list.cpp`, registered in syscall.cpp, robust_list_head in task.h | — | — |
| 3 | Implementar `personality` syscall | ✅ Stub returning 0 (PER_LINUX) — `personality.cpp` registered in syscall.cpp | — | — |
| 4 | Implementar `prctl` syscall | ✅ PR_SET/GET_NAME, PR_GET/SET_DUMPABLE, PR_SET/GET_PDEATHSIG, PR_SET/GET_NO_NEW_PRIVS — `prctl.cpp` registered | — | — |
| 5 | Thread group (tgid) e CLONE_THREAD | ✅ `tgid` field added to TaskIdentity. Set in initialize_task/fork/clone. CLONE_THREAD sets child tgid=parent tgid. exit_group kills all tasks with same tgid. Full thread group signal delivery still pending. | `task.h`, `clone.cpp` | — |

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

With 41a-41d implemented, FKernel passes all POSIX compliance checks for process management (fork, exec, exit, signals), threading (CLONE_THREAD, tgkill, thread group signals), and device discovery (/sys, /dev/block, /proc/self/fd).

```
Before: Kernel boots, init running, but no threading, no /sys, no /dev/block, no /proc/self/fd
After:  Full POSIX process model, thread groups, signal delivery, device model
```

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

## Phase 20 — POSIX Networking Syscalls (~25 remaining) — MEDIUM

Advanced socket options, multicast, `MSG_DONTWAIT`, `sendmmsg`/`recvmmsg`, `TCP_NODELAY`, etc. See `.ai-docs/ROADMAP.md#phase-20` for details.

## Phase 43 — Kernel Test Harness — HIGH

Target: kernel critical paths at 75%. See `.ai-docs/ROADMAP.md#phase-43` for full spec.

### 43a — Test Infrastructure (2 days)
| # | Task |
|---|------|
| 1 | Kernel test runner — host-side with mocked hardware |
| 2 | Mock page allocator, timer, interrupt controller |
| 3 | CI integration — `xmake run Test` covers kernel tests |

### 43b-43f — Subsystem Tests (13 days total)
| Sub-phase | Component | Days |
|-----------|-----------|------|
| 43b | VFS: path resolution, dentry cache, file description | 3 |
| 43c | Memory: buddy allocator, slab, multi-zone | 2 |
| 43d | ELF loader: header validation, relocations, segments | 2 |
| 43e | Scheduler: MLFQ demotion, QoS ordering, turnstile chain | 2 |
| 43f | TCP: handshake, sliding window, retransmit | 2 |

---

## ✅ Phase 44 — Thread Group Signal Delivery — DONE

44a: `sys_tgkill` fixed (finds by `tid`, verifies `tgid`). `SignalDelivery::deliver_to_group()` added — iterates all tasks, picks first non-blocking thread, prefers tgid leader. `kill()` now uses `deliver_to_group()` for process-directed signals. SIGCHLD on exit delivered to parent's tgid via `deliver_to_group()`. `exit_group()` already killed all sibling threads.  
44b: CLONE_THREAD signal mask inheritance already done. execve now kills sibling threads (SIGKILL loop before exec). `signals.pending` cleared on exec; signal mask preserved per POSIX (removed incorrect `blocked = 0`). sigsuspend/rt_sigtimedwait already per-thread.

---

## Phase 45 — Security Hardening — MEDIUM

CSPRNG seeding, KPTI (Meltdown), ASLR hardening (mmap/stack base randomisation). See `.ai-docs/ROADMAP.md#phase-45` for full spec.

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
| 11 | fcntl advisory locks are no-ops | `FileSystem/fcntl.cpp` | Per-node lock list with (pid, type, start, len) |
| 12 | `getrandom` uses xorshift64 (not cryptographic) | `System/getrandom.cpp`, `urandom_device.cpp` | Seed from RDTSC + interrupt jitter |
| 13 | `close()` doesn't call `node->close()` | `FileSystem/close.cpp` | ✅ `on_close()` called in `close_file_descriptor()` |

### Missing Subsystems (31d)

| # | Gap | Priority |
|---|-----|----------|
| 14 | `mmap MAP_SHARED` file-backed | MEDIUM |
| 15 | `mmap MAP_FIXED` | ✅ HIGH — Fixed: file-backed and shm paths now respect MAP_FIXED |
| 16 | File-backed mmap with `PROT_WRITE` | ✅ HIGH — Already worked via prot_to_page_flags |
| 17 | mmap shared mapping writeback / `msync` | MEDIUM |
| 18 | inotify | LOW |
| 19 | `/proc/sys/` writable nodes | LOW |
| 20 | Coredumps | LOW |

### PTY Completeness (31e)

| # | Gap | Files |
|---|-----|-------|
| 21 | `TIOCSCTTY` on PtyMaster | ✅ `pty_master.cpp` |
| 22 | `TIOCGPGRP`/`TIOCSPGRP` on PtyMaster | ✅ `pty_master.cpp` |
| 23 | ICANON line editing in PtyLineDiscipline | `pty_line_discipline.cpp` |
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

All 13 managers converted to canonical singleton pattern: `namespace fkernel`, private ctor, deleted copy/move, `m_is_initialized` set at END of `initialize()`, `is_initialized()` accessor, `using fkernel::FooManager;` alias. `DriverManager::initialize()` added to boot flow in `init.cpp`. `MemoryManager` double-init converted to early-return guard with `kdebug`. `extern "C"` linker symbols for PMM (`__kernel_start`, etc.) moved to file scope to avoid namespace mangling.

### ✅ P1 — Architecture Portability (AGENTS.md:142–170) — DONE (item 5 deferred)

All inline x86_64 asm extracted from generic code: `arch_read_tsc()`, `arch_read_tsc_serialized()`, `arch_fpu_save/restore()`, `arch_cpu_idle()`, `arch_cpu_relax()` added to `cpu_ops.h/cpp`; `arch_write_cr3`, `arch_read_cr3`, `arch_invlpg` renamed in VMM header + asm files + callers; `arch_flush_tss`, `arch_flush_gdt`, `arch_flush_idt` renamed in GDT/IDT asm + callers; `arch_inb/outb/inw/outw/inl/outl` alias layer added to `io.h`; `boot_timer.cpp` and `init.cpp` updated.

**Deferred**: Item 5 — `arch_start_secondary_cpus()` extraction from `start_aps()` is marked Phase 42c-7 (requires SMP refactor).

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

### ✅ P3 — Kernel Logging Issues (AGENTS.md:272–315) — DONE

Items 1–3 fixed: 18 `kerror()` → `kwarn()` in ATA PIO, E1000, AHCI, NVMe, ELF, syscall validation; `"Page Fault"` → `"PF"` in `pf_handler.cpp`; `"General Protection"` → `"GPF"` in `gp_handler.cpp`. AGENTS.md updated (item 4): `kerror()` returns, `kfatal()` halts.

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
