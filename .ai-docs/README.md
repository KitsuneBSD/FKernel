# FKernel AI Memory System

## Overview

This directory serves as **AI conceptual memory** -- containing architectural decisions, recent modifications, development patterns, and domain knowledge that AI agents should read to understand the current state of FKernel before making changes.

## Memory Structure

```
.ai-docs/
+-- README.md                           # This file
+-- architectural-decisions/            # High-level design decisions
|   +-- capability-ipc.md                  # seL4-style capability model
|   +-- current-state-analysis.md         # Current project state (July 2026)
|   +-- comparative-analysis.md           # FKernel vs Linux/FreeBSD/seL4/SerenityOS
|   +-- hardcoded-values-removal.md       # Hardcoded values removal (HPET, PCI ECAM, ATA)
|   +-- kqueue-over-epoll.md              # Event notification design choice
|   +-- nvme-decomposition.md             # NVMe driver architecture
+-- development-patterns/               # Established patterns and conventions
|   +-- algorithm-consolidation.md      # Algorithm consolidation policy
|   +-- one-struct-per-file.md          # SECRET RULE documentation
+-- recent-modifications/               # Track recent code changes
```

**Note**: For design philosophy, see `Docs/Architecture/design-philosophy.md`.

## Memory Access Protocol

**AI agents MUST read this directory first** before making any changes to understand:

1. **Current state** of each domain
2. **Recent modifications** and their impact
3. **Architectural decisions** made over time
4. **Established patterns** and conventions
5. **Domain boundaries** and responsibilities

## Memory Updates

Every significant change should update corresponding memory files:

- **Architectural changes** -> `architectural-decisions/`
- **Code modifications** -> `recent-modifications/`
- **Pattern establishment** -> `development-patterns/`

## See Also

- `Docs/Architecture/` for system overview and design philosophy
- `Docs/Domains/` for per-domain guides
- `Docs/Development/` for workflow and getting started
- `AGENTS.md` for build commands and coding conventions

## Memory Principles

1. **Always current** - Memory reflects real system state
2. **Conceptual clarity** - Focus on understanding, not implementation details
3. **Domain boundaries** - Clear separation of concerns
4. **Historical context** - Why decisions were made
5. **Pattern documentation** - Established conventions
