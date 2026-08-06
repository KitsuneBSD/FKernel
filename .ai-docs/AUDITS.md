# FKernel — Audit Reports

> Source-code audits and architectural gap analyses. Each section records what was found, what was fixed, and what remains open. Cross-reference with `TODO.md` for open items and `ROADMAP.md` for planned remediation phases.

---

## TODO ↔ Source Verification Audit (2026-08-05)

Verificação ponto-a-ponto de `TODO.md` contra `Include/` + `Src/` (greps, sub-agentes e leituras diretas). Objetivo: TODO.md deve refletir o código real — nada de itens ✅ que não estão no código, nada de bugs abertos já corrigidos.

### Resultado

| Auditoria | Aberto | Corrigido/Confirmado |
|-----------|--------|----------------------|
| Memória (M) | M6/M11/M12 ⚠️; get_refcount, `BuddyAllocator::initialize()` dead, resíduo M5, identidade 4 GiB parcial | M1–M5, M7–M10, M13 ✅ |
| Exceções (I) | `apic_timer_handler` dead, `send_eoi` vector−32 | I1–I5 ✅ |
| Recuperação (R) | fixup/extable, watchdog real, depth de exceção (futuro) | R1–R4 ✅ |
| LibC/LibFK (L) | L1–L11 (todos) | — |
| Conformidade (C) | C1–C4 | C5 + checkers ✅ |

**7 claims stale/invertidas corrigidas no TODO.md:** syscalls 207→206 (206 `register_syscall` em `syscall.cpp:264-469`); ext2 triple-indirect confirmado (`ext2_fs.cpp:262-296`); I1 spurious handler confirmado (`interrupt_controller.cpp:69`); R1 Design A confirmado (`user_access.cpp:20-35`); **C1 refutado** — `fadt_manager.cpp:69` ainda tem asm cru (proposta `__sync_synchronize()` não aplicada); include order **315/325 (97%)** não 320/462; DmaBuffer legacy **21 call sites**.

**Fatos novos confirmados:** slab tem **10 caches (16–8192B)** (`slab_allocator.cpp:17-18`) — header comentário 16–2048 era stale; kernel tem **10 suites / 99 testes** no target Test (xmake.lua:218-227); NVMe PRP2 (`interrupt_driven_nvme.cpp:137-144`) e AHCI async (`interrupt_driven_ahci.cpp`) implementados; `arch_cpu_idle()` implementado (`cpu_ops.cpp:151`).

**Correções de docs no mesmo dia:** 207→206 syscalls (system-overview, Syscalls README, ipc-capabilities, DocsSummary, current-state-analysis); NVMe PRP2/AHCI async documentados como implementados; slab 10 caches; split `kfatal`/`kerror` em AGENTS.md + 3 docs de logging; `arch_cpu_idle` removido do Phase 42; testes kernel 0→10 suites/99.

### Itens abertos para as próximas auditorias

- C1: 6 `asm` crus no kernel genérico + 4 no LibFK — `xmake check-arch-asm` falha em 10 arquivos.
- L6: 8 testes órfãos do target `Test`; `LibC_Testing` só compila `string/*.c` + `ctype.c`.
- M6/M11/M12, C3/C4 (detalhes em `TODO.md`).

---

## IPC Substrate Fragmentation Audit (2026-07-26)

### Finding

Source-code audit of all 10 POSIX IPC mechanisms revealed that the claimed "unified Notification/Endpoint/SharedMemory substrate" does not exist. Each mechanism used `ipc::Notification` independently as an embedded member. The seL4-style capability model (CSpace/Capability/Endpoint) is a **parallel subsystem** used only by `sys_ipc_send/receive/call` — zero POSIX mechanisms route through it.

### Reality (post-Phase 29a fixes)

