#include <Arch/x86_64/irq_handler.hpp>
#include <Driver/Pic.h>

static volatile uint64_t tick_count = 0;

extern "C" void timer_handler(void *context) {
  ++tick_count;

  send_eoi(0);
}
