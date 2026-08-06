# FKernel — Changelog (Completed Work)

> Everything listed here is verified complete in the source tree. For pending work see `TODO.md`. For future roadmap see `ROADMAP.md`. For audit findings see `AUDITS.md`.

---

## Bugfixes + Refactors — L9/L10/M6/M5-residual/PMM/BuddyAllocator + Include order + xmake partition ✅ (2026-08-06)

### L9 — vsscanf: matching failure corrigido ✅
- `Src/LibC/stdio/file.c` → extraído em `Src/LibC/stdio/sscanf.c` (compatível com LibC_Testing).
- `%i` com input sem dígitos nem prefixo: adicionado `const char *num_start`/`after_sign` tracking; agora retorna matching failure (break) em vez de escrever 0 e contar como match.
- `%d`/`%i` com sinal mas sem dígitos: `s` restaurado a `num_start` para backtracking correto.
- `%i` com prefixo "0" ou "0x" mas sem dígitos: valor 0, conta como match (correto).
- xmake.lua: `sscanf=kernel_sscanf`/`vsscanf=kernel_vsscanf` + `add_files sscanf.c` adicionados ao target `LibC_Testing`.
- 2 testes adicionados: `test_sscanf_matching_failure` em `tests/LibC/test_stdio_comprehensive.cpp`.

### L10 — vsnprintf: precision para tipos inteiros ✅
- `Src/LibC/stdio/vsnprintf.c`: `print_num()` recebe parâmetro `int precision` (-1 = nenhum).
- `prec_zeros = max(0, precision - digit_count)`: zeros de precisão inseridos entre prefixo e dígitos.
- Flag `0` (zero_pad) ignorada quando precision ≥ 0 (comportamento C99/C11 correto).
- `%d/%i/%u/%o/%x/%X` passam `precision` para `print_num`; `%p` passa -1.
- 6 sub-casos testados em `test_vsnprintf_precision_integer`: `%.5d`, `%.5d` largo, negativo, `%8.5d`, `%08.5d`, `%.5x`.

### M6 — slab chamado antes de save_and_disable_interrupts ✅
- `Src/Kernel/Memory/memory_manager.cpp`: `save_and_disable_interrupts()` movido para antes da chamada ao slab em `allocate()`, `reallocate()` e `free()`.
- Caminhos de retorno via slab chamam `restore_interrupts(flags)` antes de retornar.
- Elimina janela de IRQ não protegida entre entrada na função e chamada ao slab.

### PMM — get_refcount conflating rc==0 com "não rastreado" ✅
- `Src/Kernel/Memory/PhysicalMemory/physical_memory_manager.cpp`: `get_refcount()` agora retorna 0 para frames rastreados com refcount zero; retorna 1 somente para frames não rastreados (sem array cow_refcounts ou out-of-range).

### BuddyAllocator — dead code removido ✅
- `Include/Kernel/Memory/PhysicalMemory/Buddy/buddy_allocator.h`: declaração de `void initialize()` e construtor de 2 argumentos removidos.
- `Src/Kernel/Memory/PhysicalMemory/Buddy/buddy_allocator.cpp`: implementação de `BuddyAllocator::initialize()` e construtor `(uintptr_t, size_t)` removidos. Única initialização válida é `initialize_from_bitmap()`.

### Resíduo M5 — handle_demand_paging OR User incondicional ✅
- `Src/Kernel/Arch/x86_64/Interrupt/Handler/Exception/pf_handler.cpp`: flags default de demand-paging não incluem mais `PageFlags::User` incondicionalmente; `region.flags | PageFlags::Present` (sem OR User) preserva o User bit apenas se region.flags já o contiver.

### Include Order Reversed — bulk fix ✅
- Script `Meta/reorder_includes.py` criado; aplica ordem canônica `LibC → LibFK → Kernel → Other` a todos os `.cpp` do Kernel.
- 315/464 arquivos reordenados; `xmake` + `xmake check-layers` passando limpo.
- `Include/LibFK/Synchronization/interrupt_disabler.h`: `#include <LibFK/Types/types.h>` adicionado (header self-contained — `uint64_t` estava implicitamente disponível via Kernel transitive antes da reordenação).

### xmake.lua — Particionado por construct ✅
- `xmake.lua` (379→40 linhas): rules/policies/dirs/toolchain + `includes()`.
- `xmake/options.lua`: knobs de configuração (`initrd_mode`).
- `xmake/targets.lua`: targets `FKernel`, `LibC_Testing`, `Test` (com flags/sources).
- `xmake/tasks.lua`: todas as tasks (`check-*`, `setup-hda`, `build-initrd`, `analyze`).

Build: `xmake` limpo, `xmake run Test` → **ALL TEST SUITES PASSED** (19 testes LibC::Stdio incluindo 2 novos), `check-layers` ✅.

---

## Bugfixes — L4/C2 ✅ (2026-08-06)

### L4 — Lock rank tracker agora per-CPU ✅
- `Include/LibFK/Synchronization/lock_rank.h`: `g_current_lock_rank` (single global) → `g_cpu_lock_ranks[FK_MAX_CPUS=64]` (per-CPU array). Adicionado `cpu_lock_slot()` inline que extrai APIC ID via CPUID com guard `#ifdef __fkernel__` (retorna slot 0 em host-side tests e em caminhos pre-APIC). `current_cpu_lock_rank()` e `set_cpu_lock_rank()` agora usam o slot per-CPU.
- `Include/LibFK/Synchronization/spinlock.h`: dois blocos de CPUID inline (`lock()` e `try_lock()`) substituídos por chamada a `cpu_lock_slot()` — elimina duplicação de código.

