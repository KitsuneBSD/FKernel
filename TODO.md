# FKernel TODO

> Updated: 2026-07-19 — Implementation session (Phases 1–9 complete)

## Executive Summary

**Status**: FKernel boots successfully (PCI, VFS, drivers, scheduler all functional) but cannot start userspace — the initrd contains no executables, filesystem drivers are broken, and critical syscalls are stubs.

**Progress**: ~45% (boot complete, userspace blocked by multiple layers)
**Immediate Priority**: Write userspace binaries (P0), fix FAT32 drivers (P0), implement syscalls (P1)

---

## Current Metrics

| Component | Files | Bugs Critical | Missing Features |
|-----------|-------|--------------|-----------------|
| **LibFK** | ~65 | ~28 | ~61 |
| **LibC** | ~30 | 9 | ~180 |
| Memory | ~15 | 5 | 3 |
| Scheduler | ~12 | 5 | 5 |
| VFS | ~15 | 4 | 6 |
| Containers | ~12 | 4 | 10+ |
| Drivers | ~53 | 3 | 8+ |
| Networking | ~5 | 1 | 15+ |
| ELF Loader | ~12 | 2 | 6+ |
| IPC | ~8 | 4 | 5+ |
| Syscall | ~83 | 2 | 20+ |
| Boot/Init | 0 | 3 | 3 |
| Userspace | 0 | 3 | 5 |
| Filesystem | ~10 | 5 | 4 |
| **Total** | **~320** | **~78** | **~332** |

### Test Coverage

| Library | Tests | Coverage |
|---------|-------|----------|
| LibC (string/stdio) | ~65 | ~60% |
| LibFK containers | 2 (CircularBuffer only) | ~2% |
| LibFK smart ptrs | 0 | 0% |
| LibFK text | 0 | 0% |
| Kernel | 0 | 0% |
| **Total** | | **~10-15%** |

---

## P0 — Compilation Blockers

These prevent the codebase from compiling. Must be fixed first.

| # | Issue | File | Fix | Status |
|---|-------|------|-----|--------|
| 1 | `List` uses bare `m_head`/`m_tail`/`m_size` instead of `m_metadata.*` | `list.h:89,92-98,119,125,131,142,155,166-175,183,198,215,223` | Prefix all with `m_metadata.` | ✅ Fixed |
| 2 | `Queue` uses `IntrusiveList<T>` but `IntrusiveList` requires two template params | `queue.h:106` | Change to `IntrusiveList<T, &T::m_list_node>` or refactor Queue to use List | ✅ Fixed |
| 3 | `HashMap::remove()` breaks open-addressing probe chains — deleting entries creates gaps that `find_entry_index()` stops at | `hash_map.h:136` | Use tombstone markers or shifting rehash on delete | ✅ Fixed |
| 4 | `HashMap` never grows — hard limit of 8 entries (50% of initial capacity 16) | `hash_map.h:98` | Implement rehash when load factor > 50% | ✅ Fixed |
| 5 | `optional::value() const` returns `nullptr` as `const T&` — undefined behavior | `optional.h:144` | ASSERT/abort instead of returning nullptr | ✅ Fixed |
| 6 | `optional::value()` (non-const) dereferences raw storage without checking `has_value_` | `optional.h:125-132` | Add `has_value_` check with ASSERT | ✅ Fixed |
| 7 | `Result::value()` has ASSERT commented out — accessing value on error is silent UB | `Result.h:39,44,49,87` | Uncomment ASSERTs | ✅ Fixed |

---

## P0 — Boot Blockers (Serial Log Analysis)

These prevent the kernel from reaching userspace. Verified via `logs/serial.log`.

| # | Issue | Source | Fix | Status |
|---|-------|--------|-----|--------|
| 1 | `/sbin/init` not found in VFS — kernel cannot exec PID 1, no userspace | `serial.log:172` | Add `init` binary to initrd TAR | ✅ Fixed |
| 2 | Initrd TAR contains directory structure but only 1 file entry loaded — all directories empty | `serial.log:169` | Populate TAR with actual executables (init, shell, basic utils) | ✅ Fixed |
| 3 | Symlink `bin/sh -> shell` points to non-existent target in initrd | `serial.log:151` | Add `shell` binary or fix symlink target | ✅ Fixed |

### P0 — Missing Userspace Binaries

No native userland source files exist. `Src/Userland/` contains only runtime stubs (`crt0.asm`, `syscalls.asm`). The "minimal" build mode produces an empty initrd.

