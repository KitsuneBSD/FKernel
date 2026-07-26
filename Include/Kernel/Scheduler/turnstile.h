#pragma once

#include <Kernel/Scheduler/qos.h>

struct Task;

namespace fkernel::scheduler {

struct Turnstile {
    Task* holder{nullptr};
    Task* waiter{nullptr};
    QoSClass original_qos{QoSClass::Default};
    QoSClass boosted_qos{QoSClass::Default};
    Turnstile* chain{nullptr};
    bool active{false};
};

Turnstile* create_turnstile(Task* holder, Task* waiter);
void destroy_turnstile(Turnstile* ts);
void boost_qos_if_needed(Task* waiter, Task* holder);
void unboost_task(Task* task);
void reprioritize_task(Task* task);

} // namespace fkernel::scheduler
