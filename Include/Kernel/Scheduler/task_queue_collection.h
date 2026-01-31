#pragma once

#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Container/intrusive_list.h>
#include <LibFK/Types/processId.h>

namespace fkernel {

class TaskQueueCollection {
private:
  fk::containers::IntrusiveList<Task, &Task::wait_node> m_wait_queue;
  fk::containers::IntrusiveList<Task, &Task::zombie_node> m_zombie_queue;

public:
  void add_to_wait(Task* task) {
    if (task) {
      m_wait_queue.push_back(*task);
    }
  }

  void add_to_zombie(Task* task) {
    if (task) {
      m_zombie_queue.push_back(*task);
    }
  }

  void remove_from_wait(Task* task) {
    if (task && task->wait_node.is_linked()) {
      m_wait_queue.erase(*task);
    }
  }

  void remove_from_zombie(Task* task) {
    if (task && task->zombie_node.is_linked()) {
      m_zombie_queue.erase(*task);
    }
  }

  Task* find_in_queues(fk::ProcessId id) {
    // Search in wait queue
    for (auto& task : m_wait_queue) {
      if (task.control.lifecycle.pid == id) {
        return &task;
      }
    }

    // Search in zombie queue
    for (auto& task : m_zombie_queue) {
      if (task.control.lifecycle.pid == id) {
        return &task;
      }
    }

    return nullptr;
  }

  Task* find_terminated_child(fk::ProcessId ppid) {
    for (auto& task : m_zombie_queue) {
      if (task.control.relationship.ppid == ppid) {
        return &task;
      }
    }
    return nullptr;
  }

  Task* find_any_child(fk::ProcessId ppid) {
    // Search in wait queue
    for (auto& task : m_wait_queue) {
      if (task.control.relationship.ppid == ppid) {
        return &task;
      }
    }

    // Search in zombie queue
    for (auto& task : m_zombie_queue) {
      if (task.control.relationship.ppid == ppid) {
        return &task;
      }
    }

    return nullptr;
  }

  auto& wait_queue() { return m_wait_queue; }
  auto& zombie_queue() { return m_zombie_queue; }
};

} // namespace fkernel