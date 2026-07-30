# VFS Architecture

## Overview

FKernel's Virtual File System (VFS) is inspired by the **BSD vnode/dentry/mount** model. It provides a unified interface for multiple filesystem types, device nodes, and process information.

## Architecture

```mermaid
flowchart TD
    U["Userspace<br/>open/read/write/ioctl via syscalls"]
    FD["FileDescription<br/>Per-process file descriptor<br/>offset, flags, cloexec"]
    VFS["VirtualFilesystem<br/>Mount table, path resolution<br/>dentry cache"]
    D["Dentry<br/>Directory entry cache<br/>Node stack for mount overlay"]
    N["Node<br/>Abstract filesystem node<br/>read/write/ioctl vtable"]
    FS["FS Drivers<br/>10 on-disk + 13 virtual<br/>Ext2/3/4, FAT, TmpFs, DevFs, ..."]

    U -->|"syscall layer"| FD
    FD --> VFS
    VFS --> D
    D --> N
    N --> FS
```

### Path Resolution Flow

```mermaid
flowchart TD
    START["resolve_path(path)"]
    ABS{Starts with '/'?}
    ROOT["Start at m_root"]
    CWD["Start at CWD or base dentry"]
    PARSE["Parse next component"]
    SEP{Is separator?}
    SKIP["Skip, continue"]
    DOT{Is '.'?}
    DOTDOT{Is '..'?}
    GO_PARENT["current = current.parent"]
    LOOKUP["current.lookup(name)"]
    CACHED{In cache?}
    WALK["Walk node stack top-to-bottom<br/>call Node::lookup()"]
    CACHE["Push into dentry cache"]
    SYMLINK{Is symlink?}
    RECURSE["Resolve symlink target<br/>(depth limit: 8)"]
    MORE{More components?}
    DONE["Return final Dentry"]

    START --> ABS
    ABS -->|Yes| ROOT
    ABS -->|No| CWD
    ROOT --> PARSE
    CWD --> PARSE
    PARSE --> SEP
    SEP -->|Yes| SKIP --> PARSE
    SEP -->|No| DOT
    DOT -->|Yes| PARSE
    DOT -->|No| DOTDOT
    DOTDOT -->|Yes| GO_PARENT --> PARSE
    DOTDOT -->|No| LOOKUP
    LOOKUP --> CACHED
    CACHED -->|Yes| SYMLINK
    CACHED -->|No| WALK --> CACHE --> SYMLINK
    SYMLINK -->|Yes| RECURSE --> PARSE
    SYMLINK -->|No| MORE
    MORE -->|Yes| PARSE
    MORE -->|No| DONE
```

### Mount Point Overlay

The Dentry uses a **node stack** for mount-point overlaying. When a filesystem is mounted at `/mnt`, its `Node` is pushed onto the existing dentry's stack:

```mermaid
flowchart LR
    subgraph D["Dentry: /mnt"]
        direction TB
        TOP["Top: Fat32Node (mounted)"]
        BOT["Bottom: TmpFsNode (original)"]
    end
    TOP -->|"top_node() = active"| OPS["read/write/ioctl"]
    BOT -.->|"hidden by mount"| OPS2["..."]
```

Multiple filesystems can be stacked on the same dentry. The topmost node wins for operations. Directory listings merge entries from ALL stack layers with deduplication.

## Key Components

### FileDescription

- Per-process, per-open-file state
- Wraps a `Dentry` reference + `m_current_offset` + `m_flags` + `m_cloexec`
- Read/write operations delegate to `node()->read()/write()` and atomically advance offset
- Created by `open()`/`creat()`, duplicated by `dup()`/`dup2()`/`dup3()`

### VirtualFilesystem

- Global singleton (`VirtualFileSystem::the()`)
- Mount table management (mount/umount)
- Path resolution (`resolve_path()` -> traverse dentry tree)
- Inode number allocation (monotonic, lock-free via `__sync_fetch_and_add`)
- All operations use `ScopedLockIRQ` (interrupt-safe locking)

### Dentry

- In-memory directory entry with a **node stack** (`DentryNodeStack`) for mount overlay
- `lookup(name)` checks cached children first, then walks the node stack
- Supports `.` and `..` directly
- Lock held during lookup to prevent TOCTOU races

### Node (filesystem node)

- Abstract interface for all filesystem objects (`RefCounted`)
- I/O: `read()`, `write()`, `ioctl()`, `truncate()`, `fsync()`, `select()`, `poll()`
- Directory ops: `lookup()`, `list_dir()`, `create_child()`, `mkdir()`, `symlink()`, `rmdir()`, `unlink()`, `link()`, `rename()`, `readlink()`
- Metadata: `stat()`, `chmod()`, `chown()`, `utimens()`
- Socket ops: `bind()`, `connect()`, `accept()`, `listen()`, `shutdown()`, `getsockname()`, `getpeername()`, `setsockopt()`, `getsockopt()`
- Type queries: `is_directory()`, `is_symlink()`, `is_block_device()`, `is_character_device()`, `is_pipe()`
- Atomic inode allocation via `__sync_fetch_and_add`

## Filesystem Implementations

