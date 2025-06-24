#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>

extern "C" void kmain(const multiboot2::Info &multiboot_ptr) { return; }
