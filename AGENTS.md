# FKernel Development Guide for AI Agents

This guide provides essential information for AI agents working on the FKernel codebase.

## Build System & Commands

### Primary Build Tool
- **Build System**: XMake (Lua-based)
- **Language**: C++20 (freestanding), C17 (LibC), NASM (assembly)

### Essential Commands
```bash
# Core build commands
xmake                    # Build kernel
xmake run               # Run in QEMU  
xmake clean             # Clean build artifacts

# Testing commands
xmake -bv Test          # Build tests (verbose)
xmake run Test          # Run all tests
# Note: No single test command available - run full test suite

# Custom tasks
xmake setup-hda         # Create disk image
xmake build-initrd      # Build initrd
xmake config-initrd     # Configure initrd interactively
xmake analyze           # Analyze kernel runtime
```

### Build Artifacts
- `build/FKernel.bin` - Kernel binary
- `build/FKernel-MockOS.iso` - Bootable ISO
- `build/FKernel-HDA.qcow2` - Disk image

## Code Architecture & Dependencies

### Layer Separation (STRICT)
1. **LibC** - Minimal freestanding C library (strings, memory, types)
2. **LibFK** - STL-like library using LibC + self only
3. **Kernel** - Uses ONLY LibFK, NEVER LibC directly

### Namespace Structure
```cpp
// LibFK namespaces
namespace fk {
    namespace containers { }  // Vector, HashMap, List
    namespace text { }        // String, StringBuilder  
    namespace memory { }      // Smart pointers
    namespace core { }        // Result, Optional
    namespace utilities { }   // Aligner, Pair, etc.
}

// Kernel namespaces  
namespace fkernel {
    namespace boot { }
    namespace cpu { }
    namespace memory { }
    namespace process { }
    namespace fs { }
    namespace drivers { }
    // ... etc
}
```

## Mandatory Coding Standards (Object Calisthenics)

### 1. One Indentation Level Per Method
```cpp
// ❌ BAD: Nested hell
void process() {
    for (auto& item : items) {
        if (item.valid()) {
            if (item.ready()) {
                do_work(item);
            }
        }
    }
}

// ✅ GOOD: Extract methods
void process() {
    for (auto& item : items)
        process_item(item);
}
```

### 2. No ELSE Keyword
```cpp
// ❌ BAD
if (condition) {
    do_something();
} else {
    do_other();
}

// ✅ GOOD: Early returns
if (condition) {
    do_something();
    return;
}
do_other();
```

### 3. Wrap All Primitives
```cpp
// ❌ BAD: Raw primitives
class Process {
    int pid;
    unsigned long flags;
};

// ✅ GOOD: Type-safe wrappers
class ProcessId {
    int m_value;
public:
    explicit ProcessId(int id) : m_value(id) {}
    bool is_valid() const { return m_value > 0; }
};
```

### 4. First-Class Collections
```cpp
// ❌ BAD: Exposed vector
class Manager {
    fk::Vector& processes() { return m_processes; }
};

// ✅ GOOD: Dedicated collection class
class ProcessList {
    fk::Vector m_processes;
public:
    void add(Process* p);
    Process* find_by_pid(ProcessId pid);
    // Iterators, etc.
};
```

### 5. One Dot Per Line (Law of Demeter)
```cpp
// ❌ BAD: Chain calls
auto name = process->thread()->name();

// ✅ GOOD: Delegate
auto name = process->thread_name();
```

### 6. No Abbreviations
```cpp
// ❌ BAD: class ProcMgr { void init_procs(); }
// ✅ GOOD: class ProcessManager { void initialize_processes(); }
```

### 7. Keep Entities Small
- **Classes**: Max 200 lines
- **Methods**: Max 20 lines  
- **Files**: Max 500 lines

### 8. Max Two Instance Variables
```cpp
// ❌ BAD: Many variables
class Device {
    u16 m_iobase, m_control;
    u8 m_irq;
    bool m_dma_enabled;
    DMAController* m_dma;
};

// ✅ GOOD: Compose objects
class Device {
    DevicePorts m_ports;
    DeviceTransfer m_transfer;
};
```

