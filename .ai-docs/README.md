# FKernel AI Memory System

## Overview

This directory serves as **GEMINI's conceptual memory** - containing all architectural decisions, recent modifications, and domain knowledge that AI agents should read to understand the current state of FKernel.

## Memory Structure

```
.ai-docs/
├── architectural-decisions/     # High-level design decisions
├── recent-modifications/        # Track recent code changes  
├── conceptual-models/           # How system works conceptually
├── domain-knowledge/            # Per-domain understanding
└── development-patterns/        # Established patterns and conventions
```

## Memory Access Protocol

**GEMINI agents MUST read this directory first** before making any changes to understand:

1. **Current state** of each domain
2. **Recent modifications** and their impact
3. **Architectural decisions** made over time
4. **Established patterns** and conventions
5. **Domain boundaries** and responsibilities

## Memory Updates

Every significant change must update corresponding memory files:

- **Architectural changes** → `architectural-decisions/`
- **Code modifications** → `recent-modifications/`  
- **New concepts** → `conceptual-models/`
- **Domain changes** → `domain-knowledge/`
- **Pattern establishment** → `development-patterns/`

## Memory Principles

1. **Always current** - Memory reflects real system state
2. **Conceptual clarity** - Focus on understanding, not implementation details
3. **Domain boundaries** - Clear separation of concerns
4. **Historical context** - Why decisions were made
5. **Pattern documentation** - Established conventions

---

**This memory system ensures GEMINI agents have accurate, up-to-date conceptual understanding of FKernel before making changes.**