# TODO/Docs Verification + Sync — 2026-08-05

## Date
2026-08-05

## Category
code_review

## Priority
high

## Description
Verified every TODO.md claim against the source tree (sub-agents + greps + direct reads). Corrected 7 stale/inverted claims, re-derived all M/I/R audits, cleaned TODO.md to only open work, and synced Docs/, DocsSummary.md, AGENTS.md and .ai-docs/ to reality.

## Implementation Status
completed

## Changes Made
- File: TODO.md — syscalls 207→206; ext2 triple-indirect marked ✅; I1/R1 confirmed; C1 corrected (fadt fix NOT applied); include order 315/325 (97%); DmaBuffer legacy 21 call sites; removed all completed sections (memory audit ✅ items, exceptions sprint, recovery, Phase 43, Phase 40a, Limites Rígidos, empty scaffolding, one-handler-per-file); MEDIUM renumbered 3–18.
- File: Docs/Architecture/system-overview.md — 207→206 syscalls (2 spots); removed NVMe PRP2 / AHCI async from hardware caveats (implemented).
- File: Docs/Kernel/Syscalls/README.md — 207→206 (3 spots).
- File: Docs/Domains/ipc-capabilities.md — 207→206.
- File: DocsSummary.md — syscall 206 (6 spots), ext2 triple-indirect ✅, NotImplemented 8, test coverage rows (10 kernel suites/99 tests), kfatal/kerror split (5 spots).
- File: .ai-docs/architectural-decisions/current-state-analysis.md — slab 8→10 caches, syscalls ~139→~206, kernel tests 0→10 suites/99.
- File: AGENTS.md — arch_cpu_idle removed from Phase 42 (implemented cpu_ops.cpp:151); logging table kerror "halts" → "returns" (kfatal/kerror split exists).
- File: Docs/Kernel/Logging/README.md, Docs/Domains/logging.md, .ai-docs/development-patterns/kernel-logging.md — kfatal/kerror split reflected.
- File: Include/Kernel/Memory/ObjectMemory/slab_allocator.h — comment "16–2048 bytes" → "16–8192 bytes".
- File: .ai-docs/CHANGELOG.md — new 2026-08-05 entry (verification + corrections).
- File: .ai-docs/AUDITS.md — new "TODO ↔ Source Verification Audit (2026-08-05)" section.

## Lessons Learned
- TODO rows marked ✅ can rot in the wrong direction too: a claim "fix already applied" was false (C1/fadt). Always re-derive from source, don't trust history.
- Syscall count changed 199→207→206 across sessions; the canonical source is `rg "register_syscall" Src/Kernel/Syscall/syscall.cpp`, and the handler-file count (207) ≠ registered count (206) — one support file has zero handlers.
- DocsSummary.md is a concatenated dump of the other docs — stale claims multiply across it; fix canonical docs first, then the dump.
- Slab cache range comment (16–2048) contradicted the actual CACHE_SIZES array (16–8192) for a long time — header comments about sizing limits need periodic re-check against the array.

## Notes for Future Work
- Next concrete fixes per TODO priority: L1 (errno ABI), L2+L3+L11 (memory error paths), C3 (kfatal in operator new OOM), C1 (replace 6 raw asm with arch_* calls).
- Phase 51c (IPC fastpath reply+recv fusion) is the next sprint after stabilization.
