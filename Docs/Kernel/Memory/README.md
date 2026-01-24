# Memory Management

FKernel implements a multi-layered memory management system designed for safety, performance, and hardware abstraction.

## Architecture

The memory subsystem is divided into three main layers:

1.  **Physical Memory Management (PMM):** Manages the actual RAM frames.
2.  **Virtual Memory Management (VMM):** Manages address spaces and page tables (x86_64 Paging).
3.  **Kernel Heap:** Provides dynamic memory allocation for kernel objects.

---

## 1. Physical Memory Management (PMM)

Located in `Include/Kernel/Memory/PhysicalMemory/` and `Src/Kernel/Memory/PhysicalMemory/`.

### Buddy Allocator
The system uses a **Buddy Allocator** to manage contiguous blocks of physical memory. This allows for efficient allocation and deallocation of memory blocks of various power-of-two sizes (orders), minimizing external fragmentation.
- **Min Order:** 12 (4KB pages)
- **Max Order:** 21 (2MB huge pages)

### Zones
Physical memory is divided into **Zones** (`ZoneType`) based on hardware constraints:
- **DMA:** Memory below 16MB, used for legacy hardware.
- **NORMAL:** Standard system memory (up to 4GB).
- **HIGH:** Memory above 4GB (PAE/x86_64).

### Physical Memory Manager
The `PhysicalMemoryManager` orchestrates the zones and the buddy allocator. It also uses a **Bitmap** within each zone for fast single-page (4KB) tracking.

---

## 2. Virtual Memory Management (VMM)

Located in `Include/Kernel/Memory/VirtualMemory/` and `Src/Kernel/Memory/VirtualMemory/`.

The `VirtualMemoryManager` handles x86_64 4-level paging:
- **PML4:** Page Map Level 4.
- **PDPT:** Page Directory Pointer Table.
- **PD:** Page Directory.
- **PT:** Page Table.

Features:
- **Identity Mapping:** Used during early boot for the lower memory regions.
- **Address Space Cloning:** Supports `fork()` by deep-copying user-mode page tables.
- **Page Fault Handling:** (Integrated with architecture-specific exception handlers).

---

## 3. Kernel Heap

Located in `Include/LibFK/Memory/heap_malloc.h` and `Src/LibFK/Memory/heap_malloc.cpp`.

The kernel heap provides a `malloc`/`free` style interface for dynamic allocations.
- **Implementation:** Linked-list based allocator with block splitting and merging.
- **Alignment:** 16-byte alignment enforced for SSE and performance.
- **Safety:** Magic numbers are used in block headers to detect heap corruption and double frees.
- **C++ Integration:** `Src/LibFK/Memory/new.cpp` provides global `operator new` and `delete` overrides.

---

## Initialization Flow

1.  **Early Boot:** `setup_page_tables.asm` sets up a basic identity mapping.
2.  **Early Init:** `fk::memory::heap_allocator().initialize()` sets up the linked-list heap.
3.  **Physical Memory:** `PhysicalMemoryManager::initialize()` scans the Multiboot2/UEFI memory map and populates zones.
4.  **Virtual Memory:** `VirtualMemoryManager::initialize()` creates the final kernel PML4 and switches CR3.