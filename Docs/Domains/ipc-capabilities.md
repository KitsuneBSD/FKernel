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

## Security Properties

- Capabilities are kernel-managed, user-space cannot forge them
- Rights bitmask prevents privilege escalation
- Revocation is immediate and complete (generation mismatch)
- CSpace isolation between processes
- No ambient authority — all operations require explicit capability
