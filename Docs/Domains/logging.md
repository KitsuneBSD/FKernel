# Kernel Logging Architecture

## Overview

FKernel uses a structured logging pipeline that routes messages to multiple output targets (serial, display, ring buffer). The system is designed for debugging kernel boot and runtime behavior.

## Pipeline

```mermaid
graph TD
    subgraph "Application Layer"
        A1["klog(\"VFS\", \"Mounted %s\", path)"]
        A2["kwarn(\"NVME\", \"Size mismatch\")"]
        A3["kerror(\"MEM\", \"Alloc failed\")"]
    end
    subgraph "LibC Layer"
        B["kprintf() → vsnprintf(512B buffer) → libc_puts()"]
    end
    subgraph "Kernel Layer"
        C["kernel_puts_impl()"]
        D["Serial (COM1)"]
        E["Display (VGA/ANSI)"]
        F["DebugLogNode (64KB ring)"]
    end
    A1 --> B
    A2 --> B
    A3 --> B
    B --> C
    C --> D
    C --> E
    C --> F
```

## Design Decisions

### Why fan-out to multiple targets?
- **Serial**: Always available, works before display init, captured in `logs/serial.log`
- **Display**: Visual feedback during development
- **DebugLogNode**: Persistent ring buffer accessible via `dmesg` (sys_syslog nr 103)

### Why SpinlockIRQ for libc_puts?
The logging path is called from interrupt handlers, scheduler, and normal code. `SpinlockIRQ` prevents deadlocks when an interrupt fires while the lock is held.

### Why hook-based dispatch?
The LibC layer (`libc_puts`) cannot depend on Kernel headers. The hook pattern (`libc_register_puts_hook`) allows the Kernel to register its fan-out function without violating layer separation.

## Log Levels

| Level | Function | Behavior | When to Use |
|-------|----------|----------|-------------|
| FATAL | `kerror()` | Halts CPU | Unrecoverable: page table corruption, triple fault |
| ERROR | `kerror()` → future `kfatal()` | Halts CPU | Should be non-halting after refactor |
| EXCEPTION | `kexception()` | Returns | Exception handler output (does not halt) |
| WARN | `kwarn()` | Returns | Degraded but continues: sector size mismatch, timeout |
| INFO | `klog()` | Returns | State changes: mount, init, connection |
| DEBUG | `kdebug()` | Returns | Diagnostic: function entry, buffer contents |

## DebugFs Ring Buffers

| Node | Buffer Size | DebugFs Path | Content |
|------|------------|--------------|---------|
| DebugLogNode | 64 KB | `/debug/klog` | All kernel log output |
| SyscallLogNode | 128 KB | `/debug/syscalls` | Syscall entry/exit tracing |
| IpcLogNode | 64 KB | `/debug/ipc` | IPC endpoint/notification/signal events |

## dmesg Integration

The `syslog()` syscall (nr 103) provides Linux-compatible `dmesg` access:

```cpp
// SYSLOG_ACTION_READ_ALL (3) — read ring buffer
// SYSLOG_ACTION_SIZE_BUFFER (9) — get buffer size
// SYSLOG_ACTION_SIZE_UNREAD (10) — get unread bytes
```

## Current Limitations

1. ~~No runtime log-level filtering~~ **Implemented** — compile-time `FKERNEL_LOG_LEVEL` + runtime `get_log_level()` check
2. No compile-time log stripping in release builds
3. Panic output bypasses the logging system
4. `kerror()` halts on every call — no non-halting error level; proposed split: `kfatal()` (halt) + `kerror()` (non-halting)
5. 512-byte message truncation is silent

## Future: Proposed Log Levels

```
FATAL   — halts the system (cli;hlt) — current kerror() behavior
ERROR   — non-halting error, requires attention
WARN    — warning, operation degraded but continues
INFO    — normal operational messages (init, state changes)
DEBUG   — verbose diagnostic output (gated behind LogLevel in release)
TRACE   — extremely verbose (function entry/exit)
```

## See Also

- [Kernel Logging README](../Kernel/Logging/README.md) — file reference and API
- [Logging Development Pattern](../../.ai-docs/development-patterns/kernel-logging.md) — AI agent conventions
