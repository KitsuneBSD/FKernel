#pragma once

#include <LibC/stdint.h>

void default_handler([[maybe_unused]] uint8_t vector);
void nmi_handler([[maybe_unused]] uint8_t vector);

void timer_handler([[maybe_unused]] uint8_t vector);
