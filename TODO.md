# FKernel TODO

> **Only open bugs and pending work.** See also:
> - `.ai-docs/CHANGELOG.md` — everything completed
> - `.ai-docs/ROADMAP.md` — future phases with full detail
> - `.ai-docs/AUDITS.md` — audit findings

---

## Quick Status

| Area | Status | Key Gaps |
|------|--------|----------|
| **Boot/Init** | ✅ Working | GRUB+Multiboot2 → SMP AP startup → scheduler |
| **Memory Mgmt** | ⚠️ Audit 2026-08-03 | M1–M4/M5/M7–M9/M13 ✅; **M10 ❌** (file-backed); M6/M11/M12 ⚠️; identidade 4 GiB × zona HIGH agora alcançável; IOMMU stubbed |
| **Memory Pressure** | ❌ Ausente | **sem swap, sem page cache, sem reclaim, sem OOM killer** (slab OOM = halt) — Phase 46 |
| **Scheduler** | ✅ Working | MLFQ + SMP work-stealing + QoS; `pick_next()` affinity O(N) |
| **IPC (seL4)** | ✅ Rendezvous OK | Capabilities, CSpace, Notification, revocation OK; **Phase 27 DONE**; sync rendezvous corrigido (51a ✅); fastpath call+reply direto via `switch_to_task` (51b ✅); **Phase 51c pendente** (reply+recv fusion + asm skim) |
| **VFS** | ✅ Working | Ext2 r/w (mas **triple-indirect write ❌** `ext2_fs.cpp:260`); ext3/4, UFS, HFS+, FAT write stubs |
| **ELF Loader** | ✅ Working | ASLR, TLS, RELRO, dynamic linking |
| **Syscalls** | ⚠️ 207 registrados | **12** `NotImplemented` (IOMMU, ext4 extent, terminal…); mlock ✅; UDP connect/listen ✅ |
| **Networking** | ⚠️ Partial | TCP state machine + retransmit OK; UDP sendto/recvfrom/connect/listen OK (`accept`=EOPNOTSUPP, correto); sem congestion control; sem IPv6 |
| **Drivers** | ⚠️ Mixed | E1000, ATA DMA (>256 setores ✅), PS/2, PTY OK; **sem USB (xHCI/EHCI/HID)** — maior gap p/ laptop moderno; NVMe PRP2 ✅; AHCI async DMA ✅; 5 .cpp NVMe vazios |
| **SMP** | ⚠️ Partial | Bugs 21-36 corrigidos; up to 64 CPUs (`MAX_CPUS=64`); no IRQ affinity, no MTRR sync |
| **Security** | ⚠️ Partial | SMEP/SMAP/NX/W^X on all CPUs; ASLR via ChaCha20 (fix 2026-07-31); KPTI missing (Phase 45b); IOMMU stubbed |
| **Tests** | ⚠️ Growing | LibC/LibFK ~85%; kernel **10 suites / 99 tests**; Phase 43 target 75% |
| **Docs** | ⚠️ Partial | AGENTS.md/TODO.md excelentes; Docs/ stubs; memory-guide: slab 8192/2048 + file-backed stale (M10) |
| **Code Quality** | ⚠️ Debt | task.h ✅ refatorado; include order invertido (320/462); calisthenics violado; rb_tree morto; 5 .cpp NVMe vazios |

---

## 🔴 IMMEDIATE — Correção de Bugs

### Auditoria x86_64 (2026-07-30) — Bugs confirmados

Todos os bugs das auditorias anteriores foram corrigidos (Bugs 9, 10, 18–36 — ver `.ai-docs/CHANGELOG.md` e `.ai-docs/AUDITS.md`).

**⚪ BAIXO — único restante**

| # | Bug | Local |
|---|-----|-------|
| 35 | Bridge VESA real-mode é placeholder — VBE não funciona em x86_64 (só framebuffer multiboot2) | `Arch/x86_64/Driver/Vga/real_mode_bridge.asm` |

### Auditoria de Memória (2026-08-01; re-audit 2026-08-03)

Auditoria do subsistema de memória (PMM, VMM, slab, heap, demand paging). M1–M4 causam corrupção de memória sob condições normais de uso (DMA >4KiB, slab >4KiB, crescimento de Vector/String do kernel, double-allocation silenciosa).

**🔴 CRÍTICO**

