#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Driver/Vga/vga_adapter.h>
#include <LibC/string.h>

namespace fkernel {

class ConsoleNode final : public Node {
public:
    ConsoleNode() = default;
    virtual ~ConsoleNode() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override {
        // TODO: Implementar leitura do teclado aqui futuramente
        return fk::core::Error::NotImplemented;
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
