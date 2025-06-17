#pragma once

#include <Driver/Pic.h>
#include <LibC/stdint.h>
#include <LibFK/Log.h>

using IRQHandler = void (*)(void *);

extern IRQHandler irq_handlers[16];

extern "C" void (*const irq_stubs[16])();

void register_irq_handler(uint8_t irq, IRQHandler handler);

extern "C" void irq_dispatch(uint8_t irq, void *context);
