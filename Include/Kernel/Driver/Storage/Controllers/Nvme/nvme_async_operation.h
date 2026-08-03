#pragma once

#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Driver/Async/async_io.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <LibFK/Memory/Pointers/ref_counted.h>

namespace fkernel {

class NvmeAsyncOperation : public AsyncIoOperation,
                           public fk::memory::RefCounted<NvmeAsyncOperation> {
private:
  uint16_t m_command_id;
  uint64_t m_start_lba;
  uint32_t m_block_count;
  void* m_buffer;
  bool m_is_write;
  IoCompletionStatus m_status = IoCompletionStatus::Busy;
  uint64_t m_completion_time = 0;

public:
  NvmeAsyncOperation(uint16_t command_id, uint64_t start_lba, uint32_t block_count, void* buffer,
                     bool is_write)
      : m_command_id(command_id), m_start_lba(start_lba), m_block_count(block_count),
        m_buffer(buffer), m_is_write(is_write) {}

  void on_interrupt(uint32_t interrupt_status) override {
    if (interrupt_status & 0x1) {
      mark_success();
      m_completion_time = TickManager::the().get_ticks();
    }
  }

  bool is_completed() const override { return m_status != IoCompletionStatus::Busy; }

  IoCompletionStatus get_status() const override { return m_status; }

  IoCompletionStatus wait_for_completion(uint64_t timeout_ms = 5000) override {
    uint64_t start_time = TickManager::the().get_ticks();
    uint64_t timeout_ticks = (timeout_ms * TickManager::the().get_frequency()) / 1000;

    while (!is_completed()) {
      uint64_t current_time = TickManager::the().get_ticks();
      if ((current_time - start_time) > timeout_ticks) {
        m_status = IoCompletionStatus::Timeout;
        break;
      }
      SchedulerManager::the().yield();
    }

    return m_status;
  }

  uint16_t command_id() const { return m_command_id; }
  uint64_t start_lba() const { return m_start_lba; }
  uint32_t block_count() const { return m_block_count; }
  void* buffer() const { return m_buffer; }
  bool is_write_operation() const { return m_is_write; }
  uint64_t completed_at() const { return m_completion_time; }

  void mark_error() { m_status = IoCompletionStatus::Error; }
  void mark_success() { m_status = IoCompletionStatus::Success; }
};

} // namespace fkernel
