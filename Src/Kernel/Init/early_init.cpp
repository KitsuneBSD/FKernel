#include <Arch/x86_64/gdt.h>
#include <Init/early_init.h>
#include <LibFK/Log.h>
#include <MemoryManagement/Memory_management.h>

extern uintptr_t heap_end;

void early_init(uint64_t memory_available) {
  Log(LogLevel::INFO, "Starting GDT");
  init_gdt();
  Log(LogLevel::INFO, "Starting IDT");
  init_idt();
  Log(LogLevel::INFO, "Start MemoryManagement");
  init_memory_management(heap_end, memory_available);
}
