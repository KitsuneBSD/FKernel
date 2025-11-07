# FKernel - List of changes

## Toolchain

- [ ] Create a custom toolchain to FKernel
  - [x] Lua
  - [ ] Clang
  - [x] Nasm
  - [ ] Lld

## FKernel

- [ ] Create a base class **Hardware Interrupt** to be inherited by ([8259Pic](https://en.wikipedia.org/wiki/Intel_8259) / [APIC](https://en.wikipedia.org/wiki/Advanced_Programmable_Interrupt_Controller) ...)
- [ ] Create a base class **Timer** to be inherited by ([PIT](https://en.wikipedia.org/wiki/Programmable_interval_timer), [APIC Timer](https://en.wikipedia.org/wiki/Advanced_Programmable_Interrupt_Controller#APIC_timer) ...)

Use OpTables to configure globally

## LibFK

- [ ] Create a class to make [CRTP](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern)