| Filesystem | Mount Point | Type | Key Features |
|------------|------------|------|--------------|
| **Ext2** | `/mnt/<disk>` | Disk-backed | Linux ext2, block-oriented, inode table |
| **Ext3** | `/mnt/<disk>` | Disk-backed | ext2 + journaling, V1/V2 superblocks |
| **Ext4** | `/mnt/<disk>` | Disk-backed | extents, flex_bg, 48-bit block numbers, journal |
| **FAT32** | `/mnt/<disk>` | Disk-backed | LFN support, cluster chain traversal, write support |
| **FAT16** | `/mnt/<disk>` | Disk-backed | LFN support, cluster chain reading |
| **FAT12** | `/mnt/<disk>` | Disk-backed | Floppy images, cluster chain traversal |
| **ExFAT** | `/mnt/<disk>` | Disk-backed | Large file support, contiguous clusters |
| **ISO9660** | `/mnt/<disk>` | Disk-backed | CD/DVD images, Rock Ridge extensions |
| **MinixFS** | `/mnt/<disk>` | Disk-backed | Minix v1/v2/v3 filesystem |
| **TmpFs** | `/tmp`, `/var/run` | In-memory | Temporary file storage |
| **DevFs** | `/dev` | Virtual | Dynamic device registration, pseudo-devices |
| **ProcFs** | `/proc` | Virtual | Per-pid entries: cmdline, status, mem, fd, maps, cwd, exe, root |
| **DebugFs** | `/debug` | Virtual | Debug info, IPC log at `/debug/ipc` |
| **PtsFs** | `/dev/pts` | Virtual | PTY slave device files |
| **SemFs** | `/dev/sem` | Virtual | POSIX named semaphores |
| **MqueueFs** | `/dev/mqueue` | Virtual | POSIX message queues |
| **ShmFs** | `/dev/shm` | Virtual | POSIX shared memory |
| **Pipe** | (anonymous) | In-memory | Circular buffer, Notification-based signaling |
| **Epoll** | (anonymous) | In-memory | Full epoll implementation, fd event monitoring |
| **EventFd** | (anonymous) | In-memory | eventfd counter + Notification signaling |
| **SignalFd** | (anonymous) | In-memory | Signal-to-fd demultiplexing |
| **TimerFd** | (anonymous) | In-memory | Timer expiration via file descriptor |
| **KQueue** | (anonymous) | In-memory | BSD event polling (EVFILT_READ/WRITE/PROC/SIGNAL/TIMER) |

### Userspace FS via Capabilities

Filesystem operations in userspace are architecturally supported through the Capability model (see `ipc-capabilities.md`). A userspace process can serve as a filesystem driver by receiving Node capabilities, handling `read()`/`write()`/`lookup()` via Endpoint IPC, and exposing results through the VFS layer. This enables FUSE-like functionality natively through the capability system. **Phase: pending — not yet implemented.**

## Event Notification

Two event notification subsystems coexist in FKernel:

### KQueue (BSD-style)

Full `kqueue()` implementation with the following event filters:

| Filter | Trigger |
|--------|---------|
| `EVFILT_READ` | Data available on fd |
| `EVFILT_WRITE` | Space available on fd |
| `EVFILT_PROC` | Process lifecycle events (exit, fork, exec) |
| `EVFILT_SIGNAL` | Signal delivery |
| `EVFILT_TIMER` | Timer expiration |

### Epoll (Linux-compatible)

Full `epoll_create()`/`epoll_ctl()`/`epoll_wait()` implementation backed by `EpollNode`, supporting `EPOLLIN`, `EPOLLOUT`, `EPOLLERR`, `EPOLLET` (edge-triggered), and `EPOLLONESHOT`.

## AutoMounter and Fstab

```mermaid
flowchart TD
    BOOT["Boot / sys_mount()"]
    FSTAB{fstab exists?}
    PARSE_FSTAB["Parse fstab entries"]
    MOUNT_ENTRY["Mount each entry<br/>proc, tmpfs, devfs, device-backed"]
    SCAN["PartitionManager::scan()"]
    TRY["AutoMounter::try_mount()"]
    DETECT{Detect FS type?}
    MOUNT["Mount at /mnt/<device_name>"]
    SKIP_FS["Skip (unsupported)"]

    BOOT --> FSTAB
    FSTAB -->|Yes| PARSE_FSTAB --> MOUNT_ENTRY
    FSTAB -->|No| SCAN
    SCAN --> TRY
    TRY --> DETECT
    DETECT -->|Ext2/3/4, FAT12/16/32,<br/>ExFAT, ISO9660, MinixFS| MOUNT
    DETECT -->|Unknown| SKIP_FS
```

## Key Design Decisions

- **BSD-style layered VFS** over Linux's single-struct inode model
- **Node stack mount overlay** — multiple FS on one dentry, topmost wins
- **Dentry cache** for fast path resolution (not a full dcache like Linux)
- **FileDescription** separates per-open state from inode (like BSD's file struct)
- **Interrupt-safe locking** — all VFS operations use `ScopedLockIRQ`
- **Lock-free inode allocation** — atomic counter, no lock contention
