# Memory Management Domain Guide

## Overview

The Memory Management domain handles all memory operations in FKernel, from physical page allocation to virtual memory management. Features: dual bitmap+buddy per zone, CoW reference counting with fork support, slab allocator, demand paging for anonymous and file-backed memory, 2MB huge pages for direct map, ASLR, W^X enforcement, RELRO, and NUMA-aware zone selection.

## Architecture

```mermaid
flowchart TD
    MM["MemoryManager<br/>Top-level coordinator"]
    PMM["PhysicalMemoryManager<br/>Zone-based allocation"]
    VMM["VirtualMemoryManager<br/>4-level page tables"]
    SLAB["SlabAllocator<br/>10 caches (16B-8192B)"]
    HEAP["Kernel Heap<br/>First-fit linked list<br/>tries Slab first"]
    IOMMU["IOMMU<br/>Intel VT-d (abstract)"]

    MM --> PMM
    MM --> VMM
    MM --> SLAB
    MM --> HEAP
    MM --> IOMMU

    subgraph "Per-Zone Components"
        BUDDY["BuddyAllocator<br/>Orders 12-21 (4KB-2MB)<br/>Embedded FreeBlock in free pages"]
        BITMAP["Bitmap<br/>Individual pages<br/>O(1) alloc"]
        COW["CoW Refcounts<br/>per-frame uint16_t[]<br/>allocated from zone"]
    end
    PMM --> BUDDY
    PMM --> BITMAP
    PMM --> COW

    subgraph "Physical Zones"
        Z1["DMA Zone<br/>< 16MB"]
        Z2["NORMAL Zone<br/>16MB - 4GB"]
        Z3["HIGH Zone<br/>> 4GB"]
    end
    PMM --> Z1
    PMM --> Z2
    PMM --> Z3
```

## Initialization Flow

```mermaid
flowchart TD
    INIT["MemoryManager::initialize()"]
    PMM_INIT["PhysicalMemoryManager::initialize()"]
    TOPO["TopologyManager::initialize()<br/>NUMA discovery via SRAT"]
    MEMMAP["Read multiboot2 memory map"]
    CREATE_ZONES["create_zone() for each range"]
    CLASSIFY{Classify range}
    DMA["DMA Zone (< 16MB)"]
    NORMAL["NORMAL Zone (16MB-4GB)"]
    HIGH["HIGH Zone (> 4GB)"]
    RESERVE["Reserve kernel, heap,<br/>bitmap, AP trampoline,<br/>multiboot, modules"]
    COW_ALLOC["Allocate per-zone CoW<br/>uint16_t refcount arrays"]
    VMM_INIT["VirtualMemoryManager::initialize()"]
    PML4["Allocate PML4 page table"]
    IDENTITY["Identity-map lower memory + framebuffer"]
    CR3["Write CR3 register"]
    DIRECT_MAP["extend_direct_map()<br/>2MB huge pages at KERNEL_VIRT_BASE"]
    RECONCILE["reconcile_buddies()<br/>sync buddy from bitmap"]
    SLAB_INIT["SlabAllocator::initialize()<br/>10 caches"]
    HEAP_INIT["MemoryManager::initialize_heap()<br/>linked-list heap + LibFK backend"]

    INIT --> PMM_INIT
    PMM_INIT --> TOPO --> MEMMAP --> CREATE_ZONES
    CREATE_ZONES --> CLASSIFY
    CLASSIFY -->|"< 16MB"| DMA
    CLASSIFY -->|"16MB-4GB"| NORMAL
    CLASSIFY -->|"> 4GB"| HIGH
    CREATE_ZONES --> RESERVE
    RESERVE --> COW_ALLOC
    INIT --> VMM_INIT
    VMM_INIT --> PML4 --> IDENTITY --> CR3
    VMM_INIT --> DIRECT_MAP
    INIT --> RECONCILE
    INIT --> SLAB_INIT
    INIT --> HEAP_INIT
```

## Physical Memory Manager