### C2 — `DmaBuffer` legacy removido; todos os 21 call sites migrados ✅
- Legacy `struct DmaBuffer` (`Memory/Dma/dma_buffer.h`) + funções `dma_alloc_buffer`/`dma_free_buffer` deletados.
- 4 consumers migrados para `fkernel::DmaBuffer` (`Driver/Async/dma_buffer.h`) via API `allocate(size)` / `virtual_address()` / `physical_address().as_uintptr()`:
  - `ahci_controller.cpp` + `interrupt_driven_ahci.cpp` (6 call sites): alocação via `port.X.allocate()` diretamente no membro; `.vaddr` → `.virtual_address()`, `.phys` → `.physical_address().as_uintptr()`.
  - `nvme_controller.cpp` (12 call sites): `TRY(m_admin_queue.sq_buffer.allocate(4096))` etc.; `scan_namespaces` (void) usa `.is_error()` manual; `dma_free_buffer` eliminados (RAII).
  - `dma_strategy.cpp` (2 call sites): `!m_prdt_buffer.is_valid()` guard + `TRY(m_prdt_buffer.allocate(4096))`.
  - `e1000.cpp` (4 call sites + 2 vaddr em send/recv): `m_rx_ring.allocate()`, `.virtual_address()`, `.physical_address().as_uintptr()`.
- Headers atualizados: `ahci_port.h`, `ahci_controller.h`, `nvme_queue_pair.h`, `dma_strategy.h`, `e1000.h`.
- RAII cleanup dos DmaBuffers elimina leak de recursos que existia antes (AHCI destructor não limpava DMA buffers dos ports).

Build: `xmake` limpo, testes **42 suites / PASS**, `check-layers` ✅.

---

## Bugfixes — C3/M11/M12/L5/L8 ✅ (2026-08-06)

### C3 — FAT32 read path: null check ausente no kmalloc ✅
- `Src/Kernel/Fs/Disk/Fat32/fat_32_fs.cpp:237` (`read_from_cluster_chain`): `temp` alocado sem null check e passado direto para `m_device->read()` + `fk::memory::copy()`. Adicionado null check retornando `Error::OutOfMemory`; also added `is_error()` check on the device read to return `Error::IOError` consistently (bônus: fat16 já tinha o check, ext2/ext3/ext4/ufs/iso9660/block_device todos verificados como corretos — único caller sem check era fat32).

### M11 — `get_page_flags` não mascara mais Accessed/Dirty ✅
- `Src/Kernel/Memory/VirtualMemory/virtual_memory_manager.cpp:329-330`: removidos os dois `flags &= ~PageFlags::Accessed/Dirty`. A função agora retorna o raw PTE flags sem mascarar bits de hardware — callers que precisem excluir esses bits podem fazê-lo localmente. Nenhum caller existente inspeciona Accessed/Dirty, então a mudança é backward-compatible.

### M12 — Direct map pula buracos físicos ✅
- `Include/Kernel/Memory/PhysicalMemory/physical_memory_manager.h`: adicionado `for_each_zone(Fn&&)` template para iterar sobre zonas usáveis.
- `Src/Kernel/Memory/VirtualMemory/virtual_memory_manager.cpp` (`extend_direct_map`): adicionado lambda `chunk_has_ram(offset)` que verifica se alguma zona usável se sobrepõe com o chunk de 2 MiB; chunks sem RAM real (buracos de MMIO, ACPI, VGA, etc.) são pulados. Melhora corretude W^X: o direct map agora não mapeia regiões MMIO não-RAM como memória regular.

### L5 — `printf`: buffer global → stack-allocated ✅
- `Src/LibC/stdio/printf.c`: `static char g_printf_buf[2048]` global removido; `vprintf` usa `char buf[2048]` local. Elimina reentrância e corrupção de buffer quando chamado de contexto de IRQ concorrente.

### L8 — `strtoull`/`strtoll`/`strtoul`: overflow silencioso corrigido ✅
- `Src/LibC/stdlib.c`:
  - `strtoul`: adicionado check `result > (ULONG_MAX - digit) / base → errno=ERANGE, return ULONG_MAX`.
  - `strtoll`: reimplementado corretamente (antes delegava em `strtol`, perdendo a faixa >LONG_MAX/LLONG_MAX). Agora usa `unsigned long long` internamente com cutoff para LLONG_MAX/LLONG_MIN, `errno=ERANGE`.
  - `strtoull`: adicionado check `result > (ULLONG_MAX - digit) / base → errno=ERANGE, return ULLONG_MAX`.

Build: `xmake` limpo, testes **42 suites / 482+ PASS**, `check-layers` ✅.

---

## Conformidade AGENTS.md — C1/C4 + doc fixes ✅ (2026-08-06)

### C1 — asm x86 cru → arch_* em 7 arquivos Kernel ✅
- Novas funções em `cpu_ops.h/cpp`: `arch_read_cr0`, `arch_write_cr0`, `arch_wbinvd`, `arch_flush_tlb`, `arch_get_cpu_id`.
- `Init/init.cpp:135`: `asm volatile("cli")` → `arch_disable_interrupts()`.
- `Syscall/syscall.cpp:17-21`: `syscall_tsc_now()` com `rdtsc` inline → `arch_read_tsc()`; include `cpu_ops.h` adicionado no `#ifdef __x86_64__`.
- `Hardware/Firmware/Fadt/fadt_manager.cpp:69`: `asm volatile("" ::: "memory")` → `__sync_synchronize()`.
- `Scheduler/Core/scheduler_manager.cpp:286`: `asm volatile("lea ap_entry(%%rip), %0")` → `reinterpret_cast<uint64_t>(reinterpret_cast<void*>(ap_entry))`; `extern "C" void ap_entry(uint32_t)` declarado em `ap_entry.h`.
- `Hardware/Cpu/mtrr.cpp:12-23,56,77`: helpers estáticos locais `read_cr0/write_cr0/flush_tlb` com asm cru removidos; substituídos por `arch_read_cr0/write_cr0/arch_wbinvd/arch_flush_tlb`. Include `cpu_ops.h` adicionado.
- `Hardware/Cpu/cpu_block.h:25`: `get_current_cpu_id()` com `mov %%gs:32` → chama `arch_get_cpu_id()`. Include `cpu_ops.h` adicionado.
- Bônus: `Arch/x86_64/Interrupt/Handler/Exception/device_not_available.cpp` — removido `extern "C" uint8_t g_use_xsave` (conflito de linkage com `cpu_ops.h`); `fpu_save/fpu_restore` locais com asm cru substituídos por `arch_fpu_save/arch_fpu_restore`.
- `check-arch-asm` agora reporta **4 violações** (todas em LibFK/L7 — decisão arquitetural pendente) vs 10 anteriores.

