#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/kqueue.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Types/types.h>

// fd_set and select() constants
constexpr int FD_SETSIZE = 1024;

struct fd_set {
    unsigned long fds_bits[FD_SETSIZE / (8 * sizeof(unsigned long))];
};

#define FD_ZERO(set) memset((set), 0, sizeof(fd_set))
#define FD_SET(fd, set) ((set)->fds_bits[(fd) / (8 * sizeof(unsigned long))] |= (1UL << ((fd) % (8 * sizeof(unsigned long)))))
#define FD_CLR(fd, set) ((set)->fds_bits[(fd) / (8 * sizeof(unsigned long))] &= ~(1UL << ((fd) % (8 * sizeof(unsigned long)))))
#define FD_ISSET(fd, set) (((set)->fds_bits[(fd) / (8 * sizeof(unsigned long))] & (1UL << ((fd) % (8 * sizeof(unsigned long))))) != 0)

extern "C" {
uint64_t sys_select(uint64_t nfds_u64, uint64_t readfds_ptr, uint64_t writefds_ptr,
                     uint64_t exceptfds_ptr, [[maybe_unused]] uint64_t timeout_ptr, uint64_t, 
                     [[maybe_unused]] PtRegs* regs) {
    int nfds = (int)nfds_u64;
    auto* current_task = SchedulerManager::the().current();
    if (!current_task) {
        return -static_cast<int>(fk::core::Error::InvalidParameter);
    }

    if (nfds <= 0 || nfds > FD_SETSIZE) {
        return -static_cast<int>(fk::core::Error::InvalidParameter);
    }

    // Create a kqueue for this select operation
    auto kqueue_result = fkernel::KQueueNode::create();
    if (kqueue_result.is_error()) {
        return -static_cast<int>(kqueue_result.error());
    }
    auto kqueue = kqueue_result.value();

    // Convert fd_sets to kevent structures
    fk::containers::Vector<fkernel::kevent> changelist;
    fk::containers::Vector<fkernel::kevent> eventlist;
    
    int max_fd = -1;
    
    // Setup read descriptors
    if (readfds_ptr) {
        auto* readfds = reinterpret_cast<fd_set*>(readfds_ptr);
        for (int fd = 0; fd < nfds; ++fd) {
            if (FD_ISSET(fd, readfds)) {
                if (fd >= 0 && static_cast<size_t>(fd) < current_task->resources.files.descriptors.size()) {
                    auto& desc = current_task->resources.files.descriptors[fd];
                    if (desc) {
                        fkernel::kevent kev{};
                        kev.ident = fd;
                        kev.filter = fkernel::EVFILT_READ;
                        kev.flags = fkernel::EV_ADD;
                        changelist.push_back(kev);
                        max_fd = (fd > max_fd) ? fd : max_fd;
                    }
                }
            }
        }
    }
    
    // Setup write descriptors
    if (writefds_ptr) {
        auto* writefds = reinterpret_cast<fd_set*>(writefds_ptr);
        for (int fd = 0; fd < nfds; ++fd) {
            if (FD_ISSET(fd, writefds)) {
                if (fd >= 0 && static_cast<size_t>(fd) < current_task->resources.files.descriptors.size()) {
                    auto& desc = current_task->resources.files.descriptors[fd];
                    if (desc) {
                        fkernel::kevent kev{};
                        kev.ident = fd;
                        kev.filter = fkernel::EVFILT_WRITE;
                        kev.flags = fkernel::EV_ADD;
                        changelist.push_back(kev);
                        max_fd = (fd > max_fd) ? fd : max_fd;
                    }
                }
            }
        }
    }
    
    // Clear output fd_sets initially
    if (readfds_ptr) {
        FD_ZERO(reinterpret_cast<fd_set*>(readfds_ptr));
    }
    if (writefds_ptr) {
        FD_ZERO(reinterpret_cast<fd_set*>(writefds_ptr));
    }
    if (exceptfds_ptr) {
        FD_ZERO(reinterpret_cast<fd_set*>(exceptfds_ptr));
    }
    
    // If we have any valid FDs, call kevent
    if (changelist.size() > 0) {
        eventlist.resize(changelist.size());
        
        auto kevent_result = kqueue->kevent(changelist.begin(), changelist.size(), 
                                          eventlist.begin(), eventlist.size());
        
        if (kevent_result.is_error()) {
            return -static_cast<int>(kevent_result.error());
        }
        
        // Convert kqueue results back to select results
        int triggered_count = kevent_result.value();
        for (int i = 0; i < triggered_count; ++i) {
            auto fd = static_cast<int>(eventlist[i].ident);
            if (eventlist[i].filter == fkernel::EVFILT_READ && readfds_ptr) {
                FD_SET(fd, reinterpret_cast<fd_set*>(readfds_ptr));
            }
            if (eventlist[i].filter == fkernel::EVFILT_WRITE && writefds_ptr) {
                FD_SET(fd, reinterpret_cast<fd_set*>(writefds_ptr));
            }
        }
        
        return triggered_count;
    }
    
    return 0;
}
}