| # | Bug | Local |
|---|-----|-------|
| M1 | Semântica de `order` inconsistente: buddy usa orders absolutos (MIN_ORDER=12 → 4KiB) mas callers passam log₂(page_count) → sub-alocação + overflow (NVMe/DMA/slab alocam 4KiB e usam 8–16KiB) | `dma_buffer.cpp:82-85`, `interrupt_driven_nvme.cpp:79`, `slab_allocator.cpp:50-64` (correto: `dma_shm.cpp:20`) ✅ **feito** — `to_buddy_order`/`size_to_order`; testes em `tests/Kernel/test_buddy_allocator.cpp` |
| M2 | Accounting `m_free_memory -= (FRAME_SIZE << order)` é 4096× maior que o correto (`order_to_size(order)`); alocação de 4KiB subtrai 16 MiB | `physical_memory_manager.cpp:352,395` ✅ **feito** — `order_to_size(effective_order)` |
| M3 | Bitmap e buddy dessincronizados → double-allocation: `alloc_page` via bitmap não divide o bloco do buddy; `invalidate_page` é no-op para páginas internas de um bloco maxal | `physical_memory_manager.cpp:261-270`, `buddy_allocator.cpp:249-254` ✅ **feito** — `invalidate_page` faz split do bloco maximal + `FreeBlock.list_idx` rejeita unlink inválido; testes em `tests/Kernel/test_buddy_allocator.cpp` |
| M4 | `reallocate()` assume `BlockHeader` em ponteiro de slab → magic `0xC0FFEE` falha → `kfatal` (halt do kernel) em crescimento de qualquer `Vector`/`String` LibFK ≤2048B | `memory_manager.cpp:151-185`, `LibFK/Memory/Allocators/heap_malloc.cpp:24-27,42-46` ✅ **feito** — `SlabAllocator::is_slab_allocation()`/`reallocate()`; testes em `tests/Kernel/test_slab_allocator.cpp` |

**🟠 ALTO**

| # | Bug | Local |
|---|-----|-------|
| M5 | ✅ | CoW break preserva user-ness (`pf_handler.cpp:49-62`, "Never add the User bit here") |
| M6 | ⚠️ | slab self-locks (`ScopedLockIRQ`) mas `memory_manager.cpp:100-106` chama slab antes do `save_and_disable_interrupts()` (inofensivo hoje) |

**🟡 MÉDIO**

| # | Bug | Local |
|---|-----|-------|
| M7 | ✅ | refcounts por frame: decrementa e difere free enquanto compartilhado (`physical_memory_manager.cpp:388-394,407-422`) |
| M8 | ✅ | `candidate_zones()` com fallback 5 níveis (`physical_memory_manager.cpp:214-275`); ⚠️ consequência: pode devolver zona HIGH |
| M9 | ✅ | Huge pages de usuário tratadas em free/unmap (`virtual_memory_manager.cpp:357-361,414-416,445-467`) |
| M10 | ✅ | Demand paging file-backed implementado — `handle_demand_paging` em `pf_handler.cpp` lê do `backing_node` na falta de página; ver HIGH #5 |
| M11 | ⚠️ | `get_page_flags` mascara Accessed/Dirty (`virtual_memory_manager.cpp:202-207,240-260`); `translate()` offset add corrigido |
| M12 | ⚠️ | Direct map com NX ✅ (`virtual_memory_manager.cpp:486-543`); ainda mapeia buracos até `highest_physical_address()` |
| M13 | ✅ | Frame 0 / IVT / VGA/BIOS 0xA0000-0xFFFFF / AP trampoline reservados (`physical_memory_manager.cpp:126-154`) |

**⚪ BAIXO**

- ✅ identidade 4 GiB × zona HIGH — `alloc_page_for_pagetable()` corrige o caso das page tables; `shm_node.cpp:35,47` e acesso VMM a frames >4GiB via identidade ainda pendente (apenas se mmap físico >4GiB for necessário)
- `get_refcount` retorna 1 para rc==0 (conflaciona "não rastreado" com "refcount zerado") — `physical_memory_manager.cpp:459`
- Docs desatualizadas: slab "≤8192" vs código `≤2048` (`memory_manager.cpp:100`); demand paging file-backed (M10)
- `BuddyAllocator::initialize()` é código morto (caminho real é `initialize_from_bitmap`) com lógica de loop confusa
- Resíduo M5: `handle_demand_paging` OR `User` incondicionalmente (`pf_handler.cpp:30`), alcançável pelo caminho AC em modo kernel (`pf_handler.cpp:121-125`)

