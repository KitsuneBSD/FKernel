#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupt/pic.h>
#include <Kernel/Arch/x86_64/io.h>
#include <LibFK/log.h>

void Pic8259::remap(uint8_t offset1, uint8_t offset2) {
  uint8_t mask1 = inb(PIC1_DATA);
  uint8_t mask2 = inb(PIC2_DATA);

  klog("PIC8259",
       "Remapping PIC8259: master=0x%x, slave=0x%x -> offsets 0x%x/0x%x", mask1,
       mask2, offset1, offset2);

  outb(PIC1_CMD, 0x11);
  io_wait();
  outb(PIC2_CMD, 0x11);
  io_wait();

  outb(PIC1_DATA, offset1);
  io_wait();
  outb(PIC2_DATA, offset2);
  io_wait();

  outb(PIC1_DATA, 0x04); // escravo ligado no IRQ2
  io_wait();
  outb(PIC2_DATA, 0x02);
  io_wait();

  outb(PIC1_DATA, 0x01);
  io_wait();
  outb(PIC2_DATA, 0x01);
  io_wait();

  outb(PIC1_DATA, mask1);
  outb(PIC2_DATA, mask2);

  klog("PIC8259", "PIC8259 remap completed");
}

void Pic8259::sendEOI(uint8_t irq) {
  if (irq >= 8)
    outb(PIC2_CMD, EOI);
  outb(PIC1_CMD, EOI);
  klog("PIC8259", "Sending EOI for IRQ %u", irq);
}

void Pic8259::set_mask(uint8_t irq) {
  uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
  irq = (irq < 8) ? irq : irq - 8;
  uint8_t value = inb(port) | (1 << irq);
  outb(port, value);
  klog("PIC8259", "Set mask on IRQ %u (port=0x%x, value=0x%x)", irq, port,
       value);
}

void Pic8259::clear_mask(uint8_t irq) {
  uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
  irq = (irq < 8) ? irq : irq - 8;
  uint8_t value = inb(port) & ~(1 << irq);
  outb(port, value);
  klog("PIC8259", "Cleared mask on IRQ %u (port=0x%x, value=0x%x)", irq, port,
       value);
}

uint8_t Pic8259::get_mask_master() {
  klog("PIC8259", "Master mask read: 0x%x", PIC1_DATA);
  return inb(PIC1_DATA);
}

uint8_t Pic8259::get_mask_slave() {
  klog("PIC8259", "Slave mask read: 0x%x", PIC2_DATA);
  return inb(PIC2_DATA);
}

void Pic8259::set_mask_master(uint8_t mask) {
  outb(PIC1_DATA, mask);
  klog("PIC8259", "Master mask set to 0x%x", mask);
}

void Pic8259::set_mask_slave(uint8_t mask) {
  outb(PIC2_DATA, mask);
  klog("PIC8259", "Slave mask set to 0x%x", mask);
}
