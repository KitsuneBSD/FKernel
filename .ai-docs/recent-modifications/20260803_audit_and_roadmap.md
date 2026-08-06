# TODO.md Re-audit + New Phases 46–49 (Swap/LZFSE/Traits/Extraction)

## Date
2026-08-03

## Category
Documentation / Roadmap

## Priority
high

## Description
Re-audited `TODO.md` against actual source (3 explore subagents + direct reads + bash verification) and updated the roadmap with four new phases: compressed swap (zram/zswap), LZFSE codec reimplemented in LibFK, traits modernization, and Kernel→LibFK extraction. Alvo declarado do hardware: laptop moderno (sem PS/2, NVMe, >4 GiB RAM).

## Implementation Status
completed

## Changes Made

### TODO.md
- **Quick Status table**: removed stale claims ("47 NotImplemented", "8 suites/89 tests", "task.h viola SECRET RULE"). Added new "Memory Pressure | ❌ Ausente" row (sem swap/page cache/reclaim/OOM killer; slab OOM = halt). Added "sem USB (xHCI/EHCI/HID)" to Drivers and "triple-indirect write ❌" to VFS.
- **Memory audit (2026-08-01 → re-audit 2026-08-03)**: M5/M7/M8/M9/M13 marked ✅ (with evidence line refs); M10 stays ❌ (file-backed paging; docs `memory-management-guide.md:246-248` stale); M6/M11/M12 ⚠️. New BAIXO item promoted to ALTO: identity map 4 GiB × zone HIGH now reachable via M8 fallback (`candidate_zones` steps 4-5).
- **New HIGH section "0. Hardware Real — Laptop Moderno"**: 8 blockers ordered by real-world impact (USB #1, NVMe PRP2, 4GiB identity, swap/reclaim, M10, AHCI, VBE, ext2 triple-indirect).
- **MEDIUM item 14**: "47 NotImplemented" → "12 ocorrências em 7 arquivos" (mlock + UDP connect/listen implemented).
- **MEDIUM items 15–18**: new phase summaries (46 Swap, 47 LZFSE, 48 Traits, 49 Extraction) pointing to ROADMAP.md for full design.
- **Code Quality**: SECRET RULE table now only remaining offenders (task.h/boot_info.h/dynamic_domain.h/nvme_utilities.h refatorados, commit `fdaf30f`); Include Order count corrected to 320/462 (69%); files>500 sizes updated; added "Dead Code — rb_tree.h (0 consumers)" section.
- **LOW**: removed "UDP server" (implemented); added swap-on-disk/zswap and hardware-test-matrix rows.
- **Phase 43 header**: "3 testes / 60K linhas" → "10 suites / 99 testes / ~40K linhas".

### ROADMAP.md (`.ai-docs/ROADMAP.md`)
- **Phase 46 — Compressed Swap**: 46a Swap Core (SwapManager, slot table, swap PTE encoding using bit1 + bits 12–43, SYS_SWAPON=167/SWAPOFF=168, swap_out/in, pf_handler hook BEFORE zero-fill, synchronous reclaim, OOM fallback replacing halt), 46b ZramDevice (BlockDevice interface, inline<4KiB→LZVN, incompressible pages raw), 46c Reclaim síncrono (watermarks), 46d Zswap deferível.
- **Phase 47 — LZFSE Codec**: 47a `CompressionCodec` interface + `NullCodec` (unblocks 46a early), 47b LZVN (LZSS, mandatory <4KiB), 47c LZFSE reimplemented (LZ + static Huffman + LZMA arithmetic coder, incremental byte decoding for streaming). Golden vectors vs CLI `lzfse`.
- **Phase 48 — Traits**: void_t/declval, wrap raw builtins (`vector.h:67`, `circular_buffer.h:78`), is_constructible/is_convertible, C++20 concepts (project is C++20, `xmake.lua:6`).
- **Phase 49 — Extraction**: wins pequenos first (time_math 5×, checksum 3×, id_generator 5 sites) → slot_map (CSpace, fd table, posix timers); rules (LibFK never Kernel, allocator_backend, check-layers after each item).

## Notes for Future Work
- USB (xHCI/EHCI/HID) é o maior gap para laptop moderno — nova Phase 50.
- zswap (46d) exige swap em disco + writeback/page cache que ainda não existem.
- Resíduo do antigo M5: `handle_demand_paging` OR `User` incondicionalmente (`pf_handler.cpp:30`), alcançável via AC path kernel.
- Counts re-verificados por bash: NotImplemented=12/7 arquivos; testes 10 suites/99 testes; kernel .cpp=462; include order kernel-first=320.