---

## 🟠 HIGH — Bloqueantes para POSIX e Uso Real

### 0. Hardware Real — Laptop Moderno (audit 2026-08-03)

> Alvo declarado: laptop moderno (sem PS/2, NVMe, >4 GiB RAM). Ordenado por bloqueio real de uso.

| # | Bloqueador | Por quê | Fase |
|---|-----------|---------|------|
| 1 | **USB (xHCI + EHCI + HID)** | laptop moderno **não tem PS/2** → teclado/mouse/storage inoperáveis | nova (Phase 50) |
| 2 | ✅ **NVMe PRP2 (>4KiB)** | implementado: 2 páginas → PRP1+PRP2; >2 páginas → PRP list 4KiB em DMA memory; free após wait_for_completion | curto |
| 3 | ✅ **Identidade 4 GiB × zona HIGH** | `alloc_page_for_pagetable()` recusa HIGH zone; usado em `ensure_table()` e `extend_direct_map()` | curto |
| 4 | **Swap/reclaim mínimo** | OOM hoje = halt (`slab_allocator.cpp:135`) | Phase 46 |
| 5 | ✅ **M10 file-backed paging** | `handle_demand_paging` lê `backing_node->read(file_offset, PAGE_SIZE)` se região tem backing; zero-fill caso contrário | curto |
| 6 | ✅ **AHCI async / ATA >256 setores** | `interrupt_driven_ahci.cpp`: `setup_dma_buffers/command_header/fis/start_command` implementados; `dma_strategy.cpp`: loop >256 setores em chunks de 256 | curto |
| 7 | **VBE fallback (Bug 35)** | console em máquina sem GOP | curto |
| 8 | ✅ **Ext2 triple-indirect write** | `ext2_fs.cpp:260` implementado — L1→L2→leaf com `ensure_indirect` em cada nível | curto |

### 1. Kernel Test Harness — Phase 43 (13 dias)

Kernel tem ~0% de cobertura. 10 suites / **99 testes** para ~40K linhas (`Src/Kernel` ≈39.7K). **Isso é o maior risco do projeto.**

| Sub-phase | Componente | Dias | Status |
|-----------|-----------|------|--------|
| 43a | Test infrastructure (runner, mocks, CI) | 2 | ✅ Done |
| 43b | **VFS**: path resolution, dentry cache, file description | 3 | ✅ Done (9 dentry + 11 PathResolver + 10 FileDescription tests) |
| 43c | **Memory**: buddy allocator, slab, multi-zone | 2 | ✅ Done (BuddyState 8 tests + Zone 12 tests) |
| 43d | ELF loader: header validation | 2 | ✅ Done (15 tests) |
| 43e | **Scheduler**: MLFQ demotion, QoS, turnstile chain | 2 | ✅ Done (14 QoS + 6 MLFQQueue + 6 Turnstile tests) |
| 43f | **TCP**: handshake, sliding window, retransmit | 2 | ✅ Done (10 TcpConnection tests) |

### 2. Phase 40a — Hardware Access Primitives (IrqBinding) — 3 dias

**Único pré-requisito faltando para drivers userspace.**

| # | Primitiva | Mecanismo | Status |
|---|-----------|-----------|--------|
| 1 | **Interrupção → Endpoint** | `IrqBinding`: ISR faz `endpoint->signal(bits)`. `sys_bind_irq(vector, ep_handle)` + `CapabilityType::Irq` | ✅ Done |
| 2 | mmap físico userspace | `MAP_PHYSICAL` em mmap.cpp | ✅ Done |
| 3 | PCI config space userspace | `ioctl(PIOC_READ_CONFIG/WRITE_CONFIG)` em pci_node | ✅ Done |
| 4 | DMA compartilhável | `DmaShm` (alloc contígua + mapeamento userspace) | ✅ Done (`dma_shm.h/cpp`) |

### 3. Networking — TCP Completeness (Phase 20, ~25 syscalls)

