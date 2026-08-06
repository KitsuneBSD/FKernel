# FKernel TODO

> **Only open bugs and pending work.** See also:
> - `.ai-docs/CHANGELOG.md` — everything completed
> - `.ai-docs/ROADMAP.md` — future phases with full detail
> - `.ai-docs/AUDITS.md` — audit findings

---

## Quick Status

| Area | Status | Key Gaps |
|------|--------|----------|
| **Boot/Init** | ✅ Working | GRUB+Multiboot2 → SMP AP startup → scheduler → userspace init (CoW demand-page bug fixed 2026-08-06) |
| **Memory Mgmt** | ⚠️ Open (audit 2026-08-03) | M1–M13 ✅ (ver CHANGELOG); identidade 4 GiB × zona HIGH parcial; IOMMU stubbed |
| **Memory Pressure** | ❌ Ausente | **sem swap, sem page cache, sem reclaim, sem OOM killer** (slab OOM = `kerror`+nullptr — **C3**, callers sem check de null) — Phase 46 |
| **Scheduler** | ✅ Working | MLFQ + SMP work-stealing + QoS; `pick_next()` affinity O(N) (Phase 39b) |
| **Exceptions/Interrupts** | ✅ Sprint concluído (2026-08-04) | I1–I5 + R1–R4 ✅ (ver CHANGELOG); restam BAIXO: `apic_timer_handler` dead, `send_eoi` vector−32; Phase 51c pendente |
| **IPC (seL4)** | ✅ Rendezvous OK | Capabilities, CSpace, Notification, revocation OK; 51a/51b ✅; **Phase 51c pendente** (reply+recv fusion + asm skim) |
| **VFS** | ✅ Working | Ext2 r/w completo (triple-indirect ✅); ext3/4, UFS, HFS+ write stubs; FAT node write ✅ (`FileSystem::write` retorna `IsDirectory` = EISDIR — comportamento correto para diretório) |
| **ELF Loader** | ✅ Working | ASLR, TLS, RELRO, dynamic linking |
| **Syscalls** | ⚠️ 206 registrados | **8** `NotImplemented` em 4 arquivos (IOMMU, terminal Serial/PTY, syscall desconhecida); mlock ✅; UDP connect/listen ✅ |
| **Networking** | ⚠️ Partial | TCP state machine + retransmit OK; UDP sendto/recvfrom/connect/listen OK (`accept`=EOPNOTSUPP, correto); sem congestion control; sem IPv6 |
| **Drivers** | ⚠️ Mixed | E1000, ATA DMA (>256 setores ✅), AHCI async ✅, NVMe PRP2 ✅, PS/2, PTY OK; **USB (xHCI/EHCI/HID): headers-only, zero .cpp** — maior gap p/ laptop moderno |
| **Hardware/Firmware (ACPI)** | ⚠️ Partial | RSDP/RSDT/XSDT, MADT, FADT, MCFG, SRAT, DMAR, HPET OK; **sem interpretador AML (DSDT/SSDT)** → sem battery/thermal/sleep |
| **SMP** | ⚠️ Partial | Bugs 21-36 corrigidos; up to 64 CPUs (`MAX_CPUS=64`); no IRQ affinity, no MTRR sync |
| **Security** | ⚠️ Partial | SMEP/SMAP/NX/W^X on all CPUs; ASLR via ChaCha20; **KPTI missing** (Phase 45b); IOMMU stubbed |
| **Tests** | ⚠️ Growing | LibC/LibFK ~85% (stdiocomp + 6 suites kernel re-linkados — **L6 ✅**, 41 suites / 450 tests); kernel **17 suites / 145 tests**; Phase 43 target 75% |
| **Docs** | ⚠️ Partial | memory-guide slab + file-backed (M10) corrigidos 2026-08-04; AGENTS.md desatualizado (`arch_cpu_idle` já existe; tabela de logging ainda diz "kerror → halt") |
| **Code Quality** | ⚠️ Debt | task.h ✅ refatorado; ~~include order invertido~~ ✅ 315/464 corrigidos (2026-08-06); ~~xmake.lua monolítico~~ ✅ particionado; ~~posix_stubs.c~~ ✅ include order + pthread_create; ~~rb_tree.h~~ ✅ removido; calisthenics violado |

