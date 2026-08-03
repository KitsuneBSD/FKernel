#pragma once

#include <Kernel/Fs/Vfs/Events/kevent.h>
#include <Kernel/Fs/Vfs/Core/timespec.h>
#include <Kernel/Fs/Vfs/Events/kqueue_node.h>
#include <Kernel/Fs/Vfs/Events/kqueue_functions.h>

namespace fkernel {

// Filters
constexpr int16_t EVFILT_READ   = -1;
constexpr int16_t EVFILT_WRITE  = -2;
constexpr int16_t EVFILT_TIMER  = -3;
constexpr int16_t EVFILT_VNODE  = -4;
constexpr int16_t EVFILT_PROC   = -5;
constexpr int16_t EVFILT_SIGNAL = -6;
constexpr int16_t EVFILT_USER   = -7;

// General flags
constexpr uint16_t EV_ADD       = 0x0001;
constexpr uint16_t EV_DELETE    = 0x0002;
constexpr uint16_t EV_ENABLE    = 0x0004;
constexpr uint16_t EV_DISABLE   = 0x0008;
constexpr uint16_t EV_ONESHOT   = 0x0010;
constexpr uint16_t EV_CLEAR     = 0x0020;
constexpr uint16_t EV_RECEIPT   = 0x0040;
constexpr uint16_t EV_DISPATCH  = 0x0080;
constexpr uint16_t EV_EOF       = 0x8000;
constexpr uint16_t EV_ERROR     = 0x4000;

// Return-only flags (set by kernel in kevent.flags)
constexpr uint16_t EV_ADDED     = 0x0100;

// fflags for EVFILT_VNODE
constexpr uint32_t NOTE_DELETE  = 0x00000001;
constexpr uint32_t NOTE_WRITE   = 0x00000002;
constexpr uint32_t NOTE_EXTEND  = 0x00000004;
constexpr uint32_t NOTE_ATTRIB  = 0x00000008;
constexpr uint32_t NOTE_LINK    = 0x00000010;
constexpr uint32_t NOTE_RENAME  = 0x00000020;
constexpr uint32_t NOTE_REVOKE  = 0x00000040;

// fflags for EVFILT_PROC
constexpr uint32_t NOTE_EXIT    = 0x80000000;
constexpr uint32_t NOTE_FORK    = 0x40000000;
constexpr uint32_t NOTE_EXEC    = 0x20000000;
constexpr uint32_t NOTE_TRACK   = 0x00000001;
constexpr uint32_t NOTE_CHILD   = 0x00000002;

// fflags for EVFILT_TIMER
constexpr uint32_t NOTE_SECONDS = 0x00000001;
constexpr uint32_t NOTE_MSECONDS = 0x00000002;

// fflags for EVFILT_USER
constexpr uint32_t NOTE_TRIGGER = 0x01000000;
constexpr uint32_t NOTE_FFNOP   = 0x00000000;

} // namespace fkernel