### 9. No Getters/Setters
```cpp
// ❌ BAD: Anemic model
class Process {
    ProcessState m_state;
public:
    ProcessState state() const { return m_state; }
    void set_state(ProcessState s) { m_state = s; }
};

// ✅ GOOD: Rich model  
class Process {
    ProcessState m_state;
public:
    bool is_running() const { return m_state == Running; }
    void block() { m_state = Blocked; }
};
```

## Error Handling

### Use Result<T, Error> Pattern
```cpp
// Return type for fallible operations
Result<Page*, Error> allocate_page();

// Usage with TRY macro
auto page = TRY(allocate_page());

// Optional for nullable values
fk::Optional<Process*> find_process(ProcessId pid);
```

## Import & Include Guidelines

### Include Order
1. System headers (LibC)
2. LibFK headers  
3. Kernel headers
4. Local headers

### Include Style
```cpp
#include <LibC/string.h>
#include <LibFK/Core/Result.h>
#include <Kernel/Memory/memory_manager.h>
```

### No Standard Library
- No `<iostream>`, `<vector>`, `<string>` etc.
- Use LibFK equivalents instead
- Compile flags: `-nostdlib -nostdinc -ffreestanding`

## Testing Requirements

### Coverage Goals
- **LibC**: 90%+ coverage required
- **LibFK**: 85%+ coverage required  
- **Kernel critical paths**: 75%+ coverage required

### All Public APIs Must Have Tests
```cpp
// Test files follow pattern: tests/[component]/test_[module].cpp
TEST(memcpy, copies_data_correctly) {
    char src[10] = "hello";
    char dst[10];
    memcpy(dst, src, 6);
    EXPECT_STREQ(dst, "hello");
}
```

## Performance & Hardware

### Compiler Flags
- `-ffreestanding` - No OS runtime
- `-fno-exceptions` - No exception support
- `-fno-rtti` - No runtime type information
- `-mcmodel=kernel` - Kernel memory model
- `-mno-sse` - No SIMD (kernel context)

### Hardware Interaction
- Use `volatile` for hardware registers
- Include memory barriers when needed: `__sync_synchronize()`
- Document against hardware specifications
- Handle MMIO correctly

## Documentation Standards

### Every Public API Needs Documentation
```cpp
/// @brief Allocates a physical page frame
/// @return Result containing page address on success,
///         Error::OutOfMemory if no pages available
/// @note This function is thread-safe
/// @warning Caller must free page with free_page()
Result<Page*, Error> allocate_page();
```

## Code Style

### Naming Conventions
- **Classes**: PascalCase (`ProcessManager`)
- **Methods**: snake_case (`initialize_processes()`)  
- **Variables**: snake_case with m_ prefix for members (`m_process_count`)
- **Constants**: UPPER_SNAKE_CASE (`MAX_PROCESSES`)
- **Namespaces**: snake_case (`fkernel::memory`)

### Formatting
- No explicit formatting config found - follow existing patterns
- 2-space indentation observed in codebase
- Opening braces on same line for methods/functions
- Use `//` comments, avoid `/* */` blocks

### Memory Management
- RAII preferred - no manual cleanup
- Use LibFK smart pointers: `OwnPtr`, `RefPtr`
- Stack allocation preferred over heap
- No exceptions - use Result<T, Error> pattern

## Critical Build Dependencies

### Required Tools
- `clang`/`clang++` (C++20 support)
- `ld.lld` (linker)
- `nasm` (assembler)
- `xmake` (build system)
- `qemu-system-x86_64` (emulator)

### Optional Tools (mentioned in README)
- `clang-format` - Code formatting
- `clang-tidy` - Static analysis  
- `cppcheck` - Additional analysis

## Project Structure Notes

### Key Directories
- `Src/Kernel/` - Kernel implementation
- `Src/LibC/` - Freestanding C library
- `Src/LibFK/` - STL-like library
- `Include/` - All header files
- `Meta/` - Build tools and scripts

### File Organization
- Headers mirror source structure in `Include/`
- Implementation in `Src/`
- Architecture-specific code in `Src/Kernel/Arch/x86_64/`

## Security & Safety

### Kernel Mode Constraints
- No standard library access
- Direct hardware interaction
- Memory management awareness
- Interrupt context considerations

### Development Philosophy
- Security-first design
- Drivers in userspace (DAL architecture)
- Isolated kernel subsystems
- Production-ready, not hobby OS