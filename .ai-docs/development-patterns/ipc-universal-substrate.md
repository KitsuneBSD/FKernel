# IPC-as-Universal-Substrate Pattern

## Principle

Every POSIX IPC mechanism (pipes, signals, semaphores, message queues, shared memory, eventfd, epoll, futex) is implemented as a **VFS node** backed by one or more native **IPC primitives** (Notification, Endpoint, SharedMemory). This eliminates per-mechanism blocking logic and provides a unified concurrency model.

## Pattern

```
POSIX API → VFS Node → IPC Primitive(s) → Scheduler
```

- **Blocking** → `Notification::wait()` or `Notification::wait_timeout()`
- **Waking** → `Notification::signal()` or `Notification::signal_with_payload()`
- **Data transfer** → ring buffer in VFS node (pipes), shared pages (shm), register passing (endpoints)
- **Access control** → Capability in CSpace

## VFS Node Template

Each POSIX IPC VFS node follows this template:

```cpp
class XxxNode : public Node {
    Spinlock m_lock;           // protects state
    ipc::Notification m_read;  // reader/consumer blocks
    ipc::Notification m_write; // writer/producer blocks (optional)
    bool m_nonblock{false};    // O_NONBLOCK flag
    
    // read() → m_read.wait() or wait_timeout(0) if nonblock
    // write() → m_write.wait() or wait_timeout(0) if nonblock
    // poll() → check state without blocking
};
```

## Non-blocking Convention

```cpp
if (m_nonblock)
    return Error::WouldBlock;  // instead of wait()
m_notif.wait();                // otherwise block
```

The nonblock flag is set by the syscall layer (e.g., `sys_eventfd2` checks `EFD_NONBLOCK`).

## Namespace Convention

Named IPC objects live in `/dev/` subdirectories as DevFs children:

```
/dev/sem/    → SemDirNode (registered via DevFs::register_device("sem"))
/dev/mqueue/ → MqueueDirNode (registered via DevFs::register_device("mqueue"))
/dev/shm/    → ShmDirNode (registered via DevFs::register_device("shm"))
```

Each directory node overrides `is_directory()`, `lookup()`, `list_dir()`, and `create_child()`.

## Timeout Pattern

Both Notification and Endpoint use the same timeout mechanism:

1. Add task to internal wait list (Notification::m_waiting_tasks / Endpoint::m_senders or m_receivers)
2. Call `SchedulerManager::sleep_current(ticks)` — task goes to SleepQueue
3. On wakeup (by signal or timer), check list membership:
   - Still on list → timeout (remove self, return 0/Error::Timeout)
   - Not on list → signal() removed us, result is in `task->registers().rax`

This pattern avoids modifying the scheduler and reuses the existing sleep_current/wake_task mechanism.

## Anti-Patterns

- ❌ Per-mechanism spinlock + custom wait queue (use Notification instead)
- ❌ Busy-polling with sleep_current(1) (use wait_timeout with deadline)
- ❌ Static hash tables with linear probing for waiters (use Notification hash)
- ❌ Custom blocking logic in syscall handlers (delegate to VFS node)