| POSIX Mechanism | Notification | Endpoint | SharedMemory | CSpace | Blocking via |
|-----------------|:---:|:---:|:---:|:---:|---|
| Pipe | No | **Yes** ✅ | No | No | `m_endpoint.wait()` |
| EventFd | No | **Yes** ✅ | No | No | `m_endpoint.wait()` |
| Posix Semaphore | No | **Yes** ✅ | No | No | `m_endpoint.wait()` |
| SignalFd | No | **Yes** ✅ | No | No | `m_endpoint.wait()` |
| TimerFd | No | **Yes** ✅ | No | No | `m_endpoint.wait()` |
| Epoll | Yes (via KQueueNode) | No | No | No | Event-driven via KNoteHook |
| kqueue | Yes (1, per instance) | No | No | No | KNoteHook → m_notification.signal() |
| Futex | Yes (256 static global) | No | No | No | `notif.wait_timeout()` |
| Message Queue | No | **Yes** ✅ | No | No | `m_endpoint.wait()` |
| Shared Memory | No | No | Yes | No | N/A (page mapping) |
| Unix Socket | No | **Yes** ✅ | No | No | `m_accept_endpoint.wait()` |

### Fixes Applied

| Gap | Status |
|-----|--------|
| 6 POSIX nodes use raw Notification | ✅ Fixed — migrated to `ipc::Endpoint` (Phase 29a) |
| No unified revocation | ✅ Fixed — `SemNode`/`MqueueNode` dropped own `m_generation`, delegate to `Endpoint::generation()` |
| Epoll busy-loop | ✅ Fixed — event-driven via KNoteHook (Phase 11) |
| UnixSocket raw block_current | ✅ Fixed — migrated to `ipc::Endpoint` (Phase 29a) |
| Capability model is an island | **OPEN** — CSpace wiring for POSIX fds not yet done (Phase 27 + 29b tasks 9/11) |
| No rights decomposition for POSIX | **OPEN** — raw fds have no Send/Receive/Manage rights |

### Target Architecture

```
app A                    kernel                    app B
  │                         │                        │
  ├─ pipe()/sem_open/... ──►│                        │
  │                         ├─ POSIX thin wrapper    │
  │                         ├─ Capability{Send|Recv|Manage}
  │                         ├─ CSpace::lookup()      │
  │                         ├─ Endpoint/Notification │
  │                         ├─ generation check      │
  │  SINGLE enforcement path│                        │
  │  SINGLE revocation path │                        │
  │  SINGLE rights model    │                        │
```

### Remaining Open Tasks

| # | Task | Files | Priority |
|---|------|-------|----------|
| 9 | Wire POSIX fd operations through CSpace capability lookup | All POSIX node types + syscall handlers | HIGH |
| 11 | Add rights enforcement at POSIX syscall boundary (cap_transfer/grant on fds) | Syscall handlers + CSpace | MEDIUM |

See **Phase 27** (ROADMAP.md) for the full VFS+Capability integration plan.

---

## ELF Loader Deep Audit (2026-07-26)

### Finding

Audit of all 13 ELF loader files (10 .cpp, 3 headers). Documentation claimed "full dynamic linking." Reality: only static ELF binaries worked. Dynamically linked programs failed at two independent points.

### Critical Issues — ALL FIXED (Phase 30) ✅

| # | Issue | Fix Applied |
|---|-------|-------------|
| 1 | No `DT_NEEDED` processing | `load_dependencies()` + `load_shared_library()` implemented |
| 2 | ld.so relocations not processed | `DynamicDomain::apply_relocations()` called after `process_load_segments()` for interpreter |
| 3 | No SMAP safety in load paths | `arch_smap_begin()`/`arch_smap_end()` around all user-memory writes |

### Security Issues — 5 of 6 FIXED (Phase 30b) ✅

| # | Issue | Status |
|---|-------|--------|
| 4 | Zero W^X enforcement | ✅ `apply_final_permissions()` rejects W+X segments |
| 5 | ASLR 16-bit entropy + deterministic PRNG | ✅ ChaCha20PRNG with 30-bit entropy; ld.so base randomised |
| 6 | ld.so at fixed `0x70000000` | ✅ Now randomised in [0x10000000, 0x70000000) |
| 7 | GLOB_DAT/JUMP_SLOT ignores r_addend | ✅ Both use `resolve_symbol_cross(...) + r_addend` |
| 8 | Only first PT_GNU_RELRO processed | ✅ Removed `break`; start rounded UP; interpreter RELRO applied |

### Medium Issues — 3 of 6 FIXED ✅

