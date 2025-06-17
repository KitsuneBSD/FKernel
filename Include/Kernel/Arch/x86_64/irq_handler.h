#pragma once

#include <LibFK/Log.h>

extern "C" void timer_handler(void *context);
extern "C" void keyboard_handler(void *context);
extern "C" void cascade_handler(void *context);
extern "C" void com2_handler(void *context);
extern "C" void com1_handler(void *context);
extern "C" void legacy_peripheral_handler(void *context);