---

## 🔴 IMMEDIATE — Correção de Bugs

### Auditoria x86_64 (2026-07-30) — Bugs confirmados

Todos os bugs das auditorias anteriores foram corrigidos (Bugs 9, 10, 18–36 — ver `.ai-docs/CHANGELOG.md` e `.ai-docs/AUDITS.md`).

**⚪ BAIXO — único restante**

| # | Bug | Local |
|---|-----|-------|
| 35 | Bridge VESA real-mode é placeholder — VBE não funciona em x86_64 (só framebuffer multiboot2) | `Arch/x86_64/Driver/Vga/real_mode_bridge.asm` |
| ~~apic_timer_handler dead~~ | ✅ **Removida declaração morta** (2026-08-06): `apic_timer_handler` declarada mas nunca implementada — removida de `handlers.h`; APIC timer usa `timer_handler` em vetor 32 (APIC_TIMER_VECTOR=0x20) | — |
| ~~send_eoi(vector)~~ | ✅ **`send_eoi(vector-32)` normalizado** (2026-08-06): todos os 5 handlers IRQ passavam `vector` (32+irq) em vez de `irq`; PIC usa o arg para spurious check (IRQ7/15) e slave routing (>=8) — passagem de `vector` quebrava ambos; corrigido `timer`, `keyboard`, `mouse`, `clock`, `ata_primary`, `ata_secondary` | — |

### Auditoria de Memória (2026-08-01; re-audit 2026-08-03)

M1–M5/M7–M10/M13 corrigidos (ver `.ai-docs/CHANGELOG.md`; testes em `tests/Kernel/test_buddy_allocator.cpp` e `test_slab_allocator.cpp`). **Restam:**

**🟡 MÉDIO**

| # | Bug | Local |
|---|-----|-------|
| ~~M6~~ | ✅ **`save_and_disable_interrupts` movido para antes da chamada ao slab** (2026-08-06): `allocate()`, `reallocate()` e `free()` agora desabilitam IRQs antes de tentar slab; caminhos slab chamam `restore_interrupts` ao retornar; elimina janela de IRQ sem proteção | — |
| ~~M11~~ | ✅ **`get_page_flags` agora retorna Accessed/Dirty sem mascarar** (2026-08-06): removidos os dois `flags &= ~PageFlags::Accessed/Dirty` | — |
| ~~M12~~ | ✅ **Direct map pula buracos físicos** (2026-08-06): `for_each_zone()` adicionado ao PMM; `extend_direct_map` usa `chunk_has_ram()` para pular chunks sem RAM real | — |

**⚪ BAIXO**

- ~~`get_refcount` retorna 1 para rc==0~~: ✅ corrigido (2026-08-06) — retorna 0 para frames com refcount zerado; "não rastreado" (sem array cow_refcounts) ainda retorna 1
- ~~`BuddyAllocator::initialize()` código morto~~: ✅ removido (2026-08-06) — método e construtor de 2 args removidos do header e .cpp; única initialização válida é `initialize_from_bitmap()`
- ~~Resíduo M5~~: ✅ corrigido (2026-08-06) — `handle_demand_paging` não mais OR `User` incondicionalmente; flag User vem de `region.flags`, default sem User
- identidade 4 GiB × zona HIGH: `alloc_page_for_pagetable()` corrigido; `shm_node.cpp:35,47` e acesso VMM a frames >4GiB via identidade ainda pendente (apenas se mmap físico >4GiB for necessário)

### Auditoria de Exceções/Interrupções x86_64 (2026-08-04)

I1–I5 e R1–R4 corrigidos + sprint de estabilidade completo (ver `.ai-docs/CHANGELOG.md`). **Restam:**

**⚪ BAIXO**