| # | Issue | Status |
|---|-------|--------|
| 10 | Missing relocation types | ✅ R_X86_64_COPY, IRELATIVE, TPOFF64, DTPMOD64, DTPOFF64 |
| 11 | Missing dynamic tags | ✅ DT_INIT/FINI/INIT_ARRAY/FINI_ARRAY/FLAGS/GNU_HASH macros + extraction |
| 12 | No symbol versioning | ⚠️ PARTIAL — DT_VERSYM/VERNEED macros defined; parsing not implemented |
| 13 | SHN_COMMON | ✅ Returns 0 with debug log |
| 14 | No endianness check | ✅ EI_DATA validated |
| 15 | No file-size bounds | ✅ `p_offset + p_filesz > node->size()` checked |

### Low Issues — Remaining Open

| # | Issue | Files | Priority |
|---|-------|-------|----------|
| 16 | `parse_program_headers()` called 3-4x per load | `elf_loader_core.cpp:50,86,108,146` | LOW |
| 22 | Zero ELF loader tests | `tests/Loader/` | LOW |
| 23 | TLS setup split across 3 files | `elf_loader_core.cpp`, `execve.cpp`, `init_task.cpp` | LOW |

### Documentation vs Reality (current)

| Doc Claim | Reality (post-Phase 30) |
|-----------|------------------------|
| "full dynamic linking" | **True** ✅ — DT_NEEDED, ld.so relocs, SMAP-safe, cross-object symbol resolution |
| "ASLR: [0x10000000, 0x70000000)" | **True** ✅ — 30-bit ChaCha20 entropy, ld.so randomised |
| "Full RELRO" | **True** ✅ — all segments processed, start rounded UP, interpreter RELRO applied |
| "Bounds checking on PHDRs" | **True** ✅ — file-size bounds, alignment checks |
| "Symbol versioning" | **False** — macros defined, parsing not implemented |
| "TLS in loader" | **False** — split across execve.cpp + init_task.cpp; init_task has NO TLS setup |

---

## POSIX Compliance Audit (2026-07-26)

### Finding

Audit across 4 subsystems (syscalls, TTY/PTY, process/memory, VFS/filesystems) identified blockers for full POSIX / Linux uABI compliance. FKernel has ~194 functional syscalls against 450+ required for full POSIX.

### 31a — Critical Kernel Gaps

| # | Gap | Status |
|---|-----|--------|
| 1 | No Copy-on-Write in fork | ✅ **DONE** — verified in source (Phase 27-28) |
| 2 | No demand paging for anonymous memory | ✅ **DONE** — verified in source (Phase 28) |
| 3 | No writable persistent filesystem | **PARTIAL** — FAT32 data writes work; `create()`/`mkdir()`/`unlink()` between dirs still return `NotImplemented` or `NotADirectory` in FAT variants |

### 31b — Runtime Gaps (still open)

| # | Gap | Files | Fix |
|---|-----|-------|-----|
| 4 | No permission check in `open()` | `FileSystem/open.cpp` | Call same access check as `access()` before VFS delegation |
| 5 | `MAX_OPEN_FILES = 128` hardcoded | `task.h` | Raise to 1024 or switch `static_vector` to `Vector` |
| 6 | `exit_group` == `exit` (single-thread only) | `Process/exit_group.cpp` | Iterate all tasks in tgid, terminate each |
| 7 | `TIOCGWINSZ` missing on PtyMaster | `pty_master.cpp` | Add `TIOCGWINSZ`/`TIOCSWINSZ`; default 80x24 |
| 8 | No SIGTTIN/SIGTTOU | `vga_terminal.cpp`, `signal_delivery.cpp` | Deliver SIGTTIN on read by background process |

### 31c — Bugs (still open)

| # | Gap | Files | Fix |
|---|-----|-------|-----|
| 9 | `stat`/`chdir`/`mkdir` unsafe user pointer | `stat.cpp`, `chdir.cpp`, `mkdir.cpp` | Use `copy_from_user()` — already correct in `open.cpp`/`execve.cpp` |
| 10 | `utimensat` not registered | `syscall_numbers.h`, `syscall.cpp` | Register as syscall 280 (implementation already exists) |
| 11 | fcntl advisory locks are no-ops | `FileSystem/fcntl.cpp` | Implement per-node lock list: (pid, type, start, len) tuples |
| 12 | `getrandom` uses xorshift64 | `System/getrandom.cpp`, `urandom_device.cpp` | Seed from hardware entropy (RDTSC + jitter) |
| 13 | `close()` doesn't call `node->close()` | `FileSystem/close.cpp` | Call `desc->node()->close()` before clearing slot |

