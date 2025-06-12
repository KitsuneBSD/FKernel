#include "Arch/x86_64/gdt.h"
#include "LibFK/Log.h"
#include <Init/early_init.h>

void early_init(uint64_t memory_available) {
  Log(LogLevel::INFO, "Starting GDT");
  init_gdt();
  Log(LogLevel::INFO, "Starting IDT");
  init_idt();
  // Log(LogLevel::INFO, "Start MemoryManagement");
}
