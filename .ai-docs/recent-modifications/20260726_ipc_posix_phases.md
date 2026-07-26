# 2026-07-26: IPC/POSIX Phases 0-10

## Summary

Implemented unified IPC substrate and all POSIX IPC mechanisms on top of it. ~81 files created/modified across 10 phases. x86_64 architecture fixes applied.

## IPC Substrate (Phase 0)

- **Notification::wait_timeout()** — timed blocking with race-safe timeout detection via `sleep_current()` + list membership check
- **Notification::signal_with_payload()** — 64-byte data payload queue (16 slots) alongside bitmask signals
- **Endpoint::call()** — atomic send+receive with `m_call_sender` tracking
- **Endpoint::send_timeout() / receive_timeout()** — return `Error::Timeout` on expiry
- **SharedMemory** — page-by-page physical page allocation + per-task mapping
- **cap_transfer / cap_grant** syscalls (404, 405) — runtime capability sharing

## POSIX over IPC (Phases 1-10)

All POSIX IPC mechanisms are VFS nodes backed by Notification/Endpoint/SharedMemory:

| Mechanism | Backend | Namespace |
|-----------|---------|-----------|
| Signals (SA_SIGINFO) | signal_with_payload (siginfo_t) | — |
| Pipes/FIFOs | PipeNode + 2× Notification | /dev/ + tmpfs |
| eventfd | EventFdNode + Notification | anonymous fd |
| signalfd | SignalFdNode + Notification | anonymous fd |
| timerfd | TimerFdNode + Notification + tick registry | anonymous fd |
| epoll | EpollNode + Notification::wait_timeout() | anonymous fd |
| futex | Notification[256] hash + wait_timeout | — |
| Semaphores | SemNode + Notification | /dev/sem/ |
| Message Queues | MqueueNode (priority entries) + Notification | /dev/mqueue/ |
| Shared Memory | ShmNode + SharedMemory IPC | /dev/shm/ |
| PTY discipline | Termios + PtyLineDiscipline + signal | /dev/pts/ |

## Architecture Fixes

- IST for NMI (#2, IST2) and Machine Check (#18, IST3) per Intel SDM/AMD APM
- IOAPIC destination LAPIC ID extraction via CPUID.01h:EBX[31:24]
- Removed Intel-only MSR_CSTAR (dead code, #GP on AMD)
- `sys_kill` handles negative PIDs → `send_signal_to_pgrp()`
- `send_signal` rejects terminated tasks (UAF protection)
- `SA_SIGINFO` passes saved_regs pointer as ucontext (was NULL)
- AMD/Intel compatibility audit: all CPUID leaves, MSRs, and feature bits confirmed cross-vendor

## File Impact

- New: `Include/Kernel/Ipc/shared_memory.h`, `Include/Kernel/Fs/Virtual/SemFs/`, `Include/Kernel/Fs/Virtual/MqueueFs/`, `Include/Kernel/Fs/Virtual/ShmFs/`, `Include/Kernel/Driver/Pty/pty_line_discipline.h`
- Modified: `notification.h/cpp`, `endpoint.h/cpp`, `cspace.h`, `signal_delivery.h/cpp`, `signal_defs.h`, `signal_frame.h`, `pipe_node.h/cpp`, `event_fd_node.h/cpp`, `signal_fd_node.h/cpp`, `timer_fd_node.h/cpp`, `epoll_node.h/cpp`, `futex.cpp`, `node.h`, `syscall.cpp`, `numbers.h`, `virtual_filesystem.cpp`, `mmap.cpp`, `tcp_socket.h/cpp`, `tcp_connection.h`, `scheduler_lifecycle.cpp`, `cpu.h/cpp`, `ioapic.h/cpp`, `interrupt_controller.cpp`, `syscall_arch.h`, `signal.cpp`, `gp_handler.cpp`, `tgkill.cpp`