### 31d — Missing Subsystems

| # | Gap | Priority |
|---|-----|----------|
| 14 | No `mmap MAP_SHARED` file-backed | MEDIUM |
| 15 | No `mmap MAP_FIXED` | HIGH |
| 16 | No file-backed mmap with PROT_WRITE | HIGH |
| 17 | No mmap shared mapping writeback / msync | MEDIUM |
| 18 | No inotify | LOW |
| 19 | No `/proc/sys/` writable nodes beyond hostname | LOW |
| 20 | No coredumps | LOW |

### 31e — PTY Completeness

| # | Gap | Files |
|---|-----|-------|
| 21 | No `TIOCSCTTY` on PtyMaster | `pty_master.cpp` |
| 22 | No `TIOCGPGRP`/`TIOCSPGRP` on PtyMaster | `pty_master.cpp` |
| 23 | PtyLineDiscipline: no ICANON editing | `pty_line_discipline.cpp` |
| 24 | PtyLineDiscipline: no OPOST output processing | `pty_line_discipline.cpp` |
| 25 | No userspace terminal emulator | New program needed |

---

## LibFK Comparative Analysis (2026-07-23)

Comparison vs. SerenityOS AK and BSD libkern.

| Aspect | LibFK | AK (SerenityOS) | BSD libkern | Gap |
|--------|-------|-----------------|-------------|-----|
| HashMap strategy | Robin Hood + backshift ✅ | Robin Hood + backshift | Chaining | Fixed (was linear probing) |
| HashMap load factor | 80% ✅ | 80% | N/A | Fixed |
| String SSO | Yes (16B inline) ✅ | Yes (7B inline) | N/A | Fixed |
| Smart pointers | OwnPtr, RefPtr, NonnullOwnPtr, NonnullRefPtr, WeakPtr ✅ | Same | refcount(9) only | Fixed |
| Error handling | Result<T,E> + TRY() | ErrorOr<T,E> + TRY() | int + errno | Comparable |
| Allocator backend | Pluggable ✅ | Hardcoded kmalloc | Hardcoded malloc(9) | LibFK wins |
| Spinlock | Recursive + lock rank + IRQ save ✅ | Same | mutex(9) adaptive | Fixed |
| Format system | printf-style | {}-style, compile-time checked | printf-style | Missing type safety |
| Intrusive list | IntrusiveList (pointer-to-member) | Same | LIST/TAILQ macros | Comparable |
| RB tree | Static pool (no heap) ✅ | Heap-allocated | Splay tree | LibFK wins |
| Type safety | Strong types (ProcessId, etc.) ✅ | DistinctNumeric | Plain typedef | LibFK wins |
| memcpy/memset | rep movsb/stosb ✅ | Optimised | Arch-specific assembly | Fixed |

**Remaining gaps vs AK**: type-safe format system (lowest priority given freestanding constraint).

---

## x86_64 Architecture Audit (2026-07-26)

Gap analysis against Intel SDM Vol. 3 across all arch files.

### Critical — All Fixed ✅

| Issue | Fix |
|-------|-----|
| `g_cpu_block` global (not per-CPU) | → `g_cpu_blocks[MAX_CPUS]` array (session 16) |
| Boot PWT+PCD both set (reserved combination) | → WB cache flags (session 15) |
| CR0.WP not set | → `arch_enable_cpu_features()` (session 15) |
| CR4.OSXSAVE never set, XCR0 not programmed | → Both set in `cpu_ops.cpp` (session 15) |
| Only FXSAVE/FXRSTOR (loses AVX state) | → `xsave64`/`xrstor64` with fallback (session 16) |

### Important — Mostly Fixed ✅

