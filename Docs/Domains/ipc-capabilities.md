# IPC and Capabilities

## Overview

FKernel's IPC subsystem is inspired by **seL4's capability-based model**. Instead of traditional Unix IPC (pipes, signals, sockets), FKernel uses **capabilities** for fine-grained access control over communication channels.

## Architecture

```mermaid
flowchart TD
    subgraph "Process A"
        CSA["CSpace A"]
        CA1["Capability: Send+Receive"]
        CA2["Capability: Send only"]
        CSA --> CA1
        CSA --> CA2
    end
    subgraph "Kernel IPC Objects"
        EP["Endpoint<br/>Synchronous rendezvous"]
        NTF["Notification<br/>Async bitmask signal"]
        SHM["SharedMemory<br/>(planned)"]
    end
    subgraph "Process B"
        CSB["CSpace B"]
        CB1["Capability: Receive"]
        CB2["Capability: Manage"]
        CSB --> CB1
        CSB --> CB2
    end
    CA1 -->|"bidirectional"| EP
    CA2 -->|"send only"| EP
    CB1 -->|"receive only"| EP
    CB2 -->|"manage/revoke"| EP
    NTF -.->|"pipe/kqueue signals"| EP
```

## Capability Model

### Core Concepts
- **Capability**: An unforgeable token granting access to a kernel object
- **CSpace**: Per-task capability table (slot array with free-list)
- **Rights**: Bitmask controlling allowed operations on a capability
- **Revocation**: Invalidation of capabilities via generation counter

### Rights

| Right | Permission |
|-------|-----------|
| `Send` | Send a message through this capability |
| `Receive` | Receive a message through this capability |
| `Manage` | Modify/move/revoke this capability |

### CSpace

- Array of capability slots per task
- Free-list for O(1) slot allocation
- Each slot holds: pointer to kernel object, rights mask, generation counter
- `cspace_insert()` copies a capability between CSpaces with rights masking

### Revocation Mechanism

```mermaid
flowchart LR
    EP["Endpoint<br/>m_generation = 3"]
    CAP1["Capability A<br/>issued_gen = 3"]
    CAP2["Capability B<br/>issued_gen = 2"]
    REV["sys_cap_revoke()"]
    REV -->|"m_generation++"| EP
    EP -->|"gen mismatch"| CAP1["Valid ✓"]
    EP -->|"gen mismatch"| CAP2["Invalid ✗"]
```

No need to search all process CSpaces — just increment the generation counter and capabilities become invalid on next use.

## IPC Primitives

### Endpoint

- Bidirectional synchronous rendezvous channel
- Separate `m_senders` and `m_receivers` wait lists (both `IntrusiveList<Task>`)
- `send()`: blocks sender if no receiver waiting, otherwise delivers immediately
- `receive()`: blocks receiver if no sender waiting, otherwise delivers immediately
- Message passing via CPU registers (rdi, rsi, rdx, r10, r8, r9) — zero-copy for short messages

### MessageInfo

Packed into a single 64-bit register:

```
[Label (48 bits) | Length (12 bits) | Flags (4 bits)]
```

### Notification

- Non-blocking signaling mechanism using a 64-bit bitmask (`m_pending_bits`)
- `signal(bits)`: sets bits, wakes waiting task with accumulated bits
- `wait()`: returns pending bits immediately or blocks until signal
- `poll()`: non-blocking check, returns and clears pending bits
- Same generation-based revocation as Endpoint

### Signal Delivery

```mermaid
flowchart TD
    KILL["kill(target, signum)"]
    PENDING["Set pending bit on target"]
    WAKE{Target sleeping?}
    HANDLER{Has sigaction handler?}
    IGN{SIG_IGN?}
    DF{SIG_DFL?}
    TERM["Default: terminate"]
    STOP["SIGSTOP: stop task"]
    CONT["SIGCONT: resume task"]
    FRAME["Build KernelSignalFrame<br/>on user stack"]
    REDIR["Redirect to sa_handler<br/>via signal trampoline"]
    RET["sigreturn restores<br/>full register state"]

    KILL --> PENDING --> WAKE
    WAKE -->|Yes| HANDLER
    WAKE -->|No| HANDLER
    HANDLER --> IGN
    IGN -->|Yes| IGNORE["Ignore signal"]
    IGN -->|No| DF
    DF -->|Yes| TERM
    DF -->|Yes| STOP
    DF -->|Yes| CONT
    DF -->|No| FRAME --> REDIR
    REDIR --> RET
```

## IPC Flow

```mermaid
sequenceDiagram
    participant A as Process A
    participant EP as Endpoint
    participant B as Process B

    Note over A: sys_ipc_call(ep_handle, info)
    A->>EP: Lookup capability in CSpace<br/>Check Send right
    EP->>EP: No receiver waiting?
    EP->>A: Block sender (add to m_senders)

    Note over B: sys_ipc_receive(ep_handle)
    B->>EP: Lookup capability in CSpace<br/>Check Receive right
    EP->>EP: Sender A waiting!
    EP->>EP: deliver_message()<br/>Copy registers: rdi,rsi,rdx,r10,r8,r9
    EP->>B: Wake receiver with A's message
    EP->>A: Wake sender, resume A

    Note over A: A resumes with reply in rax
```

## Integration with Other Subsystems

```mermaid
flowchart TD
    IPC["IPC Core<br/>Endpoint, Notification"]
    PIPE["PipeNode<br/>Uses Notification for<br/>DATA_AVAILABLE / SPACE_AVAILABLE"]
    KQ["KQueue<br/>BSD event polling<br/>EVFILT_READ/WRITE"]
    PTY["PTY Master/Slave<br/>Blocking reads via<br/>Notification::wait()"]
    SIG["Unix Signals<br/>KernelSignalFrame<br/>sa_restorer trampoline"]
    PIPE --> IPC
    KQ --> IPC
    PTY --> IPC
    SIG --> IPC
```