- ~~`apic_timer_handler` declarado e nunca registrado~~: ✅ removida declaração morta (2026-08-06)
- ~~`send_eoi(vector)` vs `send_eoi(irq)`~~: ✅ normalizado `vector-32` em todos os handlers (2026-08-06)
- Phase 51c (IPC fastpath reply+recv fusion + asm skim) — ver MEDIUM 17

### Bug Crítico: CoW/demand-page sem User flag (2026-08-06)

**✅ CORRIGIDO (2026-08-06)**: `handle_demand_paging` e `handle_write_protection` não propagavam `PageFlags::User` para páginas mapeadas em faults de user-mode. `brk` estende `heap_break` sem adicionar `MemoryRegion` → scan de regiões falha → flags default sem `User` → página mapeada como supervisor-only → write fault infinito (500× na mesma VA) até fault storm matar init. Fix: passar `is_user_fault` a ambos os handlers; `if (is_user_fault) flags = flags | PageFlags::User`. Também `unmap_page` kdebug → ktrace (elimina 448 linhas de noise de guard pages IST).

### Recuperação de Falhas (pesquisa 2026-08-04)

Recuperação de exceções implementada em camadas: Layer 1 (syscall → EFAULT via `user_range_is_accessible`), Layer 2 (kill de task + fault rate-limit 500/10 ticks), Layer 3 (`panic_exception()` unificado). Tudo verificado e corrigido no sprint 2026-08-04. **Pendente (futuro):** `fixup`/`extable`, watchdog real, depth de exceção — nenhum deles bloqueia uso atual.

### Auditoria LibC/LibFK (2026-08-05)

Revisão de `Include/LibC`, `Src/LibC`, `Include/LibFK`, `Src/LibFK` (camadas base, antes do kernel). Foco: corretude de contrato ABI, caminhos de erro de memória, portabilidade e cobertura de testes.

L1 (errno ABI), L3 (signed overflow), L6 (testes órfãos) e L11 (`operator new` OOM → kfatal) corrigidos no mesmo dia (ver `.ai-docs/CHANGELOG.md`). Metade de L10 (retorno C11) também. **Restam:**

**🔴 CRÍTICO**

| # | Bug | Local |
|---|-----|-------|
| ~~L2~~ | ✅ **`Vector::push_back` overflow silencioso em OOM**: mutators (`push_back`/`push_range`/`insert_at`/`resize`) agora `[[nodiscard]] Result<void>`; `reallocate_to()` retorna `Error::OutOfMemory` (antes engolia kmalloc null). 165 call sites sweepados: `TRY(...)` quando o caller propaga `Result`/`Error`, `TRY_OR_FATAL(...)` (`kfatal`+unreachable, consistente com L11) em callers `void`/`int`/syscalls — nunca engolir OOM (2026-08-05) | `Include/LibFK/Container/Sequence/vector.h`, `Include/LibFK/Core/result.h` |
| ~~L4~~ | ✅ **Rank tracker agora per-CPU** (2026-08-06): `g_current_lock_rank` → `g_cpu_lock_ranks[FK_MAX_CPUS=64]`; `cpu_lock_slot()` extrai APIC ID via CPUID com guard `__fkernel__`; `spinlock.h` reutiliza `cpu_lock_slot()` eliminando duplicação do CPUID inline | — |

**🟠 ALTO**

| # | Bug | Local |
|---|-----|-------|
| ~~L5~~ | ✅ **`g_printf_buf` global removido** (2026-08-06): `vprintf` usa `char buf[2048]` local eliminando reentrância de IRQ. `fprintf`/`fputs`/`fputc` ignoram stream — comportamento correto em freestanding. | — |
| L7 | **asm x86 cru dentro do LibFK** (viola a política arch_* do AGENTS.md): `cpuid`/`pause`/`pushfq`/`cli`/`sti` em `spinlock.h`, `cli;hlt` em `log.h`/`cxxabi.cpp`, `LibFK/Arch/x86_64/io.h`. LibFK não-portável; resolver com camada de callbacks arch (padrão `allocator_backend.h`) ou mover para o Kernel. **Conecta à Phase 42** | `Include/LibFK/Synchronization/spinlock.h:27,44,121-138`, `Include/LibFK/Algorithms/Logging/log.h:53-55`, `Src/LibFK/Core/cxxabi.cpp:34,50`, `Include/LibFK/Arch/x86_64/io.h` |

