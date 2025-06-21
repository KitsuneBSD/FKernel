#pragma once

namespace routine {
extern "C" void timer_handler(void* context);
extern "C" void keyboard_handler(void* context);
extern "C" void cascade_handler(void* context);
extern "C" void com2_handler(void* context);
extern "C" void com1_handler(void* context);
extern "C" void legacy_peripheral_handler(void* context);
extern "C" void fdc_handler(void* context);
extern "C" void spurious_irq7_handler(void* context);
extern "C" void rtc_handler(void* context);
extern "C" void acpi_handler(void* context);
extern "C" void irq10_pci_handler(void* context);
extern "C" void irq11_pci_handler(void* context);
extern "C" void ps2_mouse_handler(void* context);
extern "C" void fpu_handler(void* context);
extern "C" void primary_ata_handler(void* context);
extern "C" void secondary_ata_handler(void* context);
}

extern "C" void irq_dispatch(LibC::uint8_t irq, void* context);