| Gap | Detalhe | Prioridade |
|-----|---------|------------|
| **UDP socket** | Parcial — `sendto`/`recvfrom` OK (`udp_socket.cpp:74,94`); `connect`/`listen`/`accept` e setsockopt não-SOL_SOCKET são `NotImplemented` | MEDIUM |
| **Congestion control** | Sem slow start, congestion avoidance, fast recovery | MEDIUM |
| **TCP options** | Sem MSS negotiation, window scaling, SACK | MEDIUM |
| **IPv6** | Ausente completamente | LOW |
| **Syscalls faltando** | `sendmmsg`/`recvmmsg`, `MSG_DONTWAIT`, multicast, `SO_*` options | MEDIUM |
| **UDP syscalls** | `sendto`/`recvfrom` funcionam; falta servidor UDP (bind de servidor + opções avançadas) | MEDIUM |

### 4. Filesystems — Write Support

| FS | Leitura | Escrita | Prioridade |
|----|---------|---------|------------|
| **Ext2** | ✅ Completo | ✅ Completo (752 linhas) | — |
| **Ext3/Ext4** | ✅ Superblock parsing | ❌ `NotImplemented` | LOW |
| **FAT12/FAT16** | ✅ Leitura | ❌ `NotImplemented` | LOW |
| **FAT32** | ✅ Completo | ⚠️ Parcial | LOW |
| **ISO9660** | ✅ Completo | ❌ `NotImplemented` | LOW |
| **HFS+** | ✅ B-tree, symlinks, catalog | ❌ Write não implementado (read link só) | LOW |
| **UFS/UFS2** | ✅ Triple-indirect, fragments | ❌ `NotImplemented` | LOW |
| **MinixFS** | ⚠️ Limitado | ⚠️ Limitado | LOW |

---

## 🟡 MEDIUM — Melhorias Importantes

### 5. Phase 33 — Volume Layer (dm-crypt) — 2-3 dias

| Sub-phase | Componente | Status |
|-----------|-----------|--------|
| 33a | `StackableBlockDevice` base class | ✅ Done |
| 33b | **dm-crypt** (AES-XTS + LUKS + PBKDF2) | ✅ Done — `dm_crypt_device.cpp`: LUKS v1 unlock (PBKDF2-HMAC-SHA1 key derivation, AF-merge com SHA1 diffuse, AES-256-XTS decryption); `read_sectors`/`write_sectors` com XTS sector-tweak correto; só AES-256-XTS (keylen=64) suportado |
| 33c | RAID 0/1 | ⚠️ Implementado mas **órfão** (zero chamadores externos) |
| 33d | LVM (linear + striped) | ⚠️ Implementado mas **órfão** (zero chamadores externos) |

> **Nota**: 33c/33d compilam mas nada os instancia — falta o wiring no discovery de block devices para a camada de volume ter efeito real. Sem dm-crypt (33b), a pilha Filesystem→LVM→RAID→Crypto não se forma.

### 6. Phase 34b — x86_64 Fixes

| # | Fix | Prioridade |
|---|-----|------------|
| 7 | **KPTI** (Meltdown mitigation) — two PML4 roots + CR3 swap on syscall entry/exit | MEDIUM |
| 13 | Early serial fallback on COM1 | LOW |

### 7. Phase 34d — SMP Hardening — 1-2 dias

| Item | Detalhe |
|------|---------|
| IRQ affinity | Atribuir IRQs a CPUs específicas |
| Microcode update on APs | Carregar microcode durante AP startup |
| MTRR synchronisation | Propagar MTRR para todos os cores |
| Trampoline relocation | Mover AP trampoline para endereço seguro |
| APIC topology | Detectar hierarquia de APICs (cluster, die, etc.) |

### 8. Phase 39b — `pick_next()` Affinity Scan — O(N) → O(1)

Atualmente `pick_next()` escaneia tasks por nível de MLFQ (O(N_lvl)). Substituir por **per-CPU bitmap por afinidade**. (~1 dia)

### 9. Phase 40b — Userspace Driver Protocol — 3-5 dias

| # | Task | Detalhe |
|---|------|---------|
| 1 | `libdriver.so` | Lib userspace para drivers |
| 2 | `/dev/udi` device events | Kernel notifica userspace sobre novos devices |
| 3 | PCI device fallback | Se sem driver kernel, expõe via `/dev/udi` |
| 4 | Initrd com drivers userspace | Script de build |