**🟡 MÉDIO**

| # | Bug | Local |
|---|-----|-------|
| ~~L8~~ | ✅ **`strtoull`/`strtoll`/`strtoul` com overflow check** (2026-08-06): `strtoul` → ERANGE+ULONG_MAX; `strtoll` reimplementado (não delega mais em `strtol`, usa range LLONG_MAX/LLONG_MIN corretamente); `strtoull` → ERANGE+ULLONG_MAX. `strtod` sem ERANGE/inf/nan permanece pendente (freestanding: não usa float). | — |
| ~~L9~~ | ✅ **`vsscanf` matching failure corrigido** (2026-08-06): `%i`/`%d` sem dígitos agora retornam matching failure; adicionado backtracking de sign (`-`) para `%d`; `sscanf.c` extraído de `file.c` e adicionado ao target LibC_Testing; 2 testes adicionados | — |
| ~~L10~~ | ✅ **`vsnprintf` precision para inteiros** (2026-08-06): `print_num` recebe `precision`; `prec_zeros = max(0, precision-digits)`; flag `0` ignorada quando precision especificado; passa precision para `%d/%i/%u/%o/%x/%X`; 1 suite de testes adicionada | — |
| ~~L11~~ | ✅ **`operator new` OOM → `kfatal` + `__builtin_unreachable()`** (com `-fno-exceptions` não há canal de propagação; simplifica L2) | `Src/LibFK/Memory/Allocators/new.cpp` |

**⚪ BAIXO**

- `Result<T>` não suporta move-only por lvalue (só `std::move`); `optional` sem `emplace`/`value_or`/`operator*`/`->`/move-assign
- ~~`__cxa_guard_acquire` spin com load sem acquire~~: ✅ corrigido (2026-08-06): `__atomic_load_n(guard_byte, __ATOMIC_ACQUIRE)` — acquire load correto para ARM/RISC-V; `__cxa_pure_virtual` também migrado para `__builtin_trap()`; pause inline asm marcado para Phase 42 (`arch_cpu_relax()`); `__cxa_atexit` nunca roda destructors (aceitável em kernel)
- ~~`assert` sempre ativo (sem gate `NDEBUG`); `LibC/assert.h` nem define `assert` (só `ASSERT`/`KASSERT`)~~: ✅ corrigido (2026-08-06): `assert(expr)` adicionado com gate `#ifdef NDEBUG` → `(void)0`; `ASSERT`/`KASSERT` mantidos para código kernel
- ~~`Spinlock::unlock` sem check de dono~~: ✅ guard adicionado (2026-08-06): `if (m_recursion_count == 0) return;` evita underflow para `0xFFFFFFFF` e lock preso; detecção de recursão por `(apic_id+1)` quebra se APIC ID = `0xFFFFFFFF` ainda pendente (saturação a slot 0 mitiga na prática)
- ~~`posix_stubs.c`~~: ✅ corrigido (2026-08-06): includes movidos para o topo (ordem `dirent/errno/fcntl/pthread/stdlib/stat/termios/unistd`); `pthread_create` retorna `EAGAIN` diretamente (padrão POSIX — pthread não usa -1/errno)
- ~~`fixed_string::assign` overflow seta `length = N`~~: ✅ corrigido (2026-08-06): check-first `if (len > N) return false` sem modificar buffer; ~~ctor `fixed_string(const char*)` não-constexpr~~: ✅ constexpr adicionado

**Prioridade de correção:** L1 ✅, L2 ✅, L3 ✅, L4 ✅, L5 ✅, L6 ✅, L8 ✅, L11 ✅ → **L7** (decisão de arquitetura: camada arch do LibFK — Phase 42) → L9/L10 (restante).

### Auditoria de Conformidade AGENTS.md (2026-08-05)

