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
| **Memory Mgmt** | ⚠️ Audit 2026-08-01 | 4 bugs críticos (order buddy, accounting, bitmap↔buddy, realloc slab) + 9 médios/altos; IOMMU stubbed |
| **Scheduler** | ✅ Working | MLFQ + SMP work-stealing + QoS; `pick_next()` affinity O(N) |
| **IPC (seL4)** | ✅ Working | Capabilities, CSpace, Endpoint, Notification, revocation; **Phase 27 (fd→CSpace) DONE** |
| **VFS** | ✅ Working | Ext2 r/w; ext3/4, UFS, HFS+, FAT write are stubs |
| **ELF Loader** | ✅ Working | ASLR, TLS, RELRO, dynamic linking; TLS split across execve+init_task |
| **Syscalls** | ⚠️ 207 registrados | 47 `NotImplemented` (mlock family, UDP connect/listen/accept, ext4 write, IOMMU) |
| **Networking** | ⚠️ Partial | TCP state machine + retransmit OK; UDP sendto/recvfrom OK; connect/listen/accept stub; sem congestion control; sem IPv6 |
| **Drivers** | ⚠️ Mixed | E1000, ATA DMA, PS/2, PTY OK; NVMe sem PRP2 (>4KiB) + 5 .cpp vazios; AHCI async stalled |
| **SMP** | ⚠️ Partial | Bugs 21-36 corrigidos; up to 32 CPUs; no IRQ affinity, no MTRR sync |
| **Security** | ⚠️ Partial | SMEP/SMAP/NX/W^X on all CPUs; ASLR via ChaCha20 (fix 2026-07-31); KPTI missing; IOMMU stubbed |
| **Tests** | ⚠️ Growing | LibC/LibFK ~85%; kernel has 8 suites / **89 tests**; Phase 43 target 75% |
| **Docs** | ⚠️ Partial | AGENTS.md/TODO.md excelentes; Docs/ tem stubs |
| **Code Quality** | ⚠️ Debt | task.h viola SECRET RULE; include order invertido; calisthenics violado; 5 .cpp NVMe vazios |

---

## 🔴 IMMEDIATE — Correção de Bugs

### Auditoria x86_64 (2026-07-30) — Bugs confirmados

Todos os bugs das auditorias anteriores foram corrigidos (Bugs 9, 10, 18–36 — ver `.ai-docs/CHANGELOG.md` e `.ai-docs/AUDITS.md`).

**⚪ BAIXO — único restante**

| # | Bug | Local |
|---|-----|-------|
| 35 | Bridge VESA real-mode é placeholder — VBE não funciona em x86_64 (só framebuffer multiboot2) | `Driver/Vga/real_mode_bridge.asm` |

### Auditoria de Memória (2026-08-01) — Bugs confirmados

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
| M5 | CoW break pode criar página **User** a partir de página kernel RO: PF handler trata write fault de kernel em página presente; branch `!(User)` recria com `\| User` | `pf_handler.cpp:64-74,100` |
| M6 | Slab sem lock e chamado **antes** de `save_and_disable_interrupts()` → race em SMP/IRQ context | `slab_allocator.cpp`, `memory_manager.cpp:100-106` |

**🟡 MÉDIO**

| # | Bug | Local |
|---|-----|-------|
| M7 | `free_contiguous` devolve bloco ao buddy incondicionalmente mesmo com refcounts CoW > 0 | `physical_memory_manager.cpp:388-394` |
| M8 | `select_zone` não faz fallback quando a zona escolhida está cheia (bitmap.alloc só na zona selecionada) → OOM com outras zonas livres | `physical_memory_manager.cpp:214-275` |
| M9 | Huge pages de usuário não tratadas em `free_address_space`/`unmap_page_range` → leak de 511 páginas por PDE 2MiB | `virtual_memory_manager.cpp:357-361,414-416` |
| M10 | Demand paging **file-backed não implementado** (docs afirmam que lê da page cache; código só zero-fill anônimo) | `pf_handler.cpp:19-33`, `Docs/Domains/memory-management-guide.md:246-248` |
| M11 | `translate()` exige alinhamento mas soma `(virt & 0xFFF)` (código morto); `get_page_flags` devolve bits crus (inclui Accessed/Dirty) comparados como `PageFlags` | `virtual_memory_manager.cpp:202-207,240-260` |
| M12 | Direct map RWX (viola W^X declarada), mapeia buracos/memória reservada, dimensionado por `total_memory()` (só disponível) | `virtual_memory_manager.cpp:486-543` |
| M13 | Frame 0 / low memory (IVT, VGA/BIOS 0xA0000-0xFFFFF) não reservados → `alloc_page` pode devolver 0, interpretado como falha pelo VMM | `physical_memory_manager.cpp:126-154`, `virtual_memory_manager.cpp:48-51` |

**⚪ BAIXO**

- Invariante não enforced: tabelas de páginas acessadas via **ponteiro físico** dependem do identity map de 4 GiB — quebraria se `alloc_page` caísse na zona HIGH (>4 GiB)
- `get_refcount` retorna 1 para rc==0 (conflaciona "não rastreado" com "refcount zerado")
- Docs desatualizadas: slab "≤8192" vs código `≤2048` (`memory_manager.cpp:100`); demand paging file-backed
- `BuddyAllocator::initialize()` é código morto (caminho real é `initialize_from_bitmap`) com lógica de loop confusa

