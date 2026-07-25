---
paths:
  - "Src/LibFK/**"
  - "Include/LibFK/**"
---

# LibFK Rules

- LibFK uses LibC + self only, NEVER Kernel headers
- Use `fk::` namespaces: `fk::containers`, `fk::text`, `fk::memory`, `fk::core`
- All public APIs must have tests in `tests/`
- Use `Result<T, Error>` for fallible operations
- **File naming: ALL files must be `snake_case`** (e.g., `error.h`, not `Error.h`)
- Prefer RAII, stack allocation, no exceptions
- Containers: use `m_` prefix for members, snake_case methods
- Smart pointers: `OwnPtr` (exclusive), `RefPtr` (shared/intrusive), `RetainPtr` (shared/non-intrusive)

## Architecture Changes

Before modifying existing LibFK architecture:

1. Search for existing abstractions (grep codebase for patterns, interfaces)
2. Reuse if possible — composition/extension over new constructs
3. If new abstraction is needed, justify:
   - Why existing abstractions are insufficient
   - Trade-offs (complexity, performance, maintenance)
   - Impact on layering (LibC/LibFK boundaries)
   - Migration path from current code