| # | Component | Source Path | Fix | Status |
|---|-----------|------------|-----|--------|
| 4 | `init` process (PID 1) | `Src/Userland/init/` | Write minimal init that mounts /dev, /proc, spawns shell | ✅ Fixed |
| 5 | `shell` (ash) | `Src/Userland/shell/` | Write minimal POSIX shell or integrate BusyBox ash | ✅ Fixed |
| 6 | Basic utils (ls, cat, uname, clear) | `Src/Userland/` | Write minimal implementations or use BusyBox | ✅ Fixed |
| 7 | `xmake setup-hda` target not defined in xmake.lua | `xmake.lua` | Add rule mapping to `create_hda.lua` | ✅ Fixed |

---

## P1 — Boot Failures (Disk/Storage)

| # | Issue | Source | Fix | Status |
|---|-------|--------|-----|--------|
| 1 | Disk `ad0` (4 GiB) has no partition table — GPT signature missing at LBA 1, MBR missing at LBA 0 | `serial.log:83-89` | Create partition table (GPT or MBR) on disk image | ✅ Fixed |
| 2 | FAT32 auto-mount reads garbage (`Invalid bytes per sector: 0`) — no partition to point to filesystem | `serial.log:97` | Partition disk first, then FAT32 reads from correct LBA | ✅ Fixed |
| 3 | No root filesystem available — disk unpartitioned + initrd empty of binaries | `serial.log:98` | End-to-end: partition disk → create filesystem → populate with userspace | ✅ Fixed |

### P1 — Filesystem Driver Gaps

FAT32 is the primary filesystem but has critical broken functionality preventing directory traversal and file lookup.

| # | Issue | File | Fix | Status |
|---|-------|------|-----|--------|
| 4 | FAT32 `lookup()` on `Fat32FileSystem` is a stub — always returns `NotFound` even at root | `fat_32_fs.cpp:13-26` | Implement directory entry search by name | ✅ Fixed |
| 5 | FAT32 `lookup()` on `Fat32Node` always returns `NotFound` — cannot traverse subdirectories | `fat_32_node.cpp:36` | Implement cluster-chain directory entry search | ✅ Fixed |
| 6 | FAT32 `list_dir()` on `Fat32Node` returns `NotImplemented` — non-root directories cannot be listed | `fat_32_node.cpp:42` | Implement subdirectory listing from cluster chains | ✅ Fixed |
| 7 | FAT32 `write()` returns `NotImplemented` — filesystem is read-only | `fat_32_node.cpp:21` | Implement write support (FAT table + data area) | MEDIUM |
| 8 | FAT12 `read_from_cluster_chain()` returns `NotImplemented` — file content unreadable | `fat_12_fs.cpp:112` | Implement cluster chain traversal and data reading | ✅ Fixed |
| 9 | FAT16 has no node class — no file reading, no subdirectory traversal | `fat_16_fs.cpp` | Create `Fat16Node` class with read/list/lookup | ✅ Fixed |
| 10 | No LFN (Long File Name) support in any FAT driver — files with names > 8.3 invisible | All FAT drivers | Implement LFN entry parsing (VFAT extension) | ✅ Fixed (FAT32 + FAT16) |
| 11 | `AutoMounter::mount_all_partitions()` declared but not implemented | `auto_mounter.h:10` | Implement the function | ✅ Fixed |

---

## P1 — Syscall Stubs

Critical syscalls that are stubbed or hardcoded, breaking userspace expectations.

| # | Issue | File | Fix | Status |
|---|-------|------|-----|--------|
| 1 | `sys_mount` only handles "proc" — returns fake success for all other types without mounting anything | `mount.cpp:59-61` | Implement real mount for FAT32, ext2, tmpfs, devfs | ✅ Fixed |
| 2 | `sys_umount2` returns fake success without unmounting | `umount.cpp` | Implement real unmount logic | ✅ Fixed |
| 3 | `sys_getppid` always returns 1 — hardcoded stub | `getppid.cpp:8` | Return actual parent PID from task structure | ✅ Fixed |
| 4 | `sys_reboot` returns success without doing anything | `reboot.cpp:5` | Implement reboot/poweroff via ACPI | ✅ Fixed |
| 5 | `mmap` only supports anonymous mappings — file-backed mmap returns `NotImplemented` | `mmap.cpp` | Implement file-backed demand paging | ✅ Fixed (eager read-into-mapped-pages) |

---

## P1 — Hardware Gaps

Storage controllers missing partition scanning and filesystem integration.

| # | Issue | File | Fix | Status |
|---|-------|------|-----|--------|
| 1 | AHCI controller does not call `PartitionManager::scan()` — AHCI devices have no partition/filesystem discovery | `ahci_controller.cpp` | Add `PartitionManager::the().scan(dev)` after port detection | ✅ Fixed |
| 2 | NVMe `scan_namespaces()` is a placeholder — no actual namespace scanning or partition detection | `nvme_controller.cpp:297` | Implement NVMe namespace discovery and partition scanning | ✅ Fixed |
| 3 | `Fstab` parser is stubbed — returns hardcoded entries ignoring actual file content | `Fstab.cpp:23` | Parse real fstab format for mount configuration | ✅ Fixed |

