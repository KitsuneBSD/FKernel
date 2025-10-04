#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/8259_pic.h>

void PIC8259::initialize() {
  outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
  io_wait();
  outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
  io_wait();

  outb(PIC1_DATA, 0x20);
  io_wait(); // ICW2: offset master
  outb(PIC2_DATA, 0x28);
  io_wait(); // ICW2: offset slave

  outb(PIC1_DATA, 0x04);
  io_wait(); // ICW3: slave PIC at IRQ2
  outb(PIC2_DATA, 0x02);
  io_wait(); // ICW3: identity do slave

  outb(PIC1_DATA, ICW4_8086);
  io_wait(); // ICW4: modo 8086
  outb(PIC2_DATA, ICW4_8086);
  io_wait();

  outb(PIC1_DATA, 0xFF); // mascarar todos IRQs
  outb(PIC2_DATA, 0xFF);

  klog("PIC",
       "Programmable Interrupt Controller initialized and all IRQs masked");
}

void PIC8259::mask_irq(uint8_t irq) {
  if (irq < 8) {
    uint8_t mask = inb(PIC1_DATA) | (1 << irq);
    outb(PIC1_DATA, mask);
  } else {
    uint8_t mask = inb(PIC2_DATA) | (1 << (irq - 8));
    outb(PIC2_DATA, mask);
  }
}

void PIC8259::unmask_irq(uint8_t irq) {
  if (irq < 8) {
    uint8_t mask = inb(PIC1_DATA) & ~(1 << irq);
    outb(PIC1_DATA, mask);
  } else {
    uint8_t mask = inb(PIC2_DATA) & ~(1 << (irq - 8));
    outb(PIC2_DATA, mask);
  }
}

void PIC8259::send_eoi(uint8_t irq) {
  if (irq >= 8)
    outb(PIC2_CMD, 0x20);
  outb(PIC1_CMD, 0x20);
}

void PIC8259::send_eoi_safe(uint8_t irq) {
  bool spurious = false;

  if (irq == 7) {
    spurious = !(get_isr() & (1 << 7));
  } else if (irq == 15) {
    spurious = !(get_isr() & (1 << 15));
  }

  if (!spurious) {
    send_eoi(irq); // envia EOI normal
  }
}