### C4 — `kerror` → `kwarn` para erros recuperáveis ✅
- `Driver/Device/driver_manager.cpp:21,32`: null pointer de entrada → `kwarn` (função retorna `Error::InvalidParameter`).
- `Driver/Storage/Controllers/Ata/ata_device.cpp:7,19`: null strategy → `kwarn` (retorna `Error::InvalidParameter`).
- `Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt_manager.cpp:73,82,90,99`: sem controller → `kwarn` (retorna graciosamente ou `Error::NoDevice`).
- `Driver/Storage/Controllers/Ata/pio_strategy.cpp:71,103`: timeout DRQ → `kwarn` (retorna `Error::IOError` ao caller).

### Doc fix — AGENTS.md ✅
- `Src/Kernel/Arch/x86_64/Panic/Panic.cpp` → `panic.cpp` (nome real do arquivo).
- Tabela logging e `arch_cpu_idle` já estavam corretos.

Build: `xmake` limpo, testes **42 suites / 482 PASS**, `check-layers`/`check-syscalls` ✅, `check-arch-asm` 4 violações (LibFK/L7 — esperado).

---

## PF handler — kmode CoW recovery sem SMAP ✅ (2026-08-05)

- **Bug**: QEMU reporta `SMAP=0` (CPU default). Sem SMAP, `stac_if_smap()` nunca executa STAC → `RFLAGS.AC` fica 0. O handler kmode do `pf_handler.cpp` exigia `ac_flag` para recuperar CoW/demand-paging em página de usuário → todo kmode PF legítimo (ex.: `sys_read` → `copy_to_user` → `memcpy` num buffer CoW RO pós-fork) matava a task com falso positivo. Exposto pelo sh interativo do MockOS: `echo hi` → fork → SIGCHLD → handler faz `read` em página CoW → PF → kill.
- **Fix** (`Src/Kernel/Arch/x86_64/Interrupt/Handler/Exception/pf_handler.cpp`): a recuperação kmode aceita o PF quando `ac_flag || !CPU::the().has_smap()`. Sem SMAP o kernel toca páginas de usuário sem restrição de hardware, então todo fault supervisor em página de usuário é CoW/demand-paging; AC só é exigido quando SMAP está presente. Include `Kernel/Hardware/Cpu/cpu.h` adicionado.
- **Verificação**: reprodução QEMU (sendkey `echo hi` via monitor socket) — 3 comandos consecutivos: `fork: parent=9, child=10/11/12` → execve echo → exit → SIGCHLD → handler → **sh sobrevive** (antes: `kmode PF CR2=0x40002040 -> kill` na 1ª execução). `xmake` limpo, testes **42 suites / 482 PASS**, `check-layers`/`check-syscalls` ✅.

---

## Auditoria LibC/LibFK — L1/L2/L3/L6/L10(metade)/L11 ✅ (2026-08-05)

### L2 — `Vector` propaga `Result` no OOM (nunca engolir) ✅
- `Include/LibFK/Container/Sequence/vector.h`: mutators `push_back`/`push_range`/`insert_at`/`resize` → `[[nodiscard]] fk::core::Result<void>`; privados `ensure_capacity`/`grow`/`reallocate_to` → `Result<void>`; `reallocate_to` retorna `Error::OutOfMemory` em overflow de tamanho e kmalloc null (antes: return sem crescer + placement-new fora da capacidade → corrupção de heap sob pressão).
- `Include/LibFK/Core/result.h`: nova macro `TRY_OR_FATAL(expr)` (kfatal + `__builtin_unreachable()`) junto do `TRY` — consistente com L11.
- **Sweep de 165 call sites** (`-Werror` + `[[nodiscard]]` tornam ignorar um erro de compilação; coletados com `-Wno-error=unused-result`): `TRY(...)` onde o caller propaga `Result`/`Error`; `TRY_OR_FATAL(...)` em callers `void`/`int`/syscall handlers (que não têm canal de propagação). 6 subagentes paralelos (grupos A–F por subsystem), re-coleta final confirmou **0 sites restantes**.
- Kernel builda limpo (`xmake`), testes **42 suites / 482 tests PASS** (incl. suites relinkadas do L6), `check-layers`/`check-syscalls` ✅. `check-arch-asm`: 10 violações pré-existentes (mtrr/fadt/init/scheduler/syscall/cpu_block/cxxabi/log.h — Phase 42, fora do escopo).


### L1 — errno ABI (contrato musl/BusyBox) ✅
- `Include/LibFK/Core/errno_codes.h` **deletado** (grep: zero referências) — fonte única virou `<LibC/errno.h>`.
- `Include/LibFK/Core/error.h` → `#include <LibC/errno.h>`; `Error::InvalidData 100→1001`, `NotASymlink 101→1000` (anotadas colisões com `ENETDOWN=100`/`ENETUNREACH=101`).
- `Include/Kernel/Posix/sys/errno.h` → inclui `<LibC/errno.h>` (fachada ABI userspace).
- `Include/Kernel/Syscall/syscall_utils.h`: `NotASymlink→22 (EINVAL)`, `InvalidData→22`, comentário `PermissionDenied→EPERM` corrigido.
- **Checker**: `check_layer_separation.lua` ganhou exceção documentada para `Kernel/Posix/sys/errno.h` (fachada ABI, não código de kernel) e agora aplica a tabela de exceções também a headers (antes só `.cpp`).
- **Teste**: `tests/Kernel/test_errno_abi.cpp` (static_asserts Linux: EAGAIN=11, ENOSYS=38, ENOTEMPTY=39, ENAMETOOLONG=36, ELOOP=40, ETIMEDOUT=110, EINVAL=22, ENETUNREACH=101, ENETDOWN=100 + `error_to_errno` runtime) → suite `Kernel::ErrnoABI`.