### Dual Allocator per Zone

| Allocator | Use Case | Operation |
|-----------|----------|-----------|
| **Bitmap** | Single 4KB pages | `bitmap.alloc()` — O(1) first clear bit |
| **Buddy** | Contiguous blocks (orders 12-21) | `buddy.alloc(order)` — power-of-two splits |

`alloc_page()` uses bitmap first, then invalidates the buddy page. `alloc_contiguous()` uses buddy first, then marks all resulting pages in bitmap. Both are reconciled via `reconcile_buddies()` after the direct map is available.

### Zone Selection (NUMA-aware)

```mermaid
flowchart TD
    REQ["alloc_page(preferred_type, preferred_node)"]
    F1{"preferred type +<br/>preferred node?"}
    F2{"any type +<br/>preferred node?"}
    F3{"preferred type +<br/>any node?"}
    F4["NORMAL zone, any node"]
    FALLBACK["zone[0] if nothing else"]
    SELECT["Select zone"]
    TRY_BITMAP["Bitmap.alloc()<br/>O(1)"]
    BITMAP_OK{Bitmap free?}
    ALLOC_FAIL["Return 0 (failure)"]

    REQ --> F1
    F1 -->|Yes| SELECT
    F1 -->|No| F2
    F2 -->|Yes| SELECT
    F2 -->|No| F3
    F3 -->|Yes| SELECT
    F3 -->|No| F4 --> SELECT
    F4 -->|No zone found| FALLBACK --> SELECT
    SELECT --> TRY_BITMAP --> BITMAP_OK
    BITMAP_OK -->|Yes| DONE["Return phys addr<br/>refcount = 1"]
    BITMAP_OK -->|No| ALLOC_FAIL
```

### Buddy Allocator

Orders 12-21 (4KB to 2MB blocks):

```mermaid
flowchart TD
    ALLOC["alloc(order)"]
    FIND["Find smallest available block<br/>in free_lists[order..MAX_ORDER]"]
    FOUND{Found at<br/>exact order?}
    SPLIT["Split: remove from free_lists[i]<br/>Add buddy to free_lists[i-1]"]
    SPLIT_LOOP["Repeat until target order"]
    RETURN["Return block"]

    ALLOC --> FIND --> FOUND
    FOUND -->|Yes| RETURN
    FOUND -->|No| SPLIT --> SPLIT_LOOP --> RETURN

    FREE["free(ptr, order)"]
    CHECK{"Buddy free<br/>and in range?"}
    MERGE["Merge: XOR buddy addr<br/>Remove buddy from free_lists<br/>Add merged to free_lists[order+1]"]
    LOOP["Repeat up to MAX_ORDER"]
    ADD["Add to free_lists[order]"]

    FREE --> CHECK
    CHECK -->|Yes| MERGE --> LOOP --> ADD
    CHECK -->|No| ADD
```

**Embedded FreeBlock**: Buddy metadata (`FreeBlock` node) is stored IN the free pages themselves, accessed via the `KERNEL_VIRT_BASE` direct map. This saves ~1MB of BSS compared to a static pool. A 16384-entry static pool is also available for bootstrap before the direct map is ready.

### Buddy Math

$$\text{buddy}(ptr, order) = ptr \oplus (2^{order})$$

Merge condition: both the block and its buddy must be free and within the zone bounds.

### CoW Reference Counting

Each zone has a per-frame `uint16_t` reference count array, allocated from the zone's own physical pages:

- `alloc_page()`: sets refcount = 1
- `free_page()`: decrements refcount; only frees when it reaches 0
- `increment_refcount(phys)`: ++ on CoW fork
- `decrement_refcount(phys)`: -- on page unmap
- `get_refcount(phys)`: read-only query

## Virtual Memory Manager

### 4-Level Page Table Walk

