# Memory Management Domain Guide

## Overview

The Memory Management domain handles all memory operations in FKernel, from physical page allocation to virtual memory management. This domain is critical for system stability and performance.

## Architecture

```mermaid
flowchart TD
    MM["MemoryManager<br/>Top-level coordinator"]
    PMM["PhysicalMemoryManager<br/>Zone-based allocation"]
    VMM["VirtualMemoryManager<br/>4-level page tables"]
    HEAP["Kernel Heap<br/>First-fit linked list"]
    IOMMU["IOMMU<br/>Intel VT-d (abstract)"]

    MM --> PMM
    MM --> VMM
    MM --> HEAP
    MM --> IOMMU

    subgraph "Physical Zones"
        Z1["DMA Zone<br/>< 16MB"]
        Z2["NORMAL Zone<br/>16MB - 4GB"]
        Z3["HIGH Zone<br/>> 4GB"]
    end
    PMM --> Z1
    PMM --> Z2
    PMM --> Z3

    subgraph "Zone Components"
        BUDDY["BuddyAllocator<br/>Power-of-2 blocks<br/>orders 12-21 (4KB-2MB)"]
        BITMAP["Bitmap<br/>Individual pages<br/>O(1) alloc"]
    end
    Z1 --> BUDDY
    Z1 --> BITMAP
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
    RESERVE["Reserve kernel, heap,<br/>bitmap, multiboot regions"]
    VMM_INIT["VirtualMemoryManager::initialize()"]
    PML4["Allocate PML4 page table"]
    IDENTITY["Identity-map lower memory"]
    FB["Map framebuffer"]
    CR3["Write CR3 register"]
    HEAP_INIT["MemoryManager::initialize_heap()"]
    BLOCKS["Set up linked-list heap<br/>magic = 0xC0FFEE"]

    INIT --> PMM_INIT
    PMM_INIT --> TOPO --> MEMMAP --> CREATE_ZONES
    CREATE_ZONES --> CLASSIFY
    CLASSIFY -->|"< 16MB"| DMA
    CLASSIFY -->|"16MB-4GB"| NORMAL
    CLASSIFY -->|"> 4GB"| HIGH
    CREATE_ZONES --> RESERVE
    INIT --> VMM_INIT
    VMM_INIT --> PML4 --> IDENTITY --> FB --> CR3
    INIT --> HEAP_INIT --> BLOCKS
```

## Physical Memory Manager

### Zone Selection (NUMA-aware)

```mermaid
flowchart TD
    REQ["alloc_page(preferred_type, preferred_node)"]
    F1{"preferred type +<br/>preferred node<br/>available?"}
    F2{"any type +<br/>preferred node<br/>available?"}
    F3{"preferred type +<br/>any node<br/>available?"}
    F4["NORMAL zone,<br/>any node"]
    SELECT["Select zone"]
    TRY_BITMAP["Try Bitmap first<br/>O(1) alloc"]
    BITMAP_OK{Bitmap has<br/>free page?}
    BUDDY_ALLOC["Fallback: BuddyAllocator<br/>alloc(order=0)"]
    FAIL["Allocation failed"]

    REQ --> F1
    F1 -->|Yes| SELECT
    F1 -->|No| F2
    F2 -->|Yes| SELECT
    F2 -->|No| F3
    F3 -->|Yes| SELECT
    F3 -->|No| F4 --> SELECT
    SELECT --> TRY_BITMAP --> BITMAP_OK
    BITMAP_OK -->|Yes| DONE["Return page"]
    BITMAP_OK -->|No| BUDDY_ALLOC --> DONE
```

### Buddy Allocator

The buddy allocator handles **contiguous** physical page allocation (orders 12-21, i.e., 4KB to 2MB blocks):

```mermaid
flowchart TD
    ALLOC["alloc(order)"]
    FIND["Find smallest available block<br/>in free_lists[order]"]
    FOUND{Found block<br/>of exact order?}
    SPLIT["Split block:<br/>Remove from free_lists[i]<br/>Add buddy to free_lists[i-1]"]
    SPLIT_LOOP["Repeat until<br/>target order reached"]
    RETURN["Return allocated block"]

    ALLOC --> FIND --> FOUND
    FOUND -->|Yes| RETURN
    FOUND -->|No| SPLIT --> SPLIT_LOOP --> RETURN

    FREE["free(ptr, order)"]
    CHECK{"Buddy also free<br/>and in range?"}
    MERGE["Merge: XOR buddy address<br/>Remove buddy from free_lists<br/>Add merged to free_lists[order+1]"]
    LOOP["Repeat merge<br/>up to max order"]
    ADD["Add to free_lists[order]"]

    FREE --> CHECK
    CHECK -->|Yes| MERGE --> LOOP --> ADD
    CHECK -->|No| ADD
```

