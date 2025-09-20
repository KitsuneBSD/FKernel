#include <Kernel/Boot/early_init.h>
#include <LibC/stddef.h>
#include <LibC/stdio.h>
#include <LibFK/log.h>
#include <LibFK/type_traits.h>

void early_init([[maybe_unused]] const multiboot2::TagMemoryMap &mmap) {
  klog("MULTIBOOT2", "Reference to multiboot2 memory map: %p", mmap);
}
