#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Hardware/Firmware/Fadt/fadt_manager.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>

static constexpr uint32_t REBOOT_MAGIC1 = 0xFEE1DEAD;
static constexpr uint32_t REBOOT_MAGIC2 = 0x28121969;

// Linux reboot commands
static constexpr uint32_t REBOOT_CMD_CAD_OFF       = 0x00000000;
static constexpr uint32_t REBOOT_CMD_CAD_ON        = 0x89ABCDEF;
static constexpr uint32_t REBOOT_CMD_RESTART       = 0x01234567;
static constexpr uint32_t REBOOT_CMD_HALT          = 0xCDEF0123;
static constexpr uint32_t REBOOT_CMD_POWER_OFF     = 0x4321FEDC;
static constexpr uint32_t REBOOT_CMD_RESTART2      = 0xA1B2C3D4;

// ACPI power management constants
static constexpr uint16_t PM1_CNT_SLP_EN  = 1 << 13;
static constexpr uint16_t PM1_CNT_SLP_TYP_SHIFT = 10;
static constexpr uint8_t  ACPI_ADDR_SPACE_IO     = 1;

static void write_acpi_register(const GenericAddressStructure &reg, uint8_t value) {
  uintptr_t addr = reg.address + (reg.register_bit_offset / 8);
  if (reg.address_space_id == ACPI_ADDR_SPACE_IO) {
    if (reg.register_bit_width <= 8) {
      outb(static_cast<uint16_t>(addr), value);
    } else if (reg.register_bit_width <= 16) {
      outw(static_cast<uint16_t>(addr), value);
    } else {
      outl(static_cast<uint16_t>(addr), value);
    }
  } else {
    auto *ptr = reinterpret_cast<volatile uint8_t *>(addr);
    *ptr = value;
  }
}

static bool acpi_reboot() {
  GenericAddressStructure reset_reg;
  if (!FadtManager::the().get_reset_register(reset_reg))
    return false;

  uint8_t reset_value = 0;
  FadtManager::the().get_reset_value(reset_value);

  fk::algorithms::klog("REBOOT", "ACPI reset: addr=0x%llx value=0x%x",
                       reset_reg.address, reset_value);
  write_acpi_register(reset_reg, reset_value);

  for (uint32_t i = 0; i < 100000; ++i) arch_cpu_relax();
  return false;
}

static bool acpi_poweroff() {
  uint32_t pm1a_port;
  if (!FadtManager::the().get_pm1a_control_block(pm1a_port) || pm1a_port == 0)
    return false;

  for (uint8_t slp_typa = 0; slp_typa <= 5; slp_typa += 5) {
    uint16_t pm1_cnt = (slp_typa << PM1_CNT_SLP_TYP_SHIFT) | PM1_CNT_SLP_EN;
    fk::algorithms::klog("REBOOT", "ACPI poweroff: PM1a=0x%x SLP_TYPa=%u",
                         pm1a_port, slp_typa);
    outw(static_cast<uint16_t>(pm1a_port), pm1_cnt);

    uint32_t pm1b_port;
    if (FadtManager::the().get_pm1b_control_block(pm1b_port) && pm1b_port != 0) {
      outw(static_cast<uint16_t>(pm1b_port), pm1_cnt);
    }

    for (uint32_t i = 0; i < 100000; ++i) arch_cpu_relax();
  }
  return false;
}

static void do_poweroff() {
  if (acpi_poweroff())
    return;

  GenericAddressStructure reset_reg;
  if (FadtManager::the().get_reset_register(reset_reg)) {
    uint8_t reset_value = 0;
    FadtManager::the().get_reset_value(reset_value);
    fk::algorithms::klog("REBOOT", "Poweroff fallback: ACPI reset");
    write_acpi_register(reset_reg, reset_value);
    for (uint32_t i = 0; i < 100000; ++i) arch_cpu_relax();
  }

  fk::algorithms::klog("REBOOT", "Poweroff fallback: QEMU ISA debug port");
  outw(0x604, 0x2000);
  outb(0x501, 0x00);
  arch_triple_fault();
}

static void do_reboot() {
  if (acpi_reboot())
    return;

  fk::algorithms::klog("REBOOT", "Reboot fallback: keyboard controller pulse");
  while (inb(0x64) & 0x02);
  outb(0x64, 0xFE);
  arch_halt_loop();
}

extern "C" uint64_t sys_reboot(uint64_t magic1, uint64_t magic2, uint64_t cmd,
                               uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    const char* name = task ? task->control.identity.name.c_str() : "unknown";

    if (static_cast<uint32_t>(magic1) != REBOOT_MAGIC1 ||
        static_cast<uint32_t>(magic2) != REBOOT_MAGIC2) {
        fk::algorithms::kwarn("REBOOT", "denied: bad magic from task '%s' (0x%x, 0x%x)",
                              name, (unsigned)magic1, (unsigned)magic2);
        return static_cast<uint64_t>(-1);
    }

    fk::algorithms::klog("REBOOT", "cmd=0x%x by task '%s'", (unsigned)cmd, name);

    if (static_cast<uint32_t>(cmd) == REBOOT_CMD_CAD_OFF ||
        static_cast<uint32_t>(cmd) == REBOOT_CMD_CAD_ON) {
        return 0;
    }
    if (static_cast<uint32_t>(cmd) == REBOOT_CMD_POWER_OFF) {
        do_poweroff();
    }
    if (static_cast<uint32_t>(cmd) == REBOOT_CMD_HALT) {
        fk::algorithms::klog("REBOOT", "halt requested");
        arch_halt_loop();
    }
    if (static_cast<uint32_t>(cmd) == REBOOT_CMD_RESTART ||
        static_cast<uint32_t>(cmd) == REBOOT_CMD_RESTART2) {
        do_reboot();
    }

    fk::algorithms::kwarn("REBOOT", "denied: unknown cmd 0x%x from task '%s'",
                          (unsigned)cmd, name);
    return static_cast<uint64_t>(-1);
}