---

## P1 — Critical Bugs (Data Corruption, UB, Race Conditions)

### Memory

| # | Issue | File | Fix | Priority |
|---|-------|------|-----|----------|
| 1 | Kernel heap `free()` only merges forward — backward blocks never coalesced → fragmentation accumulates | `memory_manager.cpp:190` | Walk list to find and merge with previous free block | ✅ Fixed |
| 2 | `reallocate()` releases lock between validation and allocate/free — TOCTOU race | `memory_manager.cpp:148` | Hold lock across entire realloc operation | ✅ Fixed |
| 3 | `allocate()` always `memset`s to zero even for non-zero allocations — performance waste | `memory_manager.cpp:111` | Only zero on explicit `calloc` path | ✅ Fixed |

### Scheduler

| # | Issue | File | Fix | Priority |
|---|-------|------|-----|----------|
| 1 | `generate_pid()` is `m_next_pid++` with no atomic/lock — duplicate PIDs on SMP | `scheduler.h:34` | Use `__sync_fetch_and_add` or protect with lock | ✅ Fixed |
| 2 | `zombify_current()` sets state to `Blocked` instead of `Zombie` | `SchedulerLifecycle.cpp:31` | Use `TaskState::Zombie` | ✅ Fixed |
| 3 | `on_tick()` only processes sleep queue on CPU 0 — tasks sleeping on other CPUs never wake | `SchedulerLifecycle.cpp:162` | Process sleep queue on all CPUs or use per-CPU sleep queues | ✅ Fixed |
| 4 | `reap_zombie()` invalidates magic but never `delete`s the Task or frees its stack/FD table — memory leak | `SchedulerLifecycle.cpp:55-63` | Deallocate Task resources before removing from queue | ✅ Fixed |
| 5 | `on_tick()` calls `Display::the().background_flush()` unconditionally — display logic in scheduler path | `SchedulerLifecycle.cpp:163` | Move to timer interrupt handler or separate thread | ✅ Fixed |

### VFS

| # | Issue | File | Fix | Priority |
|---|-------|------|-----|----------|
| 1 | `stat()` uses pointer value as `st_ino` — non-deterministic, breaks userspace | `virtual_filesystem.cpp` | Assign monotonic inode numbers | ✅ Fixed |
| 2 | `stat()` hardcodes all timestamps to magic constant `1000000` | `virtual_filesystem.cpp` | Return real time from clock subsystem | ✅ Fixed |
| 3 | `FileDescription::read/write` offset update not atomic — race on concurrent access | `file_description.cpp` | Use atomic offset or lock | ✅ Fixed |
| 4 | `open()` does not handle `O_CREAT` flag | `vfs_operations.cpp` | Implement file creation path | ✅ Fixed |

### IPC

| # | Issue | File | Fix | Priority |
|---|-------|------|-----|----------|
| 1 | Signal handler sets `regs->rip = sa_handler` without pushing return address — handler cannot return (no sigreturn trampoline) | `signal_delivery.cpp:80-85` | Push signal trampoline frame on user stack with `sigreturn` address | ✅ Partial (sa_restorer support) |
| 2 | Signal handler address not validated against userspace range — kernel pointer injection | `signal_delivery.cpp:85` | Validate `sa_handler` is in userspace before redirect | ✅ Fixed |
| 3 | `PipeNode` uses same `m_notification` for DATA_AVAILABLE and SPACE_AVAILABLE — reader can wake on space event and vice versa | `pipe_node.cpp:34,47,60` | Use separate notification objects or distinguish bit masks in `wait()` | ✅ Fixed |
| 4 | `Endpoint` senders/receivers both use `Task::wait_node` — task cannot send on one endpoint while receiving on another without corrupting the node | `endpoint.cpp` | Use separate wait nodes for send and receive queues | ✅ Fixed |

### Containers (LibFK)

| # | Issue | File | Fix | Priority |
|---|-------|------|-----|----------|
| 1 | `Stack::push()` uses `[++top_index]` — first element at index 1, slot 0 wasted (off-by-one) | `stack.h:27` | Change to `m_stack[top_index++]` | ✅ Fixed |
| 2 | `String::operator+` ignores errors from `reserve()` — silent data corruption on allocation failure | `string.cpp` | ✅ Fixed (returns empty String on reserve failure) | HIGH |
| # | `String::append(const String&)` dereferences `str.m_data.ptr()` without null check — crashes on moved-from string | `string.cpp:154` | Add null/validity check before dereferencing | ✅ Fixed |
| 4 | `CircularBuffer::clear()` does not call destructors on non-trivial types | `circular_buffer.h` | ✅ Fixed (uses `__is_trivially_destructible` to call ~T() + reconstruct active elements) | MEDIUM |

### Networking