Auditoria de regras de código em `Include/LibC`, `Include/LibFK`, `Include/Kernel`, `Src/LibC`, `Src/LibFK`, `Src/Kernel`. Verificações que passaram (layer separation, snake_case, SECRET RULE, um handler/arquivo, prefixos de log, padrão Manager) estão registradas em `.ai-docs/AUDITS.md` (2026-08-05). C5 + checkers corrigidos no mesmo dia (ver CHANGELOG). **Restam:**

**🟠 ALTO — descobertas novas**

| # | Bug | Local |
|---|-----|-------|
| ~~C1~~ | ✅ **asm x86 cru em código genérico** corrigido (2026-08-06): `cli`→`arch_disable_interrupts()`, `lea ap_entry`→ponteiro direto + declaração em `ap_entry.h`, CR0/CR3/wbinvd→`arch_read_cr0/write_cr0/arch_wbinvd/arch_flush_tlb` (novas funções em `cpu_ops.h/cpp`), `rdtsc`→`arch_read_tsc()`, `mov %%gs:32`→`arch_get_cpu_id()` (nova em `cpu_ops.h/cpp`), `asm("":::"memory")`→`__sync_synchronize()`; `device_not_available.cpp` também limpo (usava `fpu_save/restore` locais com asm cru + `extern "C"` errado em `g_use_xsave` — substituído por `arch_fpu_save/restore`). `check-arch-asm` agora falha apenas nos 4 arquivos LibFK (L7) | L7: `spinlock.h`, `interrupt_disabler.h`, `Algorithms/Logging/log.h`, `Core/cxxabi.cpp` |
| ~~C2~~ | ✅ **`DmaBuffer` legacy removido** (2026-08-06): 4 consumers migrados para `fkernel::DmaBuffer` (`allocate`/`virtual_address`/`physical_address().as_uintptr()`); `Memory/Dma/dma_buffer.h` e `.cpp` deletados; `interrupt_driven_ahci.cpp` também migrado. RAII cleanup automático elimina todos os `dma_free_buffer` explícitos | — |
| ~~C3~~ | ✅ **Callers de kmalloc sem null check corrigidos** (2026-08-06): auditados 73 call sites; único caller sem null check era `fat_32_fs.cpp:237` (`read_from_cluster_chain`) — corrigido com `Error::OutOfMemory` + `Error::IOError` no device read. Todos outros callers já verificavam. | — |
| ~~C4~~ | ✅ **`kerror` → `kwarn` para erros recuperáveis** (2026-08-06): `driver_manager.cpp`, `ata_device.cpp`, `hardware_interrupt_manager.cpp`, `pio_strategy.cpp` — todos retornam `Error::*` ao caller sem halt; reclassificados como `kwarn` | — |

**Resolvido (2026-08-06):** AGENTS.md corrigido — `Panic.cpp` → `panic.cpp` (linha 41); tabela logging e `arch_cpu_idle` já estavam corretos.

**Fix (ordem sugerida):** ~~C3 ✅~~ → ~~C2 ✅~~ → ~~C1 ✅~~ → ~~C4 ✅~~.

---

## 🟠 HIGH — Bloqueantes para POSIX e Uso Real

### 0. Hardware Real — Laptop Moderno (audit 2026-08-03)

> Alvo declarado: laptop moderno (sem PS/2, NVMe, >4 GiB RAM). Ordenado por bloqueio real de uso. NVMe PRP2, identidade 4 GiB, M10 file-backed, AHCI async e Ext2 triple-indirect ✅ (ver CHANGELOG).

| # | Bloqueador | Por quê | Fase |
|---|-----------|---------|------|
| 1 | **USB (xHCI + EHCI + HID)** | laptop moderno **não tem PS/2** → teclado/mouse/storage inoperáveis | nova (Phase 50) |
| 4 | **Swap/reclaim mínimo** | OOM hoje = `kerror` + nullptr (era halt; mudou com o split `kfatal` — ver **C3**) | Phase 46 |
| 7 | **VBE fallback (Bug 35)** | console em máquina sem GOP | curto |

