# Memory Management Domain Guide

## Overview

The Memory Management domain handles all memory operations in FKernel, from physical page allocation to virtual memory management. This domain is critical for system stability and performance.

## Architecture

```
Include/Kernel/Memory/
├── memory_manager.h          # Main memory management interface
├── PhysicalMemory/           # Physical memory management
│   ├── physical_memory_manager.h
│   ├── physical_memory_zone.h
│   └── Buddy/
│       ├── buddy_allocator.h
│       ├── buddy_order.h
│       ├── buddy_state.h
│       └── free_blocks.h
├── VirtualMemory/           # Virtual memory management
│   ├── virtual_memory_manager.h
│   └── Pages/
│       ├── page_table.h
│       └── page_flags.h
└── ObjectMemory/            # Object allocation (slab)
    └── Zone/
        ├── zone_allocator.h
        ├── zone_defs.h
        └── zone_types.h
```

## Core Components

### 1. Physical Memory Manager

**Purpose**: Allocate and manage physical memory pages

**Key Classes**:
- `PhysicalMemoryManager`: Main interface for physical allocation
- `PhysicalMemoryZone`: NUMA-aware memory zones
- `BuddyAllocator`: Buddy system for page allocation

**Allocation Strategy**:
```cpp
// Allocate with zone preference
auto page = TRY(PhysicalMemoryManager::allocate_page(
    ZoneType::NORMAL, 
    preferred_node = 0
));

// Free page
PhysicalMemoryManager::free_page(page);
```

### 2. Virtual Memory Manager  

**Purpose**: Manage virtual address spaces and page tables

**Key Classes**:
- `VirtualMemoryManager`: Virtual memory operations
- `PageTable`: Page table management
- `PageFlags`: Page permission flags

**Operations**:
```cpp
// Map virtual to physical
auto result = TRY(VirtualMemoryManager::map_page(
    virtual_address,
    physical_address,
    PageFlags::ReadWrite | PageFlags::User
));

// Unmap page
VirtualMemoryManager::unmap_page(virtual_address);
```

### 3. Object Memory (Slab Allocator)

**Purpose**: Efficient allocation of kernel objects

**Key Classes**:
- `ZoneAllocator`: Slab-based object allocation
- `ZoneDef`: Zone configuration
- `ZoneTypes`: Supported zone types

**Usage**:
```cpp
// Allocate object from zone
auto object = TRY(ZoneAllocator::allocate<Process>(ZoneType::Kernel));

// Free object
ZoneAllocator::free(object);
```

## Design Patterns

### 1. Strategy Pattern
Different allocators for different use cases:
- **Buddy**: Physical page allocation
- **Slab**: Object allocation  
- **Zone**: NUMA-aware allocation

### 2. RAII Pattern
Memory management with smart pointers:
```cpp
{
    auto page = TRY(PhysicalMemoryManager::allocate_page());
    // Automatically freed when scope ends
}
```

### 3. Result-Based Error Handling
All allocation operations return `Result<T, Error>`.

## NUMA Support

### Current Status (60% Complete)
- ✅ SRAT table parsing implemented
- ✅ Topology manager functional
- ⚠️ NUMA-aware allocation incomplete

### Missing Components
- Real NUMA-aware allocation policies
- Cross-node fallback strategies
- CPU-to-memory distance metrics

## Performance Considerations

### 1. Allocation Speed
- Buddy: O(log n) allocation
- Slab: O(1) allocation for common objects
- Zone: Constant time for pre-defined objects

### 2. Memory Fragmentation
- Buddy system minimizes external fragmentation
- Slab allocator eliminates internal fragmentation
- Regular compaction for long-running systems

### 3. Cache Efficiency
- NUMA-aware allocation improves cache locality
- Per-CPU caches reduce contention
- Alignment optimizations for hardware requirements

## Integration Points

### Hardware Dependencies
- **CPU**: Page table format, TLB operations
- **ACPI**: SRAT parsing for NUMA topology
- **IOMMU**: DMA memory management

### Kernel Subsystems
- **Process Management**: Memory mapping for processes
- **Driver Framework**: DMA buffer allocation
- **Filesystem**: Page cache management

## Development Guidelines

### Working in This Domain

1. **Understand NUMA**: Consider NUMA topology in all allocations
2. **Error Handling**: Always check allocation results
3. **Performance**: Profile allocation patterns
4. **Safety**: Validate all memory operations
5. **Testing**: Comprehensive coverage required (75%+)

### Common Patterns

```cpp
// Safe allocation pattern
template<typename T>
Result<fk::OwnPtr<T>, Error> allocate_object() {
    auto memory = TRY(ZoneAllocator::allocate<T>(ZoneType::Kernel));
    return fk::OwnPtr<T>(new (memory) T());
}

// Memory mapping pattern
Result<void, Error> map_device_memory(PhysicalAddress phys, size_t size) {
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        auto result = TRY(VirtualMemoryManager::map_page(
            virt_address + i,
            phys + i,
            PageFlags::WriteThrough | PageFlags::Device
        ));
    }
    return {};
}
```

## Testing Strategy

### Unit Tests
- Allocator correctness
- Boundary conditions
- Error handling
- NUMA topology parsing

### Integration Tests
- Virtual-to-physical mapping
- Multi-process memory isolation
- Driver DMA operations
- Filesystem page cache

### Stress Tests
- Memory fragmentation
- High allocation rates
- NUMA node failures
- Long-running stability

## Future Enhancements

### Short Term (4-6 weeks)
1. Complete NUMA-aware allocation
2. Add memory compaction
3. Implement per-CPU caches
4. Enhanced error reporting

### Long Term (8-12 weeks)  
1. Transparent huge pages
2. Memory hot-plug support
3. Advanced NUMA policies
4. Memory usage analytics

---

This domain is **critical for system stability** and requires **careful testing** and **performance optimization**.