# TODO - FKernel

This document outlines the current and future tasks required to bring FKernel to a functional Unix-like state on bare-metal.

### Reaching a 1.0.0 Version

- [x] Improve consistency of log messages  
  - Unify formatting  
  - Use structured levels (e.g., `TRACE`, `ERROR`, `INFO`)  
  - Tag logs by subsystem/module (e.g., `[PMM]`, `[VMM]`, `[INIT]`)

- [ ] Audit the codebase for potential bugs and mark them with review flags:
  - `// TODO:` — for incomplete implementations  
  - `// FIXME:` — for known incorrect or suspicious behavior  

- [ ] Expand and harden **LibFK**
  - Improve robustness of existing data structures  
  - Add unit tests for `Bitmap`, `IntrusiveList`, `Optional`, `Span`, etc.  
  - Add missing core abstractions (e.g., `Vector`, `StringView`, `ErrorOr<T>`)

- [ ] Extend and optimize **LibC**
  - Improve performance of core functions (`memcpy`, `memset`, `memcmp`)  
  - Ensure compliance with freestanding constraints  
  - Replace unnecessary libc usage in kernel components with LibFK equivalents  

- [x] Remove the internal if validators.
    - The validators need be checked in operation input and operation output only
    - Reduce the complex function to most simple

- [ ] Create a function to inlining external assembly 

---

### Pro Tips for Contributors

- **Prefer external `.asm` files** over inline assembly whenever possible.  
  This improves portability, clarity, and maintainability.

- Use `// TODO:` and `// FIXME:` annotations to mark areas of the code that:
  - Need cleanup or redesign  
  - Are partially implemented  
  - Are potential sources of bugs or undefined behavior 
