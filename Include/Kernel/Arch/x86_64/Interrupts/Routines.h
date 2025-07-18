#pragma once

#include <Kernel/Arch/x86_64/Hardware/Io_Constants.h>
#include <Kernel/Arch/x86_64/Segments/Idt.h>
#include <LibC/stdint.h>

typedef void (*irq_handler_t)(LibC::uint8_t, void*);

typedef struct {
    int irq;
    irq_handler_t handler;
} irq_entry_t;

extern "C" void irq0_handler();  // 32
extern "C" void irq1_handler();  // 33
extern "C" void irq2_handler();  // 34
extern "C" void irq3_handler();  // 35
extern "C" void irq4_handler();  // 36
extern "C" void irq5_handler();  // 37
extern "C" void irq6_handler();  // 38
extern "C" void irq7_handler();  // 39
extern "C" void irq8_handler();  // 40
extern "C" void irq9_handler();  // 41
extern "C" void irq10_handler(); // 42
extern "C" void irq11_handler(); // 43
extern "C" void irq12_handler(); // 44
extern "C" void irq13_handler(); // 45
extern "C" void irq14_handler(); // 46
extern "C" void irq15_handler(); // 47

extern "C" void (*const routine_stubs[MAX_IRQ_NUMBER])();

constexpr char const* named_irq(int irq) noexcept
{
    switch (irq) {
    case 0:
        return "System Timer (IRQ0)";
    case 1:
        return "Keyboard Controller (IRQ1)";
    case 2:
        return "Cascade (IRQ2 - PIC link)";
    case 3:
        return "COM2 / Serial Port (IRQ3)";
    case 4:
        return "COM1 / Serial Port (IRQ4)";
    case 5:
        return "LPT2 / Sound Card (IRQ5)";
    case 6:
        return "Floppy Disk Controller (IRQ6)";
    case 7:
        return "Parallel Port / LPT1 (IRQ7)";
    case 8:
        return "Real Time Clock (IRQ8)";
    case 9:
        return "ACPI / Legacy SCSI / IRQ2 Redirect (IRQ9)";
    case 10:
        return "Unused / Network / SCSI (IRQ10)";
    case 11:
        return "Unused / Network / SCSI (IRQ11)";
    case 12:
        return "PS/2 Mouse (IRQ12)";
    case 13:
        return "FPU / x87 Floating Point (IRQ13)";
    case 14:
        return "Primary ATA Channel (IRQ14)";
    case 15:
        return "Secondary ATA Channel (IRQ15)";
    default:
        return "Unknown IRQ";
    }
}

void register_irq_handler(LibC::uint8_t irq, idt::IrqHandler handler) noexcept;
void unregister_irq_handler(LibC::uint8_t irq) noexcept;
LibC::uint64_t uptime();

extern "C" irq_entry_t irq_table[MAX_IRQ_NUMBER];

extern "C" void timer_handler(LibC::uint8_t irq, void* context);
extern "C" void keyboard_handler(LibC::uint8_t irq, void* context);
extern "C" void cascade_handler(LibC::uint8_t irq, void* context);
extern "C" void com2_handler(LibC::uint8_t irq, void* context);
extern "C" void com1_handler(LibC::uint8_t irq, void* context);
extern "C" void legacy_peripheral_handler(LibC::uint8_t irq, void* context);
extern "C" void fdc_handler(LibC::uint8_t irq, void* context);
extern "C" void spurious_irq7_handler(LibC::uint8_t irq, void* context);
extern "C" void rtc_handler(LibC::uint8_t irq, void* context);
extern "C" void acpi_handler(LibC::uint8_t irq, void* context);
extern "C" void irq10_pci_handler(LibC::uint8_t irq, void* context);
extern "C" void irq11_pci_handler(LibC::uint8_t irq, void* context);
extern "C" void ps2_mouse_handler(LibC::uint8_t irq, void* context);
extern "C" void fpu_handler(LibC::uint8_t irq, void* context);
extern "C" void primary_ata_handler(LibC::uint8_t irq, void* context);
extern "C" void secondary_ata_handler(LibC::uint8_t irq, void* context);
