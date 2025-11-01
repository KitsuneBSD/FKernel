#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/8259_pic.h>

void PIC8259::initialize() {
  kdebug("PIC", "Initializing PIC8259...");

  outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
  io_wait();
  outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
  io_wait();

  kdebug("PIC", "Sending ICW2: offsets for master (0x20) and slave (0x28)");
  outb(PIC1_DATA, 0x20); // ICW2: offset master
  io_wait();
  outb(PIC2_DATA, 0x28); // ICW2: offset slave
  io_wait();

  kdebug("PIC", "Sending ICW3: wiring master/slave");
  outb(PIC1_DATA, 0x04); // ICW3: slave PIC at IRQ2
  io_wait();
  outb(PIC2_DATA, 0x02); // ICW3: identity do slave
  io_wait();

  kdebug("PIC", "Sending ICW4: mode 8086");
  outb(PIC1_DATA, ICW4_8086);
  io_wait();
  outb(PIC2_DATA, ICW4_8086);
  io_wait();

  kdebug("PIC", "Masking all IRQs initially");
  outb(PIC1_DATA, 0xFF);
  outb(PIC2_DATA, 0xFF);

  klog("PIC",
       "Programmable Interrupt Controller initialized and all IRQs masked");
}

void PIC8259::mask_irq(uint8_t irq) {
  if (irq < 8) {
    uint8_t mask = inb(PIC1_DATA) | (1 << irq);
    outb(PIC1_DATA, mask);
    kdebug("PIC", "IRQ masked on master PIC", irq, mask);
  } else {
    uint8_t mask = inb(PIC2_DATA) | (1 << (irq - 8));
    outb(PIC2_DATA, mask);
    kdebug("PIC", "IRQ masked on slave PIC", irq, mask);
  }
}

void PIC8259::unmask_irq(uint8_t irq) {
  if (irq < 8) {
    uint8_t mask = inb(PIC1_DATA) & ~(1 << irq);
    outb(PIC1_DATA, mask);
    kdebug("PIC", "IRQ unmasked on master PIC", irq, mask);
  } else {
    uint8_t mask = inb(PIC2_DATA) & ~(1 << (irq - 8));
    outb(PIC2_DATA, mask);
    kdebug("PIC", "IRQ unmasked on slave PIC", irq, mask);
  }
}

void PIC8259::send_eoi(uint8_t irq) {
  if (irq >= 8) {
    outb(PIC2_CMD, 0x20);

    if (irq != 0){
      kdebug("PIC", "EOI sent to slave PIC", irq);
    }
  }
  outb(PIC1_CMD, 0x20);

  if (irq != 0) {
    kdebug("PIC", "EOI sent to master PIC", irq);
  }
}

void PIC8259::send_eoi_safe(uint8_t irq) {
  bool spurious = false;
  
  if (irq == 7) {
    spurious = !(get_isr() & (1 << 7));
  } else if (irq == 15) {
    spurious = !(get_isr() & (1 << 15));
  }

  if (!spurious) {
    kdebug("PIC", "Sending normal EOI", irq);
    send_eoi(irq);
  } else {
    kdebug("PIC", "Spurious IRQ detected, EOI skipped", irq);
  }
}