| # | Issue | File | Fix | Priority |
|---|-------|------|-----|----------|
| 1 | `UnixSocket::accept()` is a busy-spin loop (`while(true) { pause; }`) — burns CPU indefinitely | `unix_socket.cpp:72-84` | Block the accepting task via scheduler until a client connects | ✅ Fixed |
| 2 | `UnixSocket` buffer uses identity-mapped physical address — breaks if identity mapping removed | `unix_socket.cpp:17-19` | Use proper virtual allocation via MemoryManager | ✅ Fixed |

---

## P2 — Security Issues

### x86_64 Architecture

| # | Issue | File | Fix | Priority |
|---|-------|------|-----|----------|
| 1 | NX bit not configured in page tables — all pages executable | `setup_page_tables.asm` | Add NX flag to non-executable pages | ✅ Fixed (ELF loader + user stack) |
| 2 | SMEP (Supervisor Mode Execution Prevention) disabled | Boot init | Enable CR4.SMEP | ✅ Fixed |
| 3 | SMAP (Supervisor Mode Access Prevention) disabled | Boot init | Enable CR4.SMAP | ✅ Fixed |
| 4 | No SSE/AVX context save during context switch | `context_switch.asm` | Implement FXSAVE/FXRSTOR | ✅ Fixed |
| 5 | MFENCE after INVLPG incorrect — should be SFENCE or removed | `invalid_tlb.asm` | ✅ No fence present (INVLPG is already serializing; "memory" clobber in C++ inline asm) | MEDIUM |

### Memory Safety

| # | Issue | File | Fix | Priority |
|---|-------|------|-----|----------|
| 1 | `RefCounted::ref()/unref()` are not atomic — unsafe in preemptive/SMP context | `ref_counted.h` | Use `__sync_fetch_and_add`/`__sync_sub_and_fetch` | ✅ Fixed |
| 2 | `RefPtr` reference count not thread-safe — shared objects across interrupt contexts corrupt count | `ref_ptr.h` | Make refcount atomic or document single-thread constraint | ✅ Fixed |

---

## P3 — Architecture Violations

| # | Issue | File | Fix | Priority |
|---|-------|------|-----|----------|
| 1 | `Error.h` (LibFK) includes `<Kernel/Posix/sys/errno.h>` — LibFK depends on Kernel | `Error.h:3` | Move errno definitions to LibFK or create shared header | ✅ Fixed |
| 2 | `libc_putc.cpp` (LibC) includes `<Kernel/Memory/memory_manager.h>` — LibC depends on Kernel | `libc_putc.cpp` | Remove kernel dependency; use callback/hook pattern | ✅ Fixed |
| 3 | `SchedulerManager::on_tick()` calls `Display::background_flush()` — scheduler depends on display driver | `SchedulerLifecycle.cpp:163` | Decouple via event/callback | ✅ Fixed |
| 4 | `Error` enum has duplicate integer values (InvalidParameter=22, InvalidData=22, NotASymlink=22) — error discrimination impossible | `Error.h:15,21,26` | Assign unique values | ✅ Fixed |

---

## P4 — Missing Features by Subsystem

### LibFK

#### Containers

| Container | Priority |
|-----------|----------|
| `deque<T>` | ✅ Implemented |
| `set<T>` | ✅ Implemented |
| `map<K,V>` | ✅ Implemented |
| `multiset<T>` | ✅ Implemented (`Include/LibFK/Container/multi_set.h`) |
| `multimap<K,V>` | ✅ Implemented (`Include/LibFK/Container/multi_map.h`) |
| `priority_queue<T>` | ✅ Implemented |
| `unordered_set<T>` | ✅ Implemented |
| `forward_list<T>` | ✅ Implemented (`Include/LibFK/Container/forward_list.h`) |

#### Text

```
String:     ✅ substr(), ✅ find(), ✅ rfind(), replace(), insert(), erase(),
            ✅ starts_with(), ✅ ends_with(), ✅ contains(), ✅ trim(), ✅ to_upper(), ✅ to_lower()

StringView: ✅ substr(), find(), rfind(), ✅ remove_prefix(), ✅ remove_suffix(),
            ✅ front(), ✅ back(), ✅ starts_with(), ✅ ends_with()

StringBuilder: ✅ append_hex(), ✅ append_binary(), ✅ append_octal(), ✅ append_float()
```

#### Core

| Feature | Priority |
|---------|----------|
| `ASSERT()` macro definition | ✅ Implemented (Assertions.h) |
| Type traits: `enable_if`, `remove_pointer`, `is_pointer`, `is_floating_point`, `is_signed`, `conditional` | ✅ Implemented (type_traits.h) |
| Math: `abs()`, `swap()` | ✅ Implemented (math.h) |
| `Tuple`: default ctor, move semantics, `tuple_size` | ✅ Implemented (`Include/LibFK/Utilities/tuple.h`, `get<N>`, `make_tuple`, 8 tests) |
| `Pair`: move assignment | ✅ Fixed (added move ctor + move assign + rvalue-ref ctor) |

