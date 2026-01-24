#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Driver/SerialPort/serial_port.h>

namespace fkernel {

class SerialNode final : public Node {
public:
    SerialNode() = default;
    virtual ~SerialNode() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override {
        return fk::core::Error::NotImplemented;
    }

    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t size, const uint8_t* buffer) override {
        if (!buffer) return fk::core::Error::InvalidParameter;
        
        for (size_t i = 0; i < size; ++i) {
            serial::write_char(static_cast<char>(buffer[i]));
        }
        
        return size;
    }

    virtual size_t size() const override { return 0; }
};

} // namespace fkernel