### 1. Networking — TCP Completeness (Phase 20, ~25 syscalls)

| Gap | Detalhe | Prioridade |
|-----|---------|------------|
| **UDP socket** | `sendto`/`recvfrom`/`connect`/`listen` OK (`udp_socket.cpp`); `accept`=EOPNOTSUPP (correto p/ UDP); setsockopt não-SOL_SOCKET ainda `NotImplemented` | MEDIUM |
| **Congestion control** | Sem slow start, congestion avoidance, fast recovery | MEDIUM |
| **TCP options** | Sem MSS negotiation, window scaling, SACK | MEDIUM |
| **IPv6** | Ausente completamente | LOW |
| **Syscalls faltando** | `sendmmsg`/`recvmmsg`, `MSG_DONTWAIT`, multicast, `SO_*` options | MEDIUM |

### 2. Filesystems — Write Support

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

### 3. Phase 33 — Volume Layer (dm-crypt)

| Sub-phase | Componente | Status |
|-----------|-----------|--------|
| 33a | `StackableBlockDevice` base class | ✅ Done |
| 33b | **dm-crypt** (AES-XTS + LUKS + PBKDF2) | ✅ Done — `dm_crypt_device.cpp`: LUKS v1 unlock, AES-256-XTS; só AES-256-XTS (keylen=64) |
| 33c | RAID 0/1 | ⚠️ Implementado mas **órfão** (zero chamadores externos) |
| 33d | LVM (linear + striped) | ⚠️ Implementado mas **órfão** (zero chamadores externos) |

> **Nota**: 33c/33d compilam mas nada os instancia — falta o wiring no discovery de block devices para a camada de volume ter efeito real.

### 4. Phase 34b — x86_64 Fixes

| # | Fix | Prioridade |
|---|-----|------------|
| 7 | **KPTI** (Meltdown mitigation) — two PML4 roots + CR3 swap on syscall entry/exit | MEDIUM |
| 13 | Early serial fallback on COM1 | LOW |

### 5. Phase 34d — SMP Hardening

| Item | Detalhe |
|------|---------|
| IRQ affinity | Atribuir IRQs a CPUs específicas |
| Microcode update on APs | Carregar microcode durante AP startup |
| MTRR synchronisation | Propagar MTRR para todos os cores |
| Trampoline relocation | Mover AP trampoline para endereço seguro |
| APIC topology | Detectar hierarquia de APICs (cluster, die, etc.) |

### 6. Phase 39b — `pick_next()` Affinity Scan — O(N) → O(1)

Atualmente `pick_next()` escaneia tasks por nível de MLFQ (O(N_lvl)). Substituir por **per-CPU bitmap por afinidade**. (~1 dia)

### 7. Phase 40b — Userspace Driver Protocol

| # | Task | Detalhe |
|---|------|---------|
| 1 | `libdriver.so` | Lib userspace para drivers |
| 2 | `/dev/udi` device events | Kernel notifica userspace sobre novos devices |
| 3 | PCI device fallback | Se sem driver kernel, expõe via `/dev/udi` |
| 4 | Initrd com drivers userspace | Script de build |

### 8. Phase 40c — FUSE-like Filesystem Protocol

| # | Task | Detalhe |
|---|------|---------|
| 1 | `UserFsNode` | Node que traduz VFS ops em mensagens Endpoint |
| 2 | Protocolo `user_fs.h` | Estruturas FsRequest/FsResponse |
| 3 | `libuserfs.so` | Lib userspace para FS userspace |
| 4 | Exemplo: Ext4 userspace | Prova de conceito |
| 5 | Initrd com FS userspace | Root fs via userspace |

### 9. Phase 42 — Architecture Portability Layer

Extrair dependências x86_64 para interfaces genéricas (AArch64/RISC-V no futuro). `arch_cpu_idle()`, `arch_read/write_msr`, `arch_cpuid`, `arch_*` I/O ports, `early_init.h` já existem.

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

### 10. ELF Loader — Itens Restantes

