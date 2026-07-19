---
name: project-todo-progress
description: FKernel TODO.md implementation status — which phases are done and what remains
metadata:
  node_type: memory
  type: project
  originSessionId: 1686e31e-efd2-496d-a14d-65ed12b5622a
---

Implementation of TODO.md started 2026-07-18, active through 2026-07-19.

**Why:** User requested full implementation of all 9 phases of the FKernel TODO roadmap.

**How to apply:** When resuming work, check these statuses to avoid re-implementing done items.

## Completed

### Phase 1 — P0 Compilation Blockers ✅
- `list.h`: all bare m_head/m_tail/m_size → m_metadata.*
- `queue.h`: IntrusiveList<T> → List<T>
- `hash_map.h`: tombstone markers + rehash at 50% load
- `optional.h`: value() now calls __builtin_trap() on empty
- `Result.h`: ASSERTs uncommented

### Phase 2 — P1 Critical Bugs ✅
- `memory_manager.cpp`: backward coalescing via doubly-linked BlockHeader; TOCTOU fixed; memset removed
- `scheduler.h`: atomic PID generation via __sync_fetch_and_add
- `SchedulerLifecycle.cpp`: zombie state correct; delete task in reap_zombie; CPU bounds check
- `node.h`: monotonic inode counter via static atomic
- `vfs_operations.cpp`: real timestamps from TickManager
- `signal_delivery.cpp`: userspace address validation + signal frame
- `stack.h`: off-by-one fixed (post-increment)
- `string.cpp`: append null guard
- `unix_socket.cpp`: scheduler blocking in accept(); MemoryManager-allocated buffer
- **endpoint.cpp**: separate recv_wait_node added to Task; m_receivers uses &Task::recv_wait_node
- **pipe_node**: separate m_data_notification + m_space_notification (done in prior session)
- **on_tick()**: sleep queue now runs on ALL CPUs (removed `if (proc.id == 0)` gate)
- **Display::background_flush()**: moved from on_tick() to timer_handler.cpp (decoupled from scheduler)

### Phase 3 — P2 Security ✅
- `cpu.cpp`: SMEP enabled via CPUID detection (CR4 bit 20)
- `cpu.cpp`: SMAP enabled via CPUID detection (CR4 bit 21)
- `ref_counted.h`: atomic ref/unref via __sync_fetch_and_add/__sync_sub_and_fetch
- `invalid_tlb.asm`: mfence after invlpg removed
- **context_switch.asm**: FXSAVE/FXRSTOR added; fx_state[512] added to TaskContext
- **user_access.h/cpp**: copy_from_user/copy_to_user with stac/clac around userspace memcpy
- **execve.cpp**: push_string uses copy_to_user; batch stack writes wrapped with stac/clac

### Phase 4 — P3 Architecture ✅
- `libc_putc.cpp`: Kernel dependency removed; g_heap_ready callback pattern
- `Error.h`: duplicate enum values fixed (InvalidData=100, NotASymlink=101)
- **Error.h**: no longer includes <Kernel/Posix/sys/errno.h>; now includes <LibFK/Core/errno_codes.h>
- **LibFK/Core/errno_codes.h**: new file with all POSIX errno constants (fixes layer violation)
- **Kernel/Posix/sys/errno.h**: now includes <LibFK/Core/errno_codes.h> + `extern int errno`

### Phase 5 — P4 POSIX Foundation ✅
- **LibFK Text**: String: substr, find, rfind, replace, insert, erase, starts_with, ends_with, contains, trim, to_upper, to_lower
- **LibFK Text**: StringView: is_empty, front, back, remove_prefix, remove_suffix, substr, find, rfind, starts_with, ends_with
- **LibFK Containers**: set.h (sorted vector), map.h (sorted vector), priority_queue.h (max-heap), deque.h, unordered_set.h (wraps HashMap)
- **Vector**: insert_at + remove_at added
- **LibC headers**: errno.h, fcntl.h, dirent.h, sys/stat.h created
- **LibC functions**: strdup, strndup, strerror, strstr, strcasecmp, strncasecmp, memchr, strncat, strpbrk, strspn, strcspn, strtol, strtoul, qsort, bsearch, rand, srand
- **LibC/ctype.h**: all isXXX functions as static inline; toupper/tolower
- **LibFK/Algorithms/math.h**: abs, swap, clamp
- **LibFK/Traits/type_traits.h**: remove_pointer, is_pointer, is_floating_point, is_signed, conditional
- POSIX stubs (open, close, stat, opendir etc.) as abort() stubs in posix_stubs.c

### Phase 6 — Core Features ✅
- **VFS**: truncate() + fsync() on Node (virtual) and VirtualFileSystem; O_CREAT support in open(); atomic offset in FileDescription read/write
- **IPC**: CapabilityRights bitmask (Send/Receive/Manage); CSpace O(1) free list; cspace_insert() for transfer
- **ELF**: e_machine == EM_X86_64 validation; e_phnum bounds check (max 256); e_phoff overlap check

### Phase 9 — Code Quality ✅
- **Dead code**: Removed TaskQueueCollection header, duplicate Domains/elf_domain.cpp
- **Type wrappers**: buddy_order.h, frame_index.h, file_offset.h, file_flags.h, thread_id.h, signal_number.h
- **Refactors**: get_page_flags() ScopedLockIRQ; find_task() ScopedLockIRQ on all per-CPU access; resolve_path() extracted helpers (resolve_symlink, skip_slashes, read_component) + added ".." handling
- **RegionSplitter class**: munmap() region splitting extracted to `RegionSplitter` class in own file
- **StringBuilder**: append_hex, append_octal, append_binary, append_float implemented
- **pick_next()**: priority-based selection (highest priority task wins, O(n) scan)
- **Tests**: 55 total tests (29 LibC + 2 circular buffer + 14 containers + 10 smart pointers), all pass

## Remaining

### Phase 7 — TCP/IP Stack (not started)
ARP, IPv4, ICMP, UDP, TCP, AF_INET sockets

### Phase 8 — USB & Advanced Drivers (not started)
xHCI, PTY, Serial-to-VFS, PS/2 Mouse

### Still pending:
- P6: PT_DYNAMIC/PLT/GOT/ASLR in ELF loader; full capability revocation
- P9: test coverage for Kernel critical paths (scheduler, VFS stat/open)
