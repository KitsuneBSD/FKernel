---
paths:
  - "Src/LibC/**"
  - "Include/LibC/**"
---

# LibC Rules

- Pure C (C17), no C++ constructs in .c files
- Freestanding only: no kernel or LibFK dependencies
- Follow standard POSIX function signatures where applicable
- All functions must have tests in `tests/`
- Use `kernel_` prefix for test symbol renaming (see xmake.lua LibC_Testing target)
