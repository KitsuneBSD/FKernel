#pragma once

#include <Kernel/Driver/Device/CharacterDevice/character_device.h>

namespace fkernel {

class NullDevice final : public CharacterDevice {
public:
    NullDevice() {
        set_name("null");
    }
    virtual ~NullDevice() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override {
        return 0; // EOF
    }

    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t size, const uint8_t*) override {
        return size; // Discard data
    }

    virtual size_t size() const override { return 0; }
};

class ZeroDevice final : public CharacterDevice {
public:
    ZeroDevice() {
        set_name("zero");
    }
    virtual ~ZeroDevice() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t size, uint8_t* buffer) override {
        if (!buffer) return fk::core::Error::InvalidParameter;
        for (size_t i = 0; i < size; ++i) buffer[i] = 0;
        return size;
    }

    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t size, const uint8_t*) override {
        return size; // Discard data
    }

    virtual size_t size() const override { return 0; }
};

} // namespace fkernel