### LibC

#### Headers

| Header | Priority | Description |
|--------|----------|-------------|
| `errno.h` | ✅ Implemented | Error definitions |
| `fcntl.h` | ✅ Implemented | Flags O_* |
| `sys/stat.h` | ✅ Implemented | stat structures |
| `dirent.h` | ✅ Implemented | Directory entries |
| `ctype.h` | ✅ Implemented | Classification functions |
| `float.h` | ✅ Implemented | IEEE 754 limits |
| `wchar.h` | ✅ Implemented | Wide character support |
| `signal.h` | ✅ Implemented | Signal handling |
| `time.h` | ✅ Implemented | Time functions |
| `termios.h` | ✅ Fixed | Terminal control header + stubs |
| `pthread.h` | ✅ Fixed | Single-threaded stubs for POSIX thread API |

#### Functions — ctype.h: ✅ All major functions implemented

#### Functions — string.h

```c
// ✅ Implemented: strdup(), strndup(), strerror(), strstr(), strcasecmp(),
//                 strncasecmp(), memchr(), strncat(), strtok()
// Missing: strcoll(), strxfrm(), memccpy(), ffs(), stpcpy(), stpncpy()
```

#### Functions Missing — stdio.h

```c
// ✅ Implemented: snprintf(), vsnprintf(), kprintf(), libc_puts()
// ✅ Implemented: printf(), fprintf(), vprintf(), vfprintf()
// ✅ Implemented: putchar(), puts(), fputs(), fputc()
// ✅ Implemented: FILE (type), stdin, stdout, stderr, feof(), ferror(), clearerr(), fflush()
// ✅ Implemented: sprintf(), vsprintf(), sscanf() (stub)
// Stubs (abort): fopen(), fclose(), fread(), fwrite(), fgets(), fseek(), ftell()
// Missing: getchar(), scanf(), rewind()
```

#### Functions Missing — stdlib.h

```c
// ✅ Implemented: atoi(), strtol(), strtoul(), abort(), exit(), atexit()
// ✅ Implemented: calloc(), realloc(), qsort(), bsearch()

// Missing:
// ✅ strtoll(), strtoull() — implemented
// ✅ strtod(), strtof() — declared, guarded for non-freestanding only
// ✅ abs(), labs(), llabs() — inline in stdlib.h
// ✅ div(), ldiv(), lldiv() — inline in stdlib.h
// ✅ getenv(), putenv(), setenv(), unsetenv() — stub (no env in kernel)
// Missing: strtold(), strtoimax(), strtouimax(), mblen(), mbtowc(), wctomb(), mbstowcs(), wcstombs(), system(), mkstemp(), realpath()
```

#### Constants

```c
// ✅ Access modes: F_OK, R_OK, W_OK, X_OK (limits.h)
// ✅ Exit codes: EXIT_SUCCESS, EXIT_FAILURE (stdlib.h)
// ✅ Limits: CHAR_MIN, CHAR_MAX, LONG_MIN, LONG_MAX, LLONG_MIN, LLONG_MAX (limits.h)
// ✅ Path limits: NAME_MAX, PATH_MAX, PIPE_BUF (limits.h)
// ✅ S_ISBLK(), S_ISFIFO(), S_ISSOCK() in sys/stat.h
```

### Kernel — Memory

| # | Issue | File | Fix | Priority |
|---|-------|------|-----|----------|
| 1 | `get_pte()` ignores `create` parameter | `virtual_memory_manager.cpp:271` | Respect the flag | ✅ Fixed |
| 2 | No page fault handler for mmap regions | `pf_handler.cpp` | ✅ Fixed (demand paging maps new pages for allowed mmap regions; SIGSEGV on invalid access) | HIGH |
| 3 | IOMMU is a stub | `vtd.cpp` | Implement or remove | MEDIUM |

### Kernel — Scheduler

| # | Issue | File | Fix | Priority |
|---|-------|------|-----|----------|
| 1 | `sleep()` uses busy-wait | `tick_manager.cpp:6` | Use scheduler blocking (already in `sleep_current`) | ✅ Fixed |
| 2 | Task priority not used in scheduling — round-robin only | `SchedulerManager.cpp:40` | Implement priority queue | ✅ Fixed |
| 3 | No load balancing across CPUs — tasks pinned to first affinity bit | `SchedulerLifecycle.cpp:78-84` | ✅ Implemented (work stealing in pick_next + least-loaded-CPU in add_task) | MEDIUM |

### Kernel — VFS