### L3 — signed overflow no formatting ✅
- `Src/LibC/string/itoa.c`, `Include/LibC/string.h:39` (`itoa_signed`) e `Src/LibC/stdio/vsnprintf.c:106`: magnitude calculada como `0 - (uint64_t)val` (nunca `-val` em int64/int → UB para INT_MIN/INT64_MIN, índice negativo em `digits[]`).
- **Teste**: `test_itoa_int_min` em `test_string_memory_comprehensive.cpp` + `test_format_int64_min` (INT64_MIN/INT32_MIN) no stdio.

### L6 — testes órfãos re-linkados ✅
- `LibC_Testing` passa a compilar `stdio/vsnprintf.c` + `stdio/snprintf.c` (renames `kernel_*` já existiam).
- `tests/LibC/test_stdio_comprehensive.cpp` reescrito para `kernel_snprintf`/`kernel_vsnprintf` (wrapper variádico real), **corrigido bug de teste**: `strncmp("String: test", 13)` comparava até o NUL do literal → agora `"String: test, Char: A", 21`.
- **Deletado** `tests/LibC/test_string_memory.cpp` (redundante com o comprehensive).
- **Relinkados** 6 suites kernel órfãs: `Kernel::Turnstile`, `Kernel::MLFQQueue`, `Kernel::TcpConnection`, `Kernel::PathResolver`, `Kernel::FileDescription`, `Driver::Nvme::Refactoring` (convertido de `main()` para runner). Fontes/stubs adicionados: `turnstile.cpp`, `tcp_connection.cpp`, `path_resolver.cpp`, `file_description.cpp`, `scheduler_stubs.cpp`, `vfs_resolver_stubs.cpp`.
- Total: **41 suites / 450 tests** (kernel: **17 suites / 145 tests**), `xmake run Test` verde.

### L10 (metade) — vsnprintf retorno C11 ✅
- `vsnprintf` agora conta o total mesmo com buffer cheio/null (helpers com `total*`), retornando o comprimento completo (C11 §7.21.6.5/12 — `snprintf(nullptr,0,...)` vira query de tamanho); `%p` não impõe mais width=18. Buffer null/`max==0` é seguro.
- **Restante de L10** (precision `%.5d` em inteiros) permanece em aberto no TODO.md.

### L11 — `operator new` OOM ✅
- `Src/LibFK/Memory/Allocators/new.cpp`: `operator new`/`new[]` com `heap_malloc` null → `kfatal("HEAP", ...)` + `__builtin_unreachable()` (com `-fno-exceptions` não há canal de propagação; simplifica L2).

---

## TODO ↔ source verification + docs sync ✅ (2026-08-05)

Verificação completa do `TODO.md` contra o código real (sub-agentes + greps + reads diretos). 7 claims stale/invertidas corrigidas; todas as auditorias M/I/R re-derivadas do código; docs sincronizadas.

**Claims corrigidas no TODO.md:**
- **Syscalls: 207 → 206 registrados** — verificado: 206 `register_syscall` em `syscall.cpp:264-469`; `syscall_list/` tem 207 arquivos (206 handlers + 1 suporte `Time/posix_timer.cpp` sem handler). `check-syscalls` passa.
- **Ext2 triple-indirect ✅** — `ext2_fs.cpp:262-296` implementa L1→L2→leaf com `ensure_indirect` (TODO dizia o contrário).
- **I1 confirmado** — handler spurious APIC (0xFF) registrado em `interrupt_controller.cpp:69` (no-op sem EOI; não cai no `default_handler`). Resíduo: normalizar `vector−32` no dispatch (check spurious do PIC em `8259_pic.cpp:73-76` continua código morto).
- **R1 confirmado** — `user_range_is_accessible()` em `user_access.cpp:20-35` (Design A).
- **C1 refutado** (TODO anterior dizia "fadt fix aplicado") — `fadt_manager.cpp:69` **ainda tem** `asm volatile("" ::: "memory")` cru; proposta `__sync_synchronize()` NÃO aplicada.
- **Include order: 315/325 (97%)**, não 320/462 (re-derivado via `rg` + `check_layers.lua`).
- **DmaBuffer legacy: 21 call sites** (NVMe 12, AHCI 3, ATA 2, E1000 4) — não removível sem migrar 4 consumers.

**Verificações confirmadas:** M1–M4 corrigidos com testes (`test_buddy_allocator.cpp`, `test_slab_allocator.cpp`); M5/M7–M10/M13 ✅; M6/M11/M12 ⚠️ abertos; I2–I5 ✅; R2–R4 ✅; L1–L11 abertos; C5 + checkers corrigidos; C1–C4 abertos; kernel **10 suites / 99 testes** (xmake.lua:218-227); slab **10 caches (16–8192B)** — header dizia "16–2048", corrigido.

**TODO.md limpo:** removidas todas as seções concluídas (itens ✅ das auditorias M/I/R, sprint de estabilidade, Recuperação de Falhas, Phase 43, Phase 40a, Limites Rígidos, scaffolding vazio, um-handler-por-arquivo). Restam só bugs abertos e trabalho pendente; seções MEDIUM re-numeradas 3–18.

**Docs sincronizadas:**
- `Docs/Architecture/system-overview.md`: 207→206; NVMe PRP2 + AHCI async removidos dos caveats (implementados); VBE placeholder mantido.
- `Docs/Kernel/Syscalls/README.md` (3 pontos) + `Docs/Domains/ipc-capabilities.md`: 207→206.
- `DocsSummary.md`: syscall 206 (6 pontos), ext2 triple-indirect ✅, NotImplemented 8, test coverage (10 suites/99 kernel), logging split.
- `.ai-docs/architectural-decisions/current-state-analysis.md`: slab 8→10 caches, syscalls ~139→~206, kernel tests 0→10 suites/99.
- `AGENTS.md`: `arch_cpu_idle()` removido do Phase 42 (já implementado em `cpu_ops.cpp:151`, usado em `scheduler_manager.cpp:321`); tabela de logging `kerror` "halts" → "returns" (split `kfatal`/`kerror`).
- Docs de logging (`Docs/Kernel/Logging/README.md`, `Docs/Domains/logging.md`, `.ai-docs/development-patterns/kernel-logging.md`): split `kfatal`/`kerror` refletido.
- `Include/Kernel/Memory/ObjectMemory/slab_allocator.h`: comentário "16–2048 bytes" → "16–8192 bytes".

