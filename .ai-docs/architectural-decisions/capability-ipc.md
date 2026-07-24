# Capability-Based IPC (seL4-Inspired)

> AI-agent conceptual memory. Read before modifying IPC, capability, or signal code.

## Decision

FKernel uses seL4-style capability-based IPC instead of traditional Unix IPC (pipes, shared memory, signals).

## Why Capabilities Over Pipes?

| Aspect | Traditional Unix | FKernel Capabilities |
|--------|-----------------|---------------------|
| Access control | File descriptors (implicit) | Explicit rights (Send/Receive/Manage) |
| Revocation | Close fd (manual) | Generation counter (automatic) |
| Security | Coarse-grained | Fine-grained per-object |
| Overhead | Kernel-mediated | Direct endpoint delivery |

## Core Concepts

### Capability
Typed handle with rights:
```cpp
struct Capability {
  void* object;           // Points to Endpoint/Notification/SharedMemory
  CapabilityType type;    // Endpoint | Notification | SharedMemory
  CapabilityRights rights; // Send | Receive | Manage (bitmask)
  uint64_t* revoke_counter;  // Points to object's generation counter
  uint64_t issued_generation; // Generation when capability was created
};
```

### CSpace (Capability Space)
Per-process array mapping slot numbers to capabilities. Process holds capabilities in numbered slots.

### Endpoint
Synchronous IPC channel. Sender blocks until receiver accepts. Used for request/response patterns.

### Notification
Asynchronous signal-like mechanism. Non-blocking send sets bits. Receiver polls or waits.

### Revocation
When an IPC object is destroyed, its generation counter increments. All capabilities pointing to it become invalid automatically:
```cpp
bool is_valid() const {
  if (revoke_counter && *revoke_counter != issued_generation)
    return false;  // Revoked!
  return true;
}
```

## Key Files

| File | Role |
|------|------|
| `Include/Kernel/Ipc/capability.h` | Capability struct with rights and revocation |
| `Include/Kernel/Ipc/cspace.h` | Per-process capability space |
| `Include/Kernel/Ipc/endpoint.h` | Synchronous IPC endpoint |
| `Include/Kernel/Ipc/notification.h` | Asynchronous notification |
| `Include/Kernel/Ipc/badge.h` | Badge values for endpoint differentiation |
| `Include/Kernel/Ipc/message_info.h` | IPC message metadata |
| `Include/Kernel/Ipc/global_endpoint_manager.h` | System-wide endpoint registry |
| `Include/Kernel/Ipc/signal_delivery.h` | Signal delivery via capabilities |

## Syscall Interface

| Syscall | Description |
|---------|-------------|
| `ipc_call(ep_cap, msg)` | Send message and wait for reply |
| `ipc_send(ep_cap, msg)` | Send message without waiting |
| `ipc_receive(ep_cap, msg)` | Wait for and receive message |
| `cap_revoke(cap_slot)` | Revoke a capability |

## When Modifying

- Rights checks must happen BEFORE any IPC operation
- Generation counter is the ONLY revocation mechanism — don't add manual cleanup
- `with_rights()` creates derived capabilities — don't modify original
- Signal delivery uses capability endpoints internally