| # | Issue | File | Fix | Priority |
|---|-------|------|-----|----------|
| 1 | Lock ordering in `rename()` — potential deadlock | `vfs_operations.cpp:122` | ✅ Fixed (single m_lock held throughout, no secondary dentry locks) | HIGH |
| 2 | Missing `truncate()`/`fsync()` syscalls | VFS layer | Implement | ✅ Done |
| 3 | `dentry::lookup()` releases lock during iteration — TOCTOU | `dentry.cpp` | Hold lock or use atomic check-then-insert | ✅ Fixed |

### Kernel — Networking

| Status | Component |
|--------|-----------|
| ✅ Implemented | E1000 Ethernet driver (MMIO, RX/TX rings, MAC) |
| ✅ Implemented | Unix domain sockets (AF_UNIX) |
| ✅ Implemented | TCP/IP stack (IPv4, TCP, UDP, ARP, ICMP) |
| ✅ Implemented | Socket extensions (AF_INET) |
| ✅ Implemented | TCP sliding window (peer window tracking, recv window advertisement, MSS segmentation) |
| ✅ Routing table | `Src/Kernel/Net/Routing/routing_table.cpp` — default GW + subnet routes, wired into `send_ipv4` |
| ❌ Missing | DNS resolver, DHCP client |
| ✅ Implemented | UDP socket demux (port-based dispatch to registered UdpSocket) |
| ✅ Implemented | TCP socket demux (port-based dispatch to registered TcpSocket, bind() parses sockaddr_in) |

### Kernel — ELF Loader

| Feature | Status | Priority |
|---------|--------|----------|
| PT_DYNAMIC processing | ✅ Implemented (`DynamicDomain` parses DT_RELA/DT_JMPREL/DT_SYMTAB) | HIGH |
| Symbol resolution (PLT/GOT) | ✅ Implemented (R_X86_64_RELATIVE, R_X86_64_64, GLOB_DAT, JUMP_SLOT) | HIGH |
| ASLR | ✅ Implemented (ET_DYN gets randomized base via TickManager hash) | HIGH |
| TLS (PT_TLS) | ✅ Implemented (variant II: TLS block at 0x7FFFFE000000, self-ref thread pointer, FS_BASE set) | MEDIUM |
| RELRO (Partial/Full) | ✅ Implemented (`apply_relro()` in ElfLoaderCore — `protect_page()` removes Writable after relocations) | MEDIUM |

**Security validations:**
- ✅ `e_machine == EM_X86_64` check in parser
- ✅ Bounds checking on `e_phoff`/`e_phnum`
- ✅ `PT_GNU_STACK` enforcement (NX stack by default)
- Interpreter path validation

### Kernel — IPC

| # | Issue | Fix | Priority |
|---|-------|-----|----------|
| 1 | No capability rights — all caps are raw pointers with full access | Add rights bitmask `{send, receive, manage}` | ✅ Done |
| 2 | CSpace O(n) lookup | Free list for O(1) allocation | ✅ Done |
| 3 | Capability transfer (`cspace_insert`) not implemented | Implement | ✅ Done |
| 4 | Capability revocation not implemented | ✅ Implemented (generation counter in Endpoint/Notification; SYS_CAP_REVOKE=403) | HIGH |
| 5 | Large message via SHM not implemented | Implement | MEDIUM |
| 6 | `sigreturn` incomplete — no trampoline frame setup | ✅ Implemented (KernelSignalFrame pushes full PtRegs; sys_sigreturn restores from frame) | HIGH |

### Kernel — ACPI/Hardware

| Component | Status | Action |
|-----------|--------|--------|
| FADT | Partial | Complete ACPI 6.x fields |
| DSDT/SSDT | Not implemented | Implement AML interpreter |
| HPET | Header only | Complete timer, integrate with TimerManager |
| DMAR | Not implemented | IOMMU/VT-d setup |
| SRAT | Partial | NUMA affinity integration |
| MCFG | Partial | Complete PCIe config |
| PCI BAR read/write | ✅ Implemented (`bar_base()`, `bar_size()`, `bar_is_io()`, `bar_is_64bit()` in PciDevice) | Done |
| MSI-X | ✅ Implemented (`allocate_msix_vector` in APIC — maps BAR table, writes entry) | Complete |
| IOAPIC address | ✅ Fixed (reads from ACPI MADT type-1 entry) | Done |
| MSI dest | ✅ Fixed (reads LAPIC base from IA32_APIC_BASE MSR 0x1B) | Done |
| SMP | Single-core only | Per-CPU GDT/TSS, AP startup |

### Kernel — Drivers

| Driver | Status | Priority |
|--------|--------|----------|
| PS/2 Mouse | ✅ Implemented | MEDIUM |
| Serial Terminal | ✅ Integrated to VFS (`/dev/ttyS0`, read via polling DR bit, write via UART) | MEDIUM |
| Pseudo-Terminal (PTY) | ✅ Implemented (`PtyMaster`/`PtySlave`/`PtyBuffer`, `SYS_OPENPTY=503`) | MEDIUM |
| virtio-net | Not implemented | LOW |
| VESA framebuffer | Basic | MEDIUM |
| USB/xHCI | Headers only | HIGH |

