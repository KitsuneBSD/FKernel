#pragma once

#include <Kernel/Driver/Device/CharacterDevice/character_device.h>
#include <Kernel/Driver/Terminal/terminal_manager.h>

namespace fkernel {

/// @brief /dev/tty - A proxy device that always points to the current active TTY
class CurrentTTYNode final : public CharacterDevice {
public:
    CurrentTTYNode() {
        set_name("tty");
    }
    virtual ~CurrentTTYNode() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override {
        auto* active = terminal::TerminalManager::the().active_terminal();
        if (!active) return fk::core::Error::NotFound;
        return active->read(offset, size, buffer);
    }

    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t offset, size_t size, const uint8_t* buffer) override {
        auto* active = terminal::TerminalManager::the().active_terminal();
        if (!active) return fk::core::Error::NotFound;
        return active->write(offset, size, buffer);
    }

    virtual fk::core::Result<int, fk::core::Error> ioctl(uint64_t request, uint64_t arg) override {
        auto* active = terminal::TerminalManager::the().active_terminal();
        if (!active) return fk::core::Error::NotFound;
        return active->ioctl(request, arg);
    }

    virtual size_t size() const override { return 0; }
};

} // namespace fkernel
