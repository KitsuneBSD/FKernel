# FKernel Development Instructions

## Project Overview

FKernel is a modern kernel written in C++, inspired by SerenityOS, BSD systems, and XNU. The project aims to be a production-ready kernel (not a hobby OS) through its innovative Driver Abstraction Layer (DAL).

### Core Philosophy

- **Pragmatic POSIX**: Use POSIX as a "mask" for kernel internals, not slavish compliance
- **BSD/XNU-inspired**: Follow SystemV ABI and BSD conventions
- **Security-first**: Drivers run in userspace, isolated kernel subsystems (IPUK)
- **DAL-powered**: Recompile Linux/Windows drivers against FKernel primitives (like DXVK/Wine approach)

---

## MANDATORY

THE CODE IS EXPLICIT **FREESTANDING** and **BAREMETAL** resources like exceptions doesn't exists.

The **Error Threatment** need to be make as C-Like code.

## Architecture Layers

```
┌─────────────────────────────────────┐
│         Userspace                   │
│  (Applications, Shell, Utils)       │
└──────────────┬──────────────────────┘
               │ syscalls
┌──────────────▼──────────────────────┐
│         Kernel                      │
│  (Uses ONLY LibFK)                  │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│         LibFK                       │
│  (STL-like, uses LibC + self)       │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│         LibC                        │
│  (Freestanding, minimal)            │
└─────────────────────────────────────┘
```

**Critical Rules:**

1. **LibC**: Only manipulates strings, memory, types. Freestanding.
2. **LibFK**: Uses LibC + itself. Never includes kernel headers.
3. **Kernel**: Uses ONLY LibFK. Never includes LibC directly.

---

## Code Organization

### Namespace Structure

```cpp
// LibFK namespaces
namespace fk {
    namespace containers {  // Vector, HashMap, List
        template class Vector;
        template class HashMap;
    }

    namespace text {        // String, StringBuilder
        class String;
        class StringBuilder;
    }

    namespace memory {      // Smart pointers
        template class OwnPtr;
        template class RefPtr;
    }

    namespace core {        // Error handling
        template class Result;
        template class Optional;
    }
}

// Kernel namespaces
namespace fkernel {
    namespace boot {        // early_init, multiboot
        void early_init();
    }

    namespace cpu {         // GDT, IDT, APIC
        class GDT;
        class APIC;
    }

    namespace memory {      // PMM, VMM, Heap
        class Physical;
        class Virtual;
        class AddressSpace;
    }

    namespace process {     // Scheduler, threads
        class Manager;
        class Scheduler;
        class Thread;
    }

    namespace fs {          // VFS, filesystems
        class VFS;
        namespace ext2 { ... }
        namespace fat32 { ... }
    }

    namespace storage {     // Block devices
        namespace ata { ... }
        namespace ahci { ... }
    }

    namespace net {         // Networking
        class Device;
        class Stack;
    }

    namespace drivers {     // Device drivers
        class Serial;
        class RTC;
    }

    namespace dal {         // Driver Abstraction Layer
        namespace linux { ... }
        namespace windows { ... }
    }
}
```

---

## Object Calisthenics (MANDATORY)

All code MUST follow these 9 rules:

### 1. One Level of Indentation Per Method

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

void process_item(Item& item) {
    if (!item.valid()) return;
    if (!item.ready()) return;
    do_work(item);
}
```

### 2. Don't Use ELSE Keyword

```cpp
// ❌ BAD: Else branches
Result allocate() {
    if (has_free_pages()) {
        return get_free_page();
    } else {
        return Error::OutOfMemory;
    }
}

// ✅ GOOD: Early returns
Result allocate() {
    if (has_free_pages())
        return get_free_page();

    return Error::OutOfMemory;
}
```

### 3. Wrap All Primitives and Strings

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

class ProcessFlags {
    unsigned long m_bits;
public:
    bool is_running() const { return m_bits & RUNNING; }
    void set_running() { m_bits |= RUNNING; }
};

class Process {
    ProcessId m_pid;
    ProcessFlags m_flags;
};
```