### 10. Phase 40c — FUSE-like Filesystem Protocol — 5-7 dias

| # | Task | Detalhe |
|---|------|---------|
| 1 | `UserFsNode` | Node que traduz VFS ops em mensagens Endpoint |
| 2 | Protocolo `user_fs.h` | Estruturas FsRequest/FsResponse |
| 3 | `libuserfs.so` | Lib userspace para FS userspace |
| 4 | Exemplo: Ext4 userspace | Prova de conceito |
| 5 | Initrd com FS userspace | Root fs via userspace |

### 11. Phase 42 — Architecture Portability Layer — 5-7 semanas

Extrair dependências x86_64 para interfaces genéricas (AArch64/RISC-V no futuro).

**Já feito:** `arch_cpu_idle()`, `CpuContext` (mas só x86_64), `arch_read/write_msr`, `arch_cpuid`, `arch_*` I/O ports, `early_init.h` (básico).

**Falta:**

| # | Task | Esforço |
|---|------|---------|
| 1 | Generic `VirtualMemoryManager` interface | 3d |
| 2 | Neutral `PageFlags` + x86_64 translator | 1d |
| 3 | `cpu_irq_enable/disable` extracted | 1d |
| 4 | Generic `HardwareInterruptController` interface | 2d |
| 5 | Move `Timer` abstract class | 1d |
| 6 | Port APIC/IOAPIC to new interface | 2d |
| 7 | `arch_start_secondary_cpus()` | 1d |
| 8 | `arch_switch_context()` | 2d |
| 9 | `percpu.h` abstraction | 1d |
| 10 | `PtRegs` → union per arch | 2d |
| 11 | `KernelImage` struct | 2d |
| 12 | Remove arch includes from `init.cpp` | 1d |

**Dependências**: 42a → 42b → 42d → 42e

### 12. ELF Loader — Itens Restantes

| # | Task | Prioridade |
|---|------|------------|
| 13 | Cache program headers (causou Error 0; investigar) | LOW |
| 16 | Unificar TLS (split entre execve.cpp e init_task.cpp) | LOW |
| 17 | Mais testes ELF | LOW |
| — | Symbol versioning (DT_VERSYM/VERNEED) | LOW |

### 13. IOMMU (VT-d)

`Src/Kernel/Arch/x86_64/Memory/IntelIOMMU/vtd.cpp` — todas as 3 funções retornam `NotImplemented`. Sem tradução DMA real. (~2 semanas para implementação mínima)

### 14. Limites Rígidos Removidos (2026-08-04) ✅

| Item | Antes | Depois |
|------|-------|--------|
| `MAX_CPUS` | 32 (hard, duplicado em `gdt.h`) | 64 em `arch_defs.h`; `gdt.h` usa `::MAX_CPUS` |
| POSIX timers | `s_timers[8]` global estático | `Vector<PosixTimer>` + `ScopedLockIRQ s_timer_lock` |
| FD table | `static_vector<RefPtr<FileDescription>, 1024>` (~8 KiB/task, hard cap) | `Vector<RefPtr<FileDescription>>` (heap, cresce sob demanda) + `SOFT_OPEN_FILES_LIMIT=65536` |
| Exception recovery | `halt_forever()` em qualquer fault user-task | mata apenas a task (`kill_current_from_exception`); kernel só halts em bugs reais |

### 15. 12 locais com `NotImplemented` no kernel

Recontado 2026-08-03: **12 ocorrências em 7 arquivos** — a família mlock foi implementada (`mlock.cpp` com validação real) e UDP `connect`/`listen` implementados; o "47 locais" antigo estava stale. Lista exata sempre re-derivável: `rg "NotImplemented" Src/Kernel`.

Cada um precisa ser implementado ou documentado como "não vai fazer".

### 15. Phase 46 — Compressed Swap (ZRam/ZSwap)

Resumo aqui; **design técnico completo em `.ai-docs/ROADMAP.md`**. Cadeia: swap core → zram driver → reclaim síncrono → zswap (46d deferível). **Sem swap core, zram = disco RAM** (o `CONCEPTS.md:11-13` já previa "compressão como etapa anterior ao swap"). Depende do codec da Phase 47; `NullCodec` permite começar antes.

### 16. Phase 47 — Codec LZFSE (LibFK)

