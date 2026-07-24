#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>

// Linux reboot magic constants
static constexpr uint32_t REBOOT_MAGIC1 = 0xFEE1DEAD;
static constexpr uint32_t REBOOT_MAGIC2 = 0x28121969;

// Linux reboot commands
static constexpr uint32_t REBOOT_CMD_CAD_OFF       = 0x00000000;  // Disable Ctrl-Alt-Del
static constexpr uint32_t REBOOT_CMD_CAD_ON        = 0x89ABCDEF;  // Enable Ctrl-Alt-Del
static constexpr uint32_t REBOOT_CMD_RESTART       = 0x01234567;
static constexpr uint32_t REBOOT_CMD_HALT          = 0xCDEF0123;
static constexpr uint32_t REBOOT_CMD_POWER_OFF     = 0x4321FEDC;
static constexpr uint32_t REBOOT_CMD_RESTART2      = 0xA1B2C3D4;

static void do_poweroff() {
    outw(0x604, 0x2000);
    outb(0x501, 0x00);
    arch_triple_fault();
}

static void do_reboot() {
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
        return 0; // no-op: we don't implement CAD handling
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
