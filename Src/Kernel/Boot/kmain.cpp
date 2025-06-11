#include <Kernel/Boot/kmain.h>

extern "C" void kmain() {
  while (true) {
    __asm__ volatile("hlt");
  }
}
