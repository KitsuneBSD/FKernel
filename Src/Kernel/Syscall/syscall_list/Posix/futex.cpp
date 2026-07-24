#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Synchronization/spinlock.h>

// Simple futex wait table: fixed-size hash table mapping uaddr → waiting task.
// Each bucket holds one waiter (chain collision via linear probe isn't needed
// for typical musl usage patterns — contended mutexes rarely have >1 waiter).
static constexpr size_t FUTEX_TABLE_SIZE = 64;

struct FutexEntry {
    uintptr_t uaddr{0};
    Task*     task{nullptr};
};

static FutexEntry s_futex_table[FUTEX_TABLE_SIZE];
static fk::synchronization::Spinlock s_futex_lock;

static size_t futex_bucket(uintptr_t uaddr) {
    return (uaddr >> 2) & (FUTEX_TABLE_SIZE - 1);
}

static void futex_register(uintptr_t uaddr, Task* task) {
    size_t idx = futex_bucket(uaddr);
    for (size_t i = 0; i < FUTEX_TABLE_SIZE; ++i) {
        size_t slot = (idx + i) & (FUTEX_TABLE_SIZE - 1);
        if (!s_futex_table[slot].task) {
            s_futex_table[slot].uaddr = uaddr;
            s_futex_table[slot].task  = task;
            return;
        }
    }
    // Table full — caller will fall back to yield
}

static void futex_unregister(Task* task) {
    for (size_t i = 0; i < FUTEX_TABLE_SIZE; ++i) {
        if (s_futex_table[i].task == task) {
            s_futex_table[i].task  = nullptr;
            s_futex_table[i].uaddr = 0;
            return;
        }
    }
}

// Returns number of tasks woken.
static uint32_t futex_wake(uintptr_t uaddr, uint32_t max_wake) {
    uint32_t woken = 0;
    size_t idx = futex_bucket(uaddr);
    for (size_t i = 0; i < FUTEX_TABLE_SIZE && woken < max_wake; ++i) {
        size_t slot = (idx + i) & (FUTEX_TABLE_SIZE - 1);
        if (s_futex_table[slot].uaddr == uaddr && s_futex_table[slot].task) {
            Task* t = s_futex_table[slot].task;
            s_futex_table[slot].task  = nullptr;
            s_futex_table[slot].uaddr = 0;
            SchedulerManager::the().wake_task(t);
            ++woken;
        }
    }
    return woken;
}

// Called from thread exit to wake pthread_join waiters on clear_child_tid address
extern "C" void fkernel_futex_wake_one(uintptr_t uaddr) {
    fk::synchronization::ScopedLock lock(s_futex_lock);
    futex_wake(uaddr, 1);
}

static constexpr int FUTEX_WAIT         = 0;
static constexpr int FUTEX_WAKE         = 1;
static constexpr int FUTEX_PRIVATE_FLAG = 128;

extern "C" uint64_t sys_futex(uint64_t uaddr, uint64_t op, uint64_t val,
                               uint64_t timeout_ptr, uint64_t uaddr2, uint64_t val3,
                               [[maybe_unused]] PtRegs* regs) {
    (void)uaddr2; (void)val3;
    int cmd = (int)(op & ~FUTEX_PRIVATE_FLAG);

    switch (cmd) {
    case FUTEX_WAIT: {
        if (!fkernel::memory::is_user_address(static_cast<uintptr_t>(uaddr), sizeof(int)))
            return (uint64_t)-14; // EFAULT

        // Atomically check value and register waiter under lock
        int cur_val = 0;
        {
            fk::synchronization::ScopedLockIRQ lock(s_futex_lock);
            auto res = fkernel::memory::copy_from_user(&cur_val,
                                                        reinterpret_cast<const void*>(uaddr),
                                                        sizeof(int));
            if (res.is_error()) return (uint64_t)-14;
            if (cur_val != (int)val) return (uint64_t)-11; // EAGAIN

            futex_register((uintptr_t)uaddr, SchedulerManager::the().current());
        }

        // Compute timeout ticks
        if (timeout_ptr) {
            struct { int64_t tv_sec; int64_t tv_nsec; } ts{};
            auto res = fkernel::memory::copy_from_user(&ts,
                                                        reinterpret_cast<const void*>(timeout_ptr),
                                                        sizeof(ts));
            if (res.is_error()) {
                fk::synchronization::ScopedLockIRQ lock(s_futex_lock);
                futex_unregister(SchedulerManager::the().current());
                return (uint64_t)-14;
            }
            uint32_t freq = TickManager::the().get_frequency();
            if (freq == 0) freq = 1000;
            uint64_t ns = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
            uint64_t ticks = (ns / 1000000ULL) * freq / 1000ULL;
            if (ticks == 0) ticks = 1;
            SchedulerManager::the().sleep_current(ticks);
        } else {
            SchedulerManager::the().block_current();
            SchedulerManager::the().schedule();
        }

        // Clean up registration if we weren't woken by FUTEX_WAKE
        {
            fk::synchronization::ScopedLockIRQ lock(s_futex_lock);
            futex_unregister(SchedulerManager::the().current());
        }

        return (uint64_t)-4; // EINTR — caller re-checks the value
    }

    case FUTEX_WAKE: {
        uint32_t count = (uint32_t)val;
        fk::synchronization::ScopedLockIRQ lock(s_futex_lock);
        uint32_t woken = futex_wake((uintptr_t)uaddr, count);
        return (uint64_t)woken;
    }

    default:
        return 0;
    }
}
