#pragma once

#include <LibC/stdint.h>
#include <LibC/stddef.h>

constexpr uint16_t PS2_DATA_PORT = 0x60;
constexpr uint16_t PS2_STATUS_PORT = 0x64;

constexpr size_t KEYBOARD_BUFFER_SIZE = 256;

class PS2Keyboard
{
private:
    char buffer[KEYBOARD_BUFFER_SIZE];
    size_t head = 0;
    size_t tail = 0;
    bool shift_pressed = false;

    PS2Keyboard() = default;

    void push_char(char c);
    void handle_scancode(uint8_t scancode);

public:
    static PS2Keyboard &the()
    {
        static PS2Keyboard instance;
        return instance;
    }

    void initialize();
    void irq_handler();

    bool has_key() const;
    char pop_key();
};