### Pipes
- `PipeNode` uses separate `Notification` objects for DATA_AVAILABLE and SPACE_AVAILABLE
- Reader blocks via `Notification::wait()`, writer signals via `Notification::signal()`

### KQueue
- BSD-style event notification (not epoll)
- Integrates with scheduler for proper blocking with timeout
- Used by `select()`/`poll()` implementations

### PTY
- `PtyMaster`/`PtySlave` block reader via `Notification::wait()`
- Writer signals reader via `Notification::signal()`

## Key Design Decisions

- **seL4-style capabilities** over traditional Unix permission model
- **Revocation via generation counter** (not reference counting) — O(1) revocation
- **Separate send/receive wait nodes** to prevent corruption
- **Register-passing IPC** — short messages in CPU registers, no memory copies
- **OS-level integration**: signals, pipes, kqueue all use underlying IPC primitives

## Enhanced IPC Primitives (2026-07-26)

### Notification::wait_timeout()

Blocks with a deadline. Returns delivered bits or 0 on timeout. Uses `sleep_current(ticks)` under the hood, allowing the scheduler timer to wake the task. The caller checks list membership after waking to distinguish timeout from signal.

```cpp
uint64_t timeout_ticks = freq * timeout_ms / 1000;
uint64_t result = notif.wait_timeout(timeout_ticks);
if (result == 0) // timeout
if (result > 0)  // signal received, result = accumulated bits
```

Used by: pipes O_NONBLOCK, eventfd O_NONBLOCK, epoll_wait with timeout, futex FUTEX_WAIT with timeout.

### Notification::signal_with_payload()

Enqueues a `NotificationPayload` (64-byte data + bitmask) alongside the signal. Up to 16 payloads buffered in a circular queue. Wakeup delivers accumulated bits; payload is accessible on next `poll()`.

```cpp
siginfo_t si = {.si_signo = SIGCHLD, .si_pid = child_pid, ...};
notif.signal_with_payload(1 << SIGCHLD, &si, sizeof(si));
```

Used by: signal delivery (siginfo_t → SA_SIGINFO handlers), eventfd value preservation.

### Endpoint::call()

Atomic send+receive: sends a message and immediately waits for a reply without allowing another task to intercept. The kernel marks the caller as `m_call_sender` so the reply is routed directly back.

```cpp
auto result = endpoint.call(MessageInfo::create(label, len, flags));
// result contains the reply MessageInfo
```

### Endpoint::send_timeout() / receive_timeout()

Time-bounded variants. Return `Error::Timeout` on expiry. Use same `sleep_current()` mechanism as `Notification::wait_timeout()`.

### SharedMemory

Page-by-page physical memory sharing. Allocates individual 4KB pages via `PhysicalMemoryManager::alloc_page()`. Maps into multiple tasks' address spaces via `VirtualMemoryManager::map_page()`.

```cpp
auto* shm = SharedMemory::create(4096);  // 1 page
shm->map_into(task_a, 0x700000000000, PageFlags::Present | PageFlags::Writable | PageFlags::User);
shm->map_into(task_b, 0x700000000000, PageFlags::Present | PageFlags::User);
shm->revoke();  // invalidates all capability holders
```

Integrated via `ShmNode` VFS node at `/dev/shm/`. `mmap(MAP_SHARED, fd)` calls `shm->map_into()`.

## POSIX over IPC Architecture

All POSIX IPC mechanisms are implemented as VFS nodes backed by native IPC primitives:

```mermaid
flowchart TD
    subgraph "POSIX API"
        PIPE["pipe()/mkfifo()"]
        EVENT["eventfd()"]
        SEM["sem_open/wait/post"]
        MQ["mq_open/send/receive"]
        SHM["shm_open/mmap"]
        SIG["kill/sigaction"]
        FUTEX["futex()"]
        EPOLL["epoll_wait()"]
    end
    subgraph "VFS Nodes"
        PN["PipeNode<br/>ring buffer 64KB"]
        EN["EventFdNode<br/>uint64 counter"]
        SN["SemNode<br/>count + max"]
        MQN["MqueueNode<br/>priority queue"]
        SHMN["ShmNode<br/>SharedMemory ptr"]
        EP["EpollNode<br/>fd list"]
    end
    subgraph "IPC Substrate"
        NTF["Notification<br/>wait/wait_timeout<br/>signal_with_payload"]
        EPT["Endpoint<br/>send/receive/call<br/>register-based"]
        SHM["SharedMemory<br/>page-by-page<br/>map_into/unmap_from"]
    end
    PIPE --> PN --> NTF
    EVENT --> EN --> NTF
    SEM --> SN --> NTF
    MQ --> MQN --> NTF
    SHM --> SHMN --> SHM
    SIG --> NTF
    FUTEX --> NTF
    EPOLL --> EP --> NTF
```

### Namespace Layout

```
/dev/
├── pts/       # PTY slaves (existing)
├── sem/       # POSIX named semaphores (SemDirNode → SemNode)
├── mqueue/    # POSIX message queues (MqueueDirNode → MqueueNode)
└── shm/       # POSIX shared memory (ShmDirNode → ShmNode)
```

### Capability Transfer

Two new syscalls enable runtime capability sharing between processes:

- `sys_cap_transfer(pid, handle, rights_mask)` — moves capability from current CSpace to target
- `sys_cap_grant(pid, handle, rights_mask)` — copies capability (original stays)

Both require `Manage` rights on the source capability.
