#pragma once

#include <Kernel/Driver/Device/CharacterDevice/character_device.h>
#include <Kernel/Driver/Vga/vga_adapter.h>
#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Driver/Keyboard/ps2_keyboard.h>
#include <Kernel/Scheduler/Core/scheduler.h>

namespace fkernel {

class ConsoleNode final : public CharacterDevice {
public:
    ConsoleNode() {
        set_name("console");
    }
    virtual ~ConsoleNode() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override {
        auto* active = fkernel::terminal::TerminalManager::the().active_terminal();
        if (!active) return 0;
        return active->read(offset, size, buffer);
    }

    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t offset, size_t size, const uint8_t* buffer) override {
        auto* active = fkernel::terminal::TerminalManager::the().active_terminal();
        if (!active) return 0;
        return active->write(offset, size, buffer);
    }

    virtual fk::core::Result<int, fk::core::Error> ioctl(uint64_t request, uint64_t arg) override {
        auto* active = fkernel::terminal::TerminalManager::the().active_terminal();
        if (!active) return fk::core::Error::NotFound;
        return active->ioctl(request, arg);
    }

    virtual size_t size() const override { return 0; }
};

} // namespace fkernel