Interface `CompressionCodec` + LZFSE **reimplementado** (não port do C da Apple) em LibFK freestanding, com testes round-trip + golden vectors vs CLI `lzfse`. Troca para LZVN (LZSS, sem entropia) em entradas <4KiB. Paralelo à 46.

### 17. Phase 48 — Traits (LibFK)

`void_t`/`declval` → envolver builtins crus (`vector.h:67`, `circular_buffer.h:78`) → `is_constructible`/`is_convertible` → concepts C++20. Hoje só 2 consumers produtivos de `fk::traits` (`driver_registry.cpp:52-76`).

### 18. Phase 49 — Extração Kernel→LibFK

Time/checksum/id-generator/free-list pequenos primeiro → `slot_map` (CSpace, fd table, posix timers). Continuação do padrão de `notes/fs-to-libfk-extraction.md`.

### 19. Phase 51 — IPC Fastpath (seL4-style) — ⚠️ Parcial

**51a ✅ Modelo corrigido (2026-08-04):**
- `IpcMessage` (6×u64) em `ipc_message.h`; `pending_info`/`pending_message` em `TaskIpc`
- `send/receive/call` chamam `schedule()` após `block_current_noqueue()`
- `ipc_send.cpp` empacota `arg1..arg4` → `IpcMessage`; `ipc_receive.cpp` / `ipc_call.cpp` desempacotam reply em `regs->rdx/r10/r8/r9`
- `notification.cpp` usa `pending_notification` (não `registers().rax`)
- `switch_to_task(Task*)` adicionado ao `SchedulerManager`

**51b ✅ Fastpath call/reply (2026-08-04):**
- `task_cpu_compatible()` + `fastpath_qos_ok()` em `endpoint.cpp`
- `call()`: bail se sinal pendente / CPU incompatível / QoS receiver > caller / `m_call_sender` ocupado → `switch_to_task(receiver)` diretamente (sem run-queue/IPI)
- `send()` reply: bail se sinal pendente / CPU incompatível → `requeue_running_task()` + `switch_to_task(caller)`
- `SchedulerManager::requeue_running_task()` — reenfileira task atual sem context switch

| Sub-phase | Componente | Status |
|-----------|-----------|--------|
| 51a | Modelo corrigido | ✅ Done |
| 51b | Fastpath call/reply | ✅ Done |
| 51c | **Reply+recv fusion + asm** — single-pass server loop; skim dos syscalls 400-402 no `syscall_stub.asm`; PCID opcional | ⏳ Pendente |

**Dependência:** 51a/51b corrigem a base para drivers userspace (Phase 40b) e FUSE userspace (Phase 40c).

---

## 🔴 Code Quality Debt

### ✅ Syscall handlers — um handler por arquivo — DONE

Todos os arquivos de `Src/Kernel/Syscall/syscall_list/` definem agora no máximo um `sys_*` handler; nome do arquivo = nome do handler sem o prefixo `sys_` (arquivos de suporte com zero handlers são permitidos, ex. `Time/posix_timer.cpp`). Verificado por `Meta/x86_64-tools/check_one_syscall_per_file.lua` via `xmake check-syscalls`. `PosixTimerEntry` do scheduler unificado em `Include/Kernel/Syscall/posix_timer.h`; `sys_utimes` (235) e `sys_futimesat` (261) registrados; `SYS_NEWFSTATAT` (=262) usado pelo nome.

### SECRET RULE Violations — One Class Per File + No Nested Types

**Worst offenders:**

| # | File | Types | Esforço |
|---|------|-------|---------|
| 1 | 40+ files com **2 types** | Helper + main class | 0.25 dia cada |
| 2 | 30+ files com **nested types** | Vários drivers/containers | 0.25 dia cada |

**Já refatorados** (commit `fdaf30f` e branch `fix/create-directories`): `task.h`, `boot_info.h`, `dynamic_domain.h`, `nvme_utilities.h`, `srat.h`, `exfat_bpb.h`, `ahci_controller.h`, `minix_super.h`, `ext2_super.h`, `nvme_command.h` (+ layout NVMe SQE corrigido para 64 bytes); anonymous nested de `task_ipc.h`, `task_lifecycle.h`, `task_memory.h` extraídos para `TaskSignalState`/`TaskAltStack`/`TaskITimer`/`TaskMemoryRegions`; **`nvme_controller.h`** 4 nested (`Namespace`→`NvmeNamespace`, `QueuePair`→`NvmeQueuePair`, `Command`→`NvmeCommand`, `Completion`→`NvmeCompletion`) extraídos para arquivos próprios.

