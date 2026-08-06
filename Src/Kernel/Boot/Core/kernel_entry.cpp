#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Core/assertions.h>

#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Boot/Stages/early_init.h>
#include <Kernel/Boot/Core/boot_info.h>
#include <Kernel/Driver/Serial/serial_port.h>
#include <Kernel/Driver/Vga/vga_adapter.h>
#include <Kernel/Io/kernel_puts.h>

extern char __heap_start[];
extern char __heap_end[];

extern "C" void kernel_entry() {
  Serial::init();
  // Wire up the puts hook now so every klog/kwarn/kerror below actually reaches serial.
  fkernel::io::initialize_kernel_puts();
  fk::algorithms::set_log_targets(fk::algorithms::LogTarget::Serial);

  auto &vga = VgaAdapter::the();
  vga.clear();

  assert(boot::BootInfo::the().is_initialized() && "BootInfo not initialized!");

  fk::algorithms::klog("BOOT", "FKernel starting...");

  auto boot_mode = boot::BootInfo::the().get_boot_mode();
  fk::algorithms::klog("BOOT", "Boot mode: %s",
      boot_mode == boot::BootMode::Multiboot2 ? "Multiboot2" : "Unknown");

  // Log heap layout
  uintptr_t heap_start = reinterpret_cast<uintptr_t>(__heap_start);
  uintptr_t heap_end   = reinterpret_cast<uintptr_t>(__heap_end);
  size_t    heap_size  = heap_end - heap_start;
  fk::algorithms::klog("BOOT", "Heap region: [%p – %p] size=%zu KiB",
      reinterpret_cast<void*>(heap_start),
      reinterpret_cast<void*>(heap_end),
      heap_size / 1024);

  // Log framebuffer info
  if (boot::BootInfo::the().has_framebuffer()) {
    auto fb = boot::BootInfo::the().get_framebuffer_info();
    fk::algorithms::klog("BOOT", "Framebuffer: %ux%u bpp=%u @ %p (pitch=%u)",
        fb.width, fb.height, (unsigned)fb.bpp,
        reinterpret_cast<void*>(fb.addr), fb.pitch);
  } else {
    fk::algorithms::klog("BOOT", "No framebuffer — using VGA text mode");
  }

  // Log boot modules
  const auto &modules = boot::BootInfo::the().get_modules();
  fk::algorithms::klog("BOOT", "Boot modules: %zu", modules.size());
  for (size_t i = 0; i < modules.size(); ++i) {
    const auto &m = modules[i];
    fk::algorithms::klog("BOOT", "  module[%zu]: [%p – %p] '%s'",
        i,
        reinterpret_cast<void*>(m.start),
        reinterpret_cast<void*>(m.end),
        m.cmdline ? m.cmdline : "(no cmdline)");
  }

  // Log ACPI table pointers
  auto acpi = boot::BootInfo::the().get_acpi_info();
  if (acpi.rsdp) {
    fk::algorithms::klog("BOOT", "ACPI: RSDP=%p RSDT=%p XSDT=%p",
        acpi.rsdp, acpi.rsdt, acpi.xsdt);
  } else {
    fk::algorithms::kwarn("BOOT", "ACPI: no RSDP found in boot info");
  }

  early_init();

  arch_halt_loop();
}