---

## Sprint de estabilidade — completo ✅ (2026-08-04)

Continuação do sprint de corretude + latência de exceções/interrupções.

**I2 — DPL=3 para #DB e #BP:**
- `Include/Kernel/Arch/x86_64/Interrupt/gate_type.h`: `UserTrapGate = 0xEF` (P=1, DPL=3, Type=Trap) adicionado ao enum `GateType`.
- `Src/Kernel/Arch/x86_64/Interrupt/interrupt_controller.cpp`: vetores 1 (#DB) e 3 (#BP) re-setados com `UserTrapGate` após o loop geral. `int3`/`int1` de user space agora entregam SIGTRAP em vez de #GP→SIGILL.

**I5 — static_assert PtRegs↔InterruptFrame:**
- `Src/Kernel/Arch/x86_64/Interrupt/interrupt_dispatch.cpp`: `static_assert` de `sizeof` + `__builtin_offsetof` para `rip`/`rflags`/`rsp` em ambos os structs. Qualquer mudança de layout nos dois structs falha o build imediatamente.

**R2-resíduo — `return` defensivo pós-kill:**
- `pf_handler.cpp`: `return;` adicionado após cada `kill_current_from_exception(SIGSEGV)` em `handle_demand_paging` (OOM) e `handle_write_protection` (CoW break OOM). Código com `phys=0` era dead-code-por-atributo; agora é dead-code-por-estrutura.

**R1 Design A — EFAULT em copy_from_user/copy_to_user:**
- Já implementado: `user_range_is_accessible()` em `user_access.cpp` faz validação por página via `is_address_in_allowed_regions()`. Marcado como ✅ no TODO.

**R4 / Layer 3 — panic_exception() unificado:**
- Já implementado: `panic.cpp:33-59` + macros `GENERIC_EXCEPTION_HANDLER*` em `exception_macros.h`. Marcado como ✅ no TODO.

**Hot path #PF — double O(N) scan eliminado:**
- `pf_handler.cpp`: `resolve_region_flags()` (função separada) fundida com o loop de file-backing em `handle_demand_paging`. Um único passe O(N) agora deriva flags e lida com backing; a função auxiliar foi removida.

**TSC instrumentation (Item 1):**
- `interrupt_dispatch.cpp`: `g_tsc_max_irq[256]` + `irq_tsc_now()` — max cycles per interrupt vector medido em cada `interrupt_dispatch`; dump periódico de 5 s via `tsc_latency_dump()` integrado ao loop de avaliação de IRQ storm.
- `syscall.cpp`: `g_tsc_max_syscall` — max cycles do `SyscallManager::handle()` medido em cada `syscall_dispatcher`; resetado junto com os IRQ maxes no dump de 5 s.

**I4 — sinais no epilogue do syscall:**
- Já implementado: `syscall_dispatcher` chama `handle_pending_signals(task, regs, orig_syscall_num)` antes de retornar para `sysret`. POSIX: sinais entregues antes de voltar ao user. Marcado como ✅.

**Sprint completo:** todos os 10 itens do sprint de estabilidade (corretude + latência) estão fechados. Próximo sprint: Phase 51c (IPC fastpath reply+recv fusion).

---

## IRQ storm fix + interrupt hardening ✅ (2026-08-04)

**Root cause** of 387k page-fault storm on SMP: `VirtualMemoryManager::m_pml4` is a global singleton field. On SMP, whenever any CPU calls `switch_address_space()` (e.g. CPU 1 scheduling its idle task) the shared `m_pml4` field changes globally. CPU 0, while handling a CoW write-protection fault for busybox-init, called `translate(user_vaddr)` which walked the WRONG (idle/kernel) PML4, returned 0, and `handle_write_protection` returned without fixing the mapping → infinite fault retry → 387k faults/second.

**Fixes:**

- `Src/Kernel/Memory/VirtualMemory/virtual_memory_manager.cpp`: Added `cpu_pml4()` static helper that reads the actual CPU CR3 via `arch_read_cr3()`. All per-CPU page table operations now use `cpu_pml4()` instead of the stale `m_pml4` singleton field: `translate`, `get_page_flags`, `map_page`, `protect_page`, `get_pte`, `unmap_page_range`. Kernel-init operations (`initialize`, `extend_direct_map`) keep using `m_pml4` (correct at boot, no user tasks running).

- `Src/Kernel/Arch/x86_64/Interrupt/Handler/Exception/pf_handler.cpp` + `Include/Kernel/Scheduler/Task/task_memory_regions.h`: Per-task page fault rate-limit (R3) — 500 faults per 10 ticks (100ms) triggers `kill_current_from_exception(SIGSEGV)`. Fields `pf_count`/`pf_window_ticks` added to `TaskMemoryRegions`.

- `Src/Kernel/Arch/x86_64/Interrupt/Handler/Routine/apic_spurious_handler.cpp` + `interrupt_controller.cpp` (I1): APIC spurious interrupt (vector 0xFF) now handled by a no-op that does NOT send EOI (Intel SDM §10.9). Prevents kernel halt on any EOI race with PCI/MSI + LAPIC.

- `Src/Kernel/Loader/Domains/elf_loader_core.cpp`: AT_PHDR fallback formula fixed for ET_EXEC without PT_PHDR — now uses `load_base + phdr.p_vaddr + (e_phoff - phdr.p_offset)` matching Linux `binfmt_elf.c`. For busybox: `0 + 0x400000 + (0x40 - 0) = 0x400040` (was `0x40` → musl crash at `__init_tls`).

---

## Documentation sync — hardware/storage/memory gaps ✅ (2026-08-04)

- TODO.md: `NotImplemented` 12→**8 em 4 arquivos** (re-derivado por `rg "NotImplemented" Src/Kernel`); M10 (file-backed) ✅ na Quick Status; nova linha **Hardware/Firmware (ACPI)** (AML ❌); Drivers: USB = headers-only, AHCI/NVMe interrupt-driven; nova seção 20 ACPI.
- `Docs/Domains/drivers-framework.md`: corrigido claim stale "polling-based storage (interrupt-driven removed)" → AHCI/NVMe interrupt-driven async; nova seção **USB Status (Phase 50)**; decomposição NVMe atualizada (NvmeController/NvmeQueuePair/NvmeNamespace/NvmeCommand/NvmeCommandBuilder/NvmeCompletionProcessor).
- `Docs/Domains/memory-management-guide.md`: slab-first heap ≤**2048**B (era 8192); demand paging file-backed via `backing_node->read()` (não "page cache").
- `Docs/Kernel/Hardware/README.md`: Current Status com storage interrupt-driven + USB headers-only + AML ausente.
- `Docs/Kernel/Process/README.md`: thread groups (CLONE_THREAD) parcial — tgid existe, signal routing incompleto (Phase 44).

---

## Status sync + ASLR entropy fix ✅ (session 23)

- `Src/Kernel/Loader/Domains/parser_domain.cpp`: `aslr_random_base()` agora usa `ChaCha20PRNG` (CSPRNG seeded em `init.cpp`) em vez de `TickManager::get_ticks()`. Corrige também bug de entropia: `(seed & 0x0FFFF000)` limitava o range efetivo do ASLR a 1 MiB (~14 bits) em vez de 1.5 GiB. Removido include arch-específico `tick_manager.h` do loader genérico (portabilidade Phase 42).
- Docs sincronizados com a realidade do código (verificado por grep/read em 2026-07-31):
  - TODO.md: syscalls → **207 registrados** (214 definidos na enumeração `SyscallNumber`); Phase 27 (fd→CSpace) DONE; UDP `sendto`/`recvfrom` reais (não stub); LVM/RAID implementados mas órfãos; alguns `.cpp` NVMe documentados como scaffolding.
  - system-overview.md: 199 → 207 syscalls; Phase 27 pending → done; notas honestas sobre NVMe PRP2 / AHCI async / KPTI / IOMMU.
  - ROADMAP.md: Phase 27 marcado como concluído (referência histórica mantida).

---

## Syscall handlers split — one handler per file ✅ (session 22)

- `Src/Kernel/Syscall/syscall_list/`: refactored so each file defines **at most one** `sys_*` handler; file name = handler name minus the `sys_` prefix (shared support files with zero handlers are allowed, e.g. `Time/posix_timer.cpp`). ~50+ per-handler files added across the 11 domain directories.
- `Meta/x86_64-tools/check_one_syscall_per_file.lua` (NEW): enforces the one-handler-per-file rule; wired as `xmake check-syscalls`.
- `Include/Kernel/Syscall/posix_timer.h` (NEW): unified `PosixTimer` struct replacing the scheduler's private `PosixTimerEntry`; single definition in `Src/Kernel/Syscall/syscall_list/Time/posix_timer.cpp`. `scheduler_lifecycle.cpp` now includes `<Kernel/Syscall/posix_timer.h>`.
- `Src/Kernel/Syscall/syscall.cpp`: newly registered `sys_utimes` (SYS_UTIMES=235) and `sys_futimesat` (SYS_FUTIMESAT=261); `sys_newfstatat` registration now uses the `SYS_NEWFSTATAT` constant (=262) instead of the raw number.

---

## Boot crash fix — BuddyState::remove() HHDM guard ✅ (session 21)

- **`Src/Kernel/Memory/PhysicalMemory/Buddy/buddy_state.cpp`**: `BuddyState::remove()` now checks `m_free_lists[idx] == nullptr` before dereferencing `KERNEL_VIRT_BASE + phys`.

Root cause: `alloc_page_internal()` → `buddy.invalidate_page()` → `BuddyState::remove()` is called during VMM initialization (before `extend_direct_map()` maps the HHDM). At that point all buddy lists are `nullptr` (populated only by `reconcile_buddies()` which runs after `extend_direct_map()`). The HHDM access caused a Not Present page fault at `0xffff800001000000`. The null-list guard makes `remove()` return `false` immediately without HHDM access when the buddy is empty, which is always semantically correct — an empty list cannot contain the block.

---

## x86_64 Audit Bugs 21–36 ✅ (session 21)

### 🔴 Critical

- **Bug 21** (`x2apic.h/cpp`, `ap_entry.cpp`): Added `X2APIC::initialize_on_ap()` — sets `IA32_APIC_BASE[10:11]` and enables SVR per SDM §10.12.5.1. `ap_entry` now calls it before any x2APIC MSR access.
- **Bug 22** (`bss.asm`): Expanded per-CPU stack BSS from 64 KiB (4 slots) to 512 KiB (32 slots × 16 KiB). AP≥4 no longer overflows into heap.
- **Bug 23** (`tss_stacks.h`, `gdt.cpp`): IST array reshaped to `[MAX_CPUS][7][IST_STACK_SIZE]`; `fill_tss_impl` now indexes as `ist_stacks[cpu_index][i]`. `set_kernel_stack` reads `get_current_cpu_id()` to update the correct CPU's TSS `rsp0`.
- **Bug 24** (`syscall_stub.asm`): Moved `swapgs` + user-context save + kernel RSP load to **before** the `cmp rax,512` bounds check. `invalid_syscall_handler` now runs entirely on the kernel stack.

### 🟠 High

- **Bug 25** (`ap_entry.cpp`): `CPU::the().initialize_features()` called on every AP before timer init — enables SMEP/SMAP/NX/OSXSAVE/XSAVE on all cores.
- **Bug 26** (`pit.h/cpp`, `tick_manager.cpp`): Added `PITTimer::pit_wait_ms(ms)` that polls PIT channel 2 (no IRQ, no busy-count guess). Pre-scheduler `TickManager::sleep` now delegates to it instead of `loops_per_ms=200000`.
- **Bug 27** (`pit.h/cpp`, `timer_interrupt.cpp`): Added `PITTimer::disable()` — puts channel 0 in one-shot mode with count=0, silencing periodic IRQ0. Called automatically by `TimerManager` when switching away from PIT.

### 🟡 Medium

- **Bug 28** (`syscall_init.cpp`): SFMASK corrected from `0x4700` to `0x47700` — now also clears AC (bit 18), preventing user-controlled SMAP bypass.
- **Bug 29** (`pf_handler.cpp`, `vesa.cpp`): `kerror()` → `kwarn()` in recoverable paths; user-mode PF now calls `terminate_current` without halting the kernel; VESA mode-set failure returns `IOError` without panic.
- **Bug 30** (`x2apic.cpp`): `wait_ipi_delivery` now polls ICR bit 12 (Delivery Status) per SDM §10.6.1 instead of a single `pause`.
- **Bug 31** (`msi_helpers.cpp`): MSI vector pool start raised from `0x40` to `0x60` — leaves 0x20–0x5F for up to 64 IOAPIC GSIs without collision.

### ⚪ Low

- **Bug 32** (`tick_manager.cpp`): `increment_ticks` uses `__sync_add_and_fetch` — now SMP-safe.
- **Bug 33** (`write_on_cr3.asm`): Removed unconditional `cli/sti` around CR3 write — CR3 is atomic; `sti` was breaking callers with IF=0.
- **Bug 34** (`setup_page_tables.asm`): `enable_paging` now sets `EFER.NXE` (bit 11) alongside `EFER.LME` — NX protection active from the first kernel page table.
- **Bug 36** (`syscall_stub.asm`, `syscall_init.cpp`): Removed dead BSS symbols `syscall_user_rsp` / `syscall_kernel_stack`; removed the `extern` reference and sync write from `syscall_init.cpp`.

---

## Phase 43b (partial) — Dentry cache tests ✅ (session 20)

- `tests/Kernel/test_dentry.cpp` (NEW): 9 tests covering `Dentry::create()`, `get_path()`, `lookup(".", "..")`, `add_child()` + cache hit, missing entry returns `NotFound`
- `tests/Kernel/stubs/vfs_stubs.cpp` (NEW): `current_mount_namespace() → nullptr` + linker stubs for `MountNamespace::get_stack/ensure_stack` (unreachable branches in dentry.cpp)
- `tests/test_mock.cpp` (NEW): C++ stubs for `fk::memory::allocate/reallocate/free` that forward to `kmalloc/krealloc/kfree` from `test_mock.c`; enables `fk::make_ref<Dentry>` in host builds
- `Include/LibC/string.h`: moved `strncat` outside the `__STDC_HOSTED__` guard (it has no const-returning C++ overload so cannot conflict)
- `xmake.lua`: added `test_dentry.cpp`, `dentry.cpp`, `dentry_node_stack.cpp`, `node.cpp`, `djb2.cpp`, `vfs_stubs.cpp`, `test_mock.cpp` to Test target

---

## Phase 43e (partial) — Scheduler QoS tests ✅ (session 20)

- `tests/Kernel/test_qos.cpp` (NEW): 14 tests for `qos_level()`, `priority_for_qos()` (including clamping), `allotment_for_qos()`, `quantum_for_level()` (including overflow clamp), `nice_to_priority_offset()`, `qos_from_linux_policy()`, `linux_policy_from_qos()` — all pure computation, no Task/scheduler state needed
- `xmake.lua`: added `test_qos.cpp` and `Src/Kernel/Scheduler/Qos/qos.cpp` to Test target

---

## Phase 39a — Bitmap alloc hint ✅ (session 19)

- `Include/LibFK/Container/bitmap.h`:
  - Added `m_alloc_hint{0}` (word index to start scan from)
  - `alloc()` now two-pass: starts at `m_alloc_hint`, wraps to word 0 if needed — O(1) amortized
  - `set(idx, false)` regresses hint when freeing a word before current hint
  - `clear_all()` resets hint to 0
- `tests/LibFK/test_bitmap_unordered_set.cpp`: 3 new tests — `hint_cross_word`, `hint_regresses_on_free`, `hint_wraparound`

---

## Phase 39f — KQueue O(R) → O(1) ✅ (session 19)

- `Include/Kernel/Fs/Vfs/Events/kqueue.h`:
  - Added `#include <LibFK/Container/hash_map.h>`
  - Added `HashMap<uint64_t, size_t> m_event_index` — keyed by packed (ident, filter) 64-bit composite
  - Added `uint64_t m_nearest_timer_deadline{0}` — cached min EVFILT_TIMER deadline (0 = dirty/none)
  - Added `min_timer_deadline()` private method declaration
- `Src/Kernel/Fs/Vfs/Events/kqueue.cpp`:
  - `event_key(ident, filter)`: packs `(ident & 0x0000FFFFFFFFFFFF) | (uint16_t)filter<<48` into a unique 64-bit key for practical fd/pid/signal idents
  - `process_changelist`: EV_ADD updates existing if (ident,filter) in index; EV_DELETE O(1) via index + index-consistent swap-erase; EV_ENABLE/DISABLE O(1) via index; timer min maintained on every add/remove/enable/disable
  - `scan_ready_events`: EV_ONESHOT removal now updates `m_event_index`; timer delivery sets `m_nearest_timer_deadline = 0` (dirty)
  - `min_timer_deadline()`: O(1) when clean, O(T) rescan on dirty; replaces old static O(R) scan on every wait iteration
  - Static `nearest_timer_deadline` function removed; `kevent()` now calls `min_timer_deadline()`

---

## Phase 40a #1 — IrqBinding: IRQ → Endpoint ✅ (session 19)

- `Include/Kernel/Ipc/Capabilities/capability_type.h`: added `CapabilityType::Irq`
- `Include/LibFK/Syscalls/numbers.h`: added `SYS_BIND_IRQ = 406`, `SYS_UNBIND_IRQ = 407`
- `Include/Kernel/Ipc/Notifications/irq_binding.h` (NEW): `IrqBinding` class — static `Endpoint* s_endpoints[256]` table (BSS-zeroed); `install(vector, ep)` registers ISR and stores endpoint; `remove(vector)` unregisters; `on_irq(vector, frame)` sends EOI then signals endpoint
- `Src/Kernel/Ipc/Notifications/irq_binding.cpp` (NEW): implementation; `install` validates vector ≥ 32, returns `AlreadyExists` if already bound; calls `InterruptController::the().register_interrupt(on_irq, vector)`; `on_irq` calls `HardwareInterruptManager::the().send_eoi(vector)` then `ep->signal(NotificationBits(1))`
- `Src/Kernel/Syscall/syscall_list/Ipc/sys_bind_irq.cpp` (NEW): `sys_bind_irq(vector, ep_handle)` — validates vector [32,255], resolves `CapabilityType::Endpoint` from CSpace, calls `IrqBinding::install()`, installs `CapabilityType::Irq` in caller's CSpace; `sys_unbind_irq(vector)` removes binding
- `Src/Kernel/Syscall/syscall.cpp`: extern declarations + `register_syscall(SYS_BIND_IRQ/SYS_UNBIND_IRQ, ...)`
- Also done this session: `DmaShm` (`Include/Kernel/Ipc/SharedMemory/dma_shm.h` + `Src/Kernel/Ipc/SharedMemory/dma_shm.cpp`) — contiguous physical allocation via `alloc_contiguous(order)`; mapped with `PageFlags::CacheDisabled | Writable | User`; exposes `phys_base()` for DMA address

---

## Phase 39c — CSpace::grant_all_to early-exit ✅ (session 19)

- `Include/Kernel/Ipc/Capabilities/cspace.h`: `grant_all_to()` now uses `size()` countdown — exits when all valid caps found; skips trailing free holes; O(V + holes_before_last_valid) vs prior O(C_total)

---

## Phase 39a — BuddyState::remove() O(L)→O(1) ✅ (session 19)

- `Include/Kernel/Memory/PhysicalMemory/Buddy/free_blocks.h`: added `FreeBlock* prev` — doubly-linked free list
- `Src/Kernel/Memory/PhysicalMemory/Buddy/buddy_state.cpp`:
  - `push()`: sets `prev = nullptr` on new block, updates old head's `prev`
  - `pop()`: clears `next->prev` on new head
  - `remove(idx, phys)`: computes `block = (FreeBlock*)(KERNEL_VIRT_BASE + phys)` directly (no scan), splices via `prev/next` — O(1) vs prior O(L); all 10 coalesce-step removals per `free()` are now O(1)

---

## Phase 39e — TCP Accept Queue O(Q)→O(1) ✅ (session 19)

- `Include/Kernel/Net/Tcp/tcp_socket.h`: replaced single `m_accept_queue` with two vectors: `m_pending` (SynReceived children) and `m_accept_queue` (Established, ready for `accept()`)
- `Src/Kernel/Net/Tcp/tcp_socket.cpp`:
  - `process_handshake`: child pushed to `m_pending` (not accept queue) at SynReceived state
  - `process_ack` (Listen path): scans `m_pending` for matching ACK sequence, transitions child to Established, swap-removes from `m_pending` O(1), pushes to `m_accept_queue`
  - `accept()`: `m_accept_queue` always contains only Established sockets; `pop_back()` is O(1) — no per-call scan, no left-shift

---

## Phase 43c — Memory Tests: BuddyState + Zone ✅ (session 20)

### BuddyState (8 tests) — `tests/Kernel/test_buddy_state.cpp`
- Host-testable via "fake phys" trick: `fake_phys = ptr - KERNEL_VIRT_BASE` wraps unsigned 64-bit so `KERNEL_VIRT_BASE + fake_phys == ptr`; buffer slots serve as simulated physical frames
- Compiled `Src/Kernel/Memory/PhysicalMemory/Buddy/buddy_state.cpp` into Test target
- Tests: `reset_clears_lists`, `push_pop_single`, `push_pop_lifo`, `remove_head`, `remove_tail`, `remove_middle`, `remove_unpushed_false`, `different_orders_independent`

### Zone + classify_zone (12 tests) — `tests/Kernel/test_zone_allocator.cpp`
- Tests `classify_zone()` at DMA/NORMAL/HIGH boundaries; `zone_limit()` for all three types
- Tests `Zone` default (uninitialized → accessors return 0), `populate_zone()`, constructor, frame_count math
- **Bug fixed**: `Zone(base, length, type)` constructor did not set `m_initialized = true` — accessors returned 0 despite valid data. Fixed by adding `m_initialized(true)` to constructor initializer list in `Include/Kernel/Memory/ObjectMemory/Zone/zone_allocator.h`

### List<T> new methods + fix (3 tests added to existing suite)
- `List<T>` (`Include/LibFK/Container/list.h`) gained `insert_before()`, `insert_sorted()`, and double-remove guard in `remove()` (matching `IntrusiveList` semantics)
- `tests/LibFK/test_stack_queue_staticvec.cpp`: 3 new tests — `test_list_insert_before`, `test_list_insert_sorted`, `test_list_double_remove_guard`

---

## Phase 40a #3 — PCI Config Space ioctl ✅ (session 18)

- `Include/Kernel/Hardware/Buses/Pci/pci_node.h`: Added `PIOC_READ_CONFIG = 0x5001`, `PIOC_WRITE_CONFIG = 0x5002` constants; `PiocConfigOp` struct `{bus, dev, fn, width, offset, value}`; `ioctl()` override declaration
- `Src/Kernel/Hardware/Buses/Pci/pci_node.cpp`: `PCIDeviceNode::ioctl()` — copies `PiocConfigOp` from userspace via `fkernel::memory::copy_from_user`, validates width (1/2/4) and offset (0–255), dispatches to `PciManager::read/write_config_{byte,word,dword}`, writes result back for reads; non-PCI requests return `NotImplemented`
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
- `Src/Kernel/Scheduler/Core/scheduler_lifecycle.cpp`:
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