```mermaid
flowchart LR
    PML4["PML4<br/>(CR3)"]
    PDPT["PDPT"]
    PD["PD"]
    PT["PT"]
    PTE["Page Table Entry"]

    PML4 -->|"PML4E[47:39]"| PDPT
    PDPT -->|"PDPTE[38:30]"| PD
    PD -->|"PDE[29:21]"| PT
    PT -->|"PTE[20:12]"| PTE
```

### Map Page Flow

```mermaid
flowchart TD
    MAP["map_page(virt, phys, flags)"]
    E_PML4["ensure_table(PML4, idx)<br/>Create/copy if missing<br/>COW-safe: copy kernel tables for user bit"]
    E_PDPT["ensure_table(PDPT, idx)"]
    E_PD["ensure_table(PD, idx)"]
    SET_PTE["Set PTE: phys | flags | Present"]
    TLB{"Changed<br/>parent tables?"}
    FLUSH["flush_tlb()"]
    INVLPG["invlpg(virt)"]

    MAP --> E_PML4 --> E_PDPT --> E_PD --> SET_PTE --> TLB
    TLB -->|Yes| FLUSH
    TLB -->|No| INVLPG
```

### Page Flags

| Flag | Bit | Purpose |
|------|-----|---------|
| Present | 0 | Page is in physical memory |
| Writable | 1 | Page is writable |
| User | 2 | Page accessible from ring 3 |
| WriteThrough | 3 | Write-through caching |
| CacheDisabled | 4 | Disable caching (MMIO) |
| Accessed | 5 | Page has been accessed (set by CPU) |
| Dirty | 6 | Page has been written (set by CPU) |
| HugePage | 7 | 2MB huge page (used in direct map) |
| Global | 8 | Global page (not flushed on CR3 switch) |
| ExecuteDisable | 63 | NX bit (no-execute) |

### Fork vs Exec Address Space

```mermaid
flowchart TD
    FORK["fork()"]
    CLONE_DEEP["clone_address_space(cr3)<br/>Deep copy user pages with CoW<br/>Writable → read-only in both<br/>Increment CoW refcount"]
    EXEC["execve()"]
    CLONE_SHALLOW["create_address_space()<br/>Clone page table hierarchy<br/>Share user pages (exec will swap)"]

    FORK --> CLONE_DEEP
    EXEC --> CLONE_SHALLOW
```

### CoW Fork Details

`clone_table_recursive(cr3, target_cr3, virtual_address, max_depth, deep_copy)` implements the actual page table copying:
- `deep_copy = true` (fork): Allocates new physical pages at every table level, copies entries, sets user pages read-only and increments CoW refcounts
- `deep_copy = false` (exec): Creates a shallow clone of the table hierarchy (will be swapped during ELF loading)

### Demand Paging

Memory is mapped lazily on first access. The page fault handler (`pf_handler.cpp` — `handle_demand_paging()`) handles two types of regions:

1. **Anonymous memory** (`mmap MAP_ANONYMOUS`):
   - **Not-present fault**: Allocate + zero-fill a physical page, map into user address space
   - **Write-protection fault**: CoW break — allocate new page, copy data, update PTE with Writable

2. **File-backed memory** (`mmap of a file descriptor`):
   - **Not-present fault**: Read the missing page from the file's page cache via the filesystem; map into address space
   - **Write-protection fault**: CoW break for private mappings; for shared mappings, write-through to page cache

### Direct Map

`extend_direct_map()` maps ALL physical RAM at `KERNEL_VIRT_BASE` using 2MB huge pages (`PageFlags::HugePage`). This provides a linear kernel-accessible view of all physical memory, used by:
- Buddy allocator's embedded FreeBlock metadata
- CoW refcount arrays (accessed as `phys + KERNEL_VIRT_BASE`)
- Any kernel code needing physical address access

### User Access Safety

- `copy_from_user()` / `copy_to_user()` validate addresses are in userspace (`< 0x800000000000`)
- Uses STAC/CLAC instructions when hardware SMAP is available
- Returns `Result<void, Error>` for error propagation

## ASLR, W^X, and RELRO

Implemented in Phase 30b:

