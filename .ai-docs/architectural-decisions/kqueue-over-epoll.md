# kqueue Over epoll

> AI-agent conceptual memory. Read before modifying event notification code.

## Decision

FKernel implements BSD kqueue instead of Linux epoll, despite using Linux syscall ABI.

## Rationale

| Aspect | Linux epoll | BSD kqueue |
|--------|-------------|------------|
| Event types | Files only | Files, signals, timers, processes, VM, etc. |
| Scalability | O(n) scan on trigger | O(1) notification |
| API | epoll_create + epoll_ctl + epoll_wait | Single kevent() call |
| Kernel complexity | Moderate | Lower |
| Userspace compat | Native Linux | Needs shim for musl/BusyBox |

kqueue is more unified and simpler to implement correctly. The trade-off is userspace compatibility.

## Compatibility Layer

musl and BusyBox expect `epoll_create`, `epoll_ctl`, `epoll_wait` syscalls. FKernel provides compatibility by mapping these to kqueue internally:

- `epoll_create()` → allocates kqueue
- `epoll_ctl()` → translates to kevent registration
- `epoll_wait()` → translates to kevent wait

This is transparent to userspace.

## Key Files

| File | Role |
|------|------|
| `Include/Kernel/Fs/Vfs/Events/kqueue.h` | kqueue implementation |
| `Src/Kernel/Fs/Vfs/Events/kqueue.cpp` | kqueue operations |
| `Src/Kernel/Syscall/syscall_list/FileSystem/EventOps/epoll.cpp` | epoll→kqueue shim |
| `Src/Kernel/Syscall/syscall_list/FileSystem/EventOps/kqueue.cpp` | Native kqueue syscall |
| `Src/Kernel/Syscall/syscall_list/FileSystem/EventOps/kevent.cpp` | kevent syscall |
| `Src/Kernel/Syscall/syscall_list/FileSystem/EventOps/poll.cpp` | poll→kqueue shim |
| `Src/Kernel/Syscall/syscall_list/FileSystem/EventOps/select.cpp` | select→kqueue shim |

## When Modifying

- When adding a new event type, add it to kqueue filter list
- The epoll shim translates EPOLLIN→EVFILT_READ, EPOLLOUT→EVFILT_WRITE
- poll() and select() also map through kqueue for unified implementation
- Don't bypass kqueue for event notification — it's the single source of truth
