#pragma once

#include <Kernel/Driver/Device/CharacterDevice/character_device.h>
#include <Kernel/Driver/Vga/vga_adapter.h>
#include <LibC/string.h>

#include <Kernel/Driver/Keyboard/ps2_keyboard.h>
#include <Kernel/Scheduler/scheduler.h>

namespace fkernel {

class ConsoleNode final : public CharacterDevice {
public:
    ConsoleNode() {
        set_name("console");
    }
    virtual ~ConsoleNode() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read([[maybe_unused]] uint64_t offset, size_t size, uint8_t* buffer) override {
        if (!buffer || size == 0) return 0;

        size_t count = 0;
        auto& kb = PS2Keyboard::the();
        
        while (count < size) {
            if (kb.has_key()) {
                buffer[count++] = static_cast<uint8_t>(kb.pop_key());
                // If we got at least one char, we can return (standard read behavior)
                if (count > 0) return count;
            } else {
                if (count > 0) return count;
                SchedulerManager::the().yield();
            }
        }

        return count;
    }

    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t size, const uint8_t* buffer) override {
        if (!buffer) return fk::core::Error::InvalidParameter;
        
        // Escreve o buffer no VGA
        // Como o write_ansi espera uma string null-terminated, e o buffer do write não é,
        // precisamos iterar ou copiar.
        for (size_t i = 0; i < size; ++i) {
            vga::the().put_char(static_cast<char>(buffer[i]));
        }
        
        return size;
    }

    virtual size_t size() const override { return 0; }
};

} // namespace fkernel