---

## 🟠 HIGH — Bloqueantes para POSIX e Uso Real

### 1. Kernel Test Harness — Phase 43 (13 dias)

Kernel tem ~0% de cobertura. 3 testes existem para 60K+ linhas. **Isso é o maior risco do projeto.**

| Sub-phase | Componente | Dias | Status |
|-----------|-----------|------|--------|
| 43a | Test infrastructure (runner, mocks, CI) | 2 | ✅ Done |
| 43b | **VFS**: path resolution, dentry cache, file description | 3 | ⚠️ Partial (9 dentry cache tests ✅; path resolver + file description ❌) |
| 43c | **Memory**: buddy allocator, slab, multi-zone | 2 | ✅ Done (BuddyState 8 tests + Zone 12 tests) |
| 43d | ELF loader: header validation | 2 | ✅ Done (15 tests) |
| 43e | **Scheduler**: MLFQ demotion, QoS, turnstile chain | 2 | ⚠️ Partial (14 QoS tests ✅; MLFQ demotion + turnstile ❌) |
| 43f | **TCP**: handshake, sliding window, retransmit | 2 | ❌ Open |

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
| 33b | **dm-crypt** (AES-XTS + LUKS + PBKDF2) | ❌ Open — nada implementado |
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

### 14. 47 locais com `NotImplemented` no kernel

Distribuídos por: UDP socket (7), TCP socket (6), IOMMU (3), ATA DMA (2), AHCI (1), NVMe (1), ext4 (1), HFS+ (1), ISO9660 (1), FAT16 (1), MinixFS (1), UFS (1), mlock (4), madvise (1), utimensat (2), terminal (5), PCI node (1), VGA terminal (1), unix socket (1), APIC (1), MSI (1), HardwareInterruptManager (2), syscall dispatcher (1).

Cada um precisa ser implementado ou documentado como "não vai fazer".

---

## 🔴 Code Quality Debt

### ✅ Syscall handlers — um handler por arquivo — DONE

Todos os arquivos de `Src/Kernel/Syscall/syscall_list/` definem agora no máximo um `sys_*` handler; nome do arquivo = nome do handler sem o prefixo `sys_` (arquivos de suporte com zero handlers são permitidos, ex. `Time/posix_timer.cpp`). Verificado por `Meta/x86_64-tools/check_one_syscall_per_file.lua` via `xmake check-syscalls`. `PosixTimerEntry` do scheduler unificado em `Include/Kernel/Syscall/posix_timer.h`; `sys_utimes` (235) e `sys_futimesat` (261) registrados; `SYS_NEWFSTATAT` (=262) usado pelo nome.

### SECRET RULE Violations — One Class Per File + No Nested Types

**Worst offenders:**

| # | File | Types | Esforço |
|---|------|-------|---------|
| 1 | `task.h` | **8 top-level** + **2 nested** + **4 anonymous nested** | **3-4 dias** |
| 2 | `boot_info.h` | 7 top-level types | 1 dia |
| 3 | `srat.h` | 5 types | 0.5 dia |
| 4 | `exfat_bpb.h` | 5 types | 0.5 dia |
| 5 | `minix_super.h` | 4 types | 0.5 dia |
| 6 | `ext2_super.h` | 4 types | 0.5 dia |
| 7 | `nvme_utilities.h` | 4 types | 0.5 dia |
| 8 | `nvme_command.h` | 4 types | 0.5 dia |
| 9 | `dynamic_domain.h` | 2 top-level + 2 nested | 0.5 dia |
| 10 | `ahci_controller.h` | 6 nested structs | 1 dia |
| 11 | `nvme_controller.h` | 4 nested | 1 dia |
| 12 | 40+ files com **2 types** | Helper + main class | 0.25 dia cada |
| 13 | 30+ files com **nested types** | Vários drivers/containers | 0.25 dia cada |

**Total estimado**: ~15-20 dias para resolver tudo.

### Empty Scaffolding — arquivos `.cpp` vazios

| # | Arquivos | Status | Esforço |
|---|----------|--------|---------|
| 1 | `nvme_device_configuration.cpp`, `nvme_queue_setup.cpp`, `nvme_register_mapper.cpp`, `nvme_interrupt_configurator.cpp`, `nvme_pending_operations.cpp` | Cascas vazias (`namespace fkernel {}` apenas); headers declarados usados por `test_nvme_refactoring.cpp` e `interrupt_driven_nvme.cpp` | ~1-2 dias para implementar (ou remover com ajuste do teste) |

### Include Order Reversed

Todos os `.cpp` do Kernel colocam **Kernel headers antes de LibFK headers**. Ordem correta: `LibC → LibFK → Kernel → Local`. ~200 arquivos afetados.

**Fix**: Bulk reorder com script. (~1 dia)

### Object Calisthenics Violations

| Regra | Status | Esforço |
|-------|--------|---------|
| No `else` — early returns | 30+ arquivos violam | ~5 dias |
| Max 2 instance vars | Virtualmente toda classe viola | Reforma arquitetural |
| No getters/setters | Generalizado (VGA, VFS Node, APIC, Scheduler) | ~10 dias |
| Files >500 linhas | 5 arquivos (ext2_fs.cpp: 752, exfat_fs.cpp: 703, VMM: 543, minix_fs: 521, syscall.cpp: 504) | ~3 dias |
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
| **UDP server** | Socket bind/recvfrom para UDP |
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
