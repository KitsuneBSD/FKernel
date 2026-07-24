@AGENTS.md

## Claude Code

- Use plan mode for architectural changes (memory, scheduler, VFS redesign)
- **Before changing existing architecture**:
  1. Search for an existing abstraction — grep the codebase
  2. Reuse it if possible — prefer composition/extension
  3. If introducing new abstraction, justify: why existing is insufficient, trade-offs, impact on layering, migration path
- Run `xmake` to verify kernel builds after kernel changes
- Run `xmake -bv Test && xmake run Test` after modifying LibFK or LibC
- Run `xmake check-layers` after modifying Kernel files to verify layer separation
- See @.claude/rules/ for path-specific coding guidelines

## File Naming Rules

- **All source/header files MUST use `snake_case`** — see AGENTS.md "File Naming (ENFORCED)"
- When creating new files, always use `snake_case` naming
- Renames must use `git mv` and update all `#include` references
- Naming cleanup tracked in TODO.md Phase 22

## Kernel Logging Rules

- **Always use log functions**: `fk::algorithms::klog/kwarn/kerror/kdebug` — never raw `kprintf()` in kernel code
- **Prefix convention**: `UPPER_SNAKE_CASE` subsystem name, max 20 chars (e.g., `SCHEDULER`, `VFS`, `NVME_REG`)
- **Error messages**: Include context: `"Failed to mount: error=NOT_FOUND path=/dev/sda1"`
- **Init messages**: Log at INFO level during subsystem initialization
- **Don't use `kerror()` for recoverable errors** — it halts the CPU; use `kwarn()` instead
- **Debug messages**: Use `kdebug()` for diagnostic output; these may be stripped in release builds
- See `Docs/Kernel/Logging/README.md` for full architecture
- See `.ai-docs/development-patterns/kernel-logging.md` for conventions

## Layer Separation Rules

- Kernel code MUST NOT include `<LibC/...>` headers — use LibFK equivalents (see AGENTS.md mapping table)
- LibFK MUST NOT include `<Kernel/...>` headers — use allocator backend pattern for callbacks
- Run `xmake check-layers` after modifying Kernel or LibFK files
- Only exceptions: `Panic.cpp` and `kernel_puts.cpp` (documented in AGENTS.md)

## Testing Rules

- **Kernel changes MUST include regression tests** for the modified subsystem
- **Edge cases**: Always test null inputs, boundary values, error paths, concurrent access
- Run `xmake -bv Test && xmake run Test` after any LibFK or LibC changes

## Documentation Rules

- **Mermaid only**: All diagrams in Docs/ must use Mermaid syntax
- **MathJax**: Use `$$...$$` for mathematical formulas in Docs/
- **No ASCII art**: Replace any ASCII diagrams with Mermaid equivalents
- **Source-first**: Read actual source code before writing/updating Docs/
- **Keep in sync**: When modifying kernel code, update the matching Docs/ file
- **Cross-link**: Use relative markdown links between Docs/ files
- **Stubs must be filled**: Empty README.md files under Docs/Kernel/ must have content
