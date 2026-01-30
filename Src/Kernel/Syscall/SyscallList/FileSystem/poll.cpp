#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/kqueue.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Types/types.h>

struct pollfd {
    int fd;
    short events;
    short revents;
};

extern "C" {
uint64_t sys_poll(uint64_t fds_ptr, uint64_t nfds_u64, [[maybe_unused]] uint64_t timeout_ms,
                  uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    int nfds = (int)nfds_u64;
    auto* current_task = SchedulerManager::the().current();
    if (!current_task) {
        return -static_cast<int>(fk::core::Error::InvalidParameter);
    }

    if (!fds_ptr || nfds <= 0) {
        return -static_cast<int>(fk::core::Error::InvalidParameter);
    }

    // Create a kqueue for this poll operation
    auto kqueue_result = fkernel::KQueueNode::create();
    if (kqueue_result.is_error()) {
        return -static_cast<int>(kqueue_result.error());
    }
    auto kqueue = kqueue_result.value();

    // Convert pollfd structures to kevent structures
    fk::containers::Vector<fkernel::kevent> changelist;
    fk::containers::Vector<fkernel::kevent> eventlist;
    
    auto* poll_fds = reinterpret_cast<struct pollfd*>(fds_ptr);
    
    // Setup changelist with all file descriptors
    for (int i = 0; i < nfds; ++i) {
        if (poll_fds[i].fd < 0 || 
            static_cast<size_t>(poll_fds[i].fd) >= current_task->resources.files.descriptors.size()) {
            poll_fds[i].revents = POLLNVAL;
            continue;
        }
        
        auto& desc = current_task->resources.files.descriptors[poll_fds[i].fd];
        if (!desc) {
            poll_fds[i].revents = POLLNVAL;
            continue;
        }
        
        // Map poll events to kqueue filters
        if (poll_fds[i].events & POLLIN) {
            fkernel::kevent kev{};
            kev.ident = poll_fds[i].fd;
            kev.filter = fkernel::EVFILT_READ;
            kev.flags = fkernel::EV_ADD;
            changelist.push_back(kev);
        }
        
        if (poll_fds[i].events & POLLOUT) {
            fkernel::kevent kev{};
            kev.ident = poll_fds[i].fd;
            kev.filter = fkernel::EVFILT_WRITE;
            kev.flags = fkernel::EV_ADD;
            changelist.push_back(kev);
        }
        
        poll_fds[i].revents = 0; // Initialize revents
    }
    
    // If we have any valid FDs, call kevent
    if (changelist.size() > 0) {
        eventlist.resize(changelist.size());
        
        auto kevent_result = kqueue->kevent(changelist.begin(), changelist.size(), 
                                          eventlist.begin(), eventlist.size());
        
        if (kevent_result.is_error()) {
            return -static_cast<int>(kevent_result.error());
        }
        
        // Convert kqueue results back to poll results
        int triggered_count = kevent_result.value();
        for (int i = 0; i < triggered_count && i < nfds; ++i) {
            auto fd = static_cast<int>(eventlist[i].ident);
            if (eventlist[i].filter == fkernel::EVFILT_READ) {
                poll_fds[fd].revents |= POLLIN;
            }
            if (eventlist[i].filter == fkernel::EVFILT_WRITE) {
                poll_fds[fd].revents |= POLLOUT;
            }
        }
        
        return triggered_count;
    }
    
    // Check if any FDs already have errors
    int ready_count = 0;
    for (int i = 0; i < nfds; ++i) {
        if (poll_fds[i].revents != 0) {
            ready_count++;
        }
    }
    
    return ready_count;
}
}