### 4. First Class Collections

```cpp
// ❌ BAD: Exposed vector
class Manager {
public:
    fk::Vector& processes() { return m_processes; }
private:
    fk::Vector m_processes;
};

// ✅ GOOD: Dedicated collection class
class ProcessList {
    fk::Vector m_processes;
public:
    void add(Process* p);
    Process* find_by_pid(ProcessId pid);
    size_t count() const;
    auto begin() { return m_processes.begin(); }
    auto end() { return m_processes.end(); }
};

class Manager {
    ProcessList m_processes;
};
```

### 5. One Dot Per Line (Law of Demeter)

```cpp
// ❌ BAD: Chain of calls
auto name = process->thread()->name();

// ✅ GOOD: Delegate
auto name = process->thread_name();

// Process class:
fk::String thread_name() const {
    return m_thread->name();
}
```

### 6. Don't Abbreviate

```cpp
// ❌ BAD: Abbreviations
class ProcMgr {
    void init_procs();
    int num_procs;
};

// ✅ GOOD: Full names
class ProcessManager {
    void initialize_processes();
    int process_count;
};
```

### 7. Keep All Entities Small

- **Classes**: Maximum 200 lines
- **Methods**: Maximum 20 lines
- **Files**: Maximum 500 lines

```cpp
// ❌ BAD: God class with 2000 lines
class ProcessManager {
    // scheduling logic
    // memory management
    // IPC handling
    // signal handling
    // ... 2000 lines
};

// ✅ GOOD: Focused classes
class ProcessManager {        // 150 lines
    ProcessScheduler m_scheduler;
    ProcessAllocator m_allocator;
    ProcessIPC m_ipc;
};

class ProcessScheduler { };   // 200 lines
class ProcessAllocator { };   // 150 lines
class ProcessIPC { };         // 180 lines
```

### 8. No Classes With More Than Two Instance Variables

```cpp
// ❌ BAD: Many instance variables
class ATADevice {
    u16 m_iobase;
    u16 m_control;
    u8 m_irq;
    bool m_dma_enabled;
    DMAController* m_dma;
    Buffer* m_buffer;
};

// ✅ GOOD: Compose objects
class ATADevice {
    ATAPorts m_ports;
    ATATransfer m_transfer;
};

class ATAPorts {
    u16 m_iobase;
    u16 m_control;
    u8 m_irq;
};

class ATATransfer {
    TransferMode m_mode;
    Buffer* m_buffer;
};
```

### 9. No Getters/Setters/Properties

```cpp
// ❌ BAD: Anemic domain model
class Process {
    ProcessState m_state;
public:
    ProcessState state() const { return m_state; }
    void set_state(ProcessState s) { m_state = s; }
};

// Usage:
if (proc->state() == Running)
    proc->set_state(Blocked);

// ✅ GOOD: Rich domain model
class Process {
    ProcessState m_state;
public:
    bool is_running() const { return m_state == Running; }
    void block() { m_state = Blocked; }
    void resume() { m_state = Running; }
};

// Usage:
if (proc->is_running())
    proc->block();
```

---

## Error Handling

### Use Result<T, Error> for Fallible Operations

```cpp
// ❌ BAD: Return codes
int allocate_page(Page** out) {
    if (!has_free_pages())
        return -ENOMEM;
    *out = get_free_page();
    return 0;
}

// ✅ GOOD: Result type
Result allocate_page() {
    if (!has_free_pages())
        return Error::OutOfMemory;

    return get_free_page();
}

// Usage with TRY macro:
auto page = TRY(allocate_page());
```

### Use Optional<T> for Nullable Values

```cpp
// ❌ BAD: Nullptr checks
Process* find_process(ProcessId pid) {
    // returns nullptr if not found
}

// ✅ GOOD: Optional
fk::Optional find_process(ProcessId pid) {
    // returns None if not found
}

// Usage:
if (auto proc = find_process(pid); proc.has_value())
    proc.value()->resume();
```