---

## P6 — Kernel → LibFK Migration

### A. Standalone Utility Files

3 files in `Include/Kernel/` are pure standalone utilities with zero kernel-specific logic. They belong in LibFK as domain-agnostic building blocks.

| Kernel File | LibFK Destination | Content | Status |
|-------------|------------------|---------|--------|
| `Net/byte_order.h` | `LibFK/Algorithms/byte_order.h` | `htons`, `htonl`, `ntohs`, `ntohl` — pure inline byte-swap functions | ❌ Pending |
| `Arch/x86_64/io.h` | `LibFK/Arch/x86_64/io.h` | `outb`, `inb`, `inl`, `outw`, `insw`, `invlpg` — pure inline asm I/O port functions | ❌ Pending |
| `Syscall/syscall_numbers.h` | `LibFK/Syscalls/numbers.h` | `enum SyscallNumber` — numeric constants only | ❌ Pending |

Note: Most other "pure data" kernel files (ELF types, network protocol headers, ACPI structs, partition formats, POSIX types, value types) are domain-specific format definitions that belong in their respective kernel subsystems, not in LibFK.

### B. Algorithm Consolidation

Generic algorithms duplicated across kernel subsystems should be consolidated into `LibFK/Algorithms/` per the [Algorithm Consolidation Policy](/.ai-docs/development-patterns/algorithm-consolidation.md). These are not file moves — they are new algorithm implementations that replace copy-pasted loop logic.

#### Tier 1 — Exact Duplicates (same code in 2-3 files)

| Algorithm | Duplicated In | LibFK Destination | Status |
|-----------|--------------|-------------------|--------|
| Case-insensitive string compare | `fat_12_fs.cpp`, `fat_16_fs.cpp`, `fat_32_fs.cpp` — identical function 3x | `LibFK/Algorithms/string_algorithms.h` | ❌ Pending |
| Internet checksum (RFC 1071) | `icmp_packet.cpp` (full impl), `ipv4_header.h` (inline `ip_checksum()`) | `LibFK/Algorithms/internet_checksum.h` | ❌ Pending |
| Queue dequeue-N (shift-left from front) | `tcp_socket.cpp:47-49`, `udp_socket.cpp:45-47` — identical shift loop | `LibFK/Algorithms/range_algorithms.h` | ❌ Pending |
| 8.3 FAT name formatting (trim + concat) | `fat_12_fs.cpp`, `fat_16_fs.cpp`, `fat_32_fs.cpp` — identical trim+dot logic | `LibFK/Algorithms/fat_name.h` | ❌ Pending |
| Dedup-on-insert by string key | `vfs_directory.cpp:83-88`, `ram_disk.cpp:229-235,275-281` | `LibFK/Algorithms/container_algorithms.h` | ❌ Pending |
| Byte-at-a-time range append | `tcp_socket.cpp:129`, `udp_socket.cpp:69`, `tmp_fs.cpp:24-25` — should use container `append()`/`copy()` | Replace with container method | ❌ Pending |

#### Tier 2 — Generic Patterns (4+ call sites)

| Algorithm | Call Sites | LibFK Destination | Status |
|-----------|-----------|-------------------|--------|
| Linear find-if by key | ArpTable, DevFs, TmpFs, PCI, KQueue, Dentry — 6+ implementations | `LibFK/Algorithms/container_algorithms.h` | ❌ Pending |
| Linear find-and-remove (swap-with-last) | ArpTable, KQueue, DevFs | `LibFK/Algorithms/container_algorithms.h` | ❌ Pending |
| Gather copy from iovec | `writev.cpp` — sum lengths + memcpy segments | `LibFK/Algorithms/gather.h` | ❌ Pending |
| Byte-sum checksum validation | `acpi.cpp:70-77` — ACPI table validation | `LibFK/Algorithms/byte_checksum.h` | ❌ Pending |
| lower_bound / upper_bound (binary search) | Set, Map, MultiSet, MultiMap — **5 identical copies** | `LibFK/Algorithms/binary_search.h` | ❌ Pending |

#### Tier 3 — LibFK Internal Deduplication

| Algorithm | Duplicated In | Fix | Status |
|-----------|--------------|-----|--------|
| DJB2 hash | `hash_map.h:58-66` reimplements what exists in `djb2.cpp` | Call `fk::algorithms::djb2()` instead | ❌ Pending |
| Base-N integer formatting | `string_builder.cpp` — `append_decimal/hex/octal/binary` share identical skeleton | Single `append_unsigned(value, base)` function | ❌ Pending |

---

