#include <Kernel/Boot/boot_timer.h>
#include <LibFK/Algorithms/log.h>

static inline uint64_t rdtsc_with_lfence() {
  uint32_t lo, hi;
  asm volatile("lfence\nrdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<uint64_t>(hi) << 32) | lo;
}

void BootTimer::mark(const char* name) {
  if (m_count < MAX_MARKS) {
    m_marks[m_count++] = {name, rdtsc_with_lfence()};
  }
}

void BootTimer::log_summary() {
  if (m_count < 2) return;

  fk::algorithms::klog("BOOT TIMER", "--- Boot Timing Summary ---");
  for (size_t i = 1; i < m_count; ++i) {
    uint64_t delta = m_marks[i].tsc - m_marks[i - 1].tsc;
    fk::algorithms::klog("BOOT TIMER", "  %s -> %s: %llu cycles",
                         m_marks[i - 1].name, m_marks[i].name, delta);
  }
  uint64_t total = m_marks[m_count - 1].tsc - m_marks[0].tsc;
  fk::algorithms::klog("BOOT TIMER", "  Total: %llu cycles", total);
}
