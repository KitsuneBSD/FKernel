# Kernel Logging Conventions

> This file is AI-agent conceptual memory. Read before making changes to kernel logging code.

## Current Architecture

```
Application code
    ↓
fk::algorithms::klog/kwarn/kerror/kdebug/kexception  (log level + formatting)
    ↓
kprintf() [LibC]  (vsprintf to 512-byte buffer, calls libc_puts)
    ↓
libc_puts() [LibC]  (hook-based dispatch, protected by SpinlockIRQ)
    ↓
kernel_puts_impl()  (fan-out to up to 3 targets based on bitmask)
    ├── serial::write()          [COM1 hardware output]
    ├── vga::the().write_ansi()  [framebuffer/VGA ANSI renderer]
    └── DebugLogNode::append()   [in-memory ring buffer for dmesg]
```

## 4-Layer Pipeline Detail

### Layer 1: `klog/kwarn/kerror/kdebug/kexception` (LibFK)

Public API entry points defined in `Include/LibFK/Algorithms/log.h`. Each function accepts a compile-time prefix string and variadic format arguments. The function selects the appropriate log level tag (`[INFO]`, `[WARN]`, `[ERROR]`, `[DEBUG]`, `[EXCEPTION]`), prepends it along with the subsystem prefix, and forwards to `kprintf()` for formatting. `kerror()` additionally triggers a `cli;hlt` after output.

### Layer 2: `kprintf` (LibC)

`kprintf()` in `Src/LibC/stdio/kprintf.c` is a freestanding printf implementation. It formats the message into a 512-byte stack-allocated buffer using `vsprintf`, then calls `libc_puts()` with the completed string. No heap allocation occurs. Messages longer than 512 bytes are silently truncated.

### Layer 3: `libc_puts` (LibC)

`libc_puts()` in `Src/LibC/stdio/_impl/libc_putc.cpp` is the central dispatch point. It acquires a `SpinlockIRQ` to protect the log target bitmask and hook table, then iterates over registered output hooks. Each hook receives the formatted string for delivery to its backing target. The spinlock ensures consistent bitmask reads and prevents interleaved output when logging from interrupt context.

### Layer 4: `kernel_puts_impl` (Kernel)

`kernel_puts_impl()` in `Src/Kernel/Io/kernel_puts.cpp` is the kernel-side fan-out function registered as a hook by `libc_puts`. It reads the current log target bitmask and dispatches the formatted string to each enabled target:
- `serial::write()` — direct COM1 hardware output
- `vga::the().write_ansi()` — framebuffer/VGA ANSI escape sequence renderer
- `DebugLogNode::append()` — in-memory ring buffer for dmesg

Each target is checked against the bitmask before invocation, so disabling a target skips it entirely.

## Log Target Bitmask

Log targets are controlled by a bitmask stored in a global variable. Each bit enables one output path:

| Bit | Target | Availability |
|-----|--------|-------------|
| `0x1` | **Serial (COM1)** | Always available from earliest boot |
| `0x2` | **Display (VGA/framebuffer)** | Available after display initialization |
| `0x4` | **DebugFS (ring buffer)** | Available after VFS initialization |

**Runtime control**: Targets can be changed at any time via `set_log_targets(bits)`. This is thread-safe (protected by the same SpinlockIRQ used by `libc_puts`). Disabling a target mid-session stops output to that target immediately; re-enabling resumes it.

Default bitmask at boot is `0x7` (all targets), adjusted at each boot stage as hardware comes online.

## Color Support

`klog_color()` (defined in `Include/LibFK/Algorithms/log.h`) accepts ANSI escape sequences embedded in the log message. It forwards the raw string through the standard pipeline.

The Display target (`vga::the().write_ansi()`) interprets ANSI escape sequences and renders colored text to the VGA/framebuffer terminal. The Serial and DebugFS targets receive the raw string including escape codes; serial terminal emulators (e.g., minicom, picocom) may interpret them, while DebugFS stores them as-is.

## Thread Safety

