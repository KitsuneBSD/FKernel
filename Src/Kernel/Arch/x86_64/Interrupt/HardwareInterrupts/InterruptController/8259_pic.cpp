#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/8259_pic.h>
#include <Kernel/Arch/x86_64/io.h>
#include <LibFK/Algorithms/log.h>

// PIC ports
static constexpr uint8_t PIC1_CMD = 0x20;
static constexpr uint8_t PIC1_DATA = 0x21;
static constexpr uint8_t PIC2_CMD = 0xA0;
static constexpr uint8_t PIC2_DATA = 0xA1;

// Initialization Control Words
static constexpr uint8_t ICW1_INIT = 0x11;
static constexpr uint8_t ICW1_ICW4 = 0x01;
static constexpr uint8_t ICW4_8086 = 0x01;

// Read IRR/ISR
static constexpr uint8_t PIC_READ_IRR = 0x0A;
static constexpr uint8_t PIC_READ_ISR = 0x0B;

// Low-level helper
static uint16_t __get_irq_reg(uint8_t ocw3) {
  arch_outb(PIC1_CMD, ocw3);
  arch_outb(PIC2_CMD, ocw3);
  return (arch_inb(PIC2_CMD) << 8) | arch_inb(PIC1_CMD);
}

uint16_t PIC8259::get_irr() { return __get_irq_reg(PIC_READ_IRR); }
uint16_t PIC8259::get_isr() { return __get_irq_reg(PIC_READ_ISR); }

void PIC8259::initialize() {
  /*TODO: Apply this log when we work with LogLevel
  fk::algorithms::kdebug("PIC", "Initializing PIC8259...");
  */
 
  arch_outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
  io_wait();
  arch_outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
  io_wait();
  arch_outb(PIC1_DATA, 0x20);
  io_wait();
  arch_outb(PIC2_DATA, 0x28);
  io_wait();
  arch_outb(PIC1_DATA, 0x04);
  io_wait();
  arch_outb(PIC2_DATA, 0x02);
  io_wait();
  arch_outb(PIC1_DATA, ICW4_8086);
  io_wait();
  arch_outb(PIC2_DATA, ICW4_8086);
  io_wait();

  arch_outb(PIC1_DATA, 0xFF);
  arch_outb(PIC2_DATA, 0xFF);

  fk::algorithms::klog("PIC", "PIC initialized, all IRQs masked");
}

void PIC8259::mask_interrupt(uint8_t irq) {
  if (irq < 8)
    arch_outb(PIC1_DATA, arch_inb(PIC1_DATA) | (1 << irq));
  else
    arch_outb(PIC2_DATA, arch_inb(PIC2_DATA) | (1 << (irq - 8)));
}

void PIC8259::unmask_interrupt(uint8_t irq) {
  if (irq < 8)
    arch_outb(PIC1_DATA, arch_inb(PIC1_DATA) & ~(1 << irq));
  else
    arch_outb(PIC2_DATA, arch_inb(PIC2_DATA) & ~(1 << (irq - 8)));
}

void PIC8259::send_eoi(uint8_t irq) {
  bool spurious = (irq == 7 && !(get_isr() & (1 << 7))) ||
                  (irq == 15 && !(get_isr() & (1 << 15)));
  if (spurious)
    return;

  if (irq >= 8)
    arch_outb(PIC2_CMD, 0x20);
  arch_outb(PIC1_CMD, 0x20);
}

void PIC8259::disable() {
  arch_outb(PIC1_DATA, 0xFF);
  arch_outb(PIC2_DATA, 0xFF);
  fk::algorithms::klog("PIC", "PIC disabled by masking all IRQs");
}
