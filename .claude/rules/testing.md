---
paths:
  - "tests/**"
---

# Testing Rules

- Test files follow pattern: `tests/[component]/test_[module].cpp`
- Coverage targets: LibC 90%, LibFK 85%, Kernel critical paths 75%
- Build tests: `xmake -bv Test`
- Run tests: `xmake run Test`
- New LibFK/LibC PRs require test additions
- Use the custom test framework (see `tests/test_framework.h`)
