#pragma once

class Node;

namespace fkernel {
    void notify_kqueue_readers(Node* node);
    void notify_kqueue_writers(Node* node);
    void notify_proc_kqueue(struct ::Task* task, uint32_t fflags);
    void notify_signal_kqueue(struct ::Task* task, int signum);
}