| # | Task | Prioridade |
|---|------|------------|
| 13 | Cache program headers (causou Error 0; investigar) | LOW |
| 16 | Unificar TLS (split entre execve.cpp e init_task.cpp) | LOW |
| 17 | Mais testes ELF | LOW |
| — | Symbol versioning (DT_VERSYM/VERNEED) | LOW |

### 11. IOMMU (VT-d)

`Src/Kernel/Arch/x86_64/Memory/IntelIOMMU/vtd.cpp` — todas as 3 funções retornam `NotImplemented`. Sem tradução DMA real. (~2 semanas para implementação mínima)

### 12. 8 locais com `NotImplemented` no kernel

Recontado 2026-08-04: **8 ocorrências em 4 arquivos** (era 12/7 em 2026-08-03 — mlock + UDP `connect`/`listen` foram implementados). Lista exata sempre re-derivável: `rg "NotImplemented" Src/Kernel`.

| Arquivo | Ocorrências | O quê |
|---------|-------------|-------|
| `Arch/x86_64/Memory/IntelIOMMU/vtd.cpp` | 3 | IOMMU VT-d (tradução DMA) |
| `Driver/Terminal/terminal_manager.cpp` | 2 | Criação de terminais Serial e PTY |
| `Driver/Terminal/terminal_factory.cpp` | 2 | Factory de terminais Serial/PTY |
| `Syscall/syscall.cpp` | 1 | Syscall desconhecida (correto — não é stub) |

Cada um precisa ser implementado ou documentado como "não vai fazer".

### 13. Phase 46 — Compressed Swap (ZRam/ZSwap)

Resumo aqui; **design técnico completo em `.ai-docs/ROADMAP.md`**. Cadeia: swap core → zram driver → reclaim síncrono → zswap (46d deferível). **Sem swap core, zram = disco RAM** (o `CONCEPTS.md:11-13` já previa "compressão como etapa anterior ao swap"). Depende do codec da Phase 47; `NullCodec` permite começar antes.

### 14. Phase 47 — Codec LZFSE (LibFK)

Interface `CompressionCodec` + LZFSE **reimplementado** (não port do C da Apple) em LibFK freestanding, com testes round-trip + golden vectors vs CLI `lzfse`. Troca para LZVN (LZSS, sem entropia) em entradas <4KiB. Paralelo à 46.

### 15. Phase 48 — Traits (LibFK)

`void_t`/`declval` → envolver builtins crus (`vector.h:67`, `circular_buffer.h:78`) → `is_constructible`/`is_convertible` → concepts C++20. Hoje só 2 consumers produtivos de `fk::traits` (`driver_registry.cpp:52-76`).

### 16. Phase 49 — Extração Kernel→LibFK

Time/checksum/id-generator/free-list pequenos primeiro → `slot_map` (CSpace, fd table, posix timers). Continuação do padrão de `notes/fs-to-libfk-extraction.md`.

### 17. Phase 51 — IPC Fastpath (seL4-style) — ⚠️ Parcial

51a (modelo corrigido: `IpcMessage` 6×u64, block+schedule) e 51b (fastpath call/reply via `switch_to_task`) ✅ — ver CHANGELOG. **Restante:**

| Sub-phase | Componente | Status |
|-----------|-----------|--------|
| 51c | **Reply+recv fusion** — `SYS_IPC_REPLY_RECV = 408`; `Endpoint::reply_recv()` (reply to call-sender + receive next in one syscall, fastpath switch to caller); `sys_ipc_reply_recv.cpp` registrado | ✅ Done (2026-08-06) |

**Dependência:** 51a/51b corrigem a base para drivers userspace (Phase 40b) e FUSE userspace (Phase 40c).

### 18. ACPI — Itens Restantes

| Item | Status |
|------|--------|
| Interpretador AML (DSDT/SSDT) | ❌ ausente — sem battery, thermal, sleep/wake |
| FADT power management | ⚠️ parcial — reset register usado; campos ACPI 6.x pendentes |
| Battery/ACPI events (via AML) | ❌ depende do interpretador |

