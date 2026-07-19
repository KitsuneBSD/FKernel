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
- Prefer stack allocation over heap
- No exceptions, no RTTI, no standard library
- Disable interrupts when accessing shared scheduler state
- Use `ScopedLockIRQ` for interrupt-safe locking
