#include <Kernel/Syscall/posix_timer.h>

fk::containers::Vector<PosixTimer> s_timers;
fk::synchronization::Spinlock s_timer_lock;
