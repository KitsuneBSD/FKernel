#pragma once

#include <Kernel/Driver/Device/CharacterDevice/character_device.h>
#include <Kernel/Driver/Keyboard/ps2_keyboard.h>
#include <Kernel/Scheduler/scheduler.h>

namespace fkernel {

class KeyboardNode final : public CharacterDevice {
public:
    KeyboardNode() {
        set_name("keyboard");
    }
    virtual ~KeyboardNode() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t size, uint8_t* buffer) override {
        if (!buffer) return fk::core::Error::InvalidParameter;
        
        size_t copied = 0;
        while (copied < size) {
            while (!PS2Keyboard::the().has_key()) {
                SchedulerManager::the().schedule();
            }
            
            char c = PS2Keyboard::the().pop_key();
            if (!c) continue;
            
            buffer[copied++] = static_cast<uint8_t>(c);
            if (c == '\n') break;
        }
        return copied;
    }

    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override {
        return fk::core::Error::NotImplemented;
    }

    virtual size_t size() const override { return 0; }
};

} // namespace fkernel

