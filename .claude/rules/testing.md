---
paths:
  - "tests/**"
---

# Testing Rules

- Test files follow pattern: `tests/[component]/test_[module].cpp`
- Test files must use `snake_case` naming (same as all source files)
- Coverage targets: LibC 90%, LibFK 85%, Kernel critical paths 75%
- Build tests: `xmake -bv Test`
- Run tests: `xmake run Test`
- New LibFK/LibC PRs require test additions
- **Kernel changes MUST include regression tests** for the modified subsystem
- Always test edge cases: null inputs, boundary values, error paths, concurrent access
- Use the custom test framework (see `tests/test_framework.h`)
