# 2026-07-27: Source-Code Audit and Documentation Refresh

## Summary

Conducted full source-code audit of ~15 core implementation files across all kernel subsystems (scheduler, memory, VFS, IPC, ELF loader, TCP). Identified and corrected 27+ discrepancies between actual code and documentation across 10 doc files.

## Key Findings

### Documentation vs Reality Gaps

| Doc File | Issues Found | Fixed |
|-----------|-------------|-------|
| Docs/Kernel/Scheduler/README.md | Stopped state claimed unused (code uses it), MLFQ not mentioned, quantum wrong | Yes |
| Docs/Kernel/Memory/README.md | "No demand paging", "no huge pages", "no slab" -- all implemented | Yes |
| Docs/Kernel/Loader/README.md | "Symbol resolution partial" -- full 10 types + cross-object implemented | Yes |
| Docs/Kernel/Process/README.md | Stopped state missing from diagram | Yes |
| Docs/Kernel/VFS/README.md | Only 49 lines; missing mount namespaces, pivot_root, kqueue | Yes |
| Docs/Domains/elf-loader.md | Missing DT_NEEDED, full reloc types, SMAP | Yes |
| Docs/Domains/memory-management-guide.md | Missing Slab, CoW, huge pages, correct buddy orders | Yes |
| Docs/Architecture/system-overview.md | Completion % outdated, IPC description misleading | Yes |
| .ai-docs/architectural-decisions/current-state-analysis.md | Still reported "No COW", "No slab", "SMP=1" (all fixed) | Yes |

### Implementation Confirmations (what the code actually does)

- MLFQ: MLFQ_LEVELS=4 with per-level quanta (2,4,8,16) from s_level_quanta
- QoS: 6 classes mapped to MLFQ levels via default_mlfq_level (qos.cpp)
- CoW fork: clone_table_recursive() with per-zone uint16_t refcount arrays
- SlabAllocator: 8 caches (16B-2048B), tried first in kernel heap allocate()
- Direct map: extend_direct_map() uses PageFlags::HugePage for 2MB pages
- Signals: full SA_SIGINFO, SA_RESTART (rip -= 2), SA_ONSTACK, builtin restorer trampoline
- TCP: checksums via RFC 793 pseudo-header, retransmit with exponential backoff
- ELF: all 10 relocation types with SMAP STAC/CLAC, cross-object symbol scan (65536 entries)
- VFS: mount namespaces, pivot_root, KQueue unified backend
- Scheduler: Stopped state active via SignalDelivery::apply_default()

### Remaining Gaps (confirmed in code)

- IPC fragmentation: POSIX mechanisms use Notification directly, not through CSpace/Endpoint
- ELF: no endianness check (EI_DATA), no file-size bounds on p_offset+p_filesz
- TCP: process_data() only accepts in-order segments
- CSPRNG: init.cpp:105-107 commented out; ASLR uses unseeded PRNG
- Kernel tests: 0% coverage

## Files Changed

- Docs/Kernel/Scheduler/README.md -- full rewrite
- Docs/Kernel/Memory/README.md -- full rewrite
- Docs/Kernel/Loader/README.md -- full rewrite
- Docs/Kernel/Process/README.md -- full rewrite
- Docs/Kernel/VFS/README.md -- full rewrite
- Docs/Domains/elf-loader.md -- full rewrite
- Docs/Domains/memory-management-guide.md -- full rewrite
- Docs/Architecture/system-overview.md -- patch: completion %, IPC description
- .ai-docs/architectural-decisions/current-state-analysis.md -- full rewrite
- .ai-docs/recent-modifications/20260727_source_audit.md -- new file

## Lessons Learned

- Documentation in a rapidly evolving kernel requires periodic source-code audits to stay trustworthy
- TODO.md was mostly accurate but detailed domain docs had rotted significantly
- Schedule scheduler docs said MLFQ was 4 levels but described it as generic priority-based
- Memory docs reported features as missing (demand paging, huge pages) that had been implemented in Phases 27-28
