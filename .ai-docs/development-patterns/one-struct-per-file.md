# SECRET RULE: One Struct/Class Per File

## The Golden Rule

**Every header/source file must contain exactly ONE struct or class definition.**

This is non-negotiable and enforced by GEMINI validators.

## Why This Rule Exists

### 1. Deep Autodocumentation
- File name immediately reveals its content
- No ambiguity about what's defined where
- Self-documenting codebase structure

### 2. Architectural Clarity
- Each concept lives in its own space
- Clear boundaries between components
- Easy to understand system composition

### 3. Maintenance Isolation
- Changes to one struct don't affect others
- Reduced merge conflicts
- Clear responsibility ownership

### 4. Discovery & Navigation
- Developers can find functionality by file name
- IDE support works optimally
- Code review becomes trivial

## Enforcement

The GEMINI validator checks:
- **Count**: Exactly one `class` OR one `struct` per file
- **Name**: File should match the struct/class name (camelCase → PascalCase)
- **Scope**: No nested class/struct definitions as primary types

## Examples

### ✅ CORRECT
```cpp
// cpu_context.h
class CpuContext {
    // CPU context implementation
};

// ata_controller.h  
class AtaController {
    // ATA controller implementation
};
```

### ❌ FORBIDDEN
```cpp
// devices.h
class AtaController { /* ... */ };
class NvmeController { /* ... */ };  // ❌ Multiple classes

// cpu_context.h
class CpuContext { /* ... */ };
struct CpuRegister { /* ... */ };      // ❌ Mixed types
```

## Directory Integration

With domain-based directories, this creates a **perfectly discoverable structure**:

```
Include/Kernel/Driver/Storage/
├── Ata/
│   ├── ata_controller.h    // class AtaController
│   ├── ata_device.h        // class AtaDevice
│   └── pio_strategy.h     // class PioStrategy
├── Ahci/
│   ├── ahci_controller.h   // class AhciController  
│   └── ahci_port.h        // class AhciPort
└── Nvme/
    ├── nvme_controller.h   // class NvmeController
    └── nvme_queue.h        // class NvmeQueue
```

## Domain Knowledge

Each domain directory contains **cohesive concepts** that work together:
- **Domain**: High-level responsibility area
- **Files**: Individual concepts within that domain
- **Structure**: Self-documenting hierarchy

## Compliance

All new code MUST follow this rule. Existing violations are being refactored incrementally.

**This rule is the foundation of FKernel's maintainable architecture.**