| Issue | Status |
|-------|--------|
| PCID not enabled | ✅ CR4.PCIDE enabled via CPUID |
| No MCA handling | ✅ MCi_STATUS/ADDR/MISC logged before halt |
| IA32_MISC_ENABLE not read | ✅ Fast Strings + ERMSB detected |
| MSR_SFMASK = 0x200 | ✅ Changed to 0x4700 |
| MCFG/ECAM | ✅ Already done in pci.cpp |
| HPET | ✅ Already done in timer_interrupt.cpp |
| No Meltdown mitigation (KPTI) | ⏭ Deferred (two PML4 roots, invasive) |
| No early serial fallback | ⏭ Deferred (low QEMU impact) |

### Feature Detection Gaps (Phase 34c)

| # | Gap | CPUID Leaf |
|---|-----|-----------|
| 14 | Physical/virtual address width | `0x80000008` |
| 15 | 1GB page support | `0x80000001.EDX[26]` |
| 16 | INVPCID | `0x07.EBX[10]` |
| 17 | FSGSBASE | `0x07.EBX[0]` |
| 18 | UMIP | `0x07.EBX[2]` |
| 19 | AVX2/AVX-512/FMA/BMI/RDRAND | `0x07.EBX`, `0x01.ECX` |
| 20 | LA57 (5-level paging) | `0x07.ECX[16]` |
| 21 | CET (Shadow Stack + IBT) | `0x07.ECX[7]` |

### SMP Hardening Gaps (Phase 34d)

| # | Gap | Priority |
|---|-----|----------|
| 22 | No IRQ affinity / load balancing | MEDIUM |
| 23 | No microcode update on AP | MEDIUM |
| 24 | No MTRR synchronisation | MEDIUM |
| 25 | SMP trampoline at 0x8000 (may conflict with SMM) | LOW |
| 26 | No APIC ID → topology mapping | LOW |

---

## Source Code Audit — Open Bugs (2026-07-19 / 2026-07-20)

Four bugs remain open from the comprehensive source code audit:

### Bug 9 — CSPRNG not seeded before ASLR

**Severity**: High (security)  
**Files**: `init.cpp`, `Src/LibFK/Algorithms/chacha20.cpp`  
**Detail**: `init.cpp` has no ChaCha20 initialisation. ASLR may use an unseeded PRNG producing deterministic/detectable addresses at boot.  
**Fix**: Seed ChaCha20 from RDTSC + RDRAND (or HPET counter) early in `init()`, before the first ELF load.

### Bug 10 — `s_global_libraries` not SMP-safe

**Severity**: High (data corruption on SMP)  
**Files**: `dynamic_domain.cpp:12,54-59,67-71,122-128`  
**Detail**: Global `static Vector<LibraryContext>` accessed without lock in `load_dependencies()` (push) and `load_shared_library()` (read/write). Two CPUs doing concurrent `execve()` corrupt the vector.  
**Fix**: Guard with Spinlock, or make per-process by moving from global to `LoadContext`/`ElfLoadResult`.

### Bug 18 — `Endpoint::wait()` data race on `m_pending_bits`

**Severity**: High (race condition)  
**Files**: `endpoint.cpp:250-265`  
**Detail**: After `block_current_noqueue()` returns and `ScopedLockIRQ` scope ends (:261), reads `m_pending_bits` + `clear_all()` (:262-264) without holding `m_lock`. `signal()` from another CPU can corrupt bits concurrently.  
**Fix**: Keep `m_lock` held through the read+clear, or use atomic exchange.

### Bug 19 — `Endpoint::wait_timeout()` data race

**Severity**: High (race condition)  
**Files**: `endpoint.cpp:285-296`  
**Detail**: Same pattern as Bug 18 — reads+clears `m_pending_bits` without lock at :294-296 after timeout path.  
**Fix**: Same as Bug 18 — hold lock through read+clear.

### Bug 20 — `Endpoint::signal_with_payload()` discards payload

**Severity**: Medium (silent data loss)  
**Files**: `endpoint.cpp:306-308`  
**Detail**: `data` and `len` parameters are `[[maybe_unused]]`; only calls `signal(bits)`, discarding the payload entirely.  
**Fix**: Implement payload storage (ring buffer or last-payload-wins); expose via wait/poll return.
