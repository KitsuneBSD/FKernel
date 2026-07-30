# Development Workflow Guide

## Quick Start

1. **Read AI Memory**: Always check `.ai-docs/` for current state
2. **Understand Domain**: Identify which domain you're working in
3. **Follow SECRET RULE**: One struct/class per file
4. **Object Calisthenics**: Follow all 9 rules strictly
5. **Test Everything**: Ensure coverage before commit

## Development Phases

### Phase 1: Understanding
```
1. Read .ai-docs/architectural-decisions/
2. Read domain-knowledge/[domain].md
3. Read development-patterns/
4. Understand current issues in TODO.md
```

### Phase 2: Implementation
```
1. Create/modify exactly ONE struct/class per file
2. Follow Object Calisthenics rules
3. Use Result<T, Error> for error handling
4. Add comprehensive documentation
5. Write corresponding tests
```

### Phase 3: Validation
```
1. Run: xmake build
2. Run: lua .gemini/fkernel_validator.lua
3. Run: xmake run Test (when fixed)
4. Update .ai-docs/ with changes
5. Create/update Docs/ if needed
```

## Domain-Based Development

### Understanding Domains
Each directory in `Src/` and `Include/` represents a domain:

```
Src/Kernel/
├── Memory/           # Memory management domain
├── Driver/           # Hardware drivers domain
├── Fs/              # Filesystem domain
├── Hardware/         # Hardware abstraction domain
├── Ipc/             # Inter-process communication domain
└── Scheduler/       # Process scheduling domain
```

### Working Within Domains
1. **Identify Domain**: Know which domain you're modifying
2. **Respect Boundaries**: Don't cross-contaminate domains
3. **Use Interfaces**: Follow domain interface patterns
4. **Document Decisions**: Update domain knowledge

## File Organization Rules

### The SECRET RULE
**Exactly ONE struct or class per file.**

### Naming Conventions
- **Directories**: PascalCase (domains)
- **Files**: camelCase with single concept
- **Classes**: PascalCase matching file name

### Examples
```
Include/Kernel/Driver/Storage/
├── Ata/
│   ├── ataController.h     // class AtaController
│   ├── ataDevice.h         // class AtaDevice  
│   └── pioStrategy.h      // class PioStrategy
└── Ahci/
    ├── ahciController.h    // class AhciController
    └── ahciPort.h         // class AhciPort
```

## Object Calisthenics Quick Reference

### The 9 Rules
1. **One indentation level** per method
2. **No `else` keyword** - use early returns
3. **Wrap primitives** - create type-safe wrappers
4. **First-class collections** - encapsulate data structures
5. **One dot per line** - follow Law of Demeter
6. **No abbreviations** - use descriptive names
7. **Keep entities small** - ≤200 lines/class, ≤20 lines/method
8. **Max 2 instance variables** - prefer composition
9. **No getters/setters** - use rich domain methods

### Error Handling Pattern
```cpp
auto result = some_operation();
if (result.is_error())
    return result.error();

auto value = result.value();
// Continue with value
```

## Testing Requirements

### Coverage Targets
- **LibC**: 90% coverage required
- **LibFK**: 85% coverage required  
- **Kernel**: 0% coverage today — Phase 43 target: 75%+ for critical paths

### Test Structure
```
tests/
├── LibC/
│   ├── test_string_memory.cpp
│   └── test_stdio_comprehensive.cpp
├── LibFK/
│   ├── test_containers.cpp
│   └── test_memory.cpp
└── Kernel/
    ├── test_memory_manager.cpp
    └── test_scheduler.cpp
```

## Code Review Process

### Before Commit
1. **Validator Pass**: `lua .gemini/fkernel_validator.lua`
2. **Build Success**: `xmake build`
3. **Tests Pass**: `xmake run Test` (when fixed)
4. **Documentation Updated**: Both `.ai-docs/` and `Docs/`

### Review Checklist
- [ ] One struct/class per file
- [ ] Object Calisthenics compliance
- [ ] Proper error handling
- [ ] Comprehensive documentation
- [ ] Tests written and passing
- [ ] Domain boundaries respected

## Common Pitfalls

### ❌ Don't Do This
```cpp
// Multiple classes in one file
class Controller { /* ... */ };
class Device { /* ... */ };

// Using else keyword
if (condition) {
    do_something();
} else {
    do_other();
}

// Method chaining
auto value = obj.get_manager().get_device().get_status();
```

### ✅ Do This Instead
```cpp
// Separate files
// controller.h
class Controller { /* ... */ };

// device.h  
class Device { /* ... */ };

// Early returns instead of else
if (condition) {
    do_something();
    return;
}
do_other();

// Delegation instead of chaining
auto value = obj.device_status();
```

## Getting Help

1. **Read Memory**: Check `.ai-docs/` first
2. **Check Patterns**: Look in `development-patterns/`
3. **Domain Knowledge**: Read specific domain docs
4. **Architecture**: Review system architecture guides
5. **Ask**: Use question tool for clarification

---

This workflow ensures **consistent, maintainable, and high-quality** contributions to FKernel.