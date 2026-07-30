#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Boot/boot_timer.h>
#include <LibFK/Algorithms/log.h>

static uint64_t g_tsc_freq_hz = 0;

void BootTimer::set_tsc_frequency(uint64_t freq_hz) {
  g_tsc_freq_hz = freq_hz;
}

void BootTimer::mark(const char* name) {
  if (m_count < MAX_MARKS) {
    m_marks[m_count++] = {name, arch_read_tsc_serialized()};
  }
}

static uint64_t cycles_to_ns(uint64_t cycles) {
  if (g_tsc_freq_hz == 0) return 0;
  return cycles * 1000000000ULL / g_tsc_freq_hz;
}

void BootTimer::log_summary() {
  if (m_count < 2) return;

  fk::algorithms::klog("BOOT_TIMER", "--- Boot Timing Summary ---");
  for (size_t i = 1; i < m_count; ++i) {
    uint64_t delta = m_marks[i].tsc - m_marks[i - 1].tsc;
    uint64_t ns = cycles_to_ns(delta);
    if (g_tsc_freq_hz > 0)
      fk::algorithms::klog("BOOT_TIMER", "  %s -> %s: %llu ns",
                           m_marks[i - 1].name, m_marks[i].name, ns);
    else
      fk::algorithms::klog("BOOT_TIMER", "  %s -> %s: %llu cycles",
                           m_marks[i - 1].name, m_marks[i].name, delta);
  }
  uint64_t total = m_marks[m_count - 1].tsc - m_marks[0].tsc;
  uint64_t total_ns = cycles_to_ns(total);
  if (g_tsc_freq_hz > 0)
    fk::algorithms::klog("BOOT_TIMER", "  Total: %llu ns", total_ns);
  else
    fk::algorithms::klog("BOOT_TIMER", "  Total: %llu cycles", total);
}
