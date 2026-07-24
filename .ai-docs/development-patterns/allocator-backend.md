# Allocator Backend Pattern

> AI-agent conceptual memory. Read before modifying LibFK memory allocation or Kernel heap code.

## The Problem

LibFK (STL-like library) must remain independent of the Kernel. But LibFK containers (`Vector`, `HashMap`, etc.) and smart pointers need dynamic memory allocation. The Kernel provides the actual allocator. How do they connect without LibFK including Kernel headers?

## The Solution: Callback Injection

`LibFK/Memory/allocator_backend.h` defines a C-style callback interface:

```cpp
struct AllocatorBackend {
  void *(*allocate)(size_t size);
  void *(*reallocate)(void *ptr, size_t size);
  void (*free)(void *ptr);
};
```

The Kernel sets the backend during early init:

```cpp
// In kernel heap initialization
static AllocatorBackend kernel_backend = {
  .allocate = kernel_malloc,
  .reallocate = kernel_realloc,
  .free = kernel_free,
};
fk::memory::set_allocator_backend(&kernel_backend);
```

LibFK containers and smart pointers call through the backend, never including Kernel headers.

## Layer Separation Enforcement

```
LibC (std types) → LibFK (uses allocator_backend callbacks) → Kernel (provides backend)
```

**Rule**: LibFK MUST NOT include Kernel headers. The allocator backend is the ONLY bridge for memory allocation.

## Current Violations

`Src/LibFK/Memory/heap_malloc.cpp` directly includes Kernel headers. This is a known violation tracked in TODO.md. The fix is to route all heap allocation through the backend pattern.

## Key Files

| File | Role |
|------|------|
| `Include/LibFK/Memory/allocator_backend.h` | Backend interface definition |
| `Include/LibFK/Memory/heap_malloc.h` | Heap malloc header |
| `Src/LibFK/Memory/heap_malloc.cpp` | Implementation (has layer violation) |
| `Src/Kernel/Memory/memory_manager.cpp` | Kernel-side backend registration |

## When Modifying

- **Adding new LibFK containers**: Use `fk::memory::get_allocator_backend()->allocate()` for all allocations
- **Changing Kernel heap**: Update the backend callbacks, not LibFK code
- **Adding new allocation patterns**: Add to the backend struct, not to LibFK directly
