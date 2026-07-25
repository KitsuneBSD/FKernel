---
paths:
  - "Src/Kernel/**"
  - "Include/Kernel/**"
---

# Kernel Code Rules

- Kernel uses ONLY LibFK, NEVER LibC directly
- Use `Result<T, Error>` for fallible operations, `TRY` macro for propagation
- Use `volatile` for all MMIO hardware registers
- Use memory barriers (`__sync_synchronize()`) when interacting with hardware
- Object Calisthenics mandatory: no else, max 2 instance vars, max 20 lines/method
- One struct/class per file (SECRET RULE)
- **File naming: ALL files must be `snake_case`** (e.g., `scheduler_manager.cpp`, not `SchedulerManager.cpp`)
- Prefer stack allocation over heap
- No exceptions, no RTTI, no standard library
- Disable interrupts when accessing shared scheduler state
- Use `ScopedLockIRQ` for interrupt-safe locking

## Subsystem Manager Pattern

All kernel subsystem managers MUST follow the canonical singleton pattern:

```cpp
namespace fkernel {

class FooManager {
  bool m_is_initialized{false};

  FooManager() = default;
  FooManager(const FooManager &) = delete;
  FooManager &operator=(const FooManager &) = delete;

public:
  static FooManager &the() {
    static FooManager inst;
    return inst;
  }

  bool is_initialized() const { return m_is_initialized; }
  void initialize(); // sets m_is_initialized = true at end
};

} // namespace fkernel
using fkernel::FooManager;
```

Rules:
- **Private constructor** — Meyer's singleton only; no external instantiation
- **Delete copy/move** — singletons must not be copied or moved
- **`is_initialized()`** — public accessor so boot flow can assert on success
- **`fkernel::` namespace** — all managers live in `fkernel::`; a `using` alias at the bottom preserves backward compat
- **Set `m_is_initialized = true`** at the end of `initialize()`
- **Double-init guard** — early-return if already initialized, log a `kdebug()`
- After each `initialize()` call in `init.cpp`, assert: `if (!Mgr::the().is_initialized()) kfatal(...)`

## Architecture Changes

Before modifying existing kernel architecture:

1. Search for existing abstractions (grep codebase for patterns, interfaces)
2. Reuse if possible — composition/extension over new constructs
3. If new abstraction is needed, justify:
   - Why existing abstractions are insufficient
   - Trade-offs (complexity, performance, maintenance)
   - Impact on layering (LibC/LibFK/Kernel boundaries)
   - Migration path from current code
