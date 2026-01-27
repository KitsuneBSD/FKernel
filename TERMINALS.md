# Terminal Management System

The FKernel now supports dynamic creation and deletion of TTYs through a redesigned terminal management system.

## Overview

The old static TTY system (fixed 6 TTYs in `/Fs/DevFs/`) has been replaced with a dynamic system located in `/Driver/Terminal/` that allows:

- **On-demand TTY creation** - Create terminals as needed
- **Dynamic TTY deletion** - Remove terminals when no longer needed  
- **Multiple terminal types** - VGA, Serial (future), PTY (future)
- **Proper driver architecture** - Separated from filesystem logic
- **Syscall interface** - User-space terminal management

## Architecture

### Core Components

#### `TerminalManager` (Singleton)
- Manages terminal lifecycle
- Handles dynamic creation/deletion
- Integrates with DevFs for device registration

#### `Terminal` (Abstract Base)
- Common interface for all terminal types
- Defines capabilities and operations
- Supports different I/O device attachments

#### `VGATerminal` (Concrete Implementation)
- VGA + PS/2 keyboard terminal
- Inherits from `Terminal`
- Maintains compatibility with existing TTY functionality

#### `TerminalFactory`
- Factory pattern for creating different terminal types
- Extensible design for future terminal types

#### Syscall Interface
- `SYS_TTY_CREATE (500)` - Create new terminal
- `SYS_TTY_DELETE (501)` - Delete existing terminal  
- `SYS_TTY_LIST (502)` - List all terminals

## API Reference

### Syscalls

#### `int tty_create(int type, const char* name_hint)`
Create a new terminal of the specified type.

**Parameters:**
- `type`: Terminal type (0=VGA, 1=Serial, 2=PTY)
- `name_hint`: Suggested name (e.g., "99" for "tty99")

**Returns:** Terminal ID (>0) or negative error code

#### `int tty_delete(int terminal_id)`
Delete an existing terminal.

**Parameters:**
- `terminal_id`: ID returned by `tty_create()`

**Returns:** 0 on success, negative error on failure

#### `int tty_list(unsigned int* terminal_ids, int max_count)`
List all active terminals.

**Parameters:**
- `terminal_ids`: Array to store terminal IDs
- `max_count`: Maximum number of IDs to store

**Returns:** Number of terminals found

### C++ API

```cpp
// Create terminal
auto result = TerminalManager::the().create_terminal(TerminalType::VGA, "99");
if (result.is_ok()) {
    TerminalId id = result.value();
    // Use terminal...
}

// Delete terminal  
TerminalId id(1);
auto result = TerminalManager::the().delete_terminal(id);

// Find terminal
auto terminal = TerminalManager::the().find_vga_terminal(id);
```

## Usage Examples

### Basic Terminal Creation
```c
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define SYS_TTY_CREATE  500
#define SYS_TTY_DELETE  501  
#define SYS_TTY_LIST    502

// Create VGA terminal "tty99"
int tty_id = syscall(SYS_TTY_CREATE, 0, "99");
if (tty_id > 0) {
    printf("Created terminal: /dev/tty99\n");
    
    // Use terminal like any device file
    FILE* f = fopen("/dev/tty99", "w");
    fprintf(f, "Hello from dynamic TTY!\n");
    fclose(f);
}
```

### Terminal Management
```c
// List terminals
unsigned int terminals[32];
int count = syscall(SYS_TTY_LIST, terminals, 32);

for (int i = 0; i < count; i++) {
    printf("Terminal ID: %u\n", terminals[i]);
}

// Delete terminal
syscall(SYS_TTY_DELETE, tty_id);
```

## Migration Guide

### From Static TTYs
The old system hardcoded 6 TTYs (tty0-tty5). The new system:

1. **Creates default terminals** on startup (maintains compatibility)
2. **Allows dynamic creation** of additional terminals
3. **Supports custom naming** through `name_hint`
4. **Properly handles cleanup** when terminals are deleted

### File Locations
- **Old:** `/Fs/DevFs/tty.cpp/h` 
- **New:** `/Driver/Terminal/` directory

### Namespace Changes
- **Old:** `fkernel::TTYDevice`
- **New:** `fkernel::terminal::VGATerminal`

## Future Extensions

The architecture supports easy addition of:

1. **Serial Terminals** - COM port based terminals
2. **PTY (Pseudo-terminals)** - For virtualization and SSH
3. **Network Terminals** - Telnet/SSH based
4. **GUI Terminals** - Graphical console implementations

## Testing

Build and run the terminal demo:
```bash
xmake build
xmake run
# In shell:
terminal_demo
```

The demo shows:
- Listing existing terminals
- Creating a dynamic terminal
- Using the terminal
- Cleaning up

## Implementation Details

### Memory Management
- Uses `fk::memory::OwnPtr` for automatic cleanup
- No manual memory management required
- RAII pattern throughout

### Thread Safety
- Terminal operations are not yet thread-safe
- Future version will add proper synchronization

### Error Handling
- Uses FKernel's `Result<T, Error>` pattern
- Detailed error reporting via syslog
- Graceful degradation on failures

---

**Note:** This system maintains full backward compatibility with existing TTY functionality while adding powerful dynamic management capabilities.