**Key detail**: Uses a static pool of 16384 `FreeBlock` nodes to avoid the chicken-and-egg problem of allocating metadata from the allocator you're building.

### Buddy Math

The buddy address is calculated via XOR:

$$\text{buddy}(ptr, order) = ptr \oplus (2^{\text{order}} \times \text{PAGE\_SIZE})$$

Merge condition: both the block and its buddy must be free and within the zone bounds.

## Virtual Memory Manager

### 4-Level Page Table Walk

```mermaid
flowchart LR
    PML4["PML4<br/>(CR3)"]
    PDPT["PDPT"]
    PD["PD"]
    PT["PT"]
    PTE["Page Table Entry"]

    PML4 -->|"PML4E[i]"| PDPT
    PDPT -->|"PDPTE[i]"| PD
    PD -->|"PDE[i]"| PT
    PT -->|"PTE[i]"| PTE
```

### Map Page Flow

```mermaid
flowchart TD
    MAP["map_page(virt, phys, flags)"]
    E_PML4["ensure_table(PML4, idx)<br/>Create if missing"]
    E_PDPT["ensure_table(PDPT, idx)<br/>Create if missing"]
    E_PD["ensure_table(PD, idx)<br/>Create if missing"]
    SET_PTE["Set PTE: phys | flags | Present"]
    INVLPG["invlpg(virt)"]

    MAP --> E_PML4 --> E_PDPT --> E_PD --> SET_PTE --> INVLPG
```

### Page Flags

| Flag | Bit | Purpose |
|------|-----|---------|
| Present | 0 | Page is in physical memory |
| Writable | 1 | Page is writable |
| User | 2 | Page accessible from ring 3 |
| WriteThrough | 3 | Write-through caching |
| CacheDisabled | 4 | Disable caching (MMIO) |
| Accessed | 5 | Page has been accessed |
| Dirty | 6 | Page has been written |
| HugePage | 7 | 2MB/1GB huge page |
| Global | 8 | Global page (not flushed on CR3 switch) |
| ExecuteDisable | 63 | NX bit (no-execute) |

### Fork vs Exec Address Space

```mermaid
flowchart TD
    FORK["fork()"]
    CLONE_DEEP["clone_address_space(cr3)<br/>Deep copy user pages<br/>Share kernel mappings"]
    EXEC["execve()"]
    CLONE_SHALLOW["create_address_space()<br/>Clone page table hierarchy<br/>Don't copy user pages"]

    FORK --> CLONE_DEEP
    EXEC --> CLONE_SHALLOW
```

### User Access Safety

- `copy_from_user()` / `copy_to_user()` validate addresses are in userspace (< 0x800000000000)
- Uses STAC/CLAC instructions when hardware SMAP is available

## Kernel Heap

Simple first-fit linked-list allocator:

```mermaid
flowchart LR
    H1["BlockHeader<br/>size=256<br/>is_free=false<br/>magic=0xC0FFEE"]
    H2["BlockHeader<br/>size=512<br/>is_free=true<br/>magic=0xC0FFEE"]
    H3["BlockHeader<br/>size=128<br/>is_free=false<br/>magic=0xC0FFEE"]

    H1 -->|"next"| H2
    H2 -->|"next"| H3
    H2 -->|"prev"| H1
    H3 -->|"prev"| H2
```

- `allocate(size)`: Walk block list, find free block >= size, split if large enough
- `free(ptr)`: Coalesce with adjacent free blocks (both forward and backward)
- All operations save/restore interrupts and use a `Spinlock`
- Magic number `0xC0FFEE` checked on every operation for corruption detection

## Integration Points

```mermaid
flowchart TD
    MM["Memory Manager"]
    PROC["Process Management<br/>Page tables for fork/exec"]
    DRV["Driver Framework<br/>DMA buffer allocation"]
    FS["Filesystem<br/>Page cache"]
    ELF["ELF Loader<br/>Address space setup"]
    SYS["Syscalls<br/>brk, mmap, munmap"]

    PROC --> MM
    DRV --> MM
    FS --> MM
    ELF --> MM
    SYS --> MM
```

## Key Design Decisions

- **Dual allocator per zone**: Bitmap for individual pages (fast), Buddy for contiguous blocks
- **Static buddy node pool**: 16384 pre-allocated nodes avoid chicken-and-egg allocation
- **Coalescing kernel heap**: Free merges both forward and backward with adjacent free blocks
- **NUMA-aware zone selection**: 4-level fallback across type and node preferences
- **SMAP/STAC-CLAC**: Hardware-enforced user/kernel memory access control

## Future Enhancements

### Short Term
1. Complete NUMA-aware allocation policies
2. Memory compaction for long-running systems
3. Per-CPU caches to reduce contention

### Long Term
1. Transparent huge pages (2MB/1GB)
2. Memory hot-plug support
3. Advanced NUMA policies with distance metrics
