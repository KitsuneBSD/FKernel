# Syscall Organization

> AI-agent conceptual memory. Read before adding or modifying syscall handlers.

## Structure

Syscalls are organized by domain in `Src/Kernel/Syscall/syscall_list/`:

```
syscall_list/
├── FileSystem/    (52 files — open, read, write, mount, epoll, kqueue, ...)
├── Process/       (35 files — fork, execve, clone, wait4, setpgid, ...)
├── Networking/    (16 files — socket, bind, connect, sendmsg, ...)
├── Memory/        (6 files — mmap, mprotect, brk, madvise, mlock, msync)
├── Time/          (7 files — clock_gettime, nanosleep, setitimer, ...)
├── Signals/       (5 files — tgkill, sigaltstack, sigpending, ...)
├── Posix/         (3 files — futex, openpty, signal)
├── System/        (4 files — uname, reboot, getrandom, syslog)
├── Ipc/           (4 files — ipc_call, ipc_send, ipc_receive, cap_revoke)
└── Terminal/      (3 files — tty_create, tty_delete, tty_list)
```

**Total**: ~135 syscall handlers across 10 domains.

## Adding a New Syscall

1. Choose the correct domain directory
2. Create `Src/Kernel/Syscall/syscall_list/<Domain>/<name>.cpp`
3. Implement the handler function
4. Register in `Src/Kernel/Syscall/syscall.cpp` dispatch table
5. Add number to `Include/Kernel/Syscall/syscall_numbers.h` (must match Linux x86_64)

## Handler Pattern

Every handler follows this pattern:

```cpp
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Core/result.h>

// Handler function signature
SyscallResult sys_<name>(uint64_t arg1, uint64_t arg2, uint64_t arg3,
                          uint64_t arg4, uint64_t arg5, uint64_t arg6) {
  // Validate arguments
  // Perform operation
  // Return result via SyscallResult
}
```

## Key Files

| File | Role |
|------|------|
| `Src/Kernel/Syscall/syscall.cpp` | Dispatch table (maps numbers to handlers) |
| `Include/Kernel/Syscall/syscall_numbers.h` | Linux x86_64 syscall numbers |
| `Include/Kernel/Syscall/syscall.h` | SyscallManager class |
| `Include/Kernel/Syscall/syscall_types.h` | SyscallResult type |
| `Include/Kernel/Syscall/syscall_utils.h` | Helper macros and validation |
| `Src/Kernel/Arch/x86_64/Syscall/syscall_init.cpp` | MSR setup (STAR, LSTAR) |
| `Src/Kernel/Arch/x86_64/Syscall/syscall_stub.asm` | Entry/exit stub |
| `Src/Kernel/Arch/x86_64/Syscall/syscall_stub_validation.cpp` | Argument validation |

## Gotchas

- Syscall numbers MUST match Linux x86_64 exactly — userspace binaries depend on them
- `syscall_stub_validation.cpp` validates pointer arguments before kernel access (SMAP protection)
- The stub switches from user stack to kernel stack automatically
- `kerror()` in a syscall handler halts the entire system — use `kwarn()` + error return instead