---

## Testing Requirements

The test need be run on real runtime, so you need create mocks to internal functionalities

### All Public APIs Must Have Tests

```cpp
// For LibC: tests/libc/test_string.cpp
TEST(memcpy, copies_data_correctly) {
    char src[10] = "hello";
    char dst[10];
    memcpy(dst, src, 6);
    EXPECT_STREQ(dst, "hello");
}

TEST(memcpy, handles_zero_size) {
    char dst[10] = "world";
    memcpy(dst, "hello", 0);
    EXPECT_STREQ(dst, "world");  // Unchanged
}

// For LibFK: tests/libfk/test_vector.cpp
TEST(Vector, push_back_increases_size) {
    fk::Vector v;
    v.push_back(42);
    EXPECT_EQ(v.size(), 1);
    EXPECT_EQ(v[0], 42);
}

// For Kernel: tests/kernel/test_pmm.cpp
TEST(PhysicalMemory, allocates_pages) {
    auto page = fkernel::memory::Physical::allocate_page();
    EXPECT(page.is_ok());
    EXPECT(page.value() != nullptr);
}
```

### Test Coverage Goals

- **LibC**: 90%+ coverage
- **LibFK**: 85%+ coverage
- **Kernel critical paths**: 75%+ coverage

---

## Performance Considerations

### Kernel Code Optimization

1. **Use RAII**: No manual cleanup
2. **Prefer stack allocation**: Heap only when necessary
3. **Inline hot paths**: Mark with `inline` or `__attribute__((always_inline))`
4. **Avoid virtual calls in hot paths**: Use CRTP if needed

```cpp
// Example: CRTP for static polymorphism
template
class BlockDevice {
public:
    Result read(u64 sector, void* buffer) {
        return static_cast(this)->read_impl(sector, buffer);
    }
};

class ATADevice : public BlockDevice {
public:
    Result read_impl(u64 sector, void* buffer) {
        // Implementation
    }
};
```

### Memory Management

- **No exceptions**: Freestanding environment
- **No RTTI**: Compile with `-fno-rtti`
- **No STL**: Use LibFK instead
- **Manual memory barriers**: When needed for hardware

---

## Hardware Interaction Guidelines

### Always Validate Against Specs

```cpp
// ✅ GOOD: Documented with spec reference
// From Intel AHCI 1.3.1 spec section 3.3.7
void initialize_port() {
    // Step 1: Clear ST bit (spec requirement)
    port->cmd &= ~PORT_CMD_START;

    // Step 2: Wait for CR to clear (max 500ms per spec)
    wait_until(!(port->cmd & PORT_CMD_CR), 500);

    // Step 3: Clear FRE
    port->cmd &= ~PORT_CMD_FRE;
    wait_until(!(port->cmd & PORT_CMD_FR), 500);
}
```

### Use Memory Barriers

```cpp
// ✅ GOOD: Proper ordering
void start_dma() {
    m_status = TransferInProgress;
    __sync_synchronize();  // Ensure status written before DMA start
    outb(m_bm_base + BM_COMMAND, BM_CMD_START);
}
```

### Handle MMIO Correctly

```cpp
// ✅ GOOD: Volatile for hardware registers
struct APICRegisters {
    volatile u32 id;
    volatile u32 version;
    volatile u32 tpr;
    // ...
} __attribute__((packed));
```

---

---

## Documentation Requirements

### Every Public API Needs Documentation

```cpp
/// @brief Allocates a physical page frame
///
/// @return Result containing page address on success,
///         Error::OutOfMemory if no pages available
///
/// @note This function is thread-safe
/// @warning Caller must free page with free_page()
///
/// @example
/// auto page = TRY(Physical::allocate_page());
/// // Use page...
/// Physical::free_page(page);
Result allocate_page();
```

---
