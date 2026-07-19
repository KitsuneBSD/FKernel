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
- Prefer RAII, stack allocation, no exceptions
- Containers: use `m_` prefix for members, snake_case methods
- Smart pointers: `OwnPtr` (exclusive), `RefPtr` (shared/intrusive), `RetainPtr` (shared/non-intrusive)
