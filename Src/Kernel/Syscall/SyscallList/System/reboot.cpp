#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>

// Linux reboot magic constants
static constexpr uint32_t REBOOT_MAGIC1 = 0xFEE1DEAD;
static constexpr uint32_t REBOOT_CMD_POWER_OFF = 0x4321FEDC;

static void do_poweroff() {
    // QEMU ACPI shutdown via i8042/PIIX4 — port 0x604, value 0x2000
    outw(0x604, 0x2000);
    // Fallback: QEMU ISA debug exit (0x501 = exit with code 0)
    outb(0x501, 0x00);
    // Last resort: triple fault by zeroing IDTR and triggering interrupt
    asm volatile("lidt 0; int $3" : : : "memory");
    while (true) asm volatile("hlt");
}

static void do_reboot() {
    // Pulse keyboard controller reset line (8042)
    while (inb(0x64) & 0x02);  // wait for input buffer empty
    outb(0x64, 0xFE);           // pulse reset
    while (true) asm volatile("hlt");
}

extern "C" uint64_t sys_reboot(uint64_t magic_ptr, uint64_t /*magic2*/, uint64_t cmd,
                               uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    if (static_cast<uint32_t>(magic_ptr) != REBOOT_MAGIC1) return static_cast<uint64_t>(-1);

    fk::algorithms::klog("sys_reboot", "cmd=0x%x", (unsigned)cmd);

    if (static_cast<uint32_t>(cmd) == REBOOT_CMD_POWER_OFF) {
        do_poweroff();
    }
    do_reboot();
    __builtin_unreachable();
}