---

## 🔴 Code Quality Debt

### SECRET RULE Violations — One Class Per File + No Nested Types

**Worst offenders:**

| # | File | Types | Esforço |
|---|------|-------|---------|
| 1 | 40+ files com **2 types** | Helper + main class | 0.25 dia cada |
| 2 | 30+ files com **nested types** | Vários drivers/containers | 0.25 dia cada |

Já refatorados: `task.h`, `boot_info.h`, `dynamic_domain.h`, `nvme_controller.h` (4 nested extraídos), `nvme_utilities.h`, `srat.h`, `exfat_bpb.h`, `ahci_controller.h`, `minix_super.h`, `ext2_super.h`, `nvme_command.h` (+ layout NVMe SQE corrigido para 64 bytes); anonymous nested de `task_ipc.h`/`task_lifecycle.h`/`task_memory.h` extraídos para `TaskSignalState`/`TaskAltStack`/`TaskITimer`/`TaskMemoryRegions`.

### ~~Include Order Reversed~~

✅ **315/464 arquivos reordenados** (2026-08-06): script `Meta/reorder_includes.py` aplicado; ordem agora `LibC → LibFK → Kernel → Other`; `interrupt_disabler.h` corrigido para ser auto-suficiente (`#include <LibFK/Types/types.h>`); `xmake` + `xmake check-layers` passando limpo.

### ~~xmake.lua — Particionar por Construct (decidido 2026-08-04)~~

✅ **Particionado em xmake/ (2026-08-06)**: `xmake.lua` (379→40 linhas) agora só tem rules/policies/toolchain + `includes()` para 3 arquivos; `xmake/options.lua` (configuração: initrd_mode); `xmake/targets.lua` (FKernel/LibC_Testing/Test); `xmake/tasks.lua` (check-*/setup-hda/build-initrd/analyze); `xmake` + `xmake run Test` passando limpo.

### ~~Dead Code — `LibFK/Tree/rb_tree.h`~~

✅ Removido (2026-08-06): `Include/LibFK/Tree/rb_tree.h` e diretório `Tree/` deletados — zero consumers confirmados. `map.h`/`set.h`/`multi_map.h`/`multi_set.h`/`deque.h` ainda presentes como `[[deprecated]]`; serão removidos quando HashMap substituir todos os call sites.

### Object Calisthenics Violations

| Regra | Status | Esforço |
|-------|--------|---------|
| No `else` — early returns | 30+ arquivos violam | ~5 dias |
| Max 2 instance vars | Virtualmente toda classe viola | Reforma arquitetural |
| No getters/setters | Generalizado (VGA, VFS Node, APIC, Scheduler) | ~10 dias |
| Files >500 linhas | 5 arquivos (ext2_fs.cpp: **778**, exfat_fs.cpp: 704, VMM: **693**, minix_fs: 518, syscall.cpp: **526**) | ~3 dias |
| No abbreviations | 3 violações (BootInfo, MessageInfo, DmaBuffer) | 0.5 dia |
| ~~Lowercase class names~~ | ✅ `class serial` → `Serial`, `class vga` → `VgaAdapter` (2026-08-06; `Vga` era namespace, por isso `VgaAdapter`) | — |
| ~~Directory naming~~ | ✅ `syscall_list/` → `SyscallList/` (2026-08-06; Lua checker + xmake.lua + comments atualizados) | — |
| ~~File starting with digit~~ | ✅ `8259_pic.h/.cpp` → `i8259_pic.h/.cpp` (2026-08-06; 3 includes atualizados) | — |
| ~~`m_` prefix inconsistente~~ | ✅ `display_framebuffer.h` — 15 campos sem `m_` receberam prefixo (2026-08-06); task.h substructs ainda pendente (~3 dias) | task substructs pendente |

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
- `.ai-docs/AUDITS.md` — auditorias de ELF, IPC, POSIX-readiness, x86_64, memória, exceções, LibC/LibFK, conformidade
- `AGENTS.md` — regras de codificação e estilo
- `Docs/` — documentação de arquitetura