### Empty Scaffolding — arquivos `.cpp` vazios ✅

`nvme_device_configuration.cpp`, `nvme_queue_setup.cpp`, `nvme_register_mapper.cpp`, `nvme_interrupt_configurator.cpp`, `nvme_pending_operations.cpp` — **header-only**: todas as implementações estão inline nos `.h` correspondentes; os `.cpp` existem apenas como unidades de compilação. Nenhuma implementação adicional necessária.

### Include Order Reversed

Todos os `.cpp` do Kernel colocam **Kernel headers antes de LibFK headers**. Ordem correta: `LibC → LibFK → Kernel → Local`. **320 de 462 arquivos (69%) kernel-first** — re-verificado 2026-08-03 (`rg -l` + `check_layers.lua`).

**Fix**: Bulk reorder com script. (~1 dia)

### Dead Code — `LibFK/Tree/rb_tree.h`

`Include/LibFK/Tree/rb_tree.h` tem **zero consumers**; `map.h`/`set.h`/`multi_map.h`/`multi_set.h`/`deque.h` são `[[deprecated]]`. Reviver (p/ estruturas ordenadas futuras, ex. rota/LPM) ou remover.

### Object Calisthenics Violations

| Regra | Status | Esforço |
|-------|--------|---------|
| No `else` — early returns | 30+ arquivos violam | ~5 dias |
| Max 2 instance vars | Virtualmente toda classe viola | Reforma arquitetural |
| No getters/setters | Generalizado (VGA, VFS Node, APIC, Scheduler) | ~10 dias |
| Files >500 linhas | 5 arquivos (ext2_fs.cpp: 740, exfat_fs.cpp: 704, VMM: 601, minix_fs: 518, syscall.cpp: 512) | ~3 dias |
| No abbreviations | 3 violações (BootInfo, MessageInfo, DmaBuffer) | 0.5 dia |
| Lowercase class names | `class serial`, `class vga` | 0.5 dia |
| Directory naming | `syscall_list/` → `SyscallList/` | 0.5 dia |
| File starting with digit | `8259_pic.h/.cpp` → `i8259_pic.h/.cpp` | 0.5 dia |
| `m_` prefix inconsistente | display_framebuffer.h (13/18 sem), task.h substructs (30+ sem) | ~3 dias |

---

## 🟢 LOW — Polimento e Longo Prazo

| Item | Notas |
|------|-------|
| **SquashFS** | Boot-critical, simples — ainda não implementado |
| **inotify** | Phase 31d, LOW |
| **`/proc/sys/` writable nodes** | Phase 31d, LOW |
| **Coredumps** | Phase 31d, LOW |
| **Userspace terminal emulator** | Phase 31e, novo programa |
| **Swap em disco / zswap** | Depois da Phase 46d (zswap exige swap em disco, não-zram) |
| **Matriz de teste em hardware real (laptop)** | Boot testado em silício após bloqueadores HIGH resolvidos |
| **DRM/GEM** | Para GPU userspace |
| **Audio DMA engine** | Para audio stack userspace |
| **HCI/netlink socket** | Para wireless/Bluetooth userspace |
| **/dev/tpm** | Para TPM 2.0 userspace |
| **Documentação Docs/ desatualizada** | Vários README stubs em Docs/Kernel/ |
| **Review diagramas Mermaid** | Arquitecture docs têm Mermaid; Domains/ podem estar defasados |

---

## Build Verification

| After change | Commands |
|-------------|----------|
| Kernel changes | `xmake` + `xmake check-layers` |
| LibFK/LibC changes | `xmake -bv Test && xmake run Test` |
| Full boot test | `xmake run` |
| UDI changes | `xmake check-layers` + boots QEMU |

## References

- `.ai-docs/CHANGELOG.md` — tudo que foi completado
- `.ai-docs/ROADMAP.md` — fases futuras com detalhes completos
- `.ai-docs/AUDITS.md` — auditorias de ELF, IPC, POSIX-readiness, x86_64
- `AGENTS.md` — regras de codificação e estilo
- `Docs/` — documentação de arquitetura