**ASLR (Address Space Layout Randomization)**:
- Randomizes `mmap` base address and ELF load address per process
- Stack and heap randomization included
- Entropy sources: CPU RDRAND or TSC-based seed mixed with per-process PID

**W^X Enforcement**:
- No page may be simultaneously writable and executable
- ELF segment mapping sets W or X, never both
- `mprotect` rejects PROT_WRITE | PROT_EXEC combinations
- Applied at page-table level via NX bit (bit 63) and Writable bit

**RELRO (Relocation Read-Only)**:
- After ELF relocations are applied, the GOT is marked read-only
- Full RELRO: entire GOT read-only after initialization
- Partial RELRO: GOT entries used before initialization remain writable

## Slab Allocator

`SlabAllocator` provides fast, fixed-size object allocation with 10 caches:

| Cache Size | Use Case |
|------------|----------|
| 16B | Tiny objects, pointers |
| 32B | Small objects |
| 64B | Medium objects |
| 128B | |
| 256B | |
| 512B | |
| 1024B | |
| 2048B | |
| 4096B | Page-sized allocations |
| 8192B | Large kernel objects |

The kernel heap (`MemoryManager::allocate()`) tries slab first for allocations ≤8192 bytes, falling back to the linked-list heap only when the slab cache is exhausted.

## Kernel Heap

Simple first-fit linked-list allocator:

- 16-byte alignment for all allocations
- Block splitting: if free block is large enough, carve out exactly needed size + split remainder
- Free coalescing: merges both forward and backward with adjacent free blocks
- Magic number `0xC0FFEE` checked on every operation for corruption detection
- Interrupt-safe: saves/restores RFLAGS, acquires `m_heap_lock` spinlock
- LibFK integration via `AllocatorBackend` callback structure

## Integration Points

```mermaid
flowchart TD
    MM["Memory Manager"]
    PROC["Process Management<br/>fork: clone_address_space (CoW)<br/>exec: create_address_space"]
    DRV["Driver Framework<br/>DMA buffer allocation<br/>MMIO mapping"]
    FS["Filesystem<br/>Block device read/write<br/>via direct map"]
    ELF["ELF Loader<br/>W^X enforcement, ASLR<br/>segment mapping"]
    SYS["Syscalls<br/>mmap, munmap, mprotect, brk"]
    PF["Page Fault Handler<br/>demand paging, CoW break"]

    PROC --> MM
    DRV --> MM
    FS --> MM
    ELF --> MM
    SYS --> MM
    PF --> MM
```

## Key Design Decisions

- **Dual allocator per zone**: Bitmap for fast single-page, Buddy for contiguous — reconciled, not redundant
- **Embedded buddy metadata**: FreeBlock stored in free pages via direct map, saving ~1MB BSS
- **CoW refcounts**: Per-zone uint16_t arrays for accurate shared page tracking
- **COW-safe table creation**: `ensure_table()` copies shared kernel tables when user bit needed
- **2MB huge pages**: Direct map via `PageFlags::HugePage` for low TLB pressure
- **Slab-first heap**: `allocate()` tries slab for ≤8192B, falls back to linked-list heap
- **ASLR**: Randomized mmap/ELF/stack/heap base per process (Phase 30b)
- **W^X**: No page may be simultaneously writable and executable; enforced at PTE level
- **RELRO**: GOT marked read-only after ELF relocations applied (Phase 30b)
- **NUMA-aware**: Zone selection considers proximity domain with 4-level fallback
- **SMAP/STAC-CLAC**: Hardware-enforced user/kernel memory access control

## Future Enhancements

### Short Term
1. Per-CPU page caches to reduce PMM lock contention
2. Memory compaction for long-running systems
3. Per-segment ELF bounds validation (p_offset + p_filesz)

### Long Term
1. Transparent huge pages (2MB/1GB) for user mappings
2. Swap support
3. Memory hot-plug
4. Advanced NUMA policies with distance metrics (Phase 34)
