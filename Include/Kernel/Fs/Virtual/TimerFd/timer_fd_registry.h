#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

class TimerFdNode;

namespace timer_fd_registry {

void register_timer(TimerFdNode* node);
void unregister_timer(TimerFdNode* node);
void tick_all(uint64_t now_ticks, uint32_t frequency);

} // namespace timer_fd_registry
} // namespace fkernel
