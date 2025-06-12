#include "LibFK/Log.h"
#include <Init/early_init.h>

void early_init(uint64_t memory_available) {
  Log(LogLevel::INFO, "Starting GDT");
  Log(LogLevel::INFO, "Starting IDT");
  Log(LogLevel::INFO, "Start MemoryManagement");
}
