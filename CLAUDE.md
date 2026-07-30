# FKernel

This project uses `AGENTS.md` as its single source of truth for build commands,
coding conventions, and development rules. All AI agents MUST read that file first.

See `AGENTS.md` for:
- Build commands
- Layer separation (LibC → LibFK → Kernel)
- Namespace structure
- Object calisthenics (no nested types, one class per file)
- Architecture portability (arch_* extern "C" pattern)
- Subsystem manager pattern (FooManager::the())
- Coding style
- Testing requirements
- Kernel logging conventions
- Architecture change policy