- **`libc_puts` is protected by SpinlockIRQ** — safe to call from any context (thread, interrupt handler, preemption-disabled code). The lock disables interrupts on the local CPU while held, preventing deadlocks from interrupt-context logging on the same CPU.
- **Log target bitmask changes are safe** — `set_log_targets()` acquires the same SpinlockIRQ before writing.
- **DebugLogNode/SyscallLogNode/IpcLogNode `append()` is protected by ScopedLockIRQ** — concurrent appends from multiple CPUs or interrupt contexts are serialized by the per-node lock.

## Key Files

| File | Role |
|------|------|
| `Include/LibFK/Algorithms/log.h` | Log level functions (klog, kwarn, kerror, kdebug, kexception, klog_color) |
| `Src/LibC/stdio/kprintf.c` | Printf implementation, 512-byte stack buffer |
| `Src/LibC/stdio/_impl/libc_putc.cpp` | Central dispatch: hook registration, log target bitmask, SpinlockIRQ |
| `Src/Kernel/Io/kernel_puts.cpp` | Fan-out router: serial + VGA + DebugLogNode |
| `Include/Kernel/Io/kernel_puts.h` | Declares set_log_target_bits (NOT IMPLEMENTED) |
| `Src/Kernel/Fs/DebugFs/debug_fs.cpp` | DebugLogNode + SyscallLogNode ring buffers (with ScopedLockIRQ) |
| `Src/Kernel/Ipc/ipc_log_node.cpp` | IpcLogNode ring buffer (with ScopedLockIRQ) |
| `Src/Kernel/Arch/x86_64/Panic/Panic.cpp` | Panic output (bypasses logging system) |

## Known Issues

1. **No log-level filtering** — all levels always compiled in; 20 `kdebug()` calls commented out waiting for LogLevel feature
2. **Panic bypasses logging** — `Panic.cpp` uses raw `kprintf()`, messages never reach dmesg ring buffer
3. **`kerror()` halts on every call** — no distinction between fatal and non-fatal errors; should split into `kfatal()` (halt) and `kerror()` (non-halting)
4. **Inconsistent prefix naming** — ALL_CAPS, mixed case, lowercase, hyphenated all used across codebase
5. **Dead code** — StdoutLogNode/StderrLogNode headers exist but are never instantiated
6. **`set_log_target_bits()` declared but not implemented** in `kernel_puts.h`
7. **512-byte buffer truncation is silent** — long messages are cut off with no indication to caller or reader
8. **No log-level filter at runtime** — cannot dynamically suppress DEBUG/INFO from production without recompilation
9. **Color codes leak into serial/DebugFS output** — `klog_color()` escape sequences stored raw in ring buffer and sent to serial without stripping

## Proposed Log Levels

```
FATAL   — halts the system (cli;hlt)
ERROR   — non-halting error, requires attention
WARN    — warning, operation degraded but continues
INFO    — normal operational messages (init, state changes)
DEBUG   — verbose diagnostic output (gated behind LogLevel in release)
TRACE   — extremely verbose (function entry/exit)
```

## Conventions (When Writing New Code)

- **Prefix format**: `SUBSYSTEM_NAME` — always UPPER_SNAKE_CASE, max 20 chars
- **Prefix examples**: `SCHEDULER`, `VFS`, `MEMORY`, `NETWORK`, `DRIVER_AHCI`, `SYSCALL`
- **Error messages**: Include error code/name: `"Failed to mount: error=NOT_FOUND path=/dev/sda1"`
- **Init messages**: Log at INFO level during subsystem init
- **Debug messages**: Use `kdebug()` for hot paths; gate behind LogLevel
- **Panic path**: Always route through logging system before halt
- **Never use raw `kprintf()`** in kernel code — use `fk::algorithms::klog/kwarn/kerror`
- **Never use `kerror()` for recoverable errors** — use `kwarn()` or a future `kerror` that doesn't halt

## Log Target Management

| Boot Stage | Targets | File |
|-----------|---------|------|
| Default | Display \| DebugFS \| Serial | `libc_putc.cpp` |
| Early HW init | Serial only | `init.cpp:23` |
| After display ready | Serial \| Display | `init.cpp:43-44` |
| Idle task spawns init | Serial \| DebugFS \| Display (all) | `idle_task.cpp:23-25` |