## P5 — Code Quality

### Object Calisthenics Violations

| Class | Issue | Limit | Status |
|-------|-------|-------|--------|
| `Dentry` | 5 → 4 vars (extracted `DentryNodeStack`) | max 2 | Improved |
| `MemoryManager` | 4 vars (was 7, already fixed) | max 2 | Borderline |
| `UnixSocket` | 11 → 7 vars (extracted `UnixSocketBuffer`) | max 2 | Improved |
| `PhysicalZone` | 5 vars (data struct) | max 2 | Low priority |
| `BuddyState` | 3 instance vars | borderline | Low priority |

### Type Wrappers — ✅ All Implemented

```cpp
// Include/LibFK/Types/ — all created:
// ✅ physicalAddress.h  ✅ virtualAddress.h  ✅ buddy_order.h  ✅ frame_index.h
// ✅ file_offset.h      ✅ file_flags.h      ✅ processId.h    ✅ thread_id.h
// ✅ signal_number.h
```

### Specific Refactors

| Location | Issue | Fix |
|----------|-------|-----|
| `munmap()` | 5 nesting levels | ✅ Extract `RegionSplitter` class |
| `resolve_path()` | ✅ Already refactored (max 3 levels, helper functions extracted) | Code quality |
| `pick_next()` | Priority ignored | ✅ Implement priority queue |
| `select_zone()` | ELSE chains | ✅ Already uses early returns |
| `get_page_flags()` | No lock | ✅ Uses ScopedLockIRQ |
| `find_task()` | No lock | ✅ Uses ScopedLockIRQ |

### Dead Code — ✅ Already Removed

| File | Status |
|------|--------|
| `TaskQueueCollection` | ✅ Removed |
| `InterruptDrivenNvmeController` | ✅ Removed |
| `InterruptDrivenAhciController` | ✅ Removed |
| `Src/Kernel/Loader/Domains/elf_domain.cpp` | Only `Base/elf_domain.cpp` remains |

---

## Execution Roadmap

### Phase 1 — Compilation Blockers ✅ Complete

### Phase 2 — Critical Bugs ✅ Mostly Complete
Memory, scheduler, VFS, IPC, containers all fixed. Remaining: IOMMU stub, SMP load balancing.

### Phase 3 — Security ✅ Complete
SMEP/SMAP/NX enabled, signal handler validated, refcounts atomic, NX stack enforcement.

### Phase 4 — Architecture Cleanup ✅ Complete
Layer violations fixed, Error enum deduplicated.

### Phase 5 — POSIX Foundation ✅ Complete
errno.h, fcntl.h, ctype.h, dirent.h all complete. stdio (printf/FILE), stdlib (exit/calloc/qsort), string functions all implemented.

### Phase 6 — Core Features ✅ Complete
ASLR (ET_DYN random base), IPC capability rights + revocation (SYS_CAP_REVOKE), VFS truncate/fsync. PT_DYNAMIC/PLT/GOT remain unimplemented.

### Phase 7 — Networking ✅ Mostly Complete
TCP/IP stack, AF_INET sockets, UDP demux, TCP demux, ARP, ICMP, routing table (default GW + subnet routes). Remaining: sliding window, DNS/DHCP.

### Phase 8 — USB & Advanced Drivers (Long-term) — Partial
- ✅ PS/2 Mouse driver (IRQ12, 3-byte packets, /dev/mouse)
- ❌ xHCI host controller (headers only)
- ✅ Pseudo-Terminal (PTY) — PtyMaster/PtySlave/PtyBuffer, SYS_OPENPTY=503
- ❌ USB HID

### Phase 9 — Code Quality (Continuous)
Object Calisthenics refactoring, type wrappers, dead code removal, test coverage.

### Phase 11 — Kernel → LibFK Migration
Move 3 standalone utility files + consolidate ~15 duplicated algorithms from kernel into `LibFK/Algorithms/`. Tier 1 (exact duplicates) first, then Tier 2 (generic patterns). See P6.

### Phase 10 — Userspace Bootstrap (Current Priority)
- ✅ Write minimal init process (PID 1) that mounts /dev, /proc, spawns shell
- ✅ Write minimal shell (interactive, builtins: cd/pwd/exit, fork+exec)
- ✅ Write basic utils (ls, cat, uname, clear)
- ✅ Fix FAT32 drivers (lookup, list_dir, subdirectory traversal, LFN support)
- ✅ Implement sys_mount (tmpfs + device-backed), sys_umount2, sys_getppid
- ✅ Add AHCI/NVMe partition scanning
- ✅ Partition disk image and create FAT32 filesystem
- Goal: working PID 1 → shell login loop

---

## References

- [AGENTS.md](./AGENTS.md) — Development conventions
- [README.md](./README.md) — Build system
- [Docs/](./Docs/) — Domain documentation
