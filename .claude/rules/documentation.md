# Documentation Rules

When modifying files in this project, follow these documentation maintenance rules:

## Mandatory Actions

1. **Update Docs/ when modifying kernel subsystems** — keep documentation in sync with code changes
2. **Use Mermaid for ALL diagrams** in `Docs/` — no ASCII art, no other diagram tools
3. **Use MathJax** (`$$...$$`) for mathematical formulas in `Docs/`
4. **Read source code first** before writing or updating any `Docs/` file
5. **Fill empty README stubs** under `Docs/Kernel/` with actual content
6. **Doc files must use `snake_case`** naming (e.g., `getting-started.md`, not `GettingStarted.md`)

## Docs/ File Map

| File | Subsystem | When to Update |
|------|-----------|---------------|
| `Docs/Architecture/system-overview.md` | Overall | Major architectural changes |
| `Docs/Domains/process-scheduling.md` | Scheduler | Scheduler, task, signal changes |
| `Docs/Domains/vfs-architecture.md` | VFS | VFS, mount, filesystem changes |
| `Docs/Domains/ipc-capabilities.md` | IPC | IPC, capability, signal changes |
| `Docs/Domains/memory-management-guide.md` | Memory | PMM, VMM, heap changes |
| `Docs/Domains/drivers-framework.md` | Drivers | PCI, storage, network driver changes |
| `Docs/Domains/networking.md` | Network | TCP/IP, socket, protocol changes |
| `Docs/Domains/elf-loader.md` | ELF | ELF loading, ASLR, TLS changes |
| `Docs/Kernel/Boot/README.md` | Boot | Boot sequence changes |
| `Docs/Kernel/Process/README.md` | Process | Process lifecycle changes |
| `Docs/Kernel/Syscalls/README.md` | Syscalls | New/modified syscalls |
| `Docs/Kernel/VFS/README.md` | VFS | VFS operations changes |
| `Docs/directory-structure.md` | Structure | New files/directories added |

## Quality Rules

- **Accuracy**: Docs must match actual source code — verify before writing
- **Cross-linking**: Link between related Docs/ files using relative markdown links
- **Staleness check**: When a subsystem changes, verify all related Docs/ files are still accurate
- **No placeholders**: Don't leave "TBD" or "coming soon" — either fill it or